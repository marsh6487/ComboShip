// Execute Play_Destroy with the real Wolf and SSBB cleanup at the arena boundary.
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include "combo/audio/DinsFireSfx.h"
#define COMBO_BUILD
struct GameState {};
struct Actor {
    struct { void (*shadowDraw)(); } shape;
};
struct Player { Actor actor; };
struct PlayState : GameState { Player* player; };
#define GET_PLAYER(p) ((p)->player)
static PlayState* gPlayState;
static bool actorsLive = true;
static std::vector<std::string> events;
static int wolfFrees;
static void ActorShadow_DrawFeet() {}
static void WolfLinkForm_DrawShadow() {}
static void StopChainSfx() {}
static void WolfAudio_Stop() {}
struct SSBBSkinMesh { void* vtxBuf[2]; };
struct WolfDefinition { SSBBSkinMesh* skinMesh; };
static struct {
    struct {
        WolfDefinition* def;
        void* jointTable;
    } character;
    int initialized, procOwnsPlayer, atActive;
} sWolf;
static void ZELDA_ARENA_FREE_DEBUG(void* pointer) {
    assert(actorsLive && "Wolf buffer free reached a retired scene arena");
    ++wolfFrees;
    std::free(pointer);
}
/* NATIVE_WOLF_CLEANUP */

void DinsFireSfx_UpdateContext(PlayState* play){if(play||!actorsLive)std::abort();events.push_back("audio");}
void ComboEmotes_Reset(){}void Combo_SOH_CancelCrossDungeonTransition(){}
void MmForm_OnPlayDestroy(PlayState*,Player*){events.push_back("forms");}
void GameInteractor_ExecuteOnPlayDestroy(){events.push_back("hooks");}
void TimeCtl_Reset(PlayState*){if(!actorsLive)std::abort();events.push_back("time");}
void Sm64Surfaces_RefreshActorColliders(PlayState* p){if(p||!actorsLive)std::abort();events.push_back("surfaces");}
void DestroyPrelude(GameState* thisx){/* NATIVE_TEARDOWN_PHASE */actorsLive=false;}
int main() {
    for (auto shadow : { WolfLinkForm_DrawShadow, ActorShadow_DrawFeet }) {
        // After detransforming, the human shadow is restored but Wolf's scene buffers still exist.
        Player player{};
        PlayState play{ {}, &player };
        player.actor.shape.shadowDraw = shadow;
        gPlayState = &play;
        actorsLive = true;
        wolfFrees = 0;
        events.clear();
        SSBBSkinMesh skin{ { std::malloc(64), std::malloc(64) } };
        WolfDefinition wolfDefinition{ &skin };
        sWolf.character = { &wolfDefinition, std::malloc(16) };
        sWolf.initialized = 1;

        DestroyPrelude(&play);
        assert(events == (std::vector<std::string>{ "audio", "forms", "hooks", "time", "surfaces" }));
        assert(wolfFrees == 3 && "Wolf buffers and joints must be released before the scene arena retires");
        assert(!skin.vtxBuf[0] && !skin.vtxBuf[1] && !sWolf.character.jointTable && !sWolf.initialized);
        assert(player.actor.shape.shadowDraw == ActorShadow_DrawFeet);
    }
    std::puts("PASS native teardown releases Wolf buffers while the old scene arena is live");
}
