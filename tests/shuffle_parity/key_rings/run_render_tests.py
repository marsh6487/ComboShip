"""Real host structures, matrices, GBI and graph/setup; GPU services controlled."""
from pathlib import Path
import os
import re
import subprocess
import sys
import tempfile
ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / 'scripts/diagnostics'))
from run_time_pedestal_tests import functions

def main():
    with tempfile.TemporaryDirectory(prefix='mm-key-ring-render-') as folder:
        build = Path(folder)
        for host in ('soh', 'mm'):
            native = functions((ROOT / host / 'src/code/sys_matrix.c').read_text())
            names = ['Matrix_MtxFCopy','Matrix_Push','Matrix_Pop','Matrix_Translate','Matrix_Scale',
                     'Matrix_RotateAxisF' if host == 'mm' else 'Matrix_RotateAxis']
            (build / 'key_ring_matrix.inc').write_text('\n'.join(native[name] for name in names))
            graph = functions((ROOT / host / 'src/code/graph.c').read_text())
            (build / 'key_ring_graph.inc').write_text(graph['Graph_OpenDisps'] + '\n' + graph['Graph_CloseDisps'])
            setup = functions((ROOT / host / 'src/code/z_rcp.c').read_text())
            (build / 'key_ring_setup.inc').write_text(setup['Gfx_SetupDL25_Opa' if host == 'mm' else 'Gfx_SetupDL_25Opa'])
            source = ROOT / ('mm/2s2h/Rando/DrawItem.cpp' if host == 'mm' else 'soh/soh/Enhancements/randomizer/draw.cpp')
            (build / 'key_ring_callback.inc').write_text(functions(source.read_text())['DrawKeyRing' if host == 'mm' else 'Randomizer_DrawMmKeyRing'])
            flags = ['-std=c++20','-DF3DEX_GBI_2','-DGBI_FLOAT_MTX','-DGBI_S32_VTX','-DLOG_LEVEL_GAME_PRINTS=0']
            includes = ('.',host,host+'/include',host+'/src',host+'/assets','libultraship/include','combo/menu')
            flags += ['-I'+str(ROOT/path) for path in includes] + ['-I'+str(build)]
            if host == 'mm': flags += ['-DKEY_RING_TEST_MM','-I'+str(ROOT/'mm/2s2h')]
            else:
                for config in ('CMake/soh-cvars.cmake','CMake/lus-cvars.cmake'):
                    for key,value in re.findall(r'set\((CVAR_PREFIX_\w+)\s+"?([^\s"\)]+)',(ROOT/config).read_text()):
                        flags.append(f'-D{key}="{value}"')
            if '--sanitize' in sys.argv:
                flags += ['-g','-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer','-fno-pie','-no-pie']
            for name in ('render','callback'):
                binary = build/(host+'-'+name)
                extra = ['-DCOMBO_BUILD'] if name == 'callback' else []
                subprocess.run([os.environ.get('CXX','c++'),*flags,*extra,str(ROOT/('tests/shuffle_parity/key_rings/'+name+'_test.cpp')),'-o',str(binary)],check=True)
                subprocess.run([str(binary)],check=True)
if __name__ == '__main__': main()
