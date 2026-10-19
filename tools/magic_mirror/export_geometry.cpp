#include "../../combo/gameplay/MagicMirrorGeometry.h"
#include <cstdlib>
#include <iomanip>
#include <iostream>

int main(int argc, char** argv) {
    using namespace ComboMagicMirrorGeometry;
    Mesh mesh;
    Build(argc > 1 ? static_cast<uint32_t>(std::strtoul(argv[1], nullptr, 10)) : 0, mesh);
    std::cout << std::setprecision(9) << "{\"vertexPrecision\":" << kVertexPrecision << ",\"vertices\":[";
    for (int i = 0; i < mesh.vertexCount; ++i) {
        const auto& v = mesh.vertices[i];
        if (i) std::cout << ',';
        std::cout << '[' << v.x << ',' << v.y << ',' << v.z << ',' << int(v.r) << ',' << int(v.g) << ','
                  << int(v.b) << ',' << int(v.a) << ']';
    }
    std::cout << "],\"triangles\":[";
    for (int b = 0; b < mesh.batchCount; ++b) {
        const auto& batch = mesh.batches[b];
        for (int i = 0; i < batch.triangleCount; ++i) {
            const auto& t = mesh.triangles[batch.firstTriangle + i];
            if (batch.firstTriangle + i) std::cout << ',';
            std::cout << '[' << batch.firstVertex + t.a << ',' << batch.firstVertex + t.b << ','
                      << batch.firstVertex + t.c << ',' << (b < mesh.opaqueBatchCount ? 0 : 1) << ']';
        }
    }
    std::cout << "]}\n";
}
