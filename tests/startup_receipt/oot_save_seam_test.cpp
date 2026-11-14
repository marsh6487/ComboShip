#include <cassert>
#include <cstdio>
struct GetItemEntry{int id;};
constexpr int RSK_STARTING_ZELDAS_LETTER=1,RSK_SHUFFLE_ZELDAS_LETTER=2,RC_SONG_FROM_IMPA=3,RC_HC_ZELDAS_LETTER=4,RG_ZELDAS_LULLABY=5,RG_ZELDAS_LETTER=6,RCSHOW_SAVED=7,EVENTCHKINF_OBTAINED_ZELDAS_LETTER=8,RAND_INF_ZELDAS_LETTER=9,EVENTCHKINF_LEARNED_ZELDAS_LULLABY=10;
using GetItemID=int;
static bool letter,shuffle,defer;
static int grants,marked,flags,captures;
struct Location {void SetCheckStatus(int s){assert(s==RCSHOW_SAVED);marked++;}};
struct Context{Location l;Location* GetItemLocation(int rc){assert(rc==RC_SONG_FROM_IMPA);return &l;}};
struct OTRGlobals{Context* gRandoContext;static OTRGlobals* Instance;};
Context context;OTRGlobals global{&context};OTRGlobals* OTRGlobals::Instance=&global;
bool Randomizer_GetSettingValue(int key){return key==RSK_STARTING_ZELDAS_LETTER?letter:shuffle;}
GetItemEntry Randomizer_GetItemFromKnownCheck(int check,int){return {check};}
void StartingItemGive(GetItemEntry e,int rc){assert(e.id==rc);grants++;}
bool ComboStartupOot_Capture(const GetItemEntry* e,int rc){assert(rc==RC_SONG_FROM_IMPA&&e->id==rc);captures++;return defer;}
void Flags_SetEventChkInf(int){flags++;}
void Flags_SetRandomizerInf(int){flags++;}
#include "oot_save_seam.inc"
int main(){for(int a=0;a<2;a++)for(int b=0;b<2;b++)for(int c=0;c<2;c++){
letter=a;shuffle=b;defer=c;grants=marked=flags=captures=0;InitImpa();bool eligible=a&&!b;
#ifdef COMBO_BUILD
assert(captures==eligible);assert(marked==(eligible&&c));assert(grants==(eligible?(c?1:2):0));
#else
assert(!captures&&!marked);assert(grants==(eligible?2:0));
#endif
assert(flags==(eligible?3:0));}puts("PASS actual save seam: Letter eligibility/story flags preserved, deferred check marked, immediate foreign grant skipped only on acceptance");}
