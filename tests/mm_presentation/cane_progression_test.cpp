#include <cassert>
#include <iostream>
#include "mm/2s2h/Rando/Types.h"
#define COMBO_BUILD
#include "mm/2s2h/FleetShipCombo/FleetShipCombo.h"
static int unifiedPool=0;
extern "C" {
int gComboGameMode=COMBO_GAME_BOTH;
int FleetCombo_UnifiedPoolActive(void) { return unifiedPool; }
}
using u8=unsigned char;
constexpr int CANE_SKILL_SOMARIA_STATUE=0, CANE_SKILL_SOMARIA_BLOCK=1, CANE_SKILL_SOMARIA_PLATFORM=2,
    CANE_SKILL_PACCI_FLIP=3, CANE_SKILL_PACCI_STONE=4, CANE_SKILL_PACCI_ULTRAHAND=5;
unsigned owned=0;
bool Cane_HasSkill(u8 skill) { return (owned & (1u<<skill)) != 0; }
/* RESOLVE_CANE */
struct Actor {};
RandoItemId drawn=RI_NONE;
int conversions=0;
namespace Rando {
RandoItemId ConvertItem(RandoItemId id,RandoCheckId) {++conversions;return id==RI_OOT_NEI_CANE_OF_SOMARIA?ResolveCanePresentation():RI_OOT_NEI_ROCS_CAPE;}
void DrawResolvedItem(RandoItemId id,RandoCheckId,Actor*) {drawn=id;}
void DrawItem(RandoItemId,RandoCheckId,Actor*);
}
/* RAW_PREVIEW */
int main() {
 const int bits[]={0,3,1,4,2,5};
 const RandoItemId ids[]={RI_OOT_NEI_CANE_OF_SOMARIA,RI_OOT_NEI_CANE_PACCI_FLIP,RI_OOT_NEI_CANE_SOMARIA_BLOCK,
 RI_OOT_NEI_CANE_PACCI_STONE,RI_OOT_NEI_CANE_SOMARIA_PLATFORM,RI_OOT_NEI_CANE_PACCI_ULTRAHAND};
 for(owned=0;owned<64;++owned) {
   auto expected=ids[0];
   for(int i=0;i<6;i++) if(!(owned&(1u<<bits[i]))) {expected=ids[i];break;}
   assert(ResolveCanePresentation()==expected);
   Rando::DrawItem(RI_OOT_NEI_CANE_OF_SOMARIA,RC_UNKNOWN,nullptr);
   assert(drawn==expected);
 }
 owned=0;auto frozen=ResolveCanePresentation();owned=1;
 Rando::DrawResolvedItem(frozen,RC_UNKNOWN,nullptr);assert(drawn==RI_OOT_NEI_CANE_OF_SOMARIA);
 Rando::DrawItem(RI_OOT_NEI_CANE_OF_SOMARIA,RC_UNKNOWN,nullptr);assert(drawn==RI_OOT_NEI_CANE_PACCI_FLIP);
 auto before=conversions;
 Rando::DrawItem(RI_OOT_NEI_ROCS_FEATHER,RC_UNKNOWN,nullptr);
 assert(drawn==RI_OOT_NEI_ROCS_FEATHER && before==conversions);
 Rando::DrawItem(RI_OOT_PROGRESSIVE_ROC,RC_UNKNOWN,nullptr);assert(drawn==RI_OOT_NEI_ROCS_CAPE);
 // Pure MM skips the OoT-only platform without changing ownership or preview identity.
 gComboGameMode=COMBO_GAME_MM;
 for(owned=0;owned<64;++owned) {
   auto expected=ids[0];
   for(int i=0;i<6;i++) if(i!=4 && !(owned&(1u<<bits[i]))) {expected=ids[i];break;}
   assert(ResolveCanePresentation()==expected);
   Rando::DrawItem(RI_OOT_NEI_CANE_OF_SOMARIA,RC_UNKNOWN,nullptr);
   assert(drawn==expected);
 }
 // A shared fill keeps the full six-skill order even outside the combined host.
 unifiedPool=1;
 owned=(1u<<bits[0])|(1u<<bits[1])|(1u<<bits[2])|(1u<<bits[3]);
 assert(ResolveCanePresentation()==RI_OOT_NEI_CANE_SOMARIA_PLATFORM);
 Rando::DrawItem(RI_OOT_NEI_CANE_OF_SOMARIA,RC_UNKNOWN,nullptr);
 assert(drawn==RI_OOT_NEI_CANE_SOMARIA_PLATFORM);
 std::cout<<"PASS actual Cane selection: all 64 paired/pure-MM ownership masks, unified override, raw preview, frozen first award and concrete Roc identity\n";
}
