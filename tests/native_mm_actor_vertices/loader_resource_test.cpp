#include <cassert>
#include <cstdio>
#include <memory>
#include <fast/resource/type/Vertex.h>
#include <libultraship/libultra/gbi.h>
#include "soh/soh/resource/type/Array.h"
#include "soh/mods/transformation_masks/assets/mm_display_list_patch.h"

// Compile the loader's actual selection call against the resource it consumes.
// Archive lookup is outside this contract; storage, resource methods and patching are real.
static bool Select(const std::shared_ptr<Fast::Vertex>& vertex, MmDisplayListVertexResourceView* vertexView) {
    std::shared_ptr<SOH::Array> vertexArray;
    return /* PRODUCTION_SELECTION */;
}

static uintptr_t Resolve(void* context, MmDisplayListReferenceKind kind, uint64_t hash,
                         MmDisplayListVertexResourceView* view) {
    assert(kind == MM_DISPLAY_LIST_REFERENCE_VERTEX && hash == 1);
    auto& resource = *static_cast<std::shared_ptr<Fast::Vertex>*>(context);
    assert(Select(resource, view));
    return view->pointer;
}

int main() {
#ifdef GBI_S32_VTX
    constexpr size_t expectedStride = 24;
#else
    constexpr size_t expectedStride = 16;
#endif
    auto vertex = std::make_shared<Fast::Vertex>();
    vertex->VertexList.resize(3);
    vertex->VertexList[1].v.ob[0] = 1234;
    for (uint32_t recordSize : {16u, 22u}) {
        vertex->RecordSize = recordSize;
        MmDisplayListVertexResourceView view{};
        assert(Select(vertex, &view));
        assert(view.pointer == reinterpret_cast<uintptr_t>(vertex->VertexList.data()));
        assert(view.stride == expectedStride && view.size == 3 * expectedStride);
        assert(view.recordSize == recordSize);
        MmDisplayListCommand commands[] = {{0x32001002, recordSize}, {0, 1}, {0xDF000000, 0}};
        MmDisplayListPatchStats stats{};
        assert(MmDisplayList_PatchCommands(commands, 3, Resolve, &vertex, &stats));
        assert(stats.verticesPatched == 1 && stats.malformed == 0 && stats.unresolved == 0);
        assert(commands[0].w1 == reinterpret_cast<uintptr_t>(&vertex->VertexList[1]));
        assert(reinterpret_cast<Vtx*>(commands[0].w1)->v.ob[0] == 1234);
    }
    std::printf("PASS production loader selection: real Fast::Vertex, %zu-byte runtime storage, legacy/wide archive records\n",
                expectedStride);
}
