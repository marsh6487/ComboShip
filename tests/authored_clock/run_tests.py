#!/usr/bin/env python3
"""Execute production Clock callbacks/descriptors and native foreign draw commands."""
from pathlib import Path
import os, re, subprocess, sys, tempfile
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'scripts/diagnostics'))
from run_time_pedestal_tests import functions
from foreign_gi_fixture_dependencies import startup_foreign_receipt_boundary
from run_mm_nei_tests import flags as mm_flags, write_graph_helpers

def struct(source,name):
    start=source.index('struct '+name+' {')
    return source[start:source.index('\n};',start)+3]

with tempfile.TemporaryDirectory(prefix='authored-clock-') as folder:
    temp=Path(folder)
    oot=(ROOT/'soh/soh/Enhancements/randomizer/draw.cpp').read_text()
    mm=(ROOT/'mm/2s2h/Rando/DrawItem.cpp').read_text()
    oot_export=(ROOT/'combo/menu/ComboItemDrawOOT.h').read_text()
    mm_export=(ROOT/'combo/menu/ComboItemDrawMM.h').read_text()
    of=(ROOT/'combo/menu/ComboForeignDrawOOT.h').read_text()
    mf=(ROOT/'combo/menu/ComboForeignDrawMM.h').read_text()
    graph=functions((ROOT/'soh/src/code/graph.c').read_text())
    (temp/'nei_gi_graph.inc').write_text(graph['Graph_OpenDisps']+'\n'+graph['Graph_CloseDisps'])
    (temp/'nei_gi_dispatch.inc').write_text('')
    (temp/'clock_oot.inc').write_text(functions(oot)['Randomizer_DrawClock'])
    (temp/'clock_simple.inc').write_text(functions(oot_export)['CwSimple'])
    (temp/'clock_mm.inc').write_text(functions(mm)['DrawOotClock'])
    # Compile the actual host-selection arms, rather than checking their names
    # with regex. Removing or reverting one arm changes the executed recipe.
    start=oot_export.index('        case RG_CLOCK:')
    oc=oot_export[start:oot_export.index('        case RG_',start+20)]
    (temp/'clock_oot_export.inc').write_text('static int ClockExport(int rg,CwItemDrawInfo* out) { switch(rg) {\n'+oc+'default: return 0; }}')
    start=mm_export.index('    if (id == RI_OOT_CLOCK)',mm_export.index('static int32_t MM_FillItemDrawInfo'))
    mc=mm_export[start:mm_export.index('    bool progressiveConverted',start)]
    (temp/'clock_mm_export.inc').write_text('static int ClockExport(int id,CwItemDrawInfo* out) {\n'+mc+'return 0; }')
    # The actual native MM switch arm is exercised with a deliberate half-day
    # mismatch: an alias must call DrawOotClock and never DrawClock(DAY1).
    start=mm.index('        case RI_OOT_CLOCK:',mm.index('void Rando::DrawResolvedItem'))
    end=mm.index('        case RI_WALLET_TYCOON:',start)
    (temp/'clock_mm_dispatch.inc').write_text('static void ClockDispatch(int randoItemId) { Actor* actor=nullptr; switch(randoItemId) {\n'+mm[start:end]+'default: break; }}')
    for host,source,name in [('oot',of,'ComboForeignDrawInfo'),('mm',mf,'ComboForeignDrawInfoOOT')]:
        f=functions(source.replace('inline void ','void '))
        draw=f['OOT_DrawForeignCustomGi' if host=='oot' else 'MM_DrawForeignCustomGi']
        prefix='OOT' if host=='oot' else 'MM'
        begin=source.index('#define '+prefix+'_FOREIGN_PIN_OPA')
        end=source.index('// Biggoron',begin) if host=='oot' else source.index('// Restore the segments',begin)
        macros=source[begin:end]
        if host=='oot':
            macros='#define COMBO_FOREIGN_MTX(disp) gSPMatrix(disp, Matrix_NewMtx(play->state.gfxCtx,(char*)__FILE__,__LINE__), G_MTX_MODELVIEW | G_MTX_LOAD)\n'+macros
        enum='ComboForeignResolve' if host=='oot' else 'ComboForeignResolveOOT'
        intern='ComboInternRoutedPath' if host=='oot' else 'ComboInternRoutedPathOOT'
        route_start=source.index('    static constexpr char kOtrPrefix[] = "__OTR__";',source.index('inline '+enum+' ComboFill'))
        route_end=source.index('    info.setupDlOpa = raw.setupDlOpa;',route_start)
        metadata=''.join(re.search(r'^    info\.'+field+r' = .*;$',source,re.M).group(0)+'\n'
                         for field in ('drawKind','itemShimmer','neiShimmer','opCount'))
        route='static '+enum+' ClockRoute(const CwItemDrawInfo& raw,'+name+'& info) {\n'+source[route_start:route_end]+metadata+'    return '+enum+'::Ok;\n}'
        helpers='enum class '+enum+' {Ok,Unknown,NotReady};\n'+f[intern]+'\n'+route
        (temp/f'clock_{host}_foreign.inc').write_text(struct(source,name)+'\n'+helpers+'\n'+macros+'\n'+draw)
        compositor=f[prefix+'_DrawComboForeign']
        # Unrelated item drawers are deliberately unreachable boundaries. The
        # Clock's complete production compositor, custom drawer and shimmer are
        # executed, so a missing/wrong dispatch or duplicate effect cannot hide.
        unused=sorted(set(re.findall(r'\b'+prefix+r'_DrawForeign\w+(?=\()',compositor))-
                      {prefix+'_DrawForeignCustomGi',prefix+'_DrawForeignWeaponFlame'})
        boundaries='\n'.join('template<class... A> static bool '+fn+'(A&&...) { assert(false && "unrelated foreign item draw"); return false; }'
                             for fn in unused)
        if host=='oot':
            # Retain the real receipt classifier; ordinary Clock draws must not
            # enter the separately tested frozen startup-receipt path.
            boundaries=startup_foreign_receipt_boundary(ROOT)+'\n'+boundaries
        (temp/f'clock_{host}_compositor.inc').write_text(boundaries+'\n'+compositor)
    # MM's real wrapper preserves explicit @owner routes as deferred filepath
    # commands. The old fixture's no-op wrapper cannot prove submission order.
    wrapper=functions((ROOT/'mm/src/code/stubs.c').read_text())['gSPDisplayList']
    (temp/'clock_mm_displaylist.inc').write_text(wrapper.replace('void gSPDisplayList(', 'void ClockNativeDisplayList('))
    for host,path in [('oot','soh/soh/GbiWrap.cpp'),('mm','mm/src/code/stubs.c')]:
        wrapper=functions((ROOT/path).read_text().replace('extern "C" ',''))['gDPSetTileSizeLerp']
        (temp/f'clock_{host}_tile_lerp.inc').write_text('extern "C" {\n'+wrapper+'\n}\n')
    flags=['-std=c++20','-DF3DEX_GBI_2','-DLOG_LEVEL_GAME_PRINTS=0']
    flags += ['-I'+str(ROOT/p) for p in ('.','soh','soh/include','soh/src','soh/assets','soh/mods','libultraship/include','combo','combo/menu')]
    flags += ['-I'+str(temp)]
    for config in ('CMake/soh-cvars.cmake','CMake/lus-cvars.cmake'):
        for key,value in re.findall(r'set\((CVAR_PREFIX_\w+)\s+"?([^\s"\)]+)',(ROOT/config).read_text()):
            flags.append(f'-D{key}="{value}"')
    cxx=os.environ.get('CXX','c++')
    common=['-O2','-ffunction-sections','-fdata-sections']
    binary=temp/'oot'
    subprocess.run([cxx,*flags,*common,str(ROOT/'tests/authored_clock/oot_test.cpp'),'-Wl,--gc-sections','-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
    write_graph_helpers(folder)
    (temp/'mm_song_draw.inc').write_text(functions(mm)['DrawSong'])
    native=mm_flags()+['-I'+str(temp),*common]
    objects=[]
    for source in ('NeiGiPresentation.cpp','NeiResourceRouting.cpp'):
        obj=temp/(source+'.o')
        subprocess.run([cxx,'-std=c++20',*native,'-c',str(ROOT/'mm/2s2h/Rando'/source),'-o',str(obj)],check=True)
        objects.append(str(obj))
    binary=temp/'mm'
    subprocess.run([cxx,'-std=c++20',*native,str(ROOT/'tests/authored_clock/mm_test.cpp'),*objects,'-Wl,--gc-sections','-Wl,--export-dynamic-symbol=OOT_NeiResourceExists','-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
    # Header availability in both original engines and combined engine targets.
    (temp/'header.cpp').write_text('#include "combo/NeiClockGI.h"\n')
    for host,host_flags in [('oot',flags),('mm',['-std=c++20',*native])]:
        for combo in ([],['-DCOMBO_BUILD']):
            subprocess.run([cxx,*host_flags,*combo,'-fsyntax-only',str(temp/'header.cpp')],check=True)
    # Registration is a non-executable table assignment; keep this narrow check
    # beside the callback command test, without mutating item identity fields.
    registration=(ROOT/'soh/soh/Enhancements/randomizer/item_list.cpp').read_text()
    assert 'itemTable[RG_CLOCK].SetCustomDrawFunc(Randomizer_DrawClock);' in registration
print('PASS Clock host callbacks, live foreign descriptors/full compositors, segment-free scroll, original/combined headers')
