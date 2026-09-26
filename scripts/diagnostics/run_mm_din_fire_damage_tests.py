#!/usr/bin/env python3
"""Compile MM Din with native strike/collision/drop functions and real actor tables.

The unrelated engine entry points are boundaries, not copies of gameplay logic.
Function bodies are extracted unchanged from current production source; actor
tables and grass masks are compiled from the complete actor translation units.
"""
import os
from pathlib import Path
import re
import tempfile

from run_mm_din_fire_tests import FLAGS, ROOT, run
from run_time_pedestal_tests import functions


def native_functions():
    chunks = ['MtxF* sCurrentMatrix = &currentMatrix;']
    source = (ROOT / 'mm/src/overlays/actors/ovl_player_actor/z_player.c').read_text()
    chunks.append(re.search(r'typedef struct MeleeWeaponDamageInfo \{.*?\} MeleeWeaponDamageInfo;', source, re.S)[0])
    chunks.append(re.search(r'MeleeWeaponDamageInfo D_8085D09C\[.*?^\};', source, re.M | re.S)[0])
    mine = (ROOT / 'mm/src/overlays/actors/ovl_Obj_Mine/z_obj_mine.c').read_text()
    chunks.append(re.search(r'static ObjMineMtxF3 sStandardBasis = \{.*?^\};', mine, re.M | re.S)[0])
    paths = {
        'code/z_lib.c': ['Math_Vec3s_ToVec3f', 'Math_Vec3f_Copy', 'Math_Vec3f_Yaw', 'Math_SinS', 'Math_CosS'],
        'code/sys_math3d.c': ['Math3D_Vec3fMagnitudeSq', 'Math3D_Vec3fMagnitude'],
        'code/sys_matrix.c': ['Matrix_RotateXS', 'Matrix_RotateYS', 'Matrix_MultVecZ'],
        'code/z_player_lib.c': ['Player_MeleeWeaponFromIA', 'Player_GetMeleeWeaponHeld'],
        'overlays/actors/ovl_player_actor/z_player.c': ['func_80833728', 'func_8083375C'],
        'code/z_collision_check.c': ['CollisionCheck_GetElementATDamage',
            'CollisionCheck_GetDamageAndEffectOnElementAC', 'CollisionCheck_ApplyElementATDefense',
            'CollisionCheck_NoSharedFlags', 'CollisionCheck_ApplyDamage'],
        'code/z_actor.c': ['Actor_SetDropFlag', 'Actor_SetDropFlagJntSph', 'Actor_WorldYawTowardActor'],
        'overlays/actors/ovl_Boss_Hakugin/z_boss_hakugin.c': ['BossHakugin_FrozenBeforeFight'],
        'overlays/actors/ovl_Obj_Ice_Poly/z_obj_ice_poly.c': ['func_80931A38'],
        'overlays/actors/ovl_Obj_Mine/z_obj_mine.c': ['ObjMine_GetUnitVec3f', 'ObjMine_Air_CheckAC',
                                                   'ObjMine_Water_CheckAC'],
    }
    for path, names in paths.items():
        native = functions((ROOT / 'mm/src' / path).read_text(), names)
        chunks.extend(native[name] for name in names)
    return '\n\n'.join(chunks) + '\n'


def main():
    actors = {
        'En_Dekubaba': [('DekuBaba', 'sDamageTable')],
        'En_Wf': [('Wolfos', 'sDamageTable1'), ('WhiteWolfos', 'sDamageTable2')],
        'En_Am': [('Armos', 'sDamageTable')],
        'En_Firefly': [('Keese', 'sDamageTable')],
        'En_Dinofos': [('Dinolfos', 'sDamageTable')],
        'En_Rd': [('Redead', 'sDamageTable')],
        'En_Fz': [('Freezard', 'sDamageTable')],
        'En_Wallmas': [('Wallmaster', 'sDamageTable')],
        'En_Floormas': [('Floormaster', 'sDamageTable')],
        'En_Crow': [('Crow', 'sDamageTable')],
        'En_Dekunuts': [('DekuNuts', 'sDamageTable')],
        'En_Peehat': [('Peahat', 'sDamageTable')],
    }
    with tempfile.TemporaryDirectory(prefix='mm-din-damage-') as directory:
        build = Path(directory)
        (build / 'din_fire_native.inc').write_text(native_functions())
        sine = build / 'sine.c'
        sine.write_text('#include "global.h"\n#include "libultra/gu/sins.c"\n')
        wrappers = [str(sine)]
        for actor, tables in actors.items():
            code = f'#include "overlays/actors/ovl_{actor}/z_{actor.lower()}.c"\n'
            for name, table in tables:
                code += f'DamageTable* Test_{name}Table(void) {{ return &{table}; }}\n'
            path = build / f'{actor}.c'
            path.write_text(code)
            wrappers.append(str(path))
        for actor, name in [('En_Kusa', 'Kusa'), ('Obj_Grass', 'Grass')]:
            path = build / f'{actor}.c'
            path.write_text(f'#include "overlays/actors/ovl_{actor}/z_{actor.lower()}.c"\n'
                            f'u32 Test_{name}Mask(void) {{ return sCylinderInit.elem.acDmgInfo.dmgFlags; }}\n')
            wrappers.append(str(path))
        binary = str(build / 'damage')
        run(os.environ.get('CC', 'cc'), *FLAGS, '-I' + directory,
            '-ffunction-sections', '-fdata-sections', 'mm/tests/din_fire_damage_test.c',
            'mm/src/code/din_fire_sword.c', 'mm/src/code/sys_math_atan.c',
            'mm/src/libultra/gu/coss.c',
            *wrappers, '-Wl,--gc-sections', '-lm', '-o', binary)
        run(binary)


if __name__ == '__main__':
    main()
