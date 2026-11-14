"""Compile production callbacks/resolver with native MM Player/Gfx definitions.

Only unavailable renderer/resource boundaries are stubbed. No renderer policy is
reimplemented. --source permits the exact same regression to run on the parent.
"""
from pathlib import Path
import argparse
import subprocess
import tempfile
import json
import xml.etree.ElementTree as ET


def rito_fixture(root, ref):
    """Read the actual form inventory/topology, including working-tree fixes.

    Only XML call targets/topology enter the compiled fixture; resource factories
    and the rasterizer are outside this policy/ownership regression.
    """
    prefix = 'mm/assets/custom/objects/forms/rito/object_link_boy/'
    paths = subprocess.check_output(['git', 'ls-tree', '-r', '--name-only', ref or 'HEAD', prefix],
                                    cwd=root, text=True).splitlines()
    if not ref:
        paths = sorted(set(paths) | {str(path.relative_to(root)) for path in (root / prefix).glob('*') if path.is_file()})
    def read(path):
        if not ref and (root / path).exists():
            return (root / path).read_bytes()
        return subprocess.check_output(['git', 'show', (ref or 'HEAD') + ':' + path], cwd=root)
    resources = []
    entrypoints = {'gLinkAdultRightArmOutNearDL', 'gLinkAdultRightHandOutNearDL',
                   'gLinkAdultLeftArmOutNearDL', 'gLinkAdultRightHandHoldingBowFirstPersonDL',
                   'gLinkAdultRightHandHoldingHookshotFarDL'}
    for path in paths:
        if path.endswith('.png'):
            continue
        element = ET.fromstring(read(path))
        if element.tag == 'DisplayList':
            name = Path(path).name
            if name in entrypoints:
                assert all(node.tag in {'CallDisplayList', 'EndDisplayList'} for node in element), path
                if name in {'gLinkAdultRightHandHoldingBowFirstPersonDL', 'gLinkAdultRightHandHoldingHookshotFarDL'}:
                    assert list(element)[-2].attrib['Path'] == 'objects/forms/rito/object_link_boy/gLinkAdultRightHandClosedNearDL', path
            if name in {'gLinkAdultSkelLimb_015_DL', 'gLinkAdultSkelLimb_018_DL'}:
                matrices = [node.attrib['Path'] for node in element if node.tag == 'Matrix']
                expected = '>0x0d000300' if name == 'gLinkAdultSkelLimb_015_DL' else '>0x0d0003c0'
                assert matrices[-1] == expected, (path, matrices)
            calls = [node.attrib['Path'] for node in element if node.tag == 'CallDisplayList']
            resources.append('{%s,{%s}},' % (json.dumps(path.removeprefix('mm/assets/custom/')),
                                            ','.join(json.dumps(call) for call in calls)))
    limbs = []
    skeleton = ET.fromstring(read(prefix + 'gLinkAdultSkel'))
    for limb_path in skeleton:
        limb = ET.fromstring(read('mm/assets/custom/' + limb_path.attrib['Path']))
        positions = ','.join(limb.attrib.get('LegTrans' + axis, '0') for axis in 'XYZ')
        limbs.append('{{%s},%s,%s,%s},' % (positions, limb.attrib['ChildIndex'], limb.attrib['SiblingIndex'],
                                          json.dumps(limb.attrib.get('DisplayList1', ''))))
    return ('static const RitoResource kRitoResources[] = {\n' + '\n'.join(resources) + '\n};\n'
            'static const RitoLimb kRitoLimbs[] = {\n' + '\n'.join(limbs) + '\n};\n'
            'static constexpr int kRitoMatrixCount = ' + skeleton.attrib['DisplayListCount'] + ';\n')


def function(text, signature):
    start = text.index(signature)
    brace = text.index('{', start)
    depth = 1
    end = brace + 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[start:end]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--source', type=Path)
    parser.add_argument('--mode', choices=('original', 'combo', 'both'), default='both')
    parser.add_argument('--sanitize', action='store_true')
    parser.add_argument('--rito-ref', help='Read the exact form inventory from this git ref (for red controls)')
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    source = args.source or root
    adult = (source / 'mm/mods/items/logic/adult_link_render.cpp').read_text()
    upgrade = (source / 'mm/mods/items/logic/weapon_upgrades.c').read_text()
    native = (source / 'mm/src/code/z_player_lib.c').read_text()
    production = adult[adult.index('static Gfx* sDL_LHOpen;'):adult.index('// A MOD head mesh')]
    for signature in ('static Gfx* AdultLink_LoadDL(', 'static Gfx* AdultLink_LoadModelDL(',
                      'static s32 AdultLink_Setup(', 'static void AdultLink_ApplyNeiHeldHand(',
                      'static void AdultLink_ApplyRootTransform(',
                      'static s32 AdultLink_OverrideLimb(', 'static s32 AdultLink_OverrideFirstPerson('):
        if signature in adult:
            production += '\n' + function(adult, signature)
    if 'static s32 AdultLink_OverrideFirstPerson(' in adult:
        production = '#define MODEL_FIRST_PERSON 1\n' + production
    start = upgrade.index('// Exact reviewed GI geometry')
    production = upgrade[start:upgrade.index('u8 WeaponUpgrade_IKAxeStrike(', start)] + production
    draw = adult[adult.index('    // Draw through the real engine path'):adult.index('\n    for (int i = 0; i < PLAYER_EYES_MAX;', adult.index('    // Draw through the real engine path'))]
    production += '\nstatic void ActualDraw(PlayState* play, Player* player) {\n' + draw + '\n}\n'
    native_fp = function(native, 's32 Player_OverrideLimbDrawGameplayFirstPerson(')
    harness = (root / 'tests/okayu_presentation/mm_adult_renderer_test.cpp').read_text()
    harness = harness.replace('/* RITO_INVENTORY */', rito_fixture(root, args.rito_ref))
    harness = harness.replace('/* NATIVE_FIRST_PERSON */', native_fp).replace('/* PRODUCTION */', production)
    with tempfile.TemporaryDirectory(prefix='okayu-mm-render-') as tmp:
        tmp = Path(tmp)
        (tmp / 'test.cpp').write_text(harness)
        command = ['g++', '-std=c++20', '-w', '-DF3DEX_GBI_2', '-DNEI_EQUIPMENT_MM',
                   '-DGBI_FLOAT_MTX=1', '-DGBI_S32_VTX=1',
                   '-ffunction-sections', '-fdata-sections', '-Wl,--gc-sections']
        for directory in ('mm', 'mm/2s2h', 'mm/src', 'mm/include', 'mm/assets',
                          'libultraship/include', 'libultraship/include/libultraship', 'libultraship/src', '.'):
            command += ['-I', str(root / directory)]
        command += [str(tmp / 'test.cpp'), '-o', str(tmp / 'test')]
        if args.sanitize:
            command += ['-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                        '-fno-omit-frame-pointer', '-fno-pie', '-no-pie']
        modes = ('original', 'combo') if args.mode == 'both' else (args.mode,)
        for mode in modes:
            subprocess.run(command + (['-DCOMBO_BUILD'] if mode == 'combo' else []), check=True)
            print(mode + ' native graphics ABI:', flush=True)
            subprocess.run([str(tmp / 'test')], check=True)


if __name__ == '__main__':
    main()
