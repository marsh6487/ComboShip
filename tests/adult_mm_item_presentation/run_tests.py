#!/usr/bin/env python3
"""Execute production adult eligibility, receipt update, Actor_Draw transform and MM item cameras.
Native structs and math are used. Resource readiness, animation pose, collision and GI submission
are controlled service boundaries; this is not a complete renderer or game visual proof.
"""
from pathlib import Path
import os
import re
import subprocess
import sys
import tempfile
ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts/diagnostics'))
from run_time_pedestal_tests import block_from

def function(src, name):
    # Select definitions only, preserving body and signature verbatim.
    match = re.search(r'^([^\n;{}]*\b' + re.escape(name) + r'\([^;{}]*\)\s*)\{', src, re.M)
    assert match, name
    return block_from(src, match.start())

def source():
    adult = (ROOT/'mm/mods/items/logic/adult_link_render.cpp').read_text()
    camera = (ROOT/'mm/src/code/z_camera.c').read_text()
    actor = (ROOT/'mm/src/code/z_actor.c').read_text()
    custom = (ROOT/'mm/2s2h/CustomItem/CustomItem.cpp').read_text()
    player = (ROOT/'mm/src/overlays/actors/ovl_player_actor/z_player.c').read_text()
    data = (ROOT/'mm/src/code/z_camera_data.inc').read_text()
    camera_data = data[:data.index('} CameraSetting;')+len('} CameraSetting;')]
    camera_data += '\n#define CAM_SETTING_MODE_ENTRY(func, data) {func, ARRAY_COUNT(data), data}\n'
    camera_data += '#define CAM_SETTING_MODE_NULL {CAM_FUNC_NONE, 0, nullptr}\n'
    for name in ('D_801B5338','D_801B535C','D_801B53A4','D_801B5380','D_801B53C8',
                 'D_801B53EC','D_801B5410','D_801B5458','D_801B5434','D_801B547C',
                 'sSetLongChestOpeningModeNormalData'):
        camera_data += block_from(data,data.index('CameraModeValue '+name+'['))+';\n'
    for name in ('sCamSetItem0Modes','sCamSetItem1Modes','sCamSetLongChestOpeningModes'):
        camera_data += block_from(data,data.index('CameraMode '+name+'['))+';\n'
    camera_data += 'CameraSetting sCameraSettings[CAM_SET_MAX]{};\nvoid InitCameraData() {\n'
    for setting, modes in (('CAM_SET_ITEM0','sCamSetItem0Modes'),('CAM_SET_ITEM1','sCamSetItem1Modes'),
                           ('CAM_SET_LONG_CHEST_OPENING','sCamSetLongChestOpeningModes')):
        entry=re.search(r'^\s*(\{[^\n]*'+modes+r'[^\n]*\}),',data,re.M).group(1)
        camera_data += 'sCameraSettings['+setting+'] = '+entry+';\n'
    camera_data += '}\ns32 sCameraInterfaceFlags=1, sUpdateCameraDirection=false;\n'
    pieces = []
    for name in ('AdultLink_UsesAdultPresentation', 'AdultLink_UsesAdultItemPresentation',
                 'AdultLink_GetItemPresentationHeight', 'AdultLink_GetItemPresentationYOffset'):
        if name+'(' in adult:
            pieces.append(function(adult, name))
    pieces.append(function((ROOT/'mm/src/code/z_skin_matrix.c').read_text(), 'SkinMatrix_SetScale'))
    pieces.append(function(custom, 'CustomItem00_ItemTouched'))
    pieces.append(function(custom, 'CustomItem00_Update'))
    pieces.append(function(custom, 'CustomItem00_Draw'))
    # The exact Actor_Draw matrix setup, followed by its scale, executes on the native matrix implementation.
    pos = actor.index('if (actor->flags & ACTOR_FLAG_IGNORE_QUAKE)', actor.index('void Actor_Draw('))
    transform = block_from(actor, pos)
    end = pos + len(transform)
    # if/else has two blocks.
    transform += actor[end:actor.index('    Actor_SetObjectDependency', end)]
    pieces.append('void DrawReceiptTransform(Actor* actor, PlayState* play) {\n'+transform+'\n}')
    adult_branch = player.index('if (AdultLink_ShouldHide())', player.index('void Player_Draw('))
    pieces.append('void DrawAdultBranch(PlayState* play, Player* self) {\n' +
                  re.sub(r'\bthis\b', 'self', block_from(player, adult_branch)) + '\n}')
    for src, names in ((actor, ('Player_GetHeight', 'Actor_GetFocus', 'Actor_GetWorld', 'Actor_GetWorldPosShapeRot')),
                       ((ROOT/'mm/src/code/z_lib.c').read_text(), ('Math_SinS','Math_CosS','Math_Vec3f_Copy')),
                       ((ROOT/'mm/src/code/z_olib.c').read_text(), ('OLib_Vec3fDist','OLib_VecSphToVec3f',
                        'OLib_VecGeoToVec3f','OLib_Vec3fToVecSph','OLib_Vec3fToVecGeo',
                        'OLib_Vec3fDiffToVecGeo','OLib_AddVecGeoToVec3f')),
                       (camera, ('Camera_fabsf','Camera_ScaledStepToCeilF','Camera_ScaledStepToCeilS',
                        'Camera_ScaledStepToCeilVec3f','Camera_SetStateFlag','Camera_UnsetStateFlag',
                        'Camera_SetUpdateRatesSlow','Camera_SetUpdateRatesFastPitch','Camera_SetFocalActorAtOffset',
                        'Camera_GetFocalActorHeight','Camera_CalcXZAngle','Camera_KeepOn4','Camera_Demo2'))):
        pieces.extend(function(src, name) for name in names)
    for name in ('D_801B9E64','D_801B9E84','D_801B9EB4','D_801B9ED4'):
        pos = re.search(r'^(?:VecGeo|Vec3f) '+name+r'\[\]', camera, re.M).start()
        pieces.insert(0, block_from(camera,pos)+';')
    trig=(ROOT/'mm/src/code/stubs.c').read_text()
    pieces.insert(0,block_from(trig,trig.index('static s16 sintable['))+';\n'+function(trig,'sins')+'\n'+function(trig,'coss'))
    return ('#define ADULT_ITEM_QUERY 1\n' if 'AdultLink_UsesAdultItemPresentation(' in adult else '') + (Path(__file__).parent/'native_test.cpp').read_text().replace('/* CAMERA DATA */',camera_data).replace('/* PRODUCTION */','\n'.join(pieces))

def main():
    sanitize='--sanitize' in sys.argv
    includes=['-I'+str(ROOT/p) for p in ('','mm','mm/include','mm/include/PR','mm/src','mm/assets','mm/2s2h','libultraship/include','combo')]
    flags=['-DCOMBO_BUILD','-DF3DEX_GBI_2','-DGBI_FLOAT_MTX','-DGBI_S32_VTX','-ffunction-sections','-fdata-sections','-g',*includes]
    if sanitize: flags+=['-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer','-fno-pie']
    with tempfile.TemporaryDirectory(prefix='adult-gi-') as td:
        temp=Path(td)
        (temp/'test.cpp').write_text(source())
        obj=temp/'matrix.o'
        subprocess.run([os.environ.get('CC','gcc-14'),'-std=gnu2x',*flags,'-w','-Wno-incompatible-pointer-types','-Wno-int-conversion','-c',str(ROOT/'mm/src/code/sys_matrix.c'),'-o',str(obj)],check=True)
        binary=temp/'test'
        subprocess.run([os.environ.get('CXX','g++-14'),'-std=c++20',*flags,'-Wno-narrowing','-Wno-write-strings',str(temp/'test.cpp'),str(obj),'-Wl,--gc-sections','-no-pie','-lm','-o',str(binary)],check=True)
        failed=[]
        for case in ('controls','held','item0','chest','native','eligibility','overhead','scale','scope'):
            print('CASE '+case,flush=True)
            result=subprocess.run([str(binary),case],env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
            if result.returncode: failed.append(case)
        print('FAILED: '+', '.join(failed) if failed else 'PASS all adult MM item presentation cases',flush=True)
        return bool(failed)
if __name__=='__main__':
    raise SystemExit(main())
