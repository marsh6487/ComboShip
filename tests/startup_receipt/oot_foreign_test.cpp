#include <cassert>
#include <string>
#include <cstring>
#include <unordered_map>
#include "rando/StartupCheckReceipt.h"
struct GetItemEntry {int comboMarketReceipt,comboForeignCheck,gid;};
static int checkLookups=0,queueLookups=0,namedLookups=0,fallbacks=0;
static const char* frozen="Large Quiver";
struct ComboForeignDrawInfo {};
using RandomizerCheck=int;
constexpr int RC_UNKNOWN_CHECK=0;
bool ComboStartupOot_IsReceipt(const GetItemEntry* e){return e->comboMarketReceipt & CW_STARTUP_RECEIPT_TOKEN_BIT;}
const char* ComboStartupOot_FrozenForeignName(const GetItemEntry*){return frozen;}
int OOT_GetQueuedDrawCheck(){queueLookups++;return 9;}
const ComboForeignDrawInfo* ComboResolveForeignDrawInfo(int){static ComboForeignDrawInfo i;checkLookups++;return &i;}
const ComboForeignDrawInfo* ComboResolveFrozenForeignDrawInfo(const char* n){static ComboForeignDrawInfo i;assert(!strcmp(n,"Large Quiver"));namedLookups++;return &i;}
void GetItem_Draw(void*,int){fallbacks++;}
#include "oot_foreign.inc"
int main(){GetItemEntry e{CW_STARTUP_RECEIPT_TOKEN_BIT|1,66,0};SelectForeign(nullptr,&e);assert(namedLookups==1&&checkLookups==0&&queueLookups==0);frozen=nullptr;SelectForeign(nullptr,&e);assert(fallbacks==1&&checkLookups==0&&queueLookups==0);e.comboMarketReceipt=0;SelectForeign(nullptr,&e);assert(checkLookups==1);e.comboForeignCheck=0;SelectForeign(nullptr,&e);assert(checkLookups==2&&queueLookups==1);}
