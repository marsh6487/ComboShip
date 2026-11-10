#include "shared_pool_fixture.inc"
int main() {
 for(int enabled:{0,1}) {
  json o=emptyDump(),m=emptyDump();
  add(o,"Junk",3,false);add(m,"Junk",3,false);
  if(enabled) {add(o,"Rito Mask",1,false);add(m,"Rito Mask",1,false);
   o["pool"].back()["category"]="major";m["pool"].back()["category"]="major";}
  auto result=fill(o,m,0,pairsJson({{"Rito Mask","Rito Mask"}}));
  check(result.success,"optional Rito shared fill succeeds without any logic requirement");
  if(result.success) {
   check(countPlaced(result,GAME_OOT,"Rito Mask")+countPlaced(result,GAME_MM,"Rito Mask")==enabled,"exactly one shared Rito in combined placement");
  }
 }
 if(failures)return 1;
 puts("PASS production Combo fill deduplicates optional nonadvancing Rito to one reward");
}
