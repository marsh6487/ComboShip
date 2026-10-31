"""Compile production entrance routing functions; verify tracing preserves each route."""
import pathlib
import subprocess
import tempfile

root = pathlib.Path(__file__).parent
source_root = root / 'candidate' if (root / 'candidate').is_dir() else root.parents[1]
source = (source_root / 'soh/soh/Enhancements/randomizer/randomizer_entrance.c').read_text()

def function(signature):
    start = source.index(signature)
    end = source.index('{', start) + 1
    depth = 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]

prefix = r'''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
typedef int16_t s16;
#define COMBO_BUILD 1
#define ARRAY_COUNT(x) (sizeof(x)/sizeof((x)[0]))
void trace(const char*, ...) __attribute__((format(printf,1,2)));
void trace(const char* fmt, ...) {(void)fmt;}
#define LUSLOG_INFO(...) trace(__VA_ARGS__)
enum {SCENE_CASTLE_COURTYARD_GUARDS_DAY=80, SCENE_CASTLE_COURTYARD_GUARDS_NIGHT=81,
      ENTR_CASTLE_GROUNDS_RAINBOW_BRIDGE_EXIT=30};
typedef struct {int entranceIndex, roomIndex, playerParams; struct {float x,y,z;} pos;} RespawnData;
struct {int fileNum, respawnFlag; RespawnData respawn[3];} gSaveContext;
struct PlayState {int sceneNum;};
struct PlayState play={1}; struct PlayState* gPlayState=&play;
static s16 dynamicExitList[]={100};
static int nativeCalls, grottoCalls, beforeCalls, afterCalls;
static int before=-1, after=-1, native=100;
int Combo_IsCombinedGame(void){return 1;}
void Entrance_SetEntranceDiscovered(s16 a, int b){(void)a;(void)b;}
void EntranceTracker_SetLastEntranceOverride(s16 a){(void)a;}
s16 Entrance_GetOverride(s16 a){(void)a;++nativeCalls;return native;}
int32_t Combo_SOH_CrossDungeonRoute(int32_t a,int b){(void)a;if(b){++beforeCalls;return before;}++afterCalls;return after;}
s16 Grotto_OverrideSpecialEntrance(s16 a){++grottoCalls;if(a==0x800){gSaveContext.respawnFlag=2;return 582;}return a;}
'''
main = r'''
int main(void) {
 assert(Entrance_OverrideNextIndex(100)==100 && nativeCalls==1 && beforeCalls==1 && afterCalls==1);
 puts("native-route PASS");
 native=200;assert(Entrance_OverrideNextIndex(100)==200);
 puts("native-override PASS");
 before=300;nativeCalls=afterCalls=0;assert(Entrance_OverrideNextIndex(100)==300 && nativeCalls==0 && afterCalls==0);
 puts("before-native-route PASS");
 before=-1;after=400;assert(Entrance_OverrideNextIndex(100)==400);
 puts("after-native-route PASS");
 after=0x800;grottoCalls=0;assert(Entrance_OverrideNextIndex(100)==582 && grottoCalls==1 && gSaveContext.respawnFlag==2);
 puts("positioned-grotto PASS");
 assert(Entrance_OverrideDynamicExit(0)==582 && gSaveContext.respawnFlag==2);
 puts("dynamic-exit PASS");
 gSaveContext.respawnFlag=99;assert(Entrance_TraceResolved("invalid-record",100,200,300)==300);
 puts("trace-invalid-respawn PASS");
}
'''
with tempfile.TemporaryDirectory() as folder:
    path = pathlib.Path(folder)
    test = path / 'test.c'
    test.write_text(prefix + '\n'.join(function(s) for s in [
        'static s16 Entrance_TraceResolved(', 's16 Entrance_OverrideNextIndex(', 's16 Entrance_OverrideDynamicExit('
    ]) + main)
    subprocess.run(['cc', '-std=c11', '-Werror=format', '-fsanitize=undefined', '-fno-sanitize-recover=all',
                    str(test), '-o', str(path / 'test')], check=True)
    subprocess.run([str(path / 'test')], check=True)
