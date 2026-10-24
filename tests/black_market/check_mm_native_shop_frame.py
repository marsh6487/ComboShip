#!/usr/bin/env python3
"""Project the scene-authored MM Fixed1 bank poses with native UI bounds."""
import argparse
import hashlib
import itertools
import json
import math
from pathlib import Path
import re
import sys

import numpy as np

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools/black_market'))
import mm_native_shop
from check_browsing_frame import project


def native_ui_contract():
    camera = (ROOT / 'mm/src/code/z_camera_data.inc').read_text()
    fixed = re.search(r'CameraModeValue D_801B5824\[\] = \{\s*CAM_FUNCDATA_FIXD1\(([^,]*),([^,]*),([^,]*),', camera)
    assert fixed, 'inspect changed native Fixed1 defaults before projecting MM shop poses'
    shop = (ROOT / 'mm/src/overlays/actors/ovl_En_Ossan/z_en_ossan.c').read_text()
    scale = re.search(r'\bthis->cursorPos.z = ([0-9.]+)f;', shop)
    assert scale and 'w = 16.0f * z;' in shop, 'inspect changed native cursor bounds'
    return float(fixed[3]), 16 * float(scale[1]) + 1


def framing_report(layout):
    values = mm_native_shop.contract()
    native_fov, cursor_radius = native_ui_contract()
    views = []
    for row in values['CameraRecords']:
        eye = np.array(row[:3], dtype=float)
        pitch, yaw = (row[3] * math.pi / 32768, row[4] * math.pi / 32768)
        assert row[5] == 0, 'MM projection must account for changed native camera roll'
        direction = np.array([math.sin(yaw) * math.cos(pitch), -math.sin(pitch),
                              math.cos(yaw) * math.cos(pitch)])
        views.append({'eye': eye.tolist(), 'at': (eye + direction).tolist(),
                      'fov': native_fov if row[6] == -1 else row[6], 'rotation': list(row[3:6])})
    # Native ShrinkWindow stores the requested size unchanged; View_ApplyLetterbox
    # clips each edge by that full amount, then draws a two-pixel boundary there.
    visible_top = values['LetterboxSize'] + 2
    bounds, tweens = {}, {}
    for name, camera, slots in [('right', 3, range(4)), ('left', 2, range(4, 8))]:
        view, bank, motions = views[camera], [], []
        target = np.array(values['SelectedTargets'][0 if name == 'right' else 1])
        for slot in slots:
            points = np.array([project(point, view) for point in
                               itertools.product(*zip(*layout['reserved_slot_envelopes'][slot]))])
            low, high = points.min(0), points.max(0)
            assert low[0] > 8 and high[0] < 312 and low[1] > visible_top and high[1] < 138, (
                name, slot, 'selected item/seal clips native letterbox or Y142 textbox', low, high)
            # EnGirlA_Update2 maintains actor focus five units above world pos;
            # native Actor_GetScreenPos projects that focus for the shop cursor.
            cursor = np.array(project(np.array(layout['shelves'][slot]) + [0, 5, 0], view))
            assert cursor[1] - cursor_radius > visible_top and cursor[1] + cursor_radius < 142, (
                name, slot, 'native 24px cursor plus quantization clips letterbox or textbox', cursor)
            assert cursor[0] - cursor_radius > 8 and cursor[0] + cursor_radius < 312, (name, slot, 'cursor clips side')
            bank.append({'slot': slot, 'screen_min': low.tolist(), 'screen_max': high.tolist(),
                         'cursor_center': cursor.tolist(), 'cursor_radius_including_quantization': cursor_radius})
            relative = np.array(layout['reserved_slot_envelopes'][slot]) - layout['shelves'][slot]
            selected = np.array([project(point, view) for point in itertools.product(*zip(*(relative + target)))])
            # Every corner translates on one straight segment. With all depths
            # positive, its perspective coordinates are ratios of affine terms,
            # so each coordinate is monotonic: endpoint extrema bound the full
            # continuous native tween, including every .15-step and return step.
            path_low = np.minimum(low, selected.min(0))
            path_high = np.maximum(high, selected.max(0))
            assert path_low[0] > 8 and path_high[0] < 312 and path_low[1] > visible_top and path_high[1] < 138, (
                name, slot, 'native selection tween clips letterbox or textbox', path_low, path_high)
            samples = (0, .15, .25, .3, .4, .45, .55, .6, .7, .75, .85, 1)
            for tween in samples:
                position = (1 - tween) * np.array(layout['shelves'][slot]) + tween * target
                projected = np.array([project(point, view) for point in itertools.product(*zip(*(relative + position)))])
                assert np.all(projected >= path_low - 1e-8) and np.all(projected <= path_high + 1e-8)
            motions.append({'slot': slot, 'target': target.tolist(), 'screen_min': path_low.tolist(),
                            'screen_max': path_high.tolist(), 'native_tween_samples': list(samples),
                            'continuous_translation_bounded_by_endpoints': True})
        bounds[name] = bank
        tweens[name] = motions
    return {'views': views, 'bounds': bounds, 'selection_tweens': tweens, 'native_textbox_y': 142,
            'native_letterbox_pixels_per_edge': values['LetterboxSize'],
            'visible_top_including_letterbox_boundary': visible_top, 'aspect': '4:3',
            'scene_contract_sha256': hashlib.sha256((ROOT / 'combo/BlackMarketMmShopScene.h').read_bytes()).hexdigest(),
            'runtime_proven': False,
            'scope': 'Steady native Fixed1 bank poses project all four authored .32 selected-scale/bob item and '
                     'seal envelopes plus actual native cursor bounds. All eight native selection/return '
                     'translations are conservatively bounded by those same envelopes, although the native '
                     'runtime retains scale .25. Coordinates adapt the angled custom room; they are not '
                     'donor-room byte identity. Native camera pans, arbitrary native/Alt models, room/keeper '
                     'clearance during the selection tween and configured gameplay remain unproven.'}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--layout', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    report = framing_report(json.loads(args.layout.read_text()))
    report['layout_sha256'] = hashlib.sha256(args.layout.read_bytes()).hexdigest()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + '\n')
    print('PASS native MM framing: four offers per bank and all eight selection tweens clear full 32px letterbox, native 24px cursor and Y142 textbox')


if __name__ == '__main__':
    main()
