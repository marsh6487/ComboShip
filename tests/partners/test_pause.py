"""The actual equipment cursor helpers must make the unlocked Cojiro cell reachable."""
from pathlib import Path
import subprocess
import tempfile
import unittest
import sys
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'scripts/diagnostics'))
from run_time_pedestal_tests import block_from

class Pause(unittest.TestCase):
    def test_owned_cojiro_cell_is_reachable_in_both_games(self):
        cases=[('soh/src/overlays/misc/ovl_kaleido_scope/z_kaleido_equipment.c','static s32 KaleidoEquip_UpgradeCellAvailable(', 'KaleidoEquip_UpgradeCellAvailable'),
               ('mm/src/overlays/kaleido_scope/ovl_kaleido_scope/z_kaleido_equipment.c','static s16 KaleidoEquip_UpgradeValue(', 'KaleidoEquip_UpgradeValue')]
        for path,signature,fn in cases:
            with self.subTest(game=path.split('/')[0]),tempfile.TemporaryDirectory() as tmp:
                source=(ROOT/path).read_text();body=block_from(source,source.index(signature))
                code='#include <cassert>\nusing s16=short;using s32=int;bool owned=false;\nint ExtEquip_CapeOwned(){return 0;}int ExtEquip_PendantOwned(){return 0;}bool Partners_CojiroOwned(){return owned;}\n'
                code+=body+'\nint main(){assert(!'+fn+'(2));owned=true;assert('+fn+'(2));assert(!'+fn+'(3));}\n'
                src=Path(tmp)/'test.cpp';src.write_text(code);exe=Path(tmp)/'test'
                subprocess.run(['c++','-std=c++17',str(src),'-o',str(exe)],check=True)
                subprocess.run([str(exe)],check=True)

if __name__=='__main__':unittest.main()
