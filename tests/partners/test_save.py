from pathlib import Path
import subprocess,tempfile,unittest,sys,os
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'scripts/diagnostics'))
from run_time_pedestal_tests import block_from
class Save(unittest.TestCase):
    def test_actual_mm_old_save_defaults_and_roundtrip(self):
        s=(ROOT/'mm/2s2h/BenJsonConversions.hpp').read_text()
        bodies='\n'.join(block_from(s,s.index(sig)) for sig in ('inline void to_json(json& j, const NeiSaveData& n)', 'inline void from_json(const json& j, NeiSaveData& n)'))
        code='''#include <nlohmann/json.hpp>
#include <cstring>
#include <cassert>
#include "mm/mods/nei_save.h"
#include "combo/rando/RpgStatsJson.h"
#define ARRAY_COUNT(a) (sizeof(a)/sizeof((a)[0]))
using json=nlohmann::json;
'''+bodies+'''
int main(){NeiSaveData n{};n.cojiroPartnerOwned=n.cojiroPartnerDismissed=1;
 from_json(json::object(),n);assert(n.cojiroPartnerOwned==0&&n.cojiroPartnerDismissed==0);
 n.cojiroPartnerOwned=1;n.cojiroPartnerDismissed=1;n.ootQuestItems=123;n.swordElements[3]=2;
 json saved;to_json(saved,n);NeiSaveData restored{};from_json(saved,restored);
 assert(restored.cojiroPartnerOwned==1&&restored.cojiroPartnerDismissed==1);
 assert(restored.ootQuestItems==123&&restored.swordElements[3]==2);
}
'''
        inc=os.environ.get('NLOHMANN_JSON_INCLUDE',str(ROOT.parent/'native-deps'))
        with tempfile.TemporaryDirectory() as t:
            p=Path(t)/'t.cpp';p.write_text(code);exe=Path(t)/'t'
            subprocess.run(['c++','-std=c++20','-I'+str(ROOT),'-I'+str(ROOT/'mm'),'-I'+inc,str(p),'-o',str(exe)],check=True)
            subprocess.run([str(exe)],check=True)
if __name__=='__main__':unittest.main()
