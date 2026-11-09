#!/usr/bin/env python3
"""Exercise production menu/settings/pool/emission/fill boundaries for disabled OoT actions."""
from pathlib import Path
import argparse, os, re, subprocess, tempfile, json, hashlib
ROOT=Path(__file__).resolve().parents[2]
p=argparse.ArgumentParser();p.add_argument('--output',type=Path,required=True);p.add_argument('--observe-leak',action='store_true');args=p.parse_args();args.output.mkdir(parents=True,exist_ok=True)
def read(path):return (ROOT/path).read_text()
def block(text, marker):
    start=text.index(marker);opening=text.index('{',start);depth=0
    for pos in range(opening,len(text)):
        if text[pos]=='{':depth+=1
        elif text[pos]=='}':
            depth-=1
            if not depth:return text[start:pos+1]
    raise ValueError(marker)
mm=read('mm/2s2h/Rando/Logic/GeneratePools.cpp');oot=read('soh/soh/Enhancements/randomizer/3drando/item_pool.cpp');menu=read('mm/2s2h/Rando/Menu.cpp');bridge=read('mm/2s2h/FleetShipCombo/FleetShipCombo.cpp');emit=read('mm/2s2h/BenPort.cpp');items=read('mm/2s2h/Rando/StaticData/Items.cpp');options=read('mm/2s2h/Rando/StaticData/Options.cpp')
unified=block(mm[mm.index('// Combo (OoT+MM)'):],'if (FleetCombo_UnifiedPoolActive())')
gear=block(block(mm[mm.index('// Combo (OoT+MM)'):],'if (!FleetCombo_UnifiedPoolActive())'),'if (saveInfo.randoSaveOptions[RO_SHUFFLE_OOT_GEAR]')
# The generated harness leaves these production branches unchanged.
a=oot.index('if (ctx->GetOption(RSK_SHUFFLE_CLIMB))');b=oot.index('if (ctx->GetOption(RSK_SHUFFLE_SPEAK))',a)
rusty=block(read('mm/2s2h/Rando/RustyDoorKeys.h'),'inline constexpr bool IsDoorKeyItem(')
# Keep the real pure catalog predicate while resolving its production-relative
# include from the generated fixture's temporary directory.
rusty=rusty.replace('#include "../../../combo/rando/RustyDoorKeyCatalog.inc"',
                    '#include "'+str(ROOT/'combo/rando/RustyDoorKeyCatalog.inc')+'"')
fragments={
 'mm_rusty_visual.inc':'namespace Rando::RustyDoorKeys {\n'+rusty+'\n}\n',
 'oot_add.inc':block(oot,'void AddItemToPool('),
 'oot_pool_map.inc':oot[oot.index('const std::map<RandomizerGet,'):oot.index('void AddItemToPool(')],
 'oot_actions.inc':oot[a:b],
 'mm_gear.inc':unified+'\nif (!FleetCombo_UnifiedPoolActive()) {\n'+gear+'\n}',
 'mm_active.inc':block(bridge,'int FleetCombo_UnifiedPoolActive(void)'),
 'mm_menu.inc':menu[menu.index('struct NeiRandoOption {'):menu.index('static void NeiRando_DrawSetAllButtons()')],
 'mm_options.inc':options[options.index('#define RO('):options.index('// clang-format on')],
 'mm_names.inc':block(items,'static const std::unordered_map<RandoItemId, std::string>& ComboItemEmitNames()')+'\n'+block(items,'const std::string& GetItemDisplayName('),
 'mm_settings.inc':block(emit[emit.index('extern "C" COMBO_EXPORT const char* MM_DumpRandoStaticData'):],'for (auto& [id, opt] : Rando::StaticData::Options)'),
 'mm_pool_emit.inc':block(emit[emit.index('// Pool = the real free item pool'):],'for (RandoItemId iid : itemPool)'),
 'mm_category.inc':block(emit,'auto isAdvancement =')+';\n'+block(emit,'auto categoryName =')+';',
 'mm_has_oot.inc':re.search(r'    const bool hasOotGameplay = .*?;',mm).group(0),
}
# Restrict metadata to the actual options touched by the menu function; unrelated defaults
# depend on game headers and are not part of this probe. Keep each selected row verbatim.
menu_ids=set(re.findall(r'\{\s*(RO_\w+)\s*,',fragments['mm_menu.inc']))
option_rows=[line for line in options.splitlines() if re.match(r'\s*RO\(',line) and re.search(r'RO\((RO_\w+)',line).group(1) in menu_ids]
assert len(option_rows)==len(menu_ids)
macro=options[options.index('#define RO('):options.index('// clang-format off')]
fragments['mm_options.inc']=macro+'std::map<RandoOptionId,RandoStaticOption> Options={\n'+'\n'.join(option_rows)+'\n};\n'
# Decode actual table strings and include every catalog row, so native name collision handling runs.
rows=re.findall(r'^\s+RI\((RI_\w+),\s*"(?:\\.|[^"\\])*",\s*("(?:\\.|[^"\\])*")\s*,\s*(RITYPE_\w+)',items,re.M)
assert len(rows)>200
fragments['mm_item_rows.inc']='\n'.join('{'+rid+', {'+rid+',"'+rid+'",'+name+','+typ+'}},' for rid,name,typ in rows)
with tempfile.TemporaryDirectory(prefix='action-pool-') as temp:
    tmp=Path(temp)
    for name,text in fragments.items():(tmp/name).write_text(text)
    (args.output/'extraction-hashes.json').write_text(json.dumps({n:hashlib.sha256(t.encode()).hexdigest() for n,t in fragments.items()},indent=2)+'\n')
    binary=tmp/'probe'
    cmd=['g++','-std=c++20','-O0','-g','-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-pie','-no-pie','-I'+str(ROOT),'-I'+str(ROOT/'combo'),'-I'+str(tmp),str(ROOT/'tests/action_pool/probe.cpp'),'-o',str(binary)]
    subprocess.run(cmd,check=True)
    run=[str(binary),str(args.output.resolve())]
    if args.observe_leak:run+=['--observe-leak']
    with (args.output/'fill.log').open('w') as log:
        result=subprocess.run(run,stdout=log,stderr=subprocess.PIPE,text=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
    (args.output/'verification.log').write_text(result.stderr)
    print(result.stderr,end='');raise SystemExit(result.returncode)
