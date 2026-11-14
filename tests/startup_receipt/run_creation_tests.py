#!/usr/bin/env python3
"""Exercise the real new-save receipt and foreign-map boundary together."""
import os, re, subprocess, sys, tempfile
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
def production(path):
    if '--source-ref' in sys.argv:
        ref=sys.argv[sys.argv.index('--source-ref')+1]
        return subprocess.check_output(['git','show',f'{ref}:{path}'],cwd=ROOT,text=True)
    return (ROOT/path).read_text()
def function(source, signature):
    start=source.index(signature); brace=source.index('{',start); end=brace+1; depth=1
    while depth:
        depth+=(source[end]=='{')-(source[end]=='}'); end+=1
    return source[start:end]
host=production('combo/ComboShip.cpp')
signatures=(
    'static std::string ComboReleaseMajorMinor(', 'static nlohmann::json& LoadOrCreateContainer(',
    'static bool WriteContainerDocument(', 'static bool FlushContainer(',
    'static const char* Combo_ReadGameSave(', 'static void Combo_WriteGameSave(',
    'static void Combo_CompleteStartupReceipt(', 'static bool Combo_PreserveStartupMmState(',
    'static int32_t Combo_StartupReceipt(', 'static void Combo_CopyContainer(',
)
host_body='\n'.join(function(host,s) for s in signatures)
# Keep the parent build runnable so the reproduction fails at the missing
# receipt assertion, not at an absent helper or an artificial compile error.
helper='static void Combo_PrepareStartupReceiptSeed() {'
if helper in host:
    host_body=function(host,helper)+'\n'+host_body
oot=production('soh/soh/Enhancements/Gameplay/ComboStartupReceipt.cpp')
capture='\n'.join(function(oot,s) for s in ('bool Active(', 'void BeginFile(', 'void CopyName(', 'bool SameEntry(', 'bool Capture('))
foreign=production('soh/soh/Enhancements/randomizer/hook_handlers.cpp')
foreign_body='\n'.join(function(foreign,s) for s in ('const ComboRando::ForeignItem* OOT_LookupForeign(',
    'const ComboRando::ForeignItem* OOT_LookupForeignByCheck(', 'static GetItemCategory ForeignCategoryFromName(',
    'GetItemCategory OOT_GetForeignCategory(', 'extern "C" COMBO_EXPORT void SOH_LoadComboRando('))
foreign_body=foreign_body.replace('extern "C" COMBO_EXPORT void SOH_LoadComboRando(', 'static void LoadOotForeign(')
save=production('soh/soh/Enhancements/randomizer/savefile.cpp')
seam=function(save,'    // Starting with an unshuffled letter skips child Zelda.')
init=function(save,'extern "C" void Randomizer_InitSaveFile(')
prefix=init[init.index('{')+1:init.index('    auto ctx =')]
save_body='void InitImpa(){'+prefix+seam+'}\n'
fixture=(ROOT/'tests/startup_receipt/creation_test.cpp').read_text().replace('/* HOST_FUNCTIONS */',host_body).replace('/* CAPTURE_FUNCTIONS */',capture).replace('/* FOREIGN_FUNCTIONS */',foreign_body).replace('/* SAVE_SEAM */',save_body)
flags=['-std=c++20','-DF3DEX_GBI_2','-DLOG_LEVEL_GAME_PRINTS=0','-DNON_EQUIVALENT','-DNON_MATCHING','-DCONTROLLERBUTTONS_T=uint32_t','-DSOH_BUILD_DLL','-DCOMBO_BUILD']
for path in ('CMake/lus-cvars.cmake','CMake/soh-cvars.cmake'):
    flags += [f'-D{k}="{v}"' for k,v in re.findall(r'set\((CVAR_PREFIX_\w+)\s+"?([^\s"\)]+)',(ROOT/path).read_text())]
flags += ['-I'+str(ROOT/p) for p in ('','soh/include','soh/include/PR','soh/src','soh','soh/assets','soh/mods','soh/soh','libultraship/include','libultraship/src','combo','combo/menu','combo/gui','torch/lib/n64graphics')]
if '--json-include' in sys.argv: flags += ['-I'+sys.argv[sys.argv.index('--json-include')+1]]
if '--sanitize' in sys.argv: flags += ['-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-pie','-no-pie']
with tempfile.TemporaryDirectory(prefix='startup-creation-') as tmp:
    cpp=Path(tmp)/'creation.cpp';cpp.write_text(fixture);binary=Path(tmp)/'creation'
    subprocess.run([os.environ.get('CXX','c++'),*flags,str(cpp),'-o',str(binary)],check=True,cwd=ROOT)
    subprocess.run([str(binary),tmp],check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
