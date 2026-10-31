"""Execute OoT's portable native-equipment callback dispatcher."""
from pathlib import Path
import os,subprocess,sys,tempfile
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'scripts/diagnostics'))
from run_time_pedestal_tests import functions
source=functions((ROOT/'combo/menu/ComboForeignDrawOOT.h').read_text())
names=['IronKnuckleAxe','ExtSpiritBreastplate','ExtChampionsTunic','ExtSagesTunic','ExtPegasusAnklet','ExtTrident','ExtClimbBoots','ExtRocBoots']
code='#include "combo/menu/ComboItemDrawABI.h"\n#include <cassert>\n#include <iostream>\nstruct PlayState{};struct GetItemEntry{};struct ComboForeignDrawInfo{CwDrawOp ops[CW_DRAW_MAX_OPS];};int called=-1,depth=0;void Matrix_Push(){++depth;}void Matrix_Pop(){assert(depth>0);--depth;}\n'
for i,name in enumerate(names):code+=f'void Randomizer_Draw{name}(PlayState*,GetItemEntry*){{called={i};}}\n'
code+=source.get('OOT_DrawForeignNativeEquipment','void OOT_DrawForeignNativeEquipment(PlayState*,GetItemEntry*,const ComboForeignDrawInfo*){}')
code+='\nint main(){for(int i=0;i<8;++i){ComboForeignDrawInfo info{};info.ops[0].a=CW_OOT_EQUIP_AXE+i;OOT_DrawForeignNativeEquipment(nullptr,nullptr,&info);assert(called==i&&depth==0&&"returned imported equipment must reach its real OoT native GI callback");}std::cout<<"PASS OoT native-equipment callback transport, eight concrete identities and balanced matrix scope\\n";}\n'
with tempfile.TemporaryDirectory(prefix='oot-equipment-bridge-') as td:
    p=Path(td)/'test.cpp';p.write_text(code);binary=Path(td)/'test'
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror','-I'+str(ROOT),str(p),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
