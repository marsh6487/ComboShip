/* Included from the native recorder TUs after Path/recording state definitions.
 * Only the isolated soul child is rolled back; earlier actor records are retained. */
#ifdef COMBO_BUILD
namespace {
struct ComboSoulRecording {
    bool recording;
    bool nextActor;
    bool hasInverse;
    size_t inverseIndex;
    MtxF inverse;
#ifdef COMBO_SOUL_RECORDER_MM
    bool ignoreInverse;
    size_t ignoreIndex;
#endif
    size_t depth;
    Path* parent;
    label key;
    size_t children, operations, items;
    bool hadChild, hadOperations;
};
static void ComboRestoreSoulRecording(ComboSoulRecording* state, bool rollback) noexcept {
    if (!state)
        return;
    if (state->recording) {
        if (current_path.size() >= state->depth)
            current_path.resize(state->depth);
        if (rollback) {
            auto child = state->parent->children.find(state->key);
            if (child != state->parent->children.end()) {
                if (state->hadChild)
                    child->second.resize(state->children);
                else
                    state->parent->children.erase(child);
            }
            auto ops = state->parent->ops.find(Op::OpenChild);
            if (ops != state->parent->ops.end()) {
                if (state->hadOperations)
                    ops->second.resize(state->operations);
                else
                    state->parent->ops.erase(ops);
            }
            state->parent->items.resize(state->items);
        }
        next_is_actor_pos_rot_matrix = state->nextActor;
        has_inv_actor_mtx = state->hasInverse;
        inv_actor_mtx_path_index = state->inverseIndex;
        inv_actor_mtx = state->inverse;
#ifdef COMBO_SOUL_RECORDER_MM
        ignore_inv_actor_mtx = state->ignoreInverse;
        ignore_inv_actor_mtx_path_index = state->ignoreIndex;
#endif
    }
    is_recording = state->recording;
}
} // namespace
extern "C" void* FrameInterpolation_BeginSoulDraw(const void* key) {
    if (!is_recording || current_path.empty())
        return nullptr;
    auto* state = new ComboSoulRecording{};
    state->recording = is_recording;
    state->nextActor = next_is_actor_pos_rot_matrix;
    state->hasInverse = has_inv_actor_mtx;
    state->inverseIndex = inv_actor_mtx_path_index;
    state->inverse = inv_actor_mtx;
#ifdef COMBO_SOUL_RECORDER_MM
    state->ignoreInverse = ignore_inv_actor_mtx;
    state->ignoreIndex = ignore_inv_actor_mtx_path_index;
#endif
    state->depth = current_path.size();
    state->parent = current_path.back();
    state->key = { key, 0 };
    const auto child = state->parent->children.find(state->key);
    state->hadChild = child != state->parent->children.end();
    state->children = child == state->parent->children.end() ? 0 : child->second.size();
    const auto ops = state->parent->ops.find(Op::OpenChild);
    state->hadOperations = ops != state->parent->ops.end();
    state->operations = ops == state->parent->ops.end() ? 0 : ops->second.size();
    state->items = state->parent->items.size();
    try {
        FrameInterpolation_RecordOpenChild(key, 0);
    } catch (...) {
        ComboRestoreSoulRecording(state, true);
        delete state;
        throw;
    }
    return state;
}
extern "C" void FrameInterpolation_EndSoulDraw(void* opaque, int rollback) noexcept {
    auto* state = static_cast<ComboSoulRecording*>(opaque);
    ComboRestoreSoulRecording(state, rollback != 0);
    delete state;
}
#endif
