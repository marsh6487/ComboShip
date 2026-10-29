#include "global.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct BgCheckOwnedBuffer {
    BgCheckGeometryBuffer buffer;
    struct BgCheckOwnedBuffer* next;
} BgCheckOwnedBuffer;

typedef struct BgCheckOwner {
    CollisionContext* context;
    u64 allocationEpoch;
    u64 roomEpoch;
    BgCheckOwnedBuffer* geometry;
    struct BgCheckOwner* next;
} BgCheckOwner;

static BgCheckOwner* sBgCheckOwners;
static u64 sBgCheckEpoch;

static BgCheckOwner* BgCheck_FindOwner(const CollisionContext* context) {
    BgCheckOwner* owner = sBgCheckOwners;
    while (owner != NULL && owner->context != context) {
        owner = owner->next;
    }
    return owner;
}

static u64 BgCheck_NextEpoch(void) {
    if (sBgCheckEpoch == UINT64_MAX) {
        abort();
    }
    return ++sBgCheckEpoch;
}

void BgCheck_BeginOwnership(CollisionContext* colCtx) {
    BgCheckOwner* owner = malloc(sizeof(*owner));
    if (owner == NULL) {
        abort();
    }
    // A first scene may give us uninitialized PlayState bytes. Only the external registry proves ownership.
    BgCheck_Free(colCtx);
    memset(colCtx, 0, sizeof(*colCtx));
    owner->context = colCtx;
    owner->allocationEpoch = BgCheck_NextEpoch();
    owner->roomEpoch = BgCheck_NextEpoch();
    owner->geometry = NULL;
    owner->next = sBgCheckOwners;
    sBgCheckOwners = owner;
}

void BgCheck_ReleaseOwnership(CollisionContext* colCtx) {
    BgCheckOwner** link = &sBgCheckOwners;
    BgCheckOwner* owner;
    BgCheckOwnedBuffer* buffer;
    while (*link != NULL && (*link)->context != colCtx) {
        link = &(*link)->next;
    }
    if (*link == NULL) {
        return;
    }
    owner = *link;
    *link = owner->next;
    buffer = owner->geometry;
    while (buffer != NULL) {
        BgCheckOwnedBuffer* next = buffer->next;
        free(buffer->buffer.address);
        free(buffer);
        buffer = next;
    }
    free(owner);
}

u64 BgCheck_GetAllocationEpoch(const CollisionContext* colCtx) {
    BgCheckOwner* owner = BgCheck_FindOwner(colCtx);
    return owner != NULL ? owner->allocationEpoch : 0;
}

u64 BgCheck_GetRoomEpoch(const CollisionContext* colCtx) {
    BgCheckOwner* owner = BgCheck_FindOwner(colCtx);
    return owner != NULL ? owner->roomEpoch : 0;
}

void BgCheck_InvalidateRoomSnapshots(CollisionContext* colCtx) {
    BgCheckOwner* owner = BgCheck_FindOwner(colCtx);
    if (owner != NULL) {
        owner->roomEpoch = BgCheck_NextEpoch();
    }
}

s32 BgCheck_TrackDynaGeometry(DynaCollisionContext* dyna, void* polys, size_t polyBytes, void* vertices,
                              size_t vertexBytes) {
    BgCheckOwner* owner = sBgCheckOwners;
    BgCheckOwnedBuffer* poly = NULL;
    BgCheckOwnedBuffer* vertex = NULL;
    while (owner != NULL && &owner->context->dyna != dyna) {
        owner = owner->next;
    }
    if (owner == NULL) {
        return false;
    }
    if (polys != NULL) {
        poly = malloc(sizeof(*poly));
        if (poly == NULL) {
            return false;
        }
        poly->buffer.address = polys;
        poly->buffer.size = polyBytes;
    }
    if (vertices != NULL) {
        vertex = malloc(sizeof(*vertex));
        if (vertex == NULL) {
            free(poly);
            return false;
        }
        vertex->buffer.address = vertices;
        vertex->buffer.size = vertexBytes;
    }
    // Publish ownership only after both descriptors exist; a failed request leaves the live owner unchanged.
    if (poly != NULL) {
        poly->next = owner->geometry;
        owner->geometry = poly;
    }
    if (vertex != NULL) {
        vertex->next = owner->geometry;
        owner->geometry = vertex;
    }
    return true;
}

size_t BgCheck_GetGeometryBuffers(const CollisionContext* colCtx, BgCheckGeometryBuffer* out, size_t capacity) {
    BgCheckOwner* owner = BgCheck_FindOwner(colCtx);
    BgCheckOwnedBuffer* buffer = owner != NULL ? owner->geometry : NULL;
    size_t count = 0;
    while (buffer != NULL) {
        if (out != NULL && count < capacity) {
            out[count] = buffer->buffer;
        }
        ++count;
        buffer = buffer->next;
    }
    return count;
}

s32 BgCheck_OwnsGeometryBuffer(const CollisionContext* colCtx, const void* address, size_t size) {
    BgCheckOwner* owner = BgCheck_FindOwner(colCtx);
    BgCheckOwnedBuffer* buffer = owner != NULL ? owner->geometry : NULL;
    while (buffer != NULL) {
        if (buffer->buffer.address == address && buffer->buffer.size == size) {
            return true;
        }
        buffer = buffer->next;
    }
    return false;
}
