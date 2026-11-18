from pathlib import Path
import os,re,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[2]
def block(source,name):
    match=re.search(r'static bool '+name+r'\([^{}]*\)\s*\{',source)
    assert match,name
    end=match.end();depth=1
    while depth:
        depth+=(source[end]=='{')-(source[end]=='}');end+=1
    return source[match.start():end]
for host in ['soh','mm']:
    bridge=ROOT/host/('soh/Enhancements/cosmetics/ZeldaRun.cpp' if host=='soh' else '2s2h/Enhancements/Graphics/ZeldaRun.cpp')
    source=bridge.read_text()
    with tempfile.TemporaryDirectory(prefix='zelda-run-') as tmp:
        work=Path(tmp)
        (work/'native_eligibility.inc').write_text(block(source,'ZeldaRun_Adult')+'\n'+block(source,'ZeldaRun_Eligible'))
        flags=['-std=c++20','-DF3DEX_GBI_2','-DCOMBO_BUILD','-DLOG_LEVEL_GAME_PRINTS=0','-fno-pie','-no-pie',
               '-fsanitize=address,undefined','-fno-sanitize-recover=all']
        if host=='mm':flags+=['-DZELDA_TEST_MM','-DCONTROLLERBUTTONS_T=uint32_t']
        for p in ['',host,host+'/include',host+'/include/PR',host+'/src',host+'/assets',host+'/mods',
                  host+('/soh' if host=='soh' else '/2s2h'),'libultraship/include','libultraship/src','combo/menu']:
            flags+=['-I'+str(ROOT/p)]
        subprocess.run(['g++-14',*flags,'-I'+str(work),str(ROOT/'tests/zelda_run/native_test.cpp'),'-o',str(work/'test')],check=True)
        subprocess.run([str(work/'test')],check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
        # Only the two native run selection sites are wrapped, before native range calculation.
        player=(ROOT/host/'src/overlays/actors/ovl_player_actor/z_player.c').read_text()
        assert player.count('ZeldaRun_Select(this,')==2
        assert 'ZeldaRun_Select(this, GET_PLAYER_ANIM(PLAYER_ANIMGROUP_walk' not in player
