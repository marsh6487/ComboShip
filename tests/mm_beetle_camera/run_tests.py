#!/usr/bin/env python3
"""Exercise production Beetle ownership + native MM Play camera APIs.

Native game structs and Play create/status/clear/get code are compiled unchanged.
Camera initialization/status and collider teardown are fixture service boundaries.
No scene/age rendering or live controller reproduction is claimed.
"""
from pathlib import Path
import subprocess, tempfile, os, argparse
ROOT=Path(__file__).resolve().parents[2]
p=argparse.ArgumentParser();p.add_argument('--ref');args=p.parse_args()
def read(path):
    return subprocess.check_output(['git','show',args.ref+':'+path],cwd=ROOT,text=True) if args.ref else (ROOT/path).read_text()
def block(src, marker):
    start=src.index(marker);opening=src.index('{',start);i=opening+1;depth=1
    while depth:
        depth+=(src[i]=='{')-(src[i]=='}');i+=1
    return src[start:i]
src=read('mm/mods/items/logic/item_beetle.c');play=read('mm/src/code/z_play.c')
# Baseline intentionally fails before the owned-camera fixture when no scoped
# ownership implementation is present; the old path gets a separate reproduction.
baseline = 'static u8 Beetle_OwnsSubCam(' not in src
head='''#include "global.h"
#include "mods/nei_oot_compat.h"
#include "mods/items/logic/item_beetle.h"
#include "mods/extended_player.h"
#include "mods/items/logic/item_dekuleaf.h"
#include "mods/items/logic/item_shovel.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
CustomItemState gCustomItemState;
'''
head+=src[src.index('static u8 sBeetleColInitialized'):src.index('static void Beetle_DropGrabbedActor')]
head+='''static int statusCalls, destroyed; static s16 uid=10;
void Camera_Init(Camera* camera, View* view, CollisionContext* col, PlayState* play) {
    memset(camera,0,sizeof(*camera)); camera->uid=uid++; camera->play=play;
}
s32 Camera_ChangeStatus(Camera* camera,s16 status) { assert(camera); ++statusCalls; camera->status=status; return 1; }
s32 Collider_DestroyCylinder(PlayState* play,ColliderCylinder* collider) { ++destroyed; return 1; }
'''
for marker in ['s16 Play_CreateSubCamera(', 's16 Play_GetActiveCamId(', 's32 Play_ChangeCameraStatus(', 'void Play_ClearCamera(', 'Camera* Play_GetCamera(']:head+=block(play,marker)+'\n'
if baseline:
    head+='s32 Camera_ChangeMode(Camera* camera,s16 mode) { camera->mode=mode; return 1; }\n'
    for marker in ['static void Beetle_DestroySubCam(', 'static void Beetle_CreateSubCam(']:head+=block(src,marker)+'\n'
    head+='''int main(void) {
        static PlayState play; static Camera mainCam;
        play.cameraPtrs[CAM_ID_MAIN]=&mainCam; play.activeCamId=CAM_ID_MAIN;
        s16 native=Play_CreateSubCamera(&play);Play_ChangeCameraStatus(&play,native,CAM_STATUS_ACTIVE);
        beetleSubCamId=SUBCAM_FREE;Beetle_CreateSubCam(&play);Beetle_DestroySubCam(&play);
        assert(play.activeCamId==native && "Beetle did not restore existing native camera");
        return 0;
    }\n'''
else:
    for marker in ['static u8 Beetle_OwnsSubCam(', 'static void Beetle_EndPiloting(', 'static void Beetle_DestroySubCam(', 'static u8 Beetle_CreateSubCam(', 'void Beetle_ResetTransient(']:head+=block(src,marker)+'\n'
if not baseline:
    head+='s32 Player_UpperAction_DekuLeaf(Player* p, PlayState* play) { return 0; }\n'
    head+='s32 Player_UpperAction_Shovel(Player* p, PlayState* play) { return 0; }\n'
    head+='#undef beetleActive\n#undef beetleState\n#undef beetleSubCamId\n'
    head+=block(read('mm/mods/items/custom_items_common.c'),'s32 CustomItems_BlocksMovement(')+'\n'
    head+='#define beetleActive gCustomItemState.beetleActive\n#define beetleState gCustomItemState.beetleState\n#define beetleSubCamId gCustomItemState.beetleSubCamId\n'
fixture='''
static PlayState play; static Player player; static Camera mainCamera;
static void Setup(void) {
    memset(&play,0,sizeof(play)); memset(&player,0,sizeof(player)); memset(&mainCamera,0,sizeof(mainCamera));
    memset(&gCustomItemState,0,sizeof(gCustomItemState));
    mainCamera.uid=uid++; mainCamera.status=CAM_STATUS_ACTIVE;
    mainCamera.setting=CAM_SET_DUNGEON0; mainCamera.mode=CAM_MODE_NORMAL;
    mainCamera.target=&player.actor;
    play.cameraPtrs[CAM_ID_MAIN]=&mainCamera; play.activeCamId=CAM_ID_MAIN;
    beetleSubCamId=SUBCAM_FREE; sBeetleOwner=NULL; sBeetlePlayer=NULL; sBeetleColInitialized=0;
    Beetle_ResetTransient(&player,&play); statusCalls=0;
}
int main(void) {
    Setup(); assert(!CustomItems_BlocksMovement(&player));
    beetleActive=1;beetleState=BEETLE_STATE_FLYING;beetleSubCamId=1;
    assert(CustomItems_BlocksMovement(&player));
    beetleSubCamId=SUBCAM_FREE; assert(!CustomItems_BlocksMovement(&player));
    beetleSubCamId=1;beetleState=BEETLE_STATE_RETURNING;assert(!CustomItems_BlocksMovement(&player));
    assert(!CustomItems_BlocksMovement(NULL));
    beetleActive=0; dlActive=1;dlBlowing=1;dlAnimTimer=1;
    player.heldItemAction=PLAYER_IA_DEKU_LEAF;player.upperActionFunc=Player_UpperAction_DekuLeaf;
    assert(CustomItems_BlocksMovement(&player));dlActive=0;
    shActive=1;shAnimating=1;shAnimTimer=1;
    player.heldItemAction=PLAYER_IA_SHOVEL;player.upperActionFunc=Player_UpperAction_Shovel;
    assert(CustomItems_BlocksMovement(&player));
    {
        Setup(); player.transformation=PLAYER_FORM_HUMAN;
        Camera saved=mainCamera;
        assert(Beetle_CreateSubCam(&play)); s16 owned=beetleSubCamId;
        assert(owned>=CAM_ID_SUB_FIRST && play.activeCamId==owned);
        Beetle_DestroySubCam(&play);
        assert(play.activeCamId==CAM_ID_MAIN && play.cameraPtrs[owned]==NULL);
        assert(mainCamera.setting==saved.setting && mainCamera.mode==saved.mode && mainCamera.target==saved.target);
        assert(mainCamera.status==CAM_STATUS_ACTIVE);
        // Existing native subcamera context is restored instead of forcing main.
        Setup(); s16 prior=Play_CreateSubCamera(&play); Camera* priorCam=play.cameraPtrs[prior];
        priorCam->setting=CAM_SET_DUNGEON0;priorCam->mode=CAM_MODE_TALK;priorCam->target=&player.actor;
        Play_ChangeCameraStatus(&play,prior,CAM_STATUS_ACTIVE); Camera priorSaved=*priorCam;
        assert(Beetle_CreateSubCam(&play));owned=beetleSubCamId;
        Beetle_DestroySubCam(&play);
        assert(play.activeCamId==prior && play.cameraPtrs[owned]==NULL);
        assert(priorCam->setting==priorSaved.setting && priorCam->mode==priorSaved.mode && priorCam->target==priorSaved.target);
        // Exhausted slots cannot pass CAM_ID_NONE to native aliasing APIs.
        Setup();while(Play_CreateSubCamera(&play)!=CAM_ID_NONE){}statusCalls=0;
        assert(!Beetle_CreateSubCam(&play));assert(statusCalls==0 && play.activeCamId==CAM_ID_MAIN);
        // Higher priority native cutscene wins, even when Beetle is cancelled.
        Setup();assert(Beetle_CreateSubCam(&play));owned=beetleSubCamId;
        s16 cut=Play_CreateSubCamera(&play);Play_ChangeCameraStatus(&play,cut,CAM_STATUS_ACTIVE);
        Beetle_DestroySubCam(&play);assert(play.activeCamId==cut && play.cameraPtrs[cut]!=NULL && play.cameraPtrs[owned]==NULL);
        // Recycled camera slot/address belongs to its new UID, never this tool.
        Setup();assert(Beetle_CreateSubCam(&play));owned=beetleSubCamId;
        Play_ClearCamera(&play,owned);s16 reused=Play_CreateSubCamera(&play);assert(reused==owned);
        Play_ChangeCameraStatus(&play,reused,CAM_STATUS_ACTIVE);statusCalls=0;
        Beetle_DestroySubCam(&play);assert(statusCalls==0 && play.activeCamId==reused && play.cameraPtrs[reused]!=NULL);
        // A changed previous camera UID falls back to main, without touching replacement.
        Setup();prior=Play_CreateSubCamera(&play);Play_ChangeCameraStatus(&play,prior,CAM_STATUS_ACTIVE);
        assert(Beetle_CreateSubCam(&play));owned=beetleSubCamId;
        Play_ClearCamera(&play,prior);assert(Play_CreateSubCamera(&play)==prior);
        Beetle_DestroySubCam(&play);assert(play.activeCamId==CAM_ID_MAIN && play.cameraPtrs[prior]!=NULL);
        // Destruction/re-entry tears down owned transient state; new Play ignored old handles.
        Setup();assert(Beetle_CreateSubCam(&play));owned=beetleSubCamId;
        beetleActive=1;beetleFirstPerson=1;player.unk_AA5=PLAYER_UNKAA5_3;
        player.stateFlags1|=PLAYER_STATE1_8;sBeetleColInitialized=1;
        Beetle_ResetTransient(&player,&play);
        assert(!beetleActive && !beetleFirstPerson && !sBeetleColInitialized && player.unk_AA5==PLAYER_UNKAA5_0);
        assert(!(player.stateFlags1&PLAYER_STATE1_8) && play.cameraPtrs[owned]==NULL);
        Setup();assert(Beetle_CreateSubCam(&play));owned=beetleSubCamId;
        static Player replacement; replacement.stateFlags1=PLAYER_STATE1_8;
        Beetle_ResetTransient(&replacement,&play);assert(play.cameraPtrs[owned]==NULL);
        assert(replacement.stateFlags1==PLAYER_STATE1_8);
        Setup();assert(Beetle_CreateSubCam(&play));static PlayState other;static Player otherPlayer;
        statusCalls=0;Beetle_ResetTransient(&otherPlayer,&other);assert(statusCalls==0 && !beetleActive && beetleSubCamId==SUBCAM_FREE);
    }
    puts("PASS native MM camera return context, exhausted slots, cutscene priority, recycled UIDs, transient cleanup for the human player");
}
'''
if not baseline: head+=fixture
with tempfile.TemporaryDirectory(prefix='mm-beetle-camera-') as t:
    work=Path(t);source=work/'test.c';source.write_text(head);obj=work/'test'
    includes=['.','mm','mm/2s2h','mm/include','mm/include/PR','mm/src','mm/assets','libultraship/include','combo']
    cmd=[os.environ.get('CC','cc'),'-std=gnu17','-w','-Wno-error=incompatible-pointer-types','-Wno-error=int-conversion','-DCOMBO_BUILD','-DF3DEX_GBI_2','-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer','-no-pie',*[ '-I'+str(ROOT/x) for x in includes],str(source),'-lm','-o',str(obj)]
    subprocess.run(cmd,check=True);subprocess.run([str(obj)],check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
# Compile both production rotation helpers and compare actual formula results
# under equivalent invert/mirrored options, not a mirrored implementation.
mm_rotation=block(read('mm/mods/items/helpers/camera_helper.c'),'void Projectile_UpdateRotationFromStick(')
oot_rotation=block((ROOT/'soh/mods/items/helpers/camera_helper.c').read_text(),'void Projectile_UpdateRotationFromStick(').replace('Projectile_UpdateRotationFromStick','Oot_Rotation')
rotation=r'''
#include <assert.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
typedef int16_t s16; typedef int8_t s8; typedef float f32;
typedef struct { struct { s8 stick_x,stick_y; } rel; } Input;
typedef struct { struct { Input input[1]; } state; } PlayState;
#define SQ(x) ((x)*(x))
#define GI_INVERT_FIRST_PERSON_AIM_X 0
#define GI_INVERT_FIRST_PERSON_AIM_Y 1
#define CVAR_SETTING(x) x
#define CVAR_ENHANCEMENT(x) x
static int invertX,invertY,mirror;
static int GameInteractor_InvertControl(int axis) { return axis==0 ? ((invertX^mirror)?-1:1) : (invertY?-1:1); }
static int CVarGetInteger(const char* key,int defaultValue) {
 if(!strcmp(key,"Controls.InvertAimingXAxis"))return invertX;
 if(!strcmp(key,"Controls.InvertAimingYAxis"))return invertY;
 if(!strcmp(key,"MirroredWorld"))return mirror;
 return defaultValue;
}
'''+mm_rotation+'\n'+oot_rotation+r'''
int main(void) {
 PlayState play={0};
 for(invertX=0;invertX<2;++invertX) for(invertY=0;invertY<2;++invertY) for(mirror=0;mirror<2;++mirror)
 for(int x=-60;x<=60;x+=60) for(int y=-60;y<=60;y+=60) {
  play.state.input[0].rel.stick_x=x;play.state.input[0].rel.stick_y=y;
  s16 mmYaw=100,mmPitch=0,ootYaw=100,ootPitch=0;
  Projectile_UpdateRotationFromStick(&mmYaw,&mmPitch,&play,0x360,0x3000);
  Oot_Rotation(&ootYaw,&ootPitch,&play,0x360,0x3000);
  assert(mmYaw==ootYaw && mmPitch==ootPitch);
 }
 puts("PASS production OoT/MM Beetle steering parity across both invert axes and mirrored world");
}
'''
with tempfile.TemporaryDirectory(prefix='beetle-steering-parity-') as t:
 source=Path(t)/'test.c';source.write_text(rotation);binary=Path(t)/'test'
 subprocess.run([os.environ.get('CC','cc'),'-std=gnu17','-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie',str(source),'-lm','-o',str(binary)],check=True)
 subprocess.run([str(binary)],check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
# The existing native player movement bridge must hold only piloted flight.
movement=block(read('mm/mods/items/custom_items_common.c'),'s32 CustomItems_BlocksMovement(')
assert 'beetleActive' in movement and 'beetleSubCamId != SUBCAM_FREE' in movement
flight=block(src,'static void Beetle_StateFlying(')
assert 'p->stateFlags1 |= PLAYER_STATE1_BOOMERANG_THROWN' not in flight
reset=block(read('mm/mods/items/custom_items_stow.c'),'void CustomItems_ResetTransientTools(')
assert 'Beetle_ResetTransient(p, play);' in reset
print('PASS native movement/lifecycle integration seams; no phantom Zora boomerang camera mode')
