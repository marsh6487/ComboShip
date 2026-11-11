#ifndef COMBO_STARTUP_CHECK_RECEIPT_H
#define COMBO_STARTUP_CHECK_RECEIPT_H

#include <stdint.h>

#define CW_STARTUP_RECEIPT_VERSION 1
#define CW_STARTUP_SOURCE "Song from Impa (OoT)"
/* Separate from the fixed market and vanilla-reward token namespaces. */
#define CW_STARTUP_RECEIPT_TOKEN_BIT 0x20000000

enum CwStartupReceiptState { CW_STARTUP_READY = 1, CW_STARTUP_WAIT_MM = 2 };
enum CwStartupReceiptImportance { CW_STARTUP_JUNK = 0, CW_STARTUP_MINOR = 1, CW_STARTUP_MAJOR = 2 };
enum CwStartupReceiptAction {
    CW_STARTUP_BEGIN_FILE = 1,
    CW_STARTUP_CAPTURE = 2,
    CW_STARTUP_PEEK = 3,
    CW_STARTUP_ACKNOWLEDGE = 4,
    CW_STARTUP_PREPARE_PEER_SNAPSHOT = 5,
    CW_STARTUP_PREPARE_CONTEXT_CHANGE = 6
};

/* Only copied values cross DLL/save boundaries. The original concrete OoT
 * entry can be reconstructed without resolving another progressive tier.
 * For an MM-owned reward, getItemId is the resolved RandoItemId. */
typedef struct CwStartupReceipt {
    uint32_t version;
    uint32_t generation;
    int32_t state;
    int32_t itemGame;
    int32_t modIndex;
    int32_t getItemId;
    int32_t itemId;
    int32_t tableId;
    int32_t objectId;
    int32_t field;
    int32_t gi;
    int32_t gid;
    int32_t drawItemId;
    int32_t drawModIndex;
    int32_t category;   /* Frozen native OoT category, 0..6. */
    int32_t importance; /* Junk/minor/major, for native MM policy. */
    int32_t dungeonInfo;
    int32_t trap;
    char itemName[256];    /* Concrete owner name; raw MM placement only while WAIT_MM. */
    char displayName[256]; /* Human-readable name captured before the award. */
} CwStartupReceipt;

/* BEGIN/CAPTURE are called only by OoT's new randomized-save initialization.
 * PEEK/ACK are called by the foreground presentation adapter. Return 1 on
 * success, 0 when unavailable, -1 on a rejected/malformed request. ACK is
 * idempotent for an already shown matching generation; a missing, malformed
 * or replaced generation returns -1 so adapters discard stale retry state. */
typedef int32_t (*CwStartupReceiptHost)(int32_t action, int32_t game, int32_t slot, CwStartupReceipt* receipt);
typedef void (*CwSetStartupReceiptHost)(CwStartupReceiptHost host);
/* WAIT_MM resolves/grants/shares once and changes the supplied state to READY
 * before any shared callback. The launcher publishes that local result under
 * its save mutex, then invokes READY to capture native state without another
 * conversion, award or shared pickup. */
typedef int32_t (*CwFinishMmStartupReceipt)(CwStartupReceipt* receipt);

#endif
