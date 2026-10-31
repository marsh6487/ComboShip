"""Solo MM's selected Cojiro title is a locally packaged IA4 texture."""
import hashlib
import importlib.util
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
NAMES = ROOT / 'mm/assets/custom/textures/item_name_custom'
NATIVE_PATH = '__OTR__textures/item_name_custom/gCojiroNameTex'
sys.path.insert(0, str(ROOT / 'scripts/diagnostics'))
from run_time_pedestal_tests import block_from


class CojiroTitle(unittest.TestCase):
    def test_native_selection_uses_local_texture_without_oot_owner(self):
        source = (ROOT / 'mm/src/overlays/kaleido_scope/ovl_kaleido_scope/z_kaleido_equipment.c').read_text()
        body = block_from(source, source.index('void* KaleidoEquip_GetNameTex('))
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'test.cpp'
            path.write_text('''#include <cassert>
#include <cstring>
#include <cstddef>
int sEquipCursorX=0,sEquipCursorY=2,sEquipSubPage=0;
constexpr int EQUIP_SUBPAGE_VANILLA=0;
bool owned=true;
bool Partners_CojiroOwned(){return owned;}
bool WeaponUpgrade_HasGreatFairy(){return false;}
int donorCalls=0;
void* KaleidoEquip_OotTex(const char*){++donorCalls;return nullptr;}
''' + body + '''
int main(){
 const char* title=static_cast<const char*>(KaleidoEquip_GetNameTex());
 assert(title != nullptr);
 assert(!strcmp(title,"''' + NATIVE_PATH + '''"));
 assert(donorCalls==0);
 owned=false;assert(KaleidoEquip_GetNameTex()==nullptr);
}
''')
            exe = Path(directory) / 'test'
            subprocess.run(['c++', '-std=c++17', str(path), '-o', str(exe)], check=True)
            subprocess.run([str(exe)], check=True)

    def test_cojiro_source_is_ia4_and_reproduces_existing_label_pipeline(self):
        texture = NAMES / 'gCojiroNameTex.ia4.png'
        self.assertTrue(texture.exists(), 'Cojiro title source is missing from MM custom assets')
        self.assertEqual(hashlib.sha256(texture.read_bytes()).hexdigest(),
                         '15c90d8cc2c832156056b41b03095ad5190e5ad658e011a8d9dc90ad474cd164')
        spec = importlib.util.spec_from_file_location('mm_title_generator', NAMES / 'generate_names.py')
        generator = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(generator)
        self.assertIn(('gCojiroNameTex', 'Cojiro'), generator.ALL_ITEMS)
        self.assertEqual(generator.verify_file(texture), [])
        font = ROOT / 'soh/assets/custom/fonts/CenturyGothicBold.ttf'
        size, baseline = generator.pick_base_size(str(font))
        with tempfile.TemporaryDirectory() as directory:
            regenerated = Path(directory) / texture.name
            generator.generate_name_texture('Cojiro', str(font), size, baseline, regenerated)
            self.assertEqual(texture.read_bytes(), regenerated.read_bytes())


if __name__ == '__main__':
    unittest.main()
