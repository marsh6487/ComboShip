#include "2s2h/BenJsonConversions.hpp"
#include "tests/test_require.h"
#include <memory>
#include <cstdio>
int main() {
    auto source=std::make_unique<RandoSaveInfo>();
    struct Key {RandoInf flag;RandoItemId item;};
    constexpr Key keys[]={
#define RUSTY_KEY(game,key,rg,ri,flag,name) RUSTY_SAVE_##game(ri,flag)
#define RUSTY_SAVE_MM(ri,flag) {flag,ri},
#define RUSTY_SAVE_OOT(ri,flag)
#include "rando/RustyDoorKeyCatalog.inc"
#undef RUSTY_SAVE_MM
#undef RUSTY_SAVE_OOT
#undef RUSTY_KEY
    };
    REQUIRE(std::size(keys)==39);int i=0;
    for(auto key:keys){source->randoInf[key.flag/16]|=1u<<(key.flag%16);source->randoStartingItems[i++]=key.item;}
    source->randoSaveOptions[RO_SHUFFLE_RUSTY_KEYS]=1;json saved=*source;auto loaded=std::make_unique<RandoSaveInfo>();saved.get_to(*loaded);
    REQUIRE(!memcmp(source->randoInf,loaded->randoInf,sizeof(source->randoInf)));
    REQUIRE(!memcmp(source->randoStartingItems,loaded->randoStartingItems,sizeof(source->randoStartingItems)));
    REQUIRE(loaded->randoSaveOptions[RO_SHUFFLE_RUSTY_KEYS]==1);
    auto old=saved;std::vector<u16> oldFlags((keys[0].flag+15)/16,0);oldFlags[0]=1;
    old["randoInf"]=oldFlags;old["randoSaveOptions"]=std::vector<u32>(RO_SHUFFLE_RUSTY_KEYS,0);old["randoStartingItems"]=std::vector<u16>(256,0);old.get_to(*loaded);
    REQUIRE(loaded->randoInf[0]==1);REQUIRE(loaded->randoSaveOptions[RO_SHUFFLE_RUSTY_KEYS]==0);
    for(auto key:keys)REQUIRE(!(loaded->randoInf[key.flag/16]&(1u<<(key.flag%16))));
    for(auto item:loaded->randoStartingItems)REQUIRE(item==0);
    saved["randoInf"].push_back(65535);saved["randoSaveOptions"].push_back(65535);saved.get_to(*loaded);
    REQUIRE(!memcmp(source->randoInf,loaded->randoInf,sizeof(source->randoInf)));
    std::puts("PASS actual MM JSON:39 ownership/starting keys, legacy reused zero defaults and future tails");
}
