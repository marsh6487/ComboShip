#!/usr/bin/env python3
"""Private MM presentation variant; shared retained geometry stays byte-exact."""
import xml.etree.ElementTree as ET

import forest
import market_interior
import mm_native_shop

PREFIX = 'custom/skiejer_market/mm_shop_presentation_v1/'
ROOM_ROOT = 'custom/skiejer_market/room'
GEOMETRY = market_interior.GEOMETRY
OWNERS = [ROOM_ROOT, GEOMETRY, *(f'{GEOMETRY}_{kind}_{suffix}'
          for suffix in (2, 6, 15, 17, 18) for kind in ('tri', 'vtx'))]
CLONES = {name: PREFIX + name.removeprefix('custom/skiejer_market/') for name in OWNERS}
# Whole connected wicker baskets and their retained hanging foliage planes,
# plus the complete left cauldron (body, contents and top attachments). Moving
# that cauldron toward the left wall and forward clears the close shop's
# lower row and outer shelf post; its front remains behind the counter barrier;
# the body remains inside X=-160 and the shared/OoT resources remain exact.
# Identity checks reject unexpected authoring changes instead of moving a region.
MOVES = {
    2: [([[-120, 0, 0], [-62, 54, 54]], (-35, 0, 60), 116, 106),
        ([[-103, 60, 42], [-92, 62, 53]], (-35, 0, 60), 20, 16)],
    6: [([[-104, 51, 27], [-53, 62, 53]], (-35, 0, 60), 80, 60)],
    17: [([[-85, 107, 156], [-31, 175, 210]], (-50, 0, 0), 302, 240),
         ([[34, 129, 144], [77, 181, 186]], (60, 0, 0), 226, 176)],
    15: [([[-58, 166, 167], [-58, 191, 186]], (-50, 0, 0), 6, 2),
         ([[-65, 165, 183], [-55, 178, 183]], (-50, 0, 0), 6, 2),
         ([[-61, 188, 183], [-40, 212, 183]], (-50, 0, 0), 6, 2),
         ([[53, 174, 163], [70, 194, 165]], (60, 0, 0), 6, 2),
         ([[55, 175, 160], [56, 184, 168]], (60, 0, 0), 6, 2)],
    18: [([[-114, 51, 2], [-69, 51, 47]], (-35, 0, 60), 30, 36)],
}


def encode(root):
    ET.indent(root, space='  ')
    return ET.tostring(root, encoding='utf-8') + b'\n'


def apply(resources):
    result = dict(resources)
    for original, clone in CLONES.items():
        root = ET.fromstring(resources[original])
        for node in root.iter():
            if node.get('Path') in CLONES:
                node.set('Path', CLONES[node.get('Path')])
        result[clone] = encode(root)
    for suffix, moves in MOVES.items():
        parts = market_interior.vertex_components(resources, f'{GEOMETRY}_tri_{suffix}')
        root = ET.fromstring(result[CLONES[f'{GEOMETRY}_vtx_{suffix}']])
        vertices = list(root)
        for bounds, delta, count, triangles in moves:
            matched = [part for part in parts if part['bounds'] == bounds]
            if len(matched) != 1 or len(matched[0]['indices']) != count or matched[0]['triangles'] != triangles:
                raise ValueError('private MM presentation component identity changed')
            for index in matched[0]['indices']:
                node = vertices[index]
                for axis, offset in zip('XYZ', delta):
                    node.set(axis, str(int(node.get(axis)) + offset))
        result[CLONES[f'{GEOMETRY}_vtx_{suffix}']] = encode(root)
    path = mm_native_shop.ROOM
    commands = mm_native_shop.native_commands(resources[path])
    old, new = forest.string(ROOM_ROOT), forest.string(CLONES[ROOM_ROOT])
    if commands[10].count(old) != 1:
        raise ValueError('private MM room mesh owner changed')
    commands[10] = commands[10].replace(old, new)
    result[path] = mm_native_shop._scene(resources[path], commands)
    return result
