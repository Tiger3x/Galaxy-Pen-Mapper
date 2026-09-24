#pragma once

// Separate protocol: no mapper configuration or input-writing operation exists.
#define GALAXY_DIAG_VERSION 1u
#define GALAXY_DIAG_BATCH 64u
#define GALAXY_DIAG_RING 2048u
#define GALAXY_DIAG_LEASE 300000000ULL
#define GALAXY_DIAG_START CTL_CODE(FILE_DEVICE_UNKNOWN, 0x910, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define GALAXY_DIAG_DRAIN CTL_CODE(FILE_DEVICE_UNKNOWN, 0x911, METHOD_BUFFERED, FILE_READ_ACCESS)
#define GALAXY_DIAG_STOP CTL_CODE(FILE_DEVICE_UNKNOWN, 0x912, METHOD_BUFFERED, FILE_WRITE_ACCESS)

typedef struct _GALAXY_DIAG_START_DATA {
    unsigned int Version, Reserved;
    unsigned long long Session, Key0, Key1;
} GALAXY_DIAG_START_DATA;

typedef struct _GALAXY_DIAG_RECORD {
    unsigned long long Session, Sequence, Token, Begin, End;
    unsigned int Status, Length;
    unsigned char Report[15], Valid;
} GALAXY_DIAG_RECORD;

typedef struct _GALAXY_DIAG_BATCH_DATA {
    unsigned int Version, Role, Count, Attached, Ready, Active;
    unsigned long long Session, Lost, Started, Completed;
    GALAXY_DIAG_RECORD Records[GALAXY_DIAG_BATCH];
} GALAXY_DIAG_BATCH_DATA;

typedef struct _GALAXY_DIAG_BUFFER {
    unsigned int Head, Count;
    unsigned long long Lost, Sequence;
    GALAXY_DIAG_RECORD Records[GALAXY_DIAG_RING];
} GALAXY_DIAG_BUFFER;

// Caller owns synchronization. Full buffers drop observations, never input.
static __inline void GalaxyDiagPush(GALAXY_DIAG_BUFFER* ring, GALAXY_DIAG_RECORD* record)
{
    record->Sequence = ++ring->Sequence;
    if (ring->Count == GALAXY_DIAG_RING) { ++ring->Lost; return; }
    ring->Records[(ring->Head + ring->Count) % GALAXY_DIAG_RING] = *record;
    ++ring->Count;
}
static __inline unsigned int GalaxyDiagPop(GALAXY_DIAG_BUFFER* ring, GALAXY_DIAG_RECORD* records)
{
    unsigned int n = 0;
    while (n < GALAXY_DIAG_BATCH && ring->Count) {
        records[n++] = ring->Records[ring->Head];
        ring->Head = (ring->Head + 1) % GALAXY_DIAG_RING;
        --ring->Count;
    }
    return n;
}
