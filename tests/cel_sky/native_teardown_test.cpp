// Execute native Play_Destroy's pre-retirement phase. Growable actor snapshots
// must be released while referenced scene actors and collision still exist.
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#define COMBO_BUILD
struct GameState{};struct Player{};struct PlayState:GameState{Player player;};
#define GET_PLAYER(p) (&(p)->player)
bool actorsLive=true;std::vector<std::string>events;
void ComboEmotes_Reset(){}void Combo_SOH_CancelCrossDungeonTransition(){}
void MmForm_OnPlayDestroy(PlayState*,Player*){events.push_back("forms");}
void GameInteractor_ExecuteOnPlayDestroy(){events.push_back("hooks");}
void TimeCtl_Reset(PlayState*){if(!actorsLive)std::abort();events.push_back("time");}
void Sm64Surfaces_RefreshActorColliders(PlayState* p){if(p||!actorsLive)std::abort();events.push_back("surfaces");}
void DestroyPrelude(GameState* thisx){/* NATIVE_TEARDOWN_PHASE */actorsLive=false;}
int main(){PlayState play;DestroyPrelude(&play);if(events!=std::vector<std::string>{"forms","hooks","time","surfaces"}){std::fputs("FAIL: scene snapshots not released before native teardown\n",stderr);return 1;}std::puts("cel combined native teardown: PASS");}
