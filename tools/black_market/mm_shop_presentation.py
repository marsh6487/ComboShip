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
          for suffix in (15, 17) for kind in ('tri', 'vtx'))]
CLONES = {name: PREFIX + name.removeprefix('custom/skiejer_market/') for name in OWNERS}
# Whole connected wicker baskets and their retained hanging foliage planes.
# Identity checks reject unexpected authoring changes instead of moving a region.
MOVES = {
    17: [([[-85, 107, 156], [-31, 175, 210]], -50, 302, 240),
         ([[34, 129, 144], [77, 181, 186]], 60, 226, 176)],
    15: [([[-58, 166, 167], [-58, 191, 186]], -50, 6, 2),
         ([[-65, 165, 183], [-55, 178, 183]], -50, 6, 2),
         ([[-61, 188, 183], [-40, 212, 183]], -50, 6, 2),
         ([[53, 174, 163], [70, 194, 165]], 60, 6, 2),
         ([[55, 175, 160], [56, 184, 168]], 60, 6, 2)],
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
        for bounds, dx, count, triangles in moves:
            matched = [part for part in parts if part['bounds'] == bounds]
            if len(matched) != 1 or len(matched[0]['indices']) != count or matched[0]['triangles'] != triangles:
                raise ValueError('private MM basket/foliage component identity changed')
            for index in matched[0]['indices']:
                node = vertices[index]
                node.set('X', str(int(node.get('X')) + dx))
        result[CLONES[f'{GEOMETRY}_vtx_{suffix}']] = encode(root)
    path = mm_native_shop.ROOM
    commands = mm_native_shop.native_commands(resources[path])
    old, new = forest.string(ROOM_ROOT), forest.string(CLONES[ROOM_ROOT])
    if commands[10].count(old) != 1:
        raise ValueError('private MM room mesh owner changed')
    commands[10] = commands[10].replace(old, new)
    result[path] = mm_native_shop._scene(resources[path], commands)
    return result
