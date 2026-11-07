#!/usr/bin/env python3
"""Run production Malon routing and the Ranch's existing night lifecycle."""
from pathlib import Path
import re, subprocess, tempfile, os
ROOT=Path(__file__).resolve().parents[2]
def function(source,name):
    m=re.search(r'^(?:static )?(?:void|bool|u8) '+name+r'\([^;]*?\)\s*\{',source,re.M)
    assert m, name
    depth=1; end=m.end()
    while depth:
        depth+=(source[end]=='{')-(source[end]=='}');end+=1
    return source[m.start():end]
with tempfile.TemporaryDirectory(prefix='ranch-music-') as tmp:
    src=(ROOT/'soh/src/code/code_800EC960.c').read_text()
    helpers=''
    if 'static bool Audio_UseRanchMusicSelection(' in src:
        helpers=function(src,'Audio_UseRanchMusicSelection')+'\n'+function(src,'Audio_BypassRanchProximityMusic')
    code=r'''
#include <cassert>
#include <cstdint>
#include <vector>
#include <string>
using u8=uint8_t;using u16=uint16_t;using s8=int8_t;using s16=int16_t;using f32=float;
constexpr int SEQ_PLAYER_BGM_MAIN=0,SEQ_PLAYER_FANFARE=1,SEQ_PLAYER_BGM_SUB=3,CS_STATE_IDLE=0,NA_BGM_LONLON=47,NA_BGM_NATURE_AMBIENCE=1,NA_BGM_DISABLED=65535,SCENE_LON_LON_RANCH=99;
struct PlayState {int sceneNum=99;struct {int state=0;} csCtx;};PlayState play;PlayState* gPlayState=&play;
int enabled=0,preview=0;bool pending=false,combat=false,owned=false,cutscene=false;
u16 active[4]={47,65535,65535,65535};
u8 sMalonSingingDisabled=0,sMalonSingingTimer=0;bool sIsMalonSinging;float sMalonSingingDist;
std::vector<int> commands;int channels[16]={};
int CVarGetInteger(const char* key,int){return std::string(key)=="gAudioEditor.Playing"?preview:enabled;}
bool Player_InCsMode(PlayState*){return cutscene;}
u16 func_800FA0B4(int p){return active[p];}
bool Audio_HasPendingSubBgmCommand(){return pending;}
bool Audio_IsSceneNightMusicInCombat(){return combat;}
bool Audio_IsSceneNightBgmActive(){return owned;}
void Audio_StopSceneNightBgm(){owned=false;}
void Audio_SeqCmd6(int,int,int channel,int volume){channels[channel]=volume;commands.push_back(6);}
void Audio_SeqCmdA(int,int){commands.push_back(10);}
void Audio_SeqCmd1(int p,int){commands.push_back(1);active[p]=65535;}
void Audio_PlaySequenceWithSeqPlayerIO(int p,int id,int,int,int){commands.push_back(0);active[p]=id;}
'''+helpers+'\n'+function(src,'Audio_UpdateMalonSinging')+'\n'+function(src,'Audio_ToggleMalonSinging')+r'''
int main(){
 Audio_UpdateMalonSinging(100,47);assert(commands.size()==3); // unchanged OFF/day
 commands.clear();enabled=1;sMalonSingingTimer=0;Audio_UpdateMalonSinging(100,47);assert(channels[0]==127&&channels[1]==127&&channels[13]==127);commands.clear();
 for(int age=0;age<2;++age)for(float distance:{100.f,1000.f,3000.f}) {
  Audio_UpdateMalonSinging(distance,47);Audio_ToggleMalonSinging(1);Audio_ToggleMalonSinging(0);
  assert(commands.empty()&&"enabled Ranch proximity must never fade/restart the editor's track");
 }
 enabled=0;active[0]=1;sMalonSingingTimer=0;Audio_UpdateMalonSinging(100,47);
 assert(active[3]==47&&commands.size()==4); // unchanged OFF/night, native original ID even for custom bank
 enabled=1;commands.clear();Audio_UpdateMalonSinging(100,47);
 assert(active[3]==65535&&commands.size()==1&&"live enable must release only native Malon SUB");
 commands.clear();active[0]=47;sMalonSingingTimer=1;preview=1;Audio_UpdateMalonSinging(100,47);assert(commands.empty());preview=0;
 active[1]=20;Audio_UpdateMalonSinging(100,47);assert(commands.empty());active[1]=65535;
 cutscene=true;Audio_UpdateMalonSinging(100,47);assert(commands.empty());cutscene=false;
 active[0]=1;active[3]=26;combat=true;sMalonSingingTimer=10;Audio_UpdateMalonSinging(100,47);
 assert(active[3]==26&&commands.empty());combat=false;
 active[3]=47;owned=true;Audio_UpdateMalonSinging(100,47);assert(commands.empty());owned=false;
 pending=true;Audio_UpdateMalonSinging(100,47);assert(commands.empty());pending=false;
 play.sceneNum=1;active[0]=47;Audio_UpdateMalonSinging(100,47);assert(commands.size()==3); // market/other Malon unchanged
 enabled=0;play.sceneNum=99;commands.clear();Audio_UpdateMalonSinging(100,47);assert(commands.size()==3);
}
'''
    cpp=Path(tmp)/'native.cpp';cpp.write_text(code);binary=Path(tmp)/'native'
    subprocess.run(['c++','-std=c++20','-fsanitize=address,undefined','-no-pie',str(cpp),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True,env={**os.environ,"ASAN_OPTIONS":"detect_leaks=0"})
    host=(ROOT/'tests/night_music/host_test.cpp').read_text()
    production=(ROOT/'soh/soh/Enhancements/audio/SceneNightMusic.cpp').read_text()
    production=production[production.index('static ComboNightMusic::Lifecycle'):production.index('static void RegisterSceneNightMusic')]
    host=host.replace('sceneNum=82,sceneId=82','sceneNum=99,sceneId=99')
    host=host.replace('cvars["gAudioEditor.EnableNightMusic"]=1;', 'cvars["gAudioEditor.DisableRanchProximityMusic"]=1;')
    host=host.replace('cvars["gAudioEditor.EnableNightMusic"]=0;', 'cvars["gAudioEditor.DisableRanchProximityMusic"]=0;')
    # The shared fixture's final cross-scene virtual-row assertion belongs to the global option.
    host=host.replace('sceneRow=0xff00;frame();', 'cvars["gAudioEditor.EnableNightMusic"]=1;sceneRow=0xff00;frame();')
    cpp.write_text(host.replace('/* PRODUCTION */',production))
    subprocess.run(['c++','-std=c++20','-DTEST_MM=0','-DCOMBO_BUILD','-I'+str(ROOT/'combo'),str(cpp),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True,env={**os.environ,"ASAN_OPTIONS":"detect_leaks=0"})
    # Integrate all three production layers; independent boundary fixtures cannot
    # detect an old owner stopping a new native start that resolves to the same bank.
    integrated=(ROOT/'tests/ranch_music/queued_handoff_test.cpp').read_text()
    macros=[]
    lines=iter(src.splitlines())
    for line in lines:
        if re.match(r'#define Audio_(?:StartSeq|SeqCmd[167A])\(',line):
            macro=line
            while macro.endswith('\\'):
                macro=macro[:-1]+next(lines)
            macros.append(macro)
    macros="\n".join(macros)
    native=helpers+'\n'+function(src,'Audio_PlaySequenceWithSeqPlayerIO')+'\n'+function(src,'Audio_UpdateMalonSinging')+'\n'+function(src,'Audio_ToggleMalonSinging')
    cpp.write_text(integrated.replace('/* MACROS */',macros).replace('/* NATIVE */',native).replace('/* NIGHT */',production))
    include=['-I'+str(ROOT/p) for p in ['soh/tests','soh/tests/audio_sequence_stubs','soh/include','soh','libultraship/include','combo']]
    flags=['-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer','-no-pie']
    obj=Path(tmp)/'queue.o'
    subprocess.run(['cc','-std=c11',*flags,*include,'-c',str(ROOT/'soh/src/code/code_800F9280.c'),'-o',str(obj)],check=True)
    subprocess.run(['c++','-std=c++20','-DCOMBO_BUILD',*flags,*include,str(cpp),str(obj),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True,env={**os.environ,"ASAN_OPTIONS":"detect_leaks=0"})
    editor=(ROOT/'soh/soh/Enhancements/audio/AudioEditor.cpp').read_text()
    whats=(ROOT/'combo/gui/WhatsNew.h').read_text()
    assert 'MenuDrawItem(disableRanchProximityMusic, THEME_COLOR)' in editor
    registration=editor[editor.index('    disableRanchProximityMusic = {'):]
    registration=registration[:registration.index('AddSearchWidget')]
    assert 'CVAR_AUDIO("DisableRanchProximityMusic")' in registration and '.DefaultValue(false)' in registration
    assert '"gAudioEditor.DisableRanchProximityMusic", Kind::New' in whats
    print('PASS Ranch native proximity routing, night ownership/priority, default-off UI and What’s New accent')
