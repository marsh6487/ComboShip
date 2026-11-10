#!/usr/bin/env python3
"""Load real independent owner DLLs with deliberately incompatible C++ layouts."""
import ctypes
from pathlib import Path
import subprocess
import tempfile
ROOT = Path(__file__).resolve().parents[2]
class Info(ctypes.Structure):
    _fields_ = [('rawPointer', ctypes.c_void_p), ('elementCount', ctypes.c_int32),
                ('skeletonType', ctypes.c_int32), ('dListCount', ctypes.c_int32)]
for path in ['ComboForeignAnim.h', 'ComboSoulCompositeRender.h']:
    source = (ROOT/'combo/menu'/path).read_text()
    assert 'dynamic_pointer_cast' not in source and 'SOH::Skeleton' not in source, path
with tempfile.TemporaryDirectory(prefix='soul-owner-bridge-') as td:
    build = Path(td)
    for name in ['CrossRMRegistry.h', 'ResourceManagerScope.h', 'ResourceManager.h']:
        stub = build/'ship/resource'/name
        stub.parent.mkdir(parents=True, exist_ok=True)
        stub.write_text('#pragma once\n')
    sizes = []
    for owner in ['oot', 'mm']:
        dll = build/(owner+'.so')
        command = ['c++', '-std=c++20', '-shared', '-fPIC', '-fvisibility=hidden', '-Wall', '-Wextra', '-Werror',
                   '-I'+str(build), '-I'+str(ROOT/'combo/menu'), str(ROOT/'tests/soul_mini_models/renderer_owner_bridge.cpp'), '-o', str(dll)]
        if owner == 'oot': command.insert(1, '-DOWNER_OOT')
        subprocess.run(command, check=True)
        loaded = ctypes.CDLL(str(dll), mode=ctypes.RTLD_LOCAL)
        loaded.FixtureSkeletonClassSize.restype = ctypes.c_size_t
        sizes.append(loaded.FixtureSkeletonClassSize())
        loaded.FixtureQuery.argtypes = [ctypes.c_char_p, ctypes.c_int32, ctypes.c_int32, ctypes.POINTER(Info)]
        def query(path, kind, count):
            info = Info()
            result = loaded.FixtureQuery(path.encode(), kind, count, ctypes.byref(info))
            return result, info
        loaded.FixtureConfigure(0)
        ok, info = query('__OTR__rig', 1, 3)
        assert ok == 1 and info.rawPointer and (info.elementCount, info.skeletonType, info.dListCount) == (2,1,2)
        assert query('__OTR__anim', 2, 3)[0] == 1
        assert query('__OTR__pose', 3, 3)[0] == 1
        assert query('__OTR__pose', 2, 3)[0] == 0
        for scenario in [1,2,3,7]:
            loaded.FixtureConfigure(scenario)
            assert query('__OTR__rig', 1, 3)[0] == 0
        loaded.FixtureConfigure(4)
        assert query('__OTR__anim', 2, 3)[0] == 0
        loaded.FixtureConfigure(5)
        ok, info = query('__OTR__objects/object_fsn/gFsnSkel', 1, 18)
        assert ok == (owner == 'mm')
        if ok: assert info.elementCount == 17
        loaded.FixtureConfigure(6)
        assert query('__OTR__pose', 3, 3)[0] == 0
    assert sizes[0] != sizes[1], 'fixture must prove different actual owner layouts'
    print('Independent owner DLLs: differing Skeleton vector layouts, POD metadata, Fsn local correction, invalid animation/pose bounds passed')
