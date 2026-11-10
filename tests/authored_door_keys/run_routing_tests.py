#!/usr/bin/env python3
"""Compile production callbacks against native matrices, graph and GBI in both builds."""
from pathlib import Path
import os,re,subprocess,sys,tempfile
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'scripts/diagnostics'))
from run_time_pedestal_tests import functions
with tempfile.TemporaryDirectory(prefix='authored-door-routing-') as folder:
    build=Path(folder)
    for host in ('soh','mm'):
        matrix=functions((ROOT/host/'src/code/sys_matrix.c').read_text())
        (build/'key_ring_matrix.inc').write_text('\n'.join(matrix[x] for x in ('Matrix_MtxFCopy','Matrix_Push','Matrix_Pop','Matrix_Translate','Matrix_Scale','Matrix_RotateAxisF' if host=='mm' else 'Matrix_RotateAxis')))
        (build/'door_matrix.inc').write_text('\n'.join(matrix[x] for x in (('Matrix_RotateZF','Matrix_RotateXF') if host=='mm' else ('Matrix_RotateZ','Matrix_RotateZYX','Matrix_ReplaceRotation'))))
        receiver=(ROOT/('combo/menu/ComboForeignDrawMM.h' if host=='mm' else 'combo/menu/ComboForeignDrawOOT.h')).read_text()
        (build/'door_consumer.inc').write_text(functions(receiver)['MM_DrawForeignSimple' if host=='mm' else 'OOT_DrawForeignOps'])
        graph=functions((ROOT/host/'src/code/graph.c').read_text())
        (build/'key_ring_graph.inc').write_text(graph['Graph_OpenDisps']+'\n'+graph['Graph_CloseDisps'])
        (build/'key_ring_setup.inc').write_text(functions((ROOT/host/'src/code/z_rcp.c').read_text())['Gfx_SetupDL25_Opa' if host=='mm' else 'Gfx_SetupDL_25Opa']+'\n'+functions((ROOT/host/'src/code/z_rcp.c').read_text())['Gfx_SetupDL25_Xlu' if host=='mm' else 'Gfx_SetupDL_25Xlu'])
        source=ROOT/('mm/2s2h/Rando/DrawItem.cpp' if host=='mm' else 'soh/soh/Enhancements/randomizer/draw.cpp')
        (build/'door_callback.inc').write_text(functions(source.read_text().replace('extern "C" ',''))['DrawDoorKey' if host=='mm' else 'Randomizer_DrawOverworldKey'])
        producer=(ROOT/('combo/menu/ComboItemDrawMM.h' if host=='mm' else 'combo/menu/ComboItemDrawOOT.h')).read_text()
        marker='    if (ComboDoorKeyGi::Describe(ComboDoorKeyGi::ItemOrigin('
        start=producer.index(marker)
        end=producer.index('    const char* dl;',start) if host=='mm' else producer.index('    // Jabber Nuts:',start)
        branch=producer[start:end]
        (build/'door_exporter.inc').write_text('int DescribeExportedDoor(DoorTestItem '+('id' if host=='mm' else 'rg')+', CwItemDrawInfo* out) {\n'+branch+'\nreturn 0;\n}')
        if host=='soh':
            layers=functions(producer)
            (build/'door_layers.inc').write_text(layers['CwLayerPrim']+'\n'+layers['CwLayerEnv'])
            catalog=(ROOT/'soh/soh/Enhancements/randomizer/item_list.cpp').read_text()
            setters=re.findall(r'^    itemTable\[(RG_\w+)\]\.SetCustomDrawFunc\(Randomizer_DrawOverworldKey\);',catalog,re.M)
            (build/'door_registered.inc').write_text(','.join(setters))
        else:
            dispatch=source.read_text()
            start=dispatch.index('    if (DrawDoorKey(randoItemId))')
            end=dispatch.index('    if (DungeonItem_GetKeyRingCount',start)
            (build/'door_dispatch.inc').write_text('void DispatchNativeDoor(RandoItemId randoItemId) {\n'+dispatch[start:end]+'\n++fallbacks;\n}')
        flags=['-std=c++20','-DF3DEX_GBI_2','-DGBI_FLOAT_MTX','-DGBI_S32_VTX','-DLOG_LEVEL_GAME_PRINTS=0']
        for config in ('CMake/soh-cvars.cmake','CMake/lus-cvars.cmake'):
            for key,value in re.findall(r'set\((CVAR_PREFIX_\w+)\s+"?([^\s"\)]+)',(ROOT/config).read_text()):flags.append(f'-D{key}="{value}"')
        for path in ('.',host,host+'/include',host+'/src',host+'/assets','libultraship/include','combo','combo/menu'):flags+=['-I'+str(ROOT/path)]
        if host=='mm':flags+=['-DKEY_RING_TEST_MM','-I'+str(ROOT/'mm/2s2h')]
        if '--sanitize' in sys.argv:flags+=['-g','-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer','-fno-pie','-no-pie']
        flags+=['-I'+str(build)]
        for combo in (False,True):
            binary=build/(host+('-combo' if combo else '-original'))
            subprocess.run([os.environ.get('CXX','c++'),*flags,*(['-DCOMBO_BUILD'] if combo else []),str(Path(__file__).with_name('routing_test.cpp')),'-o',str(binary)],check=True)
            subprocess.run([str(binary)],check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
