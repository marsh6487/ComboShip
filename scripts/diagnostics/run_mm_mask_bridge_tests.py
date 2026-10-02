"""Run all 24 imported masks through production producer, resolver and MM draw dispatch."""
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

ROOT=Path(__file__).resolve().parents[2]
def function(source,name):
    token=source.index(name+'(')
    start=source.rfind('\n',0,token)+1
    brace=source.index('{',token);depth=1;end=brace+1
    while depth:
        depth+=(source[end]=='{')-(source[end]=='}');end+=1
    return source[start:end]
owner=(ROOT/'combo/menu/ComboItemDrawOOT.h').read_text()
draw=(ROOT/'soh/soh/Enhancements/randomizer/draw.cpp').read_text()
host=(ROOT/'combo/menu/ComboForeignDrawMM.h').read_text()
fixture=(ROOT/'tests/mm_presentation/mm_mask_bridge_test.cpp').read_text()
a=draw.index('typedef enum {\n    MM_MASK_DRAW_OPA0_XLU1');b=draw.index('\n#ifdef COMBO_BUILD',a)
fixture=fixture.replace('/* MASK_TABLE */',draw[a:b]+'\n'+function(draw,'OOT_DescribeMmMaskDraw')+'\n'+function(draw,'MmRemainsGetDrawDL')+'\n'+function(draw,'OOT_DescribeMmRemainsDraw'))
a=owner.index('static int32_t OOT_DescribeCustomDraw(');b=owner.index('    // RPG stat models',a)
fixture=fixture.replace('/* OWNER_DESCRIPTOR */',owner[a:b]+'    return 0;\n}\n'+function(owner,'OOT_FillItemDrawInfo'))
a=host.index('struct ComboForeignDrawInfoOOT {');b=host.index('\n};',a)+3
fixture=fixture.replace('/* HOST_INFO */',host[a:b])
fixture=fixture.replace('/* HOST_RESOLVER */',function(host,'ComboFillForeignDrawInfoOOT'))
fixture=fixture.replace('/* HOST_SIMPLE_DRAW */',function(host,'MM_DrawForeignSimple'))
foreign=function(host,'MM_DrawComboForeign')
handlers=set(re.findall(r'\b(MM_DrawForeign\w+)\(info\)',foreign))
fixture=fixture.replace('/* OTHER_DRAW_HANDLERS */','\n'.join('void '+n+'(const ComboForeignDrawInfoOOT*) { assert(0); }' for n in sorted(handlers) if n!='MM_DrawForeignSimple'))
fixture=fixture.replace('/* HOST_DISPATCH */',foreign)
# Named RG and ITEM ordering must match the real registrations and native table.
ids=re.findall(r'RANDO_ENUM_ITEM\((RG_MM_MASK_\w+)\)',(ROOT/'soh/soh/Enhancements/randomizer/randomizerEnums/RandomizerGet.h').read_text())
assert len(ids)==24
items=(ROOT/'soh/soh/Enhancements/randomizer/item_list.cpp').read_text()
for index,name in enumerate(ids):
    row=next(line for line in items.splitlines() if 'itemTable['+name+']' in line)
    assert 'ITEM_MM_MASK_'+name[len('RG_MM_MASK_'):] in row
assert '0x7F' not in function(draw,'OOT_DescribeMmMaskDraw')
with tempfile.TemporaryDirectory(prefix='mm-mask-bridge-') as td:
    path=Path(td)/'test.cpp';path.write_text(fixture);binary=Path(td)/'test'
    flags=['-std=c++20','-Wall','-Wextra','-Wno-unused-parameter']
    if '--sanitize' in sys.argv: flags+=['-g','-fsanitize=address,undefined','-fno-omit-frame-pointer','-fno-pie','-no-pie']
    subprocess.run([os.environ.get('CXX','c++'),*flags,'-I'+str(ROOT),'-I'+str(ROOT/'soh/include'),str(path),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True,env={**os.environ,'ASAN_OPTIONS':os.environ.get('ASAN_OPTIONS','detect_leaks=0')})
