#include <ntddk.h>
#include <wdf.h>

DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_DEVICE_ADD GalaxyPenEvtDeviceAdd;

#ifdef GALAXY_PEN_PROBE_READ
EVT_WDF_IO_QUEUE_IO_READ GalaxyPenEvtIoRead;
EVT_WDF_REQUEST_COMPLETION_ROUTINE GalaxyPenReadComplete;

static volatile LONG64 gReadCompletions = 0;
#endif

NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    )
{
    WDF_DRIVER_CONFIG config;

    WDF_DRIVER_CONFIG_INIT(&config, GalaxyPenEvtDeviceAdd);
    return WdfDriverCreate(DriverObject, RegistryPath,
                           WDF_NO_OBJECT_ATTRIBUTES, &config, WDF_NO_HANDLE);
}

NTSTATUS
GalaxyPenEvtDeviceAdd(
    _In_ WDFDRIVER Driver,
    _Inout_ PWDFDEVICE_INIT DeviceInit
    )
{
    WDFDEVICE device;
    NTSTATUS status;

#ifdef GALAXY_PEN_PROBE_READ
    WDF_IO_QUEUE_CONFIG queueConfig;
    WDFQUEUE readQueue;
#endif

    UNREFERENCED_PARAMETER(Driver);

    WdfFdoInitSetFilter(DeviceInit);
    status = WdfDeviceCreate(&DeviceInit, WDF_NO_OBJECT_ATTRIBUTES, &device);
    if (!NT_SUCCESS(status)) {
        return status;
    }

#ifdef GALAXY_PEN_PROBE_READ
    // A non-default queue receives only reads; all other I/O is auto-forwarded.
    WDF_IO_QUEUE_CONFIG_INIT(&queueConfig, WdfIoQueueDispatchParallel);
    queueConfig.AllowZeroLengthRequests = TRUE;
    queueConfig.EvtIoRead = GalaxyPenEvtIoRead;
    status = WdfIoQueueCreate(device, &queueConfig,
                              WDF_NO_OBJECT_ATTRIBUTES, &readQueue);
    if (!NT_SUCCESS(status)) {
        return status;
    }
    return WdfDeviceConfigureRequestDispatching(device, readQueue, WdfRequestTypeRead);
#else
    // No queue: KMDF forwards every request without this driver touching it.
    return STATUS_SUCCESS;
#endif
}

#ifdef GALAXY_PEN_PROBE_READ
VOID
GalaxyPenEvtIoRead(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t Length
    )
{
    WDFDEVICE device = WdfIoQueueGetDevice(Queue);

    UNREFERENCED_PARAMETER(Length);

    WdfRequestFormatRequestUsingCurrentType(Request);
    WdfRequestSetCompletionRoutine(Request, GalaxyPenReadComplete, WDF_NO_CONTEXT);
    if (!WdfRequestSend(Request, WdfDeviceGetIoTarget(device), WDF_NO_SEND_OPTIONS)) {
        WdfRequestComplete(Request, WdfRequestGetStatus(Request));
    }
}

VOID
GalaxyPenReadComplete(
    _In_ WDFREQUEST Request,
    _In_ WDFIOTARGET Target,
    _In_ PWDF_REQUEST_COMPLETION_PARAMS CompletionParams,
    _In_ WDFCONTEXT Context
    )
{
    NTSTATUS status = CompletionParams->IoStatus.Status;
    ULONG_PTR information = WdfRequestGetInformation(Request);
    LONG64 count = InterlockedIncrement64(&gReadCompletions);

    UNREFERENCED_PARAMETER(Target);
    UNREFERENCED_PARAMETER(Context);

    // Report only the length and ID; never retain coordinates or pressure.
    if (count <= 8 || (count & (count - 1)) == 0) {
        UCHAR reportId = 0xFF;
        PVOID buffer = NULL;
        size_t bufferLength = 0;

        if (NT_SUCCESS(status) && information == 15 &&
            NT_SUCCESS(WdfRequestRetrieveOutputBuffer(Request, 1, &buffer, &bufferLength)) &&
            bufferLength >= information) {
            reportId = ((const UCHAR *)buffer)[0];
        }

        DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL,
                   "GalaxyPenReadProbe: completed=%lld status=0x%08X bytes=%Iu reportId=0x%02X\n",
                   count, status, information, reportId);
    }

    // Preserve the lower driver's status, transfer count, and output bytes.
    WdfRequestCompleteWithInformation(Request, status, information);
}
#endif
