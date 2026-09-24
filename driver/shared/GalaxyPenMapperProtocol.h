#pragma once
#include "GalaxyPenPressureLimits.h"

#define GALAXY_PEN_IOCTL_SET_CONFIG CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define GALAXY_PEN_IOCTL_QUERY_CONFIG CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_READ_ACCESS)
#define GALAXY_PEN_IOCTL_QUERY_PROBE_STATS CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED, FILE_READ_ACCESS)
#define GALAXY_PEN_PROTOCOL_VERSION 4u
#define GALAXY_PEN_PROBE_PROTOCOL_VERSION 5u
#define GALAXY_PEN_CONTROL_PATH L"\\\\.\\GalaxyPenMapperControlV4"
#define GALAXY_PEN_DEFAULT_FLOOR_PERCENT 5u
#define GALAXY_PEN_CAP_OBSERVE 0x1u
#define GALAXY_PEN_CAP_TOOL 0x2u
#define GALAXY_PEN_CAP_PRESSURE 0x4u
#define GALAXY_PEN_STATE_WAIT_FOR_LIFT 0x1u

typedef struct _GALAXY_PEN_CONFIG {
    unsigned int Version;
    unsigned int Enabled;
    unsigned int SensitivityPermille;
    unsigned int StateFlags;
    unsigned int PressureEnabled;
    unsigned int ContactFloorPercent;
} GALAXY_PEN_CONFIG;

typedef struct _GALAXY_PEN_PROBE_STATS {
    unsigned int Version;
    unsigned int CompletedReads;
    unsigned int Successful15ByteReads;
    unsigned int Report02Reads;
    unsigned int LastStatus;
    unsigned int LastBytes;
    unsigned int LastReportId;
    unsigned int Capabilities;
    unsigned int AttachedDevices;
    unsigned int ReadyForCorrection;
    unsigned int StartedReads;
    unsigned int ForwardFailures;
    unsigned int PendingReads;
    unsigned int BufferFailures;
    unsigned int ModifiedReports;
    unsigned int SnapshotValid;
    unsigned int RawFlags;
    unsigned int OutputFlags;
    unsigned int RawPressure;
    unsigned int OutputPressure;
    unsigned int ContactReports;
    unsigned int SaturatedContactReports;
    unsigned int ContactRawMin;
    unsigned int ContactRawMax;
} GALAXY_PEN_PROBE_STATS;
