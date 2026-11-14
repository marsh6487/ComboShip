#!/usr/bin/env python3
"""Compile and execute the actual OoT player receipt boundary and startup adapter."""
import os, re, subprocess, tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
def block(source, signature):
    start=source.index('{', source.index(signature)); end=start+1; depth=1
    while depth:
        depth+=(source[end]=='{')-(source[end]=='}'); end+=1
    return source[start:end]
def flags(combo=True):
    out=['-DF3DEX_GBI_2','-DLOG_LEVEL_GAME_PRINTS=0','-DNON_EQUIVALENT','-DNON_MATCHING','-DCONTROLLERBUTTONS_T=uint32_t','-DSOH_BUILD_DLL']
    if combo: out+=['-DCOMBO_BUILD']
    for path in ('CMake/lus-cvars.cmake','CMake/soh-cvars.cmake'):
        out += [f'-D{k}="{v}"' for k,v in re.findall(r'set\((CVAR_PREFIX_\w+)\s+"?([^\s"\)]+)',(ROOT/path).read_text())]
    out += ['-I'+str(ROOT/p) for p in ('soh/include','soh/include/PR','soh/src','soh','soh/assets','soh/mods','soh/soh','libultraship/include','libultraship/src','combo','combo/menu','combo/gui','torch/lib/n64graphics')]
    return out
with tempfile.TemporaryDirectory(prefix='startup-oot-') as tmp:
    build=Path(tmp)
    source=(ROOT/'soh/src/overlays/actors/ovl_player_actor/z_player.c').read_text()
    (build/'oot_receive.inc').write_text('s32 NativeReceive(PlayState* play, Player* this) '+block(source,'s32 func_8084DFF4(')+'\nvoid NativeFast(PlayState* play, Player* this, GetItemEntry* giEntry) '+block(source,'void func_8083E4C4('))
    for combo in (False,True):
        binary=build/('combo' if combo else 'original')
        command=[os.environ.get('CC','cc'),'-std=gnu11','-Wno-incompatible-pointer-types','-Wno-int-conversion',*flags(combo),'-I'+tmp,str(ROOT/'tests/startup_receipt/oot_player_test.c'),'-o',str(binary)]
        subprocess.run(command,check=True,cwd=ROOT)
        subprocess.run([str(binary)],check=True)
    if '--native' in __import__('sys').argv:
        for combo in (False,True):
            for name in ('soh/src/overlays/actors/ovl_player_actor/z_player.c',
                         'soh/src/overlays/gamestates/ovl_file_choose/z_file_choose.c',
                         'soh/src/overlays/gamestates/ovl_file_choose/z_file_nameset_PAL.c',
                         'soh/src/overlays/gamestates/ovl_file_choose/z_file_nameset_NES.c',
                         'soh/src/code/z_sram.c',
                         'soh/src/overlays/gamestates/ovl_opening/z_opening.c'):
                command=[os.environ.get('CC','cc'),'-std=gnu11',*flags(combo),'-Wno-incompatible-pointer-types','-Wno-int-conversion','-Wno-discarded-qualifiers','-Werror=implicit-function-declaration','-c',str(ROOT/name),'-o',str(build/(Path(name).stem+str(combo)+'.o'))]
                result=subprocess.run(command,capture_output=True,text=True,cwd=ROOT)
                if result.returncode: raise RuntimeError(result.stdout+result.stderr)
                print(f'PASS complete native C TU: {name} (combo={combo}, existing warnings={result.stderr.count("warning:")})')

    adapter=ROOT/'soh/soh/Enhancements/Gameplay/ComboStartupReceipt.cpp'
    assert adapter.exists(), 'startup adapter is missing'
    core=adapter.read_text().split('// BEGIN TESTED STARTUP CORE')[1].split('// END TESTED STARTUP CORE')[0]
    (build/'oot_core.inc').write_text(core)
    binary=build/'adapter'
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++20',*flags(),'-I'+tmp,str(ROOT/'tests/startup_receipt/oot_adapter_test.cpp'),'-o',str(binary)],check=True,cwd=ROOT)
    subprocess.run([str(binary)],check=True)
    draw=(ROOT/'combo/menu/ComboForeignDrawOOT.h').read_text()
    select=draw[draw.index('inline void OOT_DrawComboForeign('):]
    select=select[:select.index('    if (ComboDrawMagicMirrorForeign')]+'}\n'
    select=select.replace('inline void OOT_DrawComboForeign(PlayState* play, GetItemEntry* getItemEntry, bool shop = false)', 'void SelectForeign(void* play, GetItemEntry* getItemEntry)')
    (build/'oot_foreign.inc').write_text(select)
    binary=build/'foreign'
    subprocess.run(['c++','-std=c++20','-I'+str(ROOT/'combo'),'-I'+tmp,str(ROOT/'tests/startup_receipt/oot_foreign_test.cpp'),'-o',str(binary)],check=True,cwd=ROOT)
    subprocess.run([str(binary)],check=True)
    print('PASS actual foreign draw dispatch: frozen identity bypasses both check lookup and queue fallback, including stale tokens')
    if '--native-cpp' in __import__('sys').argv:
        for combo in (False,True):
            for name in ('soh/soh/Enhancements/Gameplay/ComboStartupReceipt.cpp','soh/soh/Enhancements/randomizer/savefile.cpp','soh/soh/Enhancements/randomizer/draw.cpp','soh/soh/Enhancements/randomizer/item_list.cpp'):
                result=subprocess.run(['c++','-std=c++20',*flags(combo),'-c',str(ROOT/name),'-o',str(build/(Path(name).stem+str(combo)+'.o'))],capture_output=True,text=True,cwd=ROOT)
                if result.returncode: raise RuntimeError(result.stdout+result.stderr)
                print(f'PASS complete native C++ TU: {name} (combo={combo}, existing warnings={result.stderr.count("warning:")})')
    save=(ROOT/'soh/soh/Enhancements/randomizer/savefile.cpp').read_text()
    seam=block(save,'    // Starting with an unshuffled letter skips child Zelda.')
    eligibility='if (Randomizer_GetSettingValue(RSK_STARTING_ZELDAS_LETTER) && !Randomizer_GetSettingValue(RSK_SHUFFLE_ZELDAS_LETTER)) '
    (build/'oot_save_seam.inc').write_text('void InitImpa(){'+eligibility+seam+'}\n')
    for combo in (False,True):
        binary=build/('save-'+str(combo))
        subprocess.run(['c++','-std=c++20',*(['-DCOMBO_BUILD'] if combo else []),'-I'+tmp,str(ROOT/'tests/startup_receipt/oot_save_seam_test.cpp'),'-o',str(binary)],check=True,cwd=ROOT)
        subprocess.run([str(binary)],check=True)
    guards=(
      ('soh/src/overlays/gamestates/ovl_file_choose/z_file_choose.c','            this->selectMode = SM_FADE_OUT;'),
      ('soh/src/overlays/gamestates/ovl_file_choose/z_file_choose.c','    gSaveContext.fileNum = this->buttonIndex;'),
      ('soh/src/overlays/gamestates/ovl_file_choose/z_file_nameset_PAL.c','                            gSaveContext.fileNum = this->buttonIndex;'),
      ('soh/src/overlays/gamestates/ovl_file_choose/z_file_nameset_NES.c','                                gSaveContext.fileNum = this->buttonIndex;'),
      ('soh/src/code/z_sram.c','    Save_LoadFile();'),
      ('soh/soh/Enhancements/debugconsole.cpp','    SET_NEXT_GAMESTATE(gGameState, TitleSetup_Init, GameState);'),
      ('soh/src/overlays/gamestates/ovl_opening/z_opening.c','    gSaveContext.gameMode = GAMEMODE_TITLE_SCREEN;'))
    context=''
    for i,(name,mutation) in enumerate(guards):
        source=(ROOT/name).read_text();before=source[:source.index(mutation)]
        begin=before.rfind('#ifdef COMBO_BUILD');fragment=before[begin:before.index('#endif',begin)+6]
        assert 'if (!ComboStartupOot_PrepareContextChange())' in fragment, name+' missing pre-overwrite guard'
        context+=f'void Attempt{i}(void){{\n'+fragment+'\nmutations++;}\n'
    context+='void (*attempts[])(void)={'+','.join(f'Attempt{i}' for i in range(7))+'};\n'
    (build/'oot_context.inc').write_text(context)
    for combo in (False,True):
        binary=build/('context-'+str(combo))
        subprocess.run(['cc','-std=gnu11',*(['-DCOMBO_BUILD'] if combo else []),'-I'+tmp,str(ROOT/'tests/startup_receipt/oot_context_test.c'),'-o',str(binary)],check=True,cwd=ROOT)
        subprocess.run([str(binary)],check=True)
    producer=(ROOT/'combo/menu/ComboItemDrawOOT.h').read_text()
    descriptor=producer[producer.index('static int32_t OOT_FillItemDrawInfo('):]
    descriptor=descriptor[:descriptor.index('    if (effRg == RG_MAGIC_MIRROR)')]+'    seen = gi; seenRg = effRg; return 1;\n}\n'
    export='int32_t OOT_GetFrozenItemDrawInfo(const CwStartupReceipt*,CwItemDrawInfo*){return 0;}'
    if 'int32_t OOT_GetFrozenItemDrawInfo(' in producer:
        export='int32_t OOT_GetFrozenItemDrawInfo(const CwStartupReceipt* receipt, CwItemDrawInfo* out) '+block(producer,'int32_t OOT_GetFrozenItemDrawInfo(')
    restore='bool RestoreFrozenDraw(const CwStartupReceipt* receipt, GetItemEntry* entry) '+block(adapter.read_text(),'bool RestoreFrozenDraw(')+'\nbool ComboStartupOot_RestoreFrozenDraw(const CwStartupReceipt* receipt,GetItemEntry* entry){return RestoreFrozenDraw(receipt,entry);}'
    anim='int32_t OOT_GetFrozenItemAnimDrawInfo(const CwStartupReceipt*,CwItemAnimDrawInfo*){return 0;}'
    if 'int32_t OOT_GetFrozenItemAnimDrawInfo(' in producer:
        anim='int32_t OOT_GetFrozenItemAnimDrawInfo(const CwStartupReceipt* receipt,CwItemAnimDrawInfo* out) '+block(producer,'int32_t OOT_GetFrozenItemAnimDrawInfo(')
    (build/'oot_frozen_draw.inc').write_text(restore+descriptor+export+anim)
    binary=build/'frozen-draw'
    subprocess.run(['c++','-std=c++20',*flags(),'-I'+tmp,str(ROOT/'tests/startup_receipt/oot_frozen_draw_test.cpp'),'-o',str(binary)],check=True,cwd=ROOT)
    subprocess.run([str(binary)],check=True)
