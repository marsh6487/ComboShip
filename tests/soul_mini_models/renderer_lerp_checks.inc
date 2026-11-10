// Compare the consumer against the actual native MM handler, including negative
// deltas (native truncates the delta before adding the lower-frame color).
using u16 = uint16_t;
using F3DPrimColor = CfaPrimColor;
using F3DEnvColor = CfaEnvColor;
using AnimatedMatColorParams = CfaColorParams;
static int sMatAnimStep;
static CfaPrimColor expectedLerpPrim;
static CfaEnvColor expectedLerpEnv;
static bool expectedLerpHasEnv;
static void AnimatedMat_SetColor(PlayState*, s32, F3DPrimColor* prim, F3DEnvColor* env) {
    expectedLerpPrim = *prim;
    expectedLerpHasEnv = env != nullptr;
    if (env) expectedLerpEnv = *env;
}
#include "renderer_native_lerp.inc"
struct RendererLerpResource : Ship::IResource {
    // Gomess native period/key shape; invented color data, with the factory's
    // owned constant terminal interval at index count. No private asset bytes.
    CfaPrimColor prim[4]{{230,151,97,255,127},{5,12,3,211,40},{231,151,99,255,127},{231,151,99,255,127}};
    CfaEnvColor env[4]{{31,73,5,255},{1,0,0,219},{31,73,5,255},{31,73,5,255}};
    uint16_t keys[4]{0,11,23,24};
    CfaColorParams params{24,3,prim,env,keys};
    CfaMatEntry entry{-2,3,&params};
    RendererLerpResource() { metadata->Type = 0x4F54414E; }
    void* GetRawPointer() override { return &entry; }
    size_t GetPointerSize() override { return sizeof(entry); }
};
static uintptr_t PackLerpColor(int r, int g, int b, int a) {
    return (uint32_t(r)<<24) | (uint32_t(g)<<16) | (uint32_t(b)<<8) | uint32_t(a);
}
static void RunRendererLerpChecks() {
    for (const char* game : {"oot", "mm"}) {
        auto owner = std::make_shared<Ship::ResourceManager>();
        Ship::CrossRMRegistry::owners[game] = owner;
        auto material = std::make_shared<RendererLerpResource>();
        owner->normal["__OTR__lerp"] = material;
        owner->normal["__OTR__body"] = std::make_shared<RendererDisplayListResource>();
        CwItemAnimDrawInfo info{};
        info.opa = 1; info.partCount = 1; info.parts[0].dlPath = "__OTR__body"; info.texAnimPath = "__OTR__lerp";
        CfaClearCaches();
        for (bool environment : {true,false}) {
            material->params.envColors = environment ? material->env : nullptr;
            for (int frame = 0; frame < 48; ++frame) {
                GraphicsContext gfx; PlayState play{{&gfx,(u32)frame},(u32)frame,{}};
                sMatAnimStep = frame;
                AnimatedMat_DrawColorLerp(&play, 9, &material->params);
                current = {};
                assert(ComboForeignAnim_Draw(&info, game, &play));
                bool found = false;
                for (Gfx* cmd = gfx.opa; cmd < gfx.op; ++cmd) {
                    if (cmd->kind != 1 || cmd->seg != 9) continue;
                    auto* dl = (Gfx*)cmd->target;
                    if (dl->kind != 4) continue;
                    assert(dl[0].target == PackLerpColor(expectedLerpPrim.r, expectedLerpPrim.g, expectedLerpPrim.b, expectedLerpPrim.a));
                    assert(dl[0].seg == expectedLerpPrim.lodFrac);
                    if (environment) {
                        assert(expectedLerpHasEnv && dl[1].kind == 5);
                        assert(dl[1].target == PackLerpColor(expectedLerpEnv.r, expectedLerpEnv.g, expectedLerpEnv.b, expectedLerpEnv.a));
                    } else assert(!expectedLerpHasEnv && dl[1].kind == 0);
                    found = true;
                }
                assert(found && matrices.empty());
            }
        }
        CfaClearCaches();
    }
    std::cout << "Native type-3 color parity: descending channels, every frame, period wrap, terminal interval and optional environment passed in both owner directions\n";
}
