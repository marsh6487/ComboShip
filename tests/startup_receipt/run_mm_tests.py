#!/usr/bin/env python3
"""Compile real MM receipt boundaries with native actor/save types and boundary doubles."""
from pathlib import Path
import os, re, shlex, subprocess, sys, tempfile
ROOT=Path(__file__).resolve().parents[2]

def function(source, signature):
    start=source.index(signature); brace=source.index('{',start); end=brace+1; depth=1
    while depth:
        depth+=(source[end]=='{')-(source[end]=='}');end+=1
    return source[start:end]

adapter=ROOT/'mm/2s2h/Enhancements/Gameplay/ComboStartupReceipt.cpp'
assert adapter.exists(), 'MM startup actor-owned presentation adapter is missing'
source=adapter.read_text()
body=source[source.index('namespace {')+len('namespace {'):source.index('void Register()')]
ben=(ROOT/'mm/2s2h/BenPort.cpp').read_text()
grant='\n'.join(function(ben,s) for s in ('static bool Combo_IsBottleRefill(', 'static void Combo_MM_GiveDormantResolvedInternal(', 'void Combo_MM_GiveDormantResolved(', 'static int StartupImportance(', 'extern "C" COMBO_EXPORT int32_t MM_FinishStartupReceipt('))
foreign=(ROOT/'combo/menu/ComboForeignDrawMM.h').read_text()
selection=function(foreign,'inline void MM_DrawComboForeign(')
selection=selection[:selection.index('#ifdef COMBO_GI_RECEIPT_TRACE')]+"    shown=info->resolvedName;pickup=mmPickup;\n}\n"
resolver=function(foreign,'inline ComboForeignResolveOOT ComboFillForeignDrawInfoOOT(')
resolver=resolver[:resolver.index('        if (rcAnim != 0) {')]+"        info.resolvedName=drawName;info.ok=rcAnim==1;return info.ok?ComboForeignResolveOOT::Ok:ComboForeignResolveOOT::NotReady;\n    }\n    info.resolvedName=drawName;info.ok=rcStatic==1;return info.ok?ComboForeignResolveOOT::Ok:ComboForeignResolveOOT::NotReady;\n}\n"
foreign_body=resolver+selection+function(foreign,'inline void MM_DrawFrozenForeign(')
items=(ROOT/'mm/2s2h/Rando/StaticData/Items.cpp').read_text()
policy=function(items, 'bool ShouldShowGetItemCutscene(')
oot=(ROOT/'soh/soh/Enhancements/Gameplay/ComboStartupReceipt.cpp').read_text()
categories=(ROOT/'soh/soh/Enhancements/item-tables/ItemTableTypes.h').read_text()
oot_policy=re.search(r'typedef enum GetItemCategory \{.*?\} GetItemCategory;',categories,re.S).group(0)
oot_policy+='\nenum {SGIA_DISABLED,SGIA_JUNK,SGIA_ALL};\n'+function(oot,'bool SkipAnimation(')
flags=['-std=c++20','-w','-DF3DEX_GBI_2','-DCOMBO_BUILD']
for path in ('','mm','mm/include','mm/include/PR','mm/2s2h','mm/src','mm/assets','libultraship/include','combo','combo/menu'):
    flags+=['-I'+str(ROOT/path)]
if '--json-include' in sys.argv:
    flags+=['-I'+sys.argv[sys.argv.index('--json-include')+1]]
elif os.environ.get('NLOHMANN_JSON_INCLUDE'):
    flags+=['-I'+os.environ['NLOHMANN_JSON_INCLUDE']]
if '--sanitize' in sys.argv:
    flags+=['-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-pie','-no-pie']
with tempfile.TemporaryDirectory(prefix='startup-mm-') as tmp:
    for name,code in [('mm_presentation_test.cpp',body),('mm_finish_test.cpp',grant),('mm_foreign_test.cpp',foreign_body)]:
        fixture=(ROOT/'tests/startup_receipt'/name).read_text().replace('/* PRODUCTION */',code).replace('/* NATIVE_POLICY */',policy).replace('/* OOT_POLICY */',oot_policy)
        cpp=Path(tmp)/name;cpp.write_text(fixture);binary=cpp.with_suffix('')
        subprocess.run([*shlex.split(os.environ.get('CXX','c++')),*flags,str(cpp),'-o',str(binary)],check=True)
        subprocess.run([str(binary)],check=True,env={**os.environ, **({"ASAN_OPTIONS": "detect_leaks=0"} if "--sanitize" in sys.argv else {})})
print('PASS MM production-boundary startup receipt suite')
