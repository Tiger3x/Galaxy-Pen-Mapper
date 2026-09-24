#include <ntddk.h>
#include <wdf.h>
#include "shared/GalaxyPenDiag.h"
#include "shared/GalaxyPenDiagToken.h"

#if GALAXY_DIAG_ROLE == 1
#define DIAG_DEVICE L"\\Device\\GalaxyPenDiagA1"
#define DIAG_LINK L"\\DosDevices\\GalaxyPenDiagA1"
#elif GALAXY_DIAG_ROLE == 2
#define DIAG_DEVICE L"\\Device\\GalaxyPenDiagB1"
#define DIAG_LINK L"\\DosDevices\\GalaxyPenDiagB1"
#else
#error Select diagnostic role A or B explicitly.
#endif

DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_DEVICE_ADD DiagAdd;
EVT_WDF_DEVICE_CONTEXT_CLEANUP DiagCleanup;
EVT_WDF_DEVICE_D0_ENTRY DiagEntry;
EVT_WDF_DEVICE_D0_EXIT DiagExit;
EVT_WDF_FILE_CLEANUP DiagClose;
EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL DiagControl;
EVT_WDF_IO_QUEUE_IO_READ DiagRead;
EVT_WDF_REQUEST_COMPLETION_ROUTINE DiagComplete;

typedef struct { BOOLEAN Registered, Ready; } DIAG_DEVICE_CONTEXT;
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DIAG_DEVICE_CONTEXT, DiagDevice);
typedef struct { ULONGLONG Generation; GALAXY_DIAG_RECORD Record; } DIAG_REQUEST_CONTEXT;
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DIAG_REQUEST_CONTEXT, DiagRequest);
static WDFSPINLOCK gLock;
static WDFWAITLOCK gLifecycle;
static WDFDEVICE gControl;
static ULONG gDevices, gReady;
static ULONGLONG gGeneration, gSession, gKey0, gKey1, gUntil, gStarted, gCompleted;
static GALAXY_DIAG_BUFFER gRing;

// gLock held: stop recording and erase private keys. Never changes input state.
static VOID DiagDisarm(VOID)
{
    ++gGeneration; gUntil = 0; gKey0 = gKey1 = 0;
}
static NTSTATUS DiagCreateControl(WDFDRIVER driver)
{
    DECLARE_CONST_UNICODE_STRING(name, DIAG_DEVICE);
    DECLARE_CONST_UNICODE_STRING(link, DIAG_LINK);
    DECLARE_CONST_UNICODE_STRING(sddl, L"D:P(A;;GA;;;SY)(A;;GA;;;BA)");
    PWDFDEVICE_INIT init = WdfControlDeviceInitAllocate(driver, &sddl);
    WDFDEVICE device;
    WDF_FILEOBJECT_CONFIG files;
    WDF_IO_QUEUE_CONFIG queue;
    NTSTATUS status;
    if (!init) return STATUS_INSUFFICIENT_RESOURCES;
    WdfDeviceInitSetDeviceType(init, FILE_DEVICE_UNKNOWN);
    WdfDeviceInitSetCharacteristics(init, FILE_DEVICE_SECURE_OPEN, FALSE);
    WdfDeviceInitSetExclusive(init, TRUE);
    WDF_FILEOBJECT_CONFIG_INIT(&files, WDF_NO_EVENT_CALLBACK, WDF_NO_EVENT_CALLBACK, DiagClose);
    WdfDeviceInitSetFileObjectConfig(init, &files, WDF_NO_OBJECT_ATTRIBUTES);
    status = WdfDeviceInitAssignName(init, &name);
    if (!NT_SUCCESS(status)) { WdfDeviceInitFree(init); return status; }
    status = WdfDeviceCreate(&init, WDF_NO_OBJECT_ATTRIBUTES, &device);
    if (!NT_SUCCESS(status)) { if (init) WdfDeviceInitFree(init); return status; }
    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&queue, WdfIoQueueDispatchSequential);
    queue.PowerManaged = WdfFalse; queue.EvtIoDeviceControl = DiagControl;
    status = WdfIoQueueCreate(device, &queue, WDF_NO_OBJECT_ATTRIBUTES, WDF_NO_HANDLE);
    if (NT_SUCCESS(status)) status = WdfDeviceCreateSymbolicLink(device, &link);
    if (!NT_SUCCESS(status)) { WdfObjectDelete(device); return status; }
    gControl = device; WdfControlFinishInitializing(device); return STATUS_SUCCESS;
}
NTSTATUS DriverEntry(PDRIVER_OBJECT object, PUNICODE_STRING path)
{
    WDF_DRIVER_CONFIG config;
    WDF_OBJECT_ATTRIBUTES attributes;
    WDFDRIVER driver;
    NTSTATUS status;
    WDF_DRIVER_CONFIG_INIT(&config, DiagAdd);
    status = WdfDriverCreate(object, path, WDF_NO_OBJECT_ATTRIBUTES, &config, &driver);
    if (!NT_SUCCESS(status)) return status;
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes); attributes.ParentObject = driver;
    status = WdfSpinLockCreate(&attributes, &gLock);
    if (!NT_SUCCESS(status)) return status;
    return WdfWaitLockCreate(&attributes, &gLifecycle);
}
VOID DiagClose(WDFFILEOBJECT file)
{
    UNREFERENCED_PARAMETER(file);
    WdfSpinLockAcquire(gLock); DiagDisarm(); gSession = 0;
    RtlZeroMemory(&gRing, sizeof(gRing)); WdfSpinLockRelease(gLock);
}
VOID DiagControl(WDFQUEUE queue, WDFREQUEST request, size_t outLength, size_t inLength, ULONG code)
{
    NTSTATUS status = STATUS_INVALID_DEVICE_REQUEST;
    ULONG_PTR written = 0;
    UNREFERENCED_PARAMETER(queue);
    if (code == GALAXY_DIAG_START) {
        GALAXY_DIAG_START_DATA* start = NULL;
        if (inLength != sizeof(*start) || outLength != 0) status = STATUS_INFO_LENGTH_MISMATCH;
        else status = WdfRequestRetrieveInputBuffer(request, sizeof(*start), (PVOID*)&start, NULL);
        if (NT_SUCCESS(status)) {
            if (start->Version != GALAXY_DIAG_VERSION || start->Reserved || !start->Session ||
                !(start->Key0 | start->Key1)) status = STATUS_INVALID_PARAMETER;
            else {
                WdfSpinLockAcquire(gLock);
                if (gDevices != 1 || gReady != 1) status = STATUS_DEVICE_NOT_READY;
                else {
                    DiagDisarm(); RtlZeroMemory(&gRing, sizeof(gRing));
                    gStarted = gCompleted = 0; gSession = start->Session;
                    gKey0 = start->Key0; gKey1 = start->Key1;
                    gUntil = KeQueryInterruptTime() + GALAXY_DIAG_LEASE;
                }
                WdfSpinLockRelease(gLock);
            }
        }
    } else if (code == GALAXY_DIAG_DRAIN) {
        GALAXY_DIAG_BATCH_DATA* batch = NULL;
        if (inLength || outLength != sizeof(*batch)) status = STATUS_INFO_LENGTH_MISMATCH;
        else status = WdfRequestRetrieveOutputBuffer(request, sizeof(*batch), (PVOID*)&batch, NULL);
        if (NT_SUCCESS(status)) {
            RtlZeroMemory(batch, sizeof(*batch));
            WdfSpinLockAcquire(gLock);
            if (gUntil && KeQueryInterruptTime() >= gUntil) DiagDisarm();
            if (gUntil) gUntil = KeQueryInterruptTime() + GALAXY_DIAG_LEASE;
            batch->Version = GALAXY_DIAG_VERSION; batch->Role = GALAXY_DIAG_ROLE;
            batch->Attached = gDevices; batch->Session = gSession;
            batch->Ready = gReady; batch->Active = gUntil != 0;
            batch->Lost = gRing.Lost; batch->Started = gStarted; batch->Completed = gCompleted;
            batch->Count = GalaxyDiagPop(&gRing, batch->Records);
            WdfSpinLockRelease(gLock); written = sizeof(*batch);
        }
    } else if (code == GALAXY_DIAG_STOP && !inLength && !outLength) {
        WdfSpinLockAcquire(gLock); DiagDisarm(); WdfSpinLockRelease(gLock); status = STATUS_SUCCESS;
    }
    WdfRequestCompleteWithInformation(request, status, written);
}
NTSTATUS DiagAdd(WDFDRIVER driver, PWDFDEVICE_INIT init)
{
    WDF_OBJECT_ATTRIBUTES attributes;
    WDF_PNPPOWER_EVENT_CALLBACKS power;
    WDF_IO_QUEUE_CONFIG queue;
    WDFDEVICE device;
    WDFQUEUE reads;
    NTSTATUS status;
    WdfFdoInitSetFilter(init);
    WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&power); power.EvtDeviceD0Entry = DiagEntry; power.EvtDeviceD0Exit = DiagExit;
    WdfDeviceInitSetPnpPowerEventCallbacks(init, &power);
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, DIAG_REQUEST_CONTEXT);
    WdfDeviceInitSetRequestAttributes(init, &attributes);
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, DIAG_DEVICE_CONTEXT);
    attributes.EvtCleanupCallback = DiagCleanup;
    status = WdfDeviceCreate(&init, &attributes, &device);
    if (!NT_SUCCESS(status)) return status;
    WDF_IO_QUEUE_CONFIG_INIT(&queue, WdfIoQueueDispatchParallel);
    queue.PowerManaged = WdfFalse; queue.AllowZeroLengthRequests = TRUE; queue.EvtIoRead = DiagRead;
    status = WdfIoQueueCreate(device, &queue, WDF_NO_OBJECT_ATTRIBUTES, &reads);
    if (NT_SUCCESS(status)) status = WdfDeviceConfigureRequestDispatching(device, reads, WdfRequestTypeRead);
    if (!NT_SUCCESS(status)) return status;
    WdfWaitLockAcquire(gLifecycle, NULL);
    if (!gDevices) status = DiagCreateControl(driver);
    if (NT_SUCCESS(status)) {
        WdfSpinLockAcquire(gLock); ++gDevices; DiagDevice(device)->Registered = TRUE; DiagDisarm(); WdfSpinLockRelease(gLock);
    }
    WdfWaitLockRelease(gLifecycle); return status;
}
VOID DiagCleanup(WDFOBJECT object)
{
    DIAG_DEVICE_CONTEXT* context = DiagDevice((WDFDEVICE)object);
    WdfWaitLockAcquire(gLifecycle, NULL);
    if (context->Registered) {
        WdfSpinLockAcquire(gLock); context->Registered = FALSE; --gDevices;
        if (context->Ready) { context->Ready = FALSE; --gReady; }
        DiagDisarm(); WdfSpinLockRelease(gLock);
        if (!gDevices && gControl) { WDFDEVICE old = gControl; gControl = NULL; WdfObjectDelete(old); }
    }
    WdfWaitLockRelease(gLifecycle);
}
NTSTATUS DiagEntry(WDFDEVICE device, WDF_POWER_DEVICE_STATE old)
{
    UNREFERENCED_PARAMETER(old);
    WdfSpinLockAcquire(gLock);
    if (!DiagDevice(device)->Ready) { DiagDevice(device)->Ready = TRUE; ++gReady; }
    DiagDisarm(); WdfSpinLockRelease(gLock); return STATUS_SUCCESS;
}
NTSTATUS DiagExit(WDFDEVICE device, WDF_POWER_DEVICE_STATE next)
{
    UNREFERENCED_PARAMETER(next);
    WdfSpinLockAcquire(gLock);
    if (DiagDevice(device)->Ready) { DiagDevice(device)->Ready = FALSE; --gReady; }
    DiagDisarm(); WdfSpinLockRelease(gLock); return STATUS_SUCCESS;
}
static VOID DiagObserve(WDFREQUEST request, NTSTATUS status, ULONG_PTR length)
{
    DIAG_REQUEST_CONTEXT* context = DiagRequest(request);
    GALAXY_DIAG_RECORD record = context->Record;
    PVOID buffer = NULL;
    size_t available = 0;
    record.End = KeQueryInterruptTime(); record.Status = (ULONG)status;
    record.Length = length > MAXULONG ? MAXULONG : (ULONG)length;
    if (context->Generation && NT_SUCCESS(status) && length == 15 &&
        NT_SUCCESS(WdfRequestRetrieveOutputBuffer(request, 15, &buffer, &available)) && available >= 15) {
        RtlCopyMemory(record.Report, buffer, 15); record.Valid = 1;
    }
    WdfSpinLockAcquire(gLock);
    if (gUntil && record.End >= gUntil) DiagDisarm();
    if (gUntil && context->Generation == gGeneration && gDevices == 1 && gReady == 1) {
        ++gCompleted; GalaxyDiagPush(&gRing, &record);
    }
    WdfSpinLockRelease(gLock);
}
VOID DiagRead(WDFQUEUE queue, WDFREQUEST request, size_t length)
{
    DIAG_REQUEST_CONTEXT* context = DiagRequest(request);
    WDFDEVICE device = WdfIoQueueGetDevice(queue);
    UNREFERENCED_PARAMETER(length);
    RtlZeroMemory(context, sizeof(*context));
    WdfSpinLockAcquire(gLock);
    if (gUntil && KeQueryInterruptTime() >= gUntil) DiagDisarm();
    if (gUntil && gDevices == 1 && gReady == 1) {
        context->Generation = gGeneration; context->Record.Session = gSession;
        context->Record.Begin = KeQueryInterruptTime();
        context->Record.Token = GalaxyDiagToken((ULONGLONG)(ULONG_PTR)WdfRequestWdmGetIrp(request), gKey0, gKey1);
        ++gStarted;
    }
    WdfSpinLockRelease(gLock);
    WdfRequestFormatRequestUsingCurrentType(request);
    WdfRequestSetCompletionRoutine(request, DiagComplete, NULL);
    if (!WdfRequestSend(request, WdfDeviceGetIoTarget(device), WDF_NO_SEND_OPTIONS)) {
        NTSTATUS status = WdfRequestGetStatus(request); DiagObserve(request, status, 0);
        WdfRequestComplete(request, status);
    }
}
VOID DiagComplete(WDFREQUEST request, WDFIOTARGET target, PWDF_REQUEST_COMPLETION_PARAMS params, WDFCONTEXT context)
{
    UNREFERENCED_PARAMETER(target); UNREFERENCED_PARAMETER(context);
    DiagObserve(request, params->IoStatus.Status, params->IoStatus.Information);
    // Original buffer, status, byte count and request are forwarded unchanged.
    WdfRequestCompleteWithInformation(request, params->IoStatus.Status, params->IoStatus.Information);
}
