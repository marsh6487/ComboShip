#include "ship/resource/Resource.h"
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <dlfcn.h>

template <typename T> static T Symbol(void* module, const char* name) {
    auto result = reinterpret_cast<T>(dlsym(module, name));
    if (!result)
        std::fprintf(stderr, "Missing %s: %s\n", name, dlerror());
    assert(result);
    return result;
}

static void* Open(const char* path) {
    void* module = dlopen(path, RTLD_NOW | RTLD_GLOBAL);
    if (!module)
        std::fprintf(stderr, "Cannot open %s: %s\n", path, dlerror());
    assert(module);
    return module;
}

int main(int argc, char** argv) {
    assert(argc == 3);
    auto* engine = Open(argv[1]);
    auto* plugin = Open(argv[2]);
    const auto createFast = Symbol<Ship::IResource* (*)(uint32_t)>(engine, "VertexDso_CreateFast");
    const auto createArray = Symbol<Ship::IResource* (*)(bool)>(plugin, "VertexMmDso_CreateArray");
    const auto recordSize = Symbol<uint32_t (*)(Ship::IResource*)>(engine, "VertexDso_RecordSize");
    const auto checkData = Symbol<bool (*)(Ship::IResource*, int32_t, int32_t)>(engine, "VertexDso_CheckData");
    const auto destroy = Symbol<void (*)(Ship::IResource*)>(engine, "VertexDso_Destroy");
    const auto engineType = Symbol<const void* (*)()>(engine, "VertexDso_InterfaceTypeInfo");
    const auto pluginType = Symbol<const void* (*)()>(plugin, "VertexMmDso_InterfaceTypeInfo");
    // Hidden MM RTTI must really be local; the successful cast then proves the
    // toolchain's cross-DSO operation instead of accidental symbol interposition.
    assert(engineType() != pluginType());
    for (uint32_t stride : { 16u, 22u }) {
        auto* resource = createFast(stride);
        assert(recordSize(resource) == stride && checkData(resource, 80000, -90000));
        destroy(resource);
    }
    for (int iteration = 0; iteration < 8; ++iteration) {
        auto* vertex = createArray(true);
        assert(recordSize(vertex) == 16 && checkData(vertex, 12345, -23456));
        auto* scalar = createArray(false);
        assert(recordSize(scalar) == 0);
        destroy(vertex); // Engine deletion dispatches to the real hidden MM type.
        destroy(scalar);
    }
    assert(dlclose(plugin) == 0);
    assert(dlclose(engine) == 0);
    std::puts("PASS two-DSO vertex interface: default engine casts real hidden MMArray with distinct RTTI under RTLD_GLOBAL");
}
