#!/usr/bin/env python3
"""Compile production provider with only resource-boundary stubs and real OoT bytes."""
from pathlib import Path
import sys, zipfile, tempfile, subprocess, hashlib
from asset_probe import font
ROOT=Path(__file__).resolve().parents[2]
archive=Path(sys.argv[1])
with zipfile.ZipFile(archive) as z, tempfile.TemporaryDirectory(prefix='oot-adult-native-') as td:
    td=Path(td)
    source=(ROOT/'mm/mods/voice_pack/oot_adult_voice.cpp').read_text()
    native=(ROOT/'soh/soh/mixer.c').read_text()
    start=native.index('void aADPCMdecImpl('); end=native.index('\nvoid aResampleImpl',start)
    (td/'native_decoder.inc').write_text(native[start:end])
    (td/'provider.inc').write_text('\n'.join(line for line in source.splitlines() if not line.startswith('#include <ship/')))
    for n in z.namelist():
        if n.startswith('audio/samples/'):
            path=td/n;path.parent.mkdir(parents=True,exist_ok=True);path.write_bytes(z.read(n))
    f=font(z.read('audio/fonts/00_Sound_Effects_1'))
    # Tables emitted by the actual 0x6800 bank channel script, not guessed names.
    b=z.read('audio/sequences/000_Sound_Effects')[68:]
    expected=[0x615d,0x6175,0x619b,0x61c1,0x61d5,0x61ea,0x6201,0x6218,0x622d,0x6242,0x6257,0x625f,0x626f,0x6284,0x6287,0x6299,0x62ab,0x62b4,0x62c6,0x649b,0x62ce,0x62ec,0x62f4,0x62fe,0x6306,0x630e,0x6316,0x631e,0x6326,0x622d,0x632e,0x615d]
    assert [int.from_bytes(b[0x5ff2+i*2:0x5ff4+i*2],'big') for i in range(32)]==expected
    for index in range(135):
        if f[index] and f[index][0] in source: assert f[index][1]==0.625
    for combo in (False,True):
        target=td/('combo' if combo else 'solo')
        command=['c++','-std=c++17','-DFMT_HEADER_ONLY','-Wall','-Wextra','-Wno-unused-parameter','-I'+str(td),'-I'+str(ROOT/'mm/mods/voice_pack'),str(ROOT/'tests/oot_adult_voice/provider_test.cpp'),'-o',str(target)]
        if combo: command.insert(1,'-DCOMBO_BUILD')
        subprocess.run(command,check=True)
        subprocess.run([str(target),str(td)],check=True)
    print('PASS pinned native OoT sequence action pointers and 0.625 font tuning')
    print('OoT asset SHA256',hashlib.sha256(archive.read_bytes()).hexdigest())
