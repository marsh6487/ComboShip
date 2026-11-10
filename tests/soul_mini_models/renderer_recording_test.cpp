#include <cassert>
#include <map>
#include <stdexcept>
#include <utility>
#include <vector>
#include <iostream>
using namespace std;
struct MtxF { float value = 0; };
using label = pair<const void*, int>;
enum class Op { OpenChild, MatrixPush };
struct Data { struct { label key; size_t idx; } open_child; };
struct Path {
    map<label, vector<Path>> children;
    map<Op, vector<Data>> ops;
    vector<pair<Op, size_t>> items;
};
static bool is_recording = true, next_is_actor_pos_rot_matrix = true, has_inv_actor_mtx = true;
static size_t inv_actor_mtx_path_index = 7;
static MtxF inv_actor_mtx{42};
#ifdef COMBO_SOUL_RECORDER_MM
static bool ignore_inv_actor_mtx = true;
static size_t ignore_inv_actor_mtx_path_index = 8;
#endif
static Path root;
static vector<Path*> current_path{&root};
static bool throwAppend;
static Data& append(Op op) {
    if (throwAppend) { throwAppend = false; throw runtime_error("injected recorder allocation"); }
    auto& m = current_path.back()->ops[op];
    current_path.back()->items.emplace_back(op, m.size());
    return m.emplace_back();
}
#include "native_recording.inc"
#include "ComboFrameInterpolationTransaction.h"
int main() {
    append(Op::MatrixPush);
    const size_t originalItems = root.items.size();
    const void* id = (void*)41;
    void* handle = FrameInterpolation_BeginSoulDraw(id);
    FrameInterpolation_RecordOpenChild((void*)42, 0);
    append(Op::MatrixPush);
    next_is_actor_pos_rot_matrix = false; has_inv_actor_mtx = false; inv_actor_mtx.value = 99;
#ifdef COMBO_SOUL_RECORDER_MM
    ignore_inv_actor_mtx = false;
#endif
    FrameInterpolation_EndSoulDraw(handle, 1);
    assert(current_path.size() == 1 && current_path.back() == &root && is_recording);
    assert(root.items.size() == originalItems && root.children.empty() && !root.ops.contains(Op::OpenChild));
    assert(root.ops[Op::MatrixPush].size() == 1);
    assert(next_is_actor_pos_rot_matrix && has_inv_actor_mtx && inv_actor_mtx.value == 42 && inv_actor_mtx_path_index == 7);
#ifdef COMBO_SOUL_RECORDER_MM
    assert(ignore_inv_actor_mtx && ignore_inv_actor_mtx_path_index == 8);
#endif
    // A failed Begin itself must remove its half-recorded parent OpenChild.
    throwAppend = true;
    try { FrameInterpolation_BeginSoulDraw(id); assert(false); } catch (const runtime_error&) {}
    assert(root.children.empty() && current_path.size() == 1 && root.items.size() == originalItems);
    // Committed records remain available, and rollback of a second same-key item
    // removes only that new child, retaining the first complete item's matrices.
    handle = FrameInterpolation_BeginSoulDraw(id); append(Op::MatrixPush);
    FrameInterpolation_EndSoulDraw(handle, 0);
    assert(root.children.at({id,0}).size() == 1 && root.items.size() == originalItems + 1);
    handle = FrameInterpolation_BeginSoulDraw(id); append(Op::MatrixPush);
    FrameInterpolation_RecordOpenChild((void*)99, 0);
    FrameInterpolation_EndSoulDraw(handle, 1);
    assert(root.children.at({id,0}).size() == 1 && root.items.size() == originalItems + 1 && current_path.size() == 1);
    is_recording = false; assert(!FrameInterpolation_BeginSoulDraw(id)); FrameInterpolation_EndSoulDraw(nullptr, 1);
    cout << "Native recorder: nested failed draw, partial Begin, inverse actor flags and earlier committed child preservation passed\n";
}
