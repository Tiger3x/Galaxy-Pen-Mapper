#include <ntddk.h>
#include <wdf.h>
#include "shared/GalaxyPenMapperProtocol.h"
#include "shared/GalaxyPenEngine.h"
#include "shared/GalaxyPenPressureStats.h"

DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_DEVICE_ADD GalaxyPenEvtDeviceAdd;
EVT_WDF_DEVICE_CONTEXT_CLEANUP GalaxyPenEvtDeviceCleanup;
EVT_WDF_DEVICE_D0_ENTRY GalaxyPenEvtD0Entry;
EVT_WDF_DEVICE_D0_EXIT GalaxyPenEvtD0Exit;
EVT_WDF_DEVICE_FILE_CREATE GalaxyPenEvtFileCreate;
EVT_WDF_FILE_CLEANUP GalaxyPenEvtFileCleanup;
EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL GalaxyPenEvtIoDeviceControl;
EVT_WDF_IO_QUEUE_IO_READ GalaxyPenEvtIoRead;
EVT_WDF_IO_QUEUE_IO_STOP GalaxyPenEvtIoStop;
EVT_WDF_REQUEST_COMPLETION_ROUTINE GalaxyPenReadComplete;

typedef struct _GALAXY_DEVICE_CONTEXT {
    BOOLEAN Registered;
    BOOLEAN Ready;
} GALAXY_DEVICE_CONTEXT;
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(GALAXY_DEVICE_CONTEXT, GalaxyDeviceContext);
typedef struct _GALAXY_REQUEST_CONTEXT {
    unsigned long long Generation;
} GALAXY_REQUEST_CONTEXT;
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(GALAXY_REQUEST_CONTEXT, GalaxyRequestContext);

static WDFSPINLOCK gConfigLock;
static WDFWAITLOCK gLifecycleLock;
static WDFDEVICE gControlDevice;
static ULONG gDeviceCount; // Lifecycle lock.
static ULONG gReadyCount;  // Config lock.
static BOOLEAN gReportSeen;
static GALAXY_PEN_ENGINE gEngine = {1, 0, 0, 0, 0, GALAXY_PEN_DEFAULT_SENSITIVITY, GALAXY_PEN_DEFAULT_FLOOR_PERCENT};
static GALAXY_PEN_PROBE_STATS gStats;

static ULONG GalaxyCapabilities(VOID)
{
#ifdef GALAXY_PEN_PROBE_READ
    return GALAXY_PEN_CAP_OBSERVE;
#else
    return GALAXY_PEN_CAP_OBSERVE | GALAXY_PEN_CAP_TOOL | GALAXY_PEN_CAP_PRESSURE;
#endif
}

static BOOLEAN GalaxyReadyForCorrection(VOID)
{
    // Caller holds config lock. Historical pre-suspend reads cannot arm correction.
    return gStats.AttachedDevices == 1 && gReadyCount == 1 && gReportSeen;
}

static NTSTATUS GalaxyPenCreateControlDevice(WDFDRIVER Driver)
{
    DECLARE_CONST_UNICODE_STRING(deviceName, L"\\Device\\GalaxyPenMapperControlV4");
    DECLARE_CONST_UNICODE_STRING(symbolicLink, L"\\DosDevices\\GalaxyPenMapperControlV4");
    DECLARE_CONST_UNICODE_STRING(adminSddl, L"D:P(A;;GA;;;SY)(A;;GA;;;BA)");
    PWDFDEVICE_INIT deviceInit = WdfControlDeviceInitAllocate(Driver, &adminSddl);
    WDFDEVICE device;
    WDF_IO_QUEUE_CONFIG queueConfig;
    WDF_FILEOBJECT_CONFIG fileConfig;
    NTSTATUS status;
    if (deviceInit == NULL) return STATUS_INSUFFICIENT_RESOURCES;
    WdfDeviceInitSetDeviceType(deviceInit, FILE_DEVICE_UNKNOWN);
    WdfDeviceInitSetCharacteristics(deviceInit, FILE_DEVICE_SECURE_OPEN, FALSE);
    WdfDeviceInitSetExclusive(deviceInit, TRUE);
    WDF_FILEOBJECT_CONFIG_INIT(&fileConfig, GalaxyPenEvtFileCreate,
                               WDF_NO_EVENT_CALLBACK, GalaxyPenEvtFileCleanup);
    WdfDeviceInitSetFileObjectConfig(deviceInit, &fileConfig, WDF_NO_OBJECT_ATTRIBUTES);
    status = WdfDeviceInitAssignName(deviceInit, &deviceName);
    if (!NT_SUCCESS(status)) {
        WdfDeviceInitFree(deviceInit);
        return status;
    }
    status = WdfDeviceCreate(&deviceInit, WDF_NO_OBJECT_ATTRIBUTES, &device);
    if (!NT_SUCCESS(status)) {
        if (deviceInit != NULL) WdfDeviceInitFree(deviceInit);
        return status;
    }
    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&queueConfig, WdfIoQueueDispatchSequential);
    queueConfig.PowerManaged = WdfFalse;
    queueConfig.EvtIoDeviceControl = GalaxyPenEvtIoDeviceControl;
    status = WdfIoQueueCreate(device, &queueConfig, WDF_NO_OBJECT_ATTRIBUTES, WDF_NO_HANDLE);
    if (NT_SUCCESS(status)) status = WdfDeviceCreateSymbolicLink(device, &symbolicLink);
    if (!NT_SUCCESS(status)) {
        WdfObjectDelete(device);
        return status;
    }
    gControlDevice = device;
    WdfControlFinishInitializing(device);
    return STATUS_SUCCESS;
}

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
    WDF_DRIVER_CONFIG config;
    WDFDRIVER driver;
    NTSTATUS status;
    WDF_DRIVER_CONFIG_INIT(&config, GalaxyPenEvtDeviceAdd);
    status = WdfDriverCreate(DriverObject, RegistryPath, WDF_NO_OBJECT_ATTRIBUTES, &config, &driver);
    if (!NT_SUCCESS(status)) return status;
    status = WdfSpinLockCreate(WDF_NO_OBJECT_ATTRIBUTES, &gConfigLock);
    if (!NT_SUCCESS(status)) return status;
    // Create the control device only when a filter device has actually attached.
    return WdfWaitLockCreate(WDF_NO_OBJECT_ATTRIBUTES, &gLifecycleLock);
}

VOID GalaxyPenEvtFileCreate(WDFDEVICE Device, WDFREQUEST Request, WDFFILEOBJECT FileObject)
{
    UNREFERENCED_PARAMETER(Device);
    UNREFERENCED_PARAMETER(FileObject);
    WdfSpinLockAcquire(gConfigLock);
    GalaxyPenResetContactStats(&gStats);
    WdfSpinLockRelease(gConfigLock);
    WdfRequestComplete(Request, STATUS_SUCCESS);
}

VOID GalaxyPenEvtFileCleanup(WDFFILEOBJECT FileObject)
{
    UNREFERENCED_PARAMETER(FileObject);
    WdfSpinLockAcquire(gConfigLock);
    GalaxyPenEngineDisable(&gEngine);
    WdfSpinLockRelease(gConfigLock);
}

VOID GalaxyPenEvtIoDeviceControl(WDFQUEUE Queue, WDFREQUEST Request,
    size_t OutputBufferLength, size_t InputBufferLength, ULONG IoControlCode)
{
    NTSTATUS status = STATUS_INVALID_DEVICE_REQUEST;
    GALAXY_PEN_CONFIG* config = NULL;
    UNREFERENCED_PARAMETER(Queue);
    if (IoControlCode == GALAXY_PEN_IOCTL_QUERY_PROBE_STATS) {
        GALAXY_PEN_PROBE_STATS* stats = NULL;
        if (OutputBufferLength < sizeof(*stats)) status = STATUS_BUFFER_TOO_SMALL;
        else status = WdfRequestRetrieveOutputBuffer(Request, sizeof(*stats), (PVOID*)&stats, NULL);
        if (NT_SUCCESS(status)) {
            WdfSpinLockAcquire(gConfigLock);
            GalaxyPenEngineExpire(&gEngine, KeQueryInterruptTime());
            *stats = gStats;
            stats->Version = GALAXY_PEN_PROBE_PROTOCOL_VERSION;
            stats->Capabilities = GalaxyCapabilities();
            stats->ReadyForCorrection = GalaxyReadyForCorrection() ? 1u : 0u;
            WdfSpinLockRelease(gConfigLock);
            WdfRequestCompleteWithInformation(Request, STATUS_SUCCESS, sizeof(*stats));
            return;
        }
    } else if (IoControlCode == GALAXY_PEN_IOCTL_SET_CONFIG) {
        if (InputBufferLength != sizeof(*config)) status = STATUS_INFO_LENGTH_MISMATCH;
        else status = WdfRequestRetrieveInputBuffer(Request, sizeof(*config), (PVOID*)&config, NULL);
        if (NT_SUCCESS(status)) {
            if (config->Version != GALAXY_PEN_PROTOCOL_VERSION || config->Enabled > 1 ||
                config->PressureEnabled > 1 || config->StateFlags != 0 ||
                config->ContactFloorPercent < 1 || config->ContactFloorPercent > 25 ||
                config->SensitivityPermille < GALAXY_PEN_MIN_SENSITIVITY ||
                config->SensitivityPermille > GALAXY_PEN_MAX_SENSITIVITY) {
                status = STATUS_INVALID_PARAMETER;
            } else if (config->Enabled && (GalaxyCapabilities() & GALAXY_PEN_CAP_TOOL) == 0) {
                status = STATUS_NOT_SUPPORTED; // Probe rejects activation even from another client.
            } else {
                WdfSpinLockAcquire(gConfigLock);
                if (config->Enabled && !GalaxyReadyForCorrection()) status = STATUS_DEVICE_NOT_READY;
                else {
                    if (gEngine.Enabled != config->Enabled || gEngine.PressureEnabled != config->PressureEnabled ||
                        gEngine.Sensitivity != config->SensitivityPermille || gEngine.ContactFloorPercent != config->ContactFloorPercent)
                        GalaxyPenResetContactStats(&gStats);
                    GalaxyPenEngineConfigure(&gEngine, config->Enabled, config->PressureEnabled,
                        config->SensitivityPermille, config->ContactFloorPercent, KeQueryInterruptTime());
                }
                WdfSpinLockRelease(gConfigLock);
            }
        }
    } else if (IoControlCode == GALAXY_PEN_IOCTL_QUERY_CONFIG) {
        if (OutputBufferLength < sizeof(*config)) status = STATUS_BUFFER_TOO_SMALL;
        else status = WdfRequestRetrieveOutputBuffer(Request, sizeof(*config), (PVOID*)&config, NULL);
        if (NT_SUCCESS(status)) {
            WdfSpinLockAcquire(gConfigLock);
            GalaxyPenEngineExpire(&gEngine, KeQueryInterruptTime());
            config->Version = GALAXY_PEN_PROTOCOL_VERSION;
            config->Enabled = gEngine.Enabled;
            config->PressureEnabled = gEngine.PressureEnabled;
            config->SensitivityPermille = gEngine.Sensitivity;
            config->ContactFloorPercent = gEngine.ContactFloorPercent;
            config->StateFlags = gEngine.WaitForLift ? GALAXY_PEN_STATE_WAIT_FOR_LIFT : 0;
            WdfSpinLockRelease(gConfigLock);
            WdfRequestCompleteWithInformation(Request, STATUS_SUCCESS, sizeof(*config));
            return;
        }
    }
    WdfRequestComplete(Request, status);
}

NTSTATUS GalaxyPenEvtDeviceAdd(WDFDRIVER Driver, PWDFDEVICE_INIT DeviceInit)
{
    WDFDEVICE device;
    WDFQUEUE readQueue;
    WDF_OBJECT_ATTRIBUTES attributes;
    WDF_IO_QUEUE_CONFIG queueConfig;
    WDF_PNPPOWER_EVENT_CALLBACKS powerCallbacks;
    NTSTATUS status;
    WdfFdoInitSetFilter(DeviceInit);
    WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&powerCallbacks);
    powerCallbacks.EvtDeviceD0Entry = GalaxyPenEvtD0Entry;
    powerCallbacks.EvtDeviceD0Exit = GalaxyPenEvtD0Exit;
    WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit, &powerCallbacks);
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, GALAXY_REQUEST_CONTEXT);
    WdfDeviceInitSetRequestAttributes(DeviceInit, &attributes);
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, GALAXY_DEVICE_CONTEXT);
    attributes.EvtCleanupCallback = GalaxyPenEvtDeviceCleanup;
    status = WdfDeviceCreate(&DeviceInit, &attributes, &device);
    if (!NT_SUCCESS(status)) return status;

    WDF_IO_QUEUE_CONFIG_INIT(&queueConfig, WdfIoQueueDispatchParallel);
    queueConfig.PowerManaged = WdfFalse; // The lower stack owns power policy.
    queueConfig.AllowZeroLengthRequests = TRUE;
    queueConfig.EvtIoRead = GalaxyPenEvtIoRead;
    queueConfig.EvtIoStop = GalaxyPenEvtIoStop;
    status = WdfIoQueueCreate(device, &queueConfig, WDF_NO_OBJECT_ATTRIBUTES, &readQueue);
    if (!NT_SUCCESS(status)) return status;
    status = WdfDeviceConfigureRequestDispatching(device, readQueue, WdfRequestTypeRead);
    if (!NT_SUCCESS(status)) return status;
    WdfWaitLockAcquire(gLifecycleLock, NULL);
    if (gDeviceCount == 0) status = GalaxyPenCreateControlDevice(Driver);
    if (NT_SUCCESS(status)) {
        ++gDeviceCount;
        GalaxyDeviceContext(device)->Registered = TRUE;
        WdfSpinLockAcquire(gConfigLock);
        gStats.AttachedDevices = gDeviceCount;
        gReportSeen = FALSE;
        gStats.SnapshotValid = 0;
        GalaxyPenEngineDisable(&gEngine);
        WdfSpinLockRelease(gConfigLock);
    }
    WdfWaitLockRelease(gLifecycleLock);
    return status;
}

VOID GalaxyPenEvtDeviceCleanup(WDFOBJECT Object)
{
    GALAXY_DEVICE_CONTEXT* context = GalaxyDeviceContext((WDFDEVICE)Object);
    // WDFDEVICE cleanup runs at PASSIVE_LEVEL, as required for CDO deletion.
    WdfWaitLockAcquire(gLifecycleLock, NULL);
    if (context->Registered) {
        context->Registered = FALSE;
        --gDeviceCount;
        WdfSpinLockAcquire(gConfigLock);
        if (context->Ready) { context->Ready = FALSE; --gReadyCount; }
        gStats.AttachedDevices = gDeviceCount;
        gReportSeen = FALSE;
        gStats.SnapshotValid = 0;
        GalaxyPenEngineDisable(&gEngine);
        WdfSpinLockRelease(gConfigLock);
        if (gDeviceCount == 0 && gControlDevice != NULL) {
            WDFDEVICE controlDevice = gControlDevice;
            gControlDevice = NULL;
            WdfObjectDelete(controlDevice);
        }
    }
    WdfWaitLockRelease(gLifecycleLock);
}

NTSTATUS GalaxyPenEvtD0Entry(WDFDEVICE Device, WDF_POWER_DEVICE_STATE PreviousState)
{
    GALAXY_DEVICE_CONTEXT* context = GalaxyDeviceContext(Device);
    UNREFERENCED_PARAMETER(PreviousState);
    WdfSpinLockAcquire(gConfigLock);
    if (!context->Ready) { context->Ready = TRUE; ++gReadyCount; }
    gReportSeen = FALSE;
    gStats.SnapshotValid = 0;
    GalaxyPenResetContactStats(&gStats);
    GalaxyPenEngineDisable(&gEngine);
    WdfSpinLockRelease(gConfigLock);
    return STATUS_SUCCESS;
}

NTSTATUS GalaxyPenEvtD0Exit(WDFDEVICE Device, WDF_POWER_DEVICE_STATE TargetState)
{
    GALAXY_DEVICE_CONTEXT* context = GalaxyDeviceContext(Device);
    UNREFERENCED_PARAMETER(TargetState);
    WdfSpinLockAcquire(gConfigLock);
    if (context->Ready) { context->Ready = FALSE; --gReadyCount; }
    gReportSeen = FALSE;
    gStats.SnapshotValid = 0;
    GalaxyPenResetContactStats(&gStats);
    GalaxyPenEngineDisable(&gEngine);
    WdfSpinLockRelease(gConfigLock);
    return STATUS_SUCCESS;
}

VOID GalaxyPenEvtIoRead(WDFQUEUE Queue, WDFREQUEST Request, size_t Length)
{
    WDFDEVICE device = WdfIoQueueGetDevice(Queue);
    UNREFERENCED_PARAMETER(Length);
    WdfSpinLockAcquire(gConfigLock);
    GalaxyRequestContext(Request)->Generation = gEngine.Generation;
    ++gStats.StartedReads;
    ++gStats.PendingReads;
    WdfSpinLockRelease(gConfigLock);
    WdfRequestFormatRequestUsingCurrentType(Request);
    WdfRequestSetCompletionRoutine(Request, GalaxyPenReadComplete, NULL);
    if (!WdfRequestSend(Request, WdfDeviceGetIoTarget(device), WDF_NO_SEND_OPTIONS)) {
        NTSTATUS status = WdfRequestGetStatus(Request);
        WdfSpinLockAcquire(gConfigLock);
        ++gStats.ForwardFailures;
        --gStats.PendingReads;
        gStats.LastStatus = (ULONG)status;
        WdfSpinLockRelease(gConfigLock);
        WdfRequestComplete(Request, status);
    }
}

VOID GalaxyPenEvtIoStop(WDFQUEUE Queue, WDFREQUEST Request, ULONG ActionFlags)
{
    UNREFERENCED_PARAMETER(Queue);
    if ((ActionFlags & WdfRequestStopActionSuspend) != 0) WdfRequestStopAcknowledge(Request, FALSE);
    else WdfRequestCancelSentRequest(Request); // The completion routine remains the completion owner.
}

VOID GalaxyPenReadComplete(WDFREQUEST Request, WDFIOTARGET Target,
    PWDF_REQUEST_COMPLETION_PARAMS CompletionParams, WDFCONTEXT Context)
{
    NTSTATUS status = CompletionParams->IoStatus.Status;
    ULONG_PTR information = CompletionParams->IoStatus.Information;
    PVOID buffer = NULL;
    size_t bufferLength = 0;
    BOOLEAN validBuffer = FALSE;
    UCHAR reportId = 0xFF;
    UNREFERENCED_PARAMETER(Target);
    UNREFERENCED_PARAMETER(Context);
    if (NT_SUCCESS(status) && information > 0 &&
        NT_SUCCESS(WdfRequestRetrieveOutputBuffer(Request, 1, &buffer, &bufferLength)) &&
        bufferLength >= information) {
        validBuffer = TRUE;
        reportId = ((const UCHAR*)buffer)[0];
    }
    WdfSpinLockAcquire(gConfigLock);
    ++gStats.CompletedReads;
    --gStats.PendingReads;
    gStats.LastStatus = (ULONG)status;
    gStats.LastBytes = (ULONG)information;
    gStats.LastReportId = reportId;
    if (NT_SUCCESS(status) && information > 0 && !validBuffer) ++gStats.BufferFailures;
    if (NT_SUCCESS(status) && information == 15 && validBuffer && bufferLength >= 15) {
        ++gStats.Successful15ByteReads;
        if (reportId == 0x02) {
            const UCHAR* bytes = (const UCHAR*)buffer;
            ++gStats.Report02Reads;
            gStats.SnapshotValid = 1;
            gStats.RawFlags = bytes[1];
            gStats.RawPressure = (ULONG)bytes[6] | ((ULONG)bytes[7] << 8);
            if (GalaxyRequestContext(Request)->Generation == gEngine.Generation && gReadyCount == 1)
                GalaxyPenRecordContact(&gStats, gStats.RawFlags, gStats.RawPressure);
            if (gReadyCount == 1 && gStats.AttachedDevices == 1 &&
                GalaxyRequestContext(Request)->Generation == gEngine.Generation) gReportSeen = TRUE;
#ifndef GALAXY_PEN_PROBE_READ
            if (GalaxyReadyForCorrection()) {
                gStats.ModifiedReports += GalaxyPenEngineProcess(&gEngine, (UCHAR*)buffer,
                    (size_t)information, GalaxyRequestContext(Request)->Generation, KeQueryInterruptTime());
            }
#endif
            gStats.OutputFlags = bytes[1];
            gStats.OutputPressure = (ULONG)bytes[6] | ((ULONG)bytes[7] << 8);
        }
    }
    WdfSpinLockRelease(gConfigLock);
    WdfRequestCompleteWithInformation(Request, status, information);
}
