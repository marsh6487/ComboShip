#include "global.h"

#include "combo/render/ZfgWalletDraw.h"
#include <cassert>
#include <cstring>
#include <iostream>
#include <vector>

static GraphicsContext gfx;
static PlayState play;
static Gfx opa[128], xlu[128], overlay[8], setup[1], tp[1], cloth[1], badge[1];
static Mtx matrix;
static bool mounted, loadable, classicReady, bodyRoom = true, xluRoom = true;
static int classicLookups;
static int stackDepth;
static const char* marketRoot;

extern "C" {
#include "wallet_graph.inc"

void gSPDisplayList(Gfx* command, Gfx* list) {
    __gSPDisplayList(command, list);
}
void FrameInterpolation_RecordOpenChild(const void*, int) {
}
void FrameInterpolation_RecordCloseChild() {
}
void Matrix_Push() {
    ++stackDepth;
}
void Matrix_Pop() {
    assert(stackDepth == 1);
    --stackDepth;
}
#ifdef COMBO_ZFG_WALLET_HOST_MM
Mtx* Matrix_Finalize(GraphicsContext*) {
    return &matrix;
}
void Matrix_Translate(float, float, float, MatrixMode) {
}
void Matrix_Scale(float, float, float, MatrixMode) {
}
void Matrix_RotateYF(float, MatrixMode) {
}
void Matrix_RotateZF(float, MatrixMode) {
}
#define SETUP_OPA Gfx_SetupDL25_Opa
#define SETUP_XLU Gfx_SetupDL25_Xlu
#else
Mtx* Matrix_NewMtx(GraphicsContext*, char*, s32) {
    return &matrix;
}
void Matrix_Translate(float, float, float, u8) {
}
void Matrix_Scale(float, float, float, u8) {
}
void Matrix_RotateY(float, u8) {
}
void Matrix_RotateZ(float, u8) {
}
#define SETUP_OPA Gfx_SetupDL_25Opa
#define SETUP_XLU Gfx_SetupDL_25Xlu
#endif
void SETUP_OPA(GraphicsContext* ctx) {
    OPEN_DISPS(ctx);
    __gSPDisplayList(POLY_OPA_DISP++, setup);
    CLOSE_DISPS(ctx);
}
void SETUP_XLU(GraphicsContext* ctx) {
    OPEN_DISPS(ctx);
    __gSPDisplayList(POLY_XLU_DISP++, setup);
    CLOSE_DISPS(ctx);
}
uint8_t ResourceMgr_FileExists(const char* path) {
    assert(std::strcmp(path, ComboZfgWallet::kTwilightModel) == 0);
    return mounted;
}
Gfx* ResourceMgr_LoadGfxByName(const char* path) {
    assert(mounted && std::strcmp(path, ComboZfgWallet::kTwilightModel) == 0);
    return loadable ? tp : nullptr;
}
bool NeiGi_CanDrawLayers(PlayState* current, size_t matrices, size_t opaCommands, size_t xluCommands) {
    assert(current == &play);
    if (matrices == 1) {
        assert(opaCommands >= 24 && xluCommands >= 2);
        return bodyRoom;
    }
    assert(matrices == 4 && opaCommands >= 2 && xluCommands >= 34);
    return xluRoom;
}
void NeiGi_ReportBlackMarketModel(const char* owner, const char* root, const char* second) {
    assert(std::strcmp(owner, "active") == 0 && !second);
    marketRoot = root;
}
NeiGi::Basis NeiGi_CameraBasis(PlayState* current) {
    assert(current == &play);
    return {};
}
}

static void Reset() {
    play.state.gfxCtx = &gfx;
    play.gameplayFrames = 9;
    gfx.polyOpa.p = opa;
    gfx.polyOpa.d = opa + 128;
    gfx.polyXlu.p = xlu;
    gfx.polyXlu.d = xlu + 128;
    gfx.overlay.p = overlay;
    gfx.overlay.d = overlay + 8;
    marketRoot = nullptr;
}

namespace ComboZfgWallet {
static ClassicLists LoadClassicLists(PlayState* current, const char*) {
    assert(current == &play);
    ++classicLookups;
    return classicReady ? ClassicLists{ cloth, badge } : ClassicLists{};
}
} // namespace ComboZfgWallet

static void CheckDraw(bool twilight) {
    using namespace ComboZfgWallet;
    Reset();
    const int lookups = classicLookups;
    Draw(&play, "active");
    assert(classicLookups == lookups + !twilight);
    assert(stackDepth == 0 && gfx.overlay.p == overlay);
    assert(std::strcmp(marketRoot, twilight ? kTwilightModel : gGiWalletDL) == 0);
    std::vector<const char*> bodyPaths;
    bool wine = false;
    for (auto* p = opa; p < gfx.polyOpa.p; ++p) {
        const unsigned op = p->words.w0 >> 24;
        if (op == G_DL && p->words.w1 != reinterpret_cast<uintptr_t>(setup))
            bodyPaths.push_back(reinterpret_cast<const char*>(p->words.w1));
        if (op == G_SETPRIMCOLOR && uint32_t(p->words.w1) == 0x9C2A3DFF)
            wine = true;
    }
    assert(wine == !twilight);
    if (!twilight && classicReady) {
        assert(bodyPaths.size() == 10);
        assert(bodyPaths[1] == reinterpret_cast<const char*>(cloth));
        assert(bodyPaths[3] == reinterpret_cast<const char*>(sBindBadge));
        assert(!std::strcmp(bodyPaths[4], gGiWalletRupeeOuterDL));
        assert(bodyPaths[6] == reinterpret_cast<const char*>(sBindCord));
        assert(!std::strcmp(bodyPaths[7], gGiWalletStringDL));
        assert(bodyPaths[9] == reinterpret_cast<const char*>(badge));
    } else {
        assert(bodyPaths.size() == (twilight ? 1 : 8));
        assert(!std::strcmp(bodyPaths[twilight ? 0 : 1], marketRoot));
    }
    if (!xluRoom) {
        assert(gfx.polyXlu.p == xlu);
        return;
    }
    int scope = 0, faded = 0, drawsInScope = 0, loads = 0;
    for (auto* p = xlu; p < gfx.polyXlu.p; ++p) {
        switch (p->words.w0 >> 24) {
            case G_MODEL_OPACITY_PUSH:
                assert(scope == 0);
                scope = 1;
                drawsInScope = 0;
                break;
            case G_MODEL_OPACITY_POP:
                assert(scope == 1 && drawsInScope == 4);
                scope = 0;
                ++faded;
                break;
            case G_MTX:
                ++loads;
                break;
            case G_DL: {
                if (!scope)
                    break;
                const char* path = reinterpret_cast<const char*>(p->words.w1);
                if (drawsInScope == 1)
                    assert(!std::strcmp(path, gGiRupeeInnerDL));
                if (drawsInScope == 3)
                    assert(!std::strcmp(path, gGiRupeeOuterDL));
                ++drawsInScope;
                break;
            }
        }
    }
    assert(faded == 3 && loads == 4 && scope == 0);
}

int main() {
    using namespace ComboZfgWallet;
    const auto command = [](uintptr_t opcode, uintptr_t w0, uintptr_t w1) {
        Gfx result{};
        result.words.w0 = (opcode << 24) | w0;
        result.words.w1 = w1;
        return result;
    };
    const auto image = command(G_SETTIMG_OTR_HASH, 0x900000, 0);
    const auto imageHash = command(G_ENDDL, 0x12, 0x12345678);
    const auto tile = command(G_SETTILESIZE, 0, 0x000FC0FC);
    const auto vertex = command(G_VTX_OTR_HASH, 0x3006, 0);
    const auto vertexHash = command(G_SETTIMG_OTR_HASH, 0x123456, 0);
    const auto end = command(G_ENDDL, 0, 0);
    const Gfx source[] = { image, imageHash, tile, vertex, vertexHash, end };
    const auto patched = ClassicTextureList(source, kClothTexture);
    assert(patched.size() == 5 && patched[0].words.w0 == (uintptr_t(G_SETTIMG_OTR_FILEPATH) << 24 | 0x900000));
    assert(patched[0].words.w1 == reinterpret_cast<uintptr_t>(kClothTexture));
    assert(source[0].words.w0 == image.words.w0 && patched[3].words.w0 == vertexHash.words.w0);
    assert(ClassicTextureList(std::span(source).first(5), kClothTexture).empty());
    const Gfx otherTile[] = { image, imageHash, command(G_SETTILESIZE, 0, 0), end };
    assert(ClassicTextureList(otherTile, kClothTexture).empty());
    const auto gemScale = command(G_TEXTURE, 2, 0x13881388);
    const Gfx gem[] = { gemScale, image, imageHash, tile, vertex, vertexHash, end };
    const auto patchedGem = ClassicTextureList(gem, kBadgeTexture, true);
    assert(patchedGem.size() == 6 && patchedGem[0].words.w1 == 0xFFFF4000);
    assert(gem[0].words.w1 == gemScale.words.w1);
    assert(ClassicTextureList(source, kBadgeTexture, true).empty());

    CheckDraw(false);
    classicReady = true;
    CheckDraw(false);
    mounted = true;
    CheckDraw(false); // A mounted but unreadable root still uses the classic pouch.
    loadable = true;
    CheckDraw(true);
    xluRoom = false;
    CheckDraw(true);
    Reset();
    bodyRoom = false;
    ComboZfgWallet::Draw(&play, "active");
    assert(!marketRoot && gfx.polyOpa.p == opa && gfx.polyXlu.p == xlu);
    std::cout << "PASS native classic/TP pouch, private classic textures, real rupee roots and XLU fade scopes\n";
}
