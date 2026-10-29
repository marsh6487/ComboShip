#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdlib>

using s32 = int32_t;
using u32 = uint32_t;
using u64 = uint64_t;
using u8 = uint8_t;
#define SS_NULL 0xFFFFFFFFu
#define LOG_HUNGUP_THREAD() ((void)0)
struct Refused {};
static unsigned allocationCalls;
static size_t requestedBytes;
[[noreturn]] void TestAbort() { throw Refused{}; }
void* TestRealloc(void* ptr, size_t size) {
    ++allocationCalls; requestedBytes = size;
    if (size > 4096) return nullptr;
    return std::realloc(ptr, size);
}
struct SSNode { s32 polyId; u32 next; };
struct SSNodeList { u32 max, count; SSNode* tbl; u8* polyCheckTbl; };
struct DynaSSNodeList { SSNode* tbl; s32 count, max; };
#define realloc TestRealloc
#define abort TestAbort
/* ACTUAL NODE GROWTH */
#undef realloc
#undef abort

int main() {
    DynaSSNodeList d{ reinterpret_cast<SSNode*>(1), INT32_MAX, INT32_MAX };
    allocationCalls = 0;
    try { DynaSSNodeList_GetNextNodeIdx(&d); assert(false); } catch (const Refused&) {}
    assert(d.count == INT32_MAX && d.max == INT32_MAX && d.tbl == reinterpret_cast<SSNode*>(1));
    assert(allocationCalls == 0);
    d.count = d.max = 0x50000000;
    try { DynaSSNodeList_GetNextNodeIdx(&d); assert(false); } catch (const Refused&) {}
    assert(d.count == 0x50000000 && d.max == 0x50000000 && d.tbl == reinterpret_cast<SSNode*>(1));
    if (SIZE_MAX / sizeof(SSNode) >= INT32_MAX) {
        assert(allocationCalls == 1 && requestedBytes == (size_t)INT32_MAX * sizeof(SSNode));
    } else {
        assert(allocationCalls == 0);
    }
    SSNodeList s{ UINT32_MAX, UINT32_MAX, reinterpret_cast<SSNode*>(1), nullptr };
    allocationCalls = 0;
    try { SSNodeList_GetNextNodeIdx(&s); assert(false); } catch (const Refused&) {}
    assert(s.count == UINT32_MAX && s.max == UINT32_MAX && allocationCalls == 0);
    d = { static_cast<SSNode*>(std::malloc(2 * sizeof(SSNode))), 2, 2 };
    assert(DynaSSNodeList_GetNextNodeIdx(&d) == 2 && d.count == 3 && d.max == 4);
    std::free(d.tbl);
    s = { 2, 2, static_cast<SSNode*>(std::malloc(2 * sizeof(SSNode))), nullptr };
    assert(SSNodeList_GetNextNodeIdx(&s) == 2 && s.count == 3 && s.max == 4);
    std::free(s.tbl);
}
