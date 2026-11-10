#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
#define main legacy_skeleton_main
#include "legacy_skeleton_fixture.cpp"
#undef main
#pragma GCC diagnostic pop
#include "renderer_checks.inc"
#include "renderer_lerp_checks.inc"
#if __has_include("renderer_catalog_checks.inc")
#include "renderer_catalog_checks.inc"
#endif
int main() {
#if __has_include("renderer_catalog_checks.inc")
    RunSoulCatalogChecks();
#endif
    legacy_skeleton_main();
    RunRendererFailureChecks();
    RunRendererLerpChecks();
    auto owner = std::make_shared<Ship::ResourceManager>();
    auto host = std::make_shared<Ship::ResourceManager>();
    Ship::CrossRMRegistry::owners["oot"] = owner;
    Ship::Context::GetRawInstance()->rm = host;
    CwItemAnimDrawInfo info{};
    info.opa = 1;
    info.partCount = 2;
    owner->normal["__OTR__static_body"] = std::make_shared<RendererDisplayListResource>();
    owner->normal["__OTR__static_eyes"] = std::make_shared<RendererDisplayListResource>();
    info.parts[0].dlPath = "__OTR__static_body";
    info.parts[1].dlPath = "__OTR__static_eyes";
    info.parts[1].xlu = 1;
    info.matrixCount = 4; info.matrixSegment = 12;
    for (int i = 0; i < 4; ++i) info.matrices[i].translate[1] = (i + 1) * 1000.f;
    info.translatePre[0] = 20;
    info.scale = .01f;
    GraphicsContext c; PlayState p{{&c, 7}, 9, {}};
    current = {}; current.x = 99;
    assert(ComboForeignAnim_Draw(&info, "oot", &p));
    assert(current.x == 99 && current.sx == 1 && matrices.empty());
    assert(Ship::Context::GetRawInstance()->rm == host);
    int meshes = 0, palettes = 0;
    for (Gfx* cmd = c.opa; cmd < c.op; ++cmd) {
        if (cmd->kind == 3) ++meshes;
        if (cmd->kind == 1 && cmd->seg == 12) {
            if (palettes++ == 0) {
                const Mtx* palette = (Mtx*)cmd->target;
                for (int i = 0; i < 4; ++i) assert(palette[i].y == (i + 1) * 1000.f);
            }
        }
    }
    assert(meshes == 1 && palettes == 2);
    std::cout << "Bounded static composite, palette and inherited transform restoration passed\n";
}
