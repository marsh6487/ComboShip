"""Exercise the real OoT registration boundary; no renderer/game runtime required."""
from pathlib import Path
import subprocess
import tempfile
import unittest
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts/diagnostics'))
from run_time_pedestal_tests import block_from


class Registration(unittest.TestCase):
    def test_missing_assets_never_register_partial_enemy_set(self):
        source = (ROOT / 'soh/mods/actors/trutefel/trutefel_actor_reg.cpp').read_text()
        body = block_from(source, source.index('void Trutefel_EnsureActorsRegistered(void) {'))
        prefix = r'''
#include <cassert>
#include <string>
#include <vector>
using ActorFunc = void(*)();
void EnMiniblin_Init(){} void EnMiniblin_Destroy(){} void EnMiniblin_Update(){} void EnMiniblin_Draw(){}
void EnHammergeist_Init(){} void EnHammergeist_Destroy(){} void EnHammergeist_Update(){} void EnHammergeist_Draw(){}
void EnSbeetle_Init(){} void EnSbeetle_Destroy(){} void EnSbeetle_Update(){} void EnSbeetle_Draw(){}
int gEnMiniblinId=-1,gEnHammergeistId=-1,gEnSbeetleId=-1;
int gEnMiniblinStructSize=11,gEnHammergeistStructSize=22,gEnSbeetleStructSize=33;
constexpr int ACTORCAT_ENEMY=5,OBJECT_GAMEPLAY_KEEP=1;
constexpr int TRUTEFEL_MINIBLIN_FLAGS=17,TRUTEFEL_HAMMERGEIST_FLAGS=18,TRUTEFEL_SBEETLE_FLAGS=19;
const char*sMiniblinGatePath="miniblin",*sHammergeistGatePath="molmauk",*sSbeetleGatePath="beetle";
int assets=0;
bool ResourceMgr_FileExists(const char*p){return assets&(p==sMiniblinGatePath?1:p==sHammergeistGatePath?2:4);}
struct ActorDBInit {std::string name,desc;int category=0,flags=0,objectId=0,instanceSize=0;
 ActorFunc init=nullptr,destroy=nullptr,update=nullptr,draw=nullptr;};
struct ActorDB {struct Entry{struct{int id;}entry;};static ActorDB*Instance;
 std::vector<ActorDBInit> rows; Entry next;
 Entry&AddEntry(const ActorDBInit&i){rows.push_back(i);next.entry.id=100+rows.size();return next;}};
ActorDB db;ActorDB*ActorDB::Instance=&db;
'''
        main = r'''
int main(){
 for(assets=0;assets<7;++assets){Trutefel_EnsureActorsRegistered();assert(db.rows.empty());assert(gEnMiniblinId==-1);}
 assets=7;Trutefel_EnsureActorsRegistered();assert(db.rows.size()==3);
 assert(gEnMiniblinId==101&&gEnHammergeistId==102&&gEnSbeetleId==103);
 assert(db.rows[0].init==EnMiniblin_Init&&db.rows[0].draw==EnMiniblin_Draw);
 assert(db.rows[1].update==EnHammergeist_Update&&db.rows[1].instanceSize==22);
 assert(db.rows[2].destroy==EnSbeetle_Destroy&&db.rows[2].flags==19);
 for(const auto&r:db.rows)assert(r.category==5&&r.objectId==1&&r.init&&r.destroy&&r.update&&r.draw);
 Trutefel_EnsureActorsRegistered();assert(db.rows.size()==3);
}
'''
        with tempfile.TemporaryDirectory() as tmp:
            src=Path(tmp)/'test.cpp'; src.write_text(prefix+body+main)
            exe=Path(tmp)/'test'
            subprocess.run(['c++','-std=c++17','-Wall','-Wextra','-Werror',str(src),'-o',str(exe)],check=True)
            subprocess.run([str(exe)],check=True)


if __name__ == '__main__':
    unittest.main()
