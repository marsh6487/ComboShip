#!/usr/bin/env python3
"""Compare compiled public/native enum IDs; source discovery never assigns values."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import tempfile


def enums(source):
    result = {}
    for match in re.finditer(r'typedef\s+enum(?:\s+\w+)?\s*\{([^{}]*)\}\s*(\w+)\s*;', source, re.S):
        names = []
        # These production enum initializers have no comma operators. Reject
        # unexpected entries rather than silently omit an identifier.
        for entry in match[1].split(','):
            entry = entry.strip()
            if not entry:
                continue
            name = re.match(r'(\w+)\s*(?:=|$)', entry)
            if not name:
                raise ValueError('Unrecognized native enum member: ' + entry)
            names.append(name[1])
        result[match[2]] = names
    assert result, 'native enum headers did not preprocess'
    return result


def snapshot(repo, game, mode, folder):
    compiler = os.environ.get('CXX', 'g++-14')
    flags = ['-std=c++20', '-I' + str(repo)]
    if mode == 'combo':
        flags.append('-DCOMBO_BUILD')
    header = ('soh/soh/Enhancements/randomizer/randomizerEnums.h' if game == 'oot'
              else 'mm/2s2h/Rando/Types.h' if game == 'mm'
              else ('soh/soh/' if game == 'fc-oot' else 'mm/2s2h/') + 'FleetShipCombo/FleetComboItems.h')
    prefix = '#include <cstdint>\n#include <cstddef>\n'
    if game.startswith('fc-'):
        ids = header.replace('FleetComboItems.h', 'FleetComboIds.h')
        prefix += '#include "' + ids + '"\n'
    if game in ('oot', 'mm'):
        prefix += '#include "' + ('soh' if game == 'oot' else 'mm') + '/include/z64item.h"\n'
    if game.startswith('fc-'):
        host = 'soh' if game == 'fc-oot' else 'mm'
        flags.append('-I' + str(repo / host))
        prefix += '#include "' + host + '/mods/nei_save.h"\n'
    prefix += '#include "' + header + '"\n'
    preprocessed = subprocess.check_output([compiler, *flags, '-E', '-P', '-x', 'c++', '-'], input=prefix.encode(), cwd=repo).decode()
    declarations = enums(preprocessed)
    # Standard integer typedefs have no enum bodies. Everything discovered here
    # comes from the production native headers, including expanded .inc rows.
    source = prefix + '#include <cstdio>\nint main(){\n'
    for enum, names in declarations.items():
        for name in names:
            source += 'printf("' + enum + '\\t' + name + '\\t%lld\\n",(long long)' + name + ');\n'
    if game.startswith('fc-'):
        for name in ('FC_COMBO_OBTAINED_SIZE', 'FC_COMBO_OBTAINED_FC_SIZE', 'FC_REGISTRY_VERSION'):
            source += 'printf("NativeCapacity\\t' + name + '\\t%lld\\n",(long long)' + name + ');\n'
    if game.startswith('fc-'):
        source += 'printf("NativeCapacity\\tActualNeiFcStoreBytes\\t%lld\\n",(long long)sizeof(((NeiSaveData*)nullptr)->comboObtainedFc));\n'
    source += '}\n'
    cpp = folder / (game + '-' + mode + '.cpp'); cpp.write_text(source)
    binary = cpp.with_suffix('')
    subprocess.run([compiler, *flags, str(cpp), '-o', str(binary)], check=True, cwd=repo)
    output = subprocess.check_output([str(binary)]).decode()
    rows = {}
    for line in output.splitlines():
        kind, name, value = line.split('\t')
        rows.setdefault(kind, {})[name] = int(value)
    return rows


def compare(before, after):
    changes = []; sentinel_changes = []; counts = {}
    for enum, names in before.items():
        end = list(names)[-1]
        for name, value in names.items():
            current = after.get(enum, {}).get(name)
            if current == value:
                counts[enum] = counts.get(enum, 0) + 1
            elif current is not None and ((name == end and name.endswith(('_MAX', '_COUNT'))) or name == 'RI_MAX_TRAP'):
                sentinel_changes.append({'enum': enum, 'name': name, 'before': value, 'after': current})
            else:
                changes.append({'enum': enum, 'name': name, 'before': value, 'after': current})
    return {'stableCounts': counts, 'movedSentinels': sentinel_changes, 'incompatibleChanges': changes}


def option_calls(repo):
    source = (repo / 'soh/soh/Enhancements/randomizer/settings.cpp').read_text()
    source = re.sub(r'("(?:\\.|[^"\\])*")|//[^\n]*|/\*.*?\*/', lambda m: m[1] or '', source, flags=re.S)
    result = {}
    for match in re.finditer(r'^\s*OPT_(BOOL|U8)\(', source, re.M):
        begin = match.end(); depth = 0; quoted = False; escape = False; parts = []; start = begin
        for index in range(begin, len(source)):
            ch = source[index]
            if quoted:
                if escape: escape = False
                elif ch == '\\': escape = True
                elif ch == '"': quoted = False
                continue
            if ch == '"': quoted = True; continue
            if ch in '({[': depth += 1
            elif ch in ')}]':
                if ch == ')' and depth == 0:
                    parts.append(source[start:index].strip()); break
                depth -= 1
            elif ch == ',' and depth == 0:
                parts.append(source[start:index].strip()); start = index + 1
        key = parts[0]
        if not re.fullmatch(r'RSK_\w+', key):
            raise ValueError('Dynamic option declaration needs explicit audit: ' + key)
        named = len(parts) > 2 and parts[1].startswith('"')
        if match[1] == 'U8': default_index = 7 if named else 5
        elif named: default_index = 6
        elif parts[1].startswith('{'): default_index = 5
        else: default_index = 4
        cvar = next((re.sub(r'\s+', '', x) for x in parts if re.search(r'CVAR_\w+\(', x)), '')
        default = re.sub(r'\s+', '', parts[default_index]) if len(parts) > default_index else '<native-factory-default>'
        result[key] = {'cvarExpression': cvar, 'defaultExpression': default, 'kind': match[1]}
    assert result
    return result


def mm_options(repo, mode, folder):
    source = '#include <cstring>\n#include <cstdio>\n#include "Rando/StaticData/StaticData.h"\nint main(){for(const auto&[id,opt]:Rando::StaticData::Options)printf("%d\\t%s\\t%u\\n",(int)id,opt.cvar,(unsigned)opt.defaultValue);}\n'
    cpp = folder / ('mm-options-' + mode + '.cpp'); cpp.write_text(source); binary = cpp.with_suffix('')
    flags = ['-std=c++20', '-DF3DEX_GBI_2', '-DGBI_FLOAT_MTX', '-DGBI_S32_VTX', '-DCONTROLLERBUTTONS_T=uint32_t',
             '-include', str(repo / 'tests/dungeon_rewards/headless.h')]
    if mode == 'combo': flags.append('-DCOMBO_BUILD')
    for path in ('.', 'combo', 'mm/include', 'mm/include/PR', 'mm', 'mm/2s2h', 'mm/src', 'mm/assets', 'libultraship/include'):
        flags.append('-I' + str(repo / path))
    subprocess.run([os.environ.get('CXX', 'g++-14'), *flags, str(cpp), str(repo / 'mm/2s2h/Rando/StaticData/Options.cpp'), '-o', str(binary)], check=True, cwd=repo)
    return {row[0]: {'cvar': row[1], 'default': int(row[2])}
            for row in (line.split('\t') for line in subprocess.check_output([str(binary)]).decode().splitlines())}


def oot_defaults(repo, options, folder, label):
    header = (repo / 'soh/soh/Enhancements/randomizer/option.h').read_text()
    defaults = re.findall(r'(?:uint8_t|bool) defaultOption_ = ([^,\n]+)', header)
    assert defaults and set(defaults) <= {'0', 'false'}, 'inspect changed native factory defaults'
    factory = defaults[0]
    source = '#include <cstdio>\n#include "soh/soh/Enhancements/randomizer/randomizerTypes.h"\n#include "combo/NeiSeasonsPolicy.h"\nint main(){\n'
    for key, row in options.items():
        expression = factory if row['defaultExpression'] == '<native-factory-default>' else row['defaultExpression']
        source += 'printf("' + key + '\\t%u\\n",(unsigned)(' + expression + '));\n'
    source += '}\n'
    cpp = folder / ('oot-defaults-' + label + '.cpp'); cpp.write_text(source); binary = cpp.with_suffix('')
    subprocess.run([os.environ.get('CXX', 'g++-14'), '-std=c++20', '-I' + str(repo),
                    '-I' + str(repo / 'soh/include'), str(cpp), '-o', str(binary)], check=True, cwd=repo)
    for line in subprocess.check_output([str(binary)]).decode().splitlines():
        key, value = line.split('\t'); options[key]['defaultValue'] = int(value)
    return options


def settings_audit(baseline, current, folder):
    before = oot_defaults(baseline, option_calls(baseline), folder, 'baseline')
    after = oot_defaults(current, option_calls(current), folder, 'current')
    def identity(row): return None if row is None else {key: row[key] for key in ('cvarExpression', 'kind', 'defaultValue')}
    changed = [{'key': key, 'before': value, 'after': after.get(key)} for key, value in before.items() if identity(after.get(key)) != identity(value)]
    # Factory defaults and CVar prefix expansion are production declarations;
    # their unchanged bytes make omitted arguments and string identities exact.
    stable_sources = {}
    for name in ('soh/soh/Enhancements/randomizer/option.h', 'soh/soh/Enhancements/randomizer/option.cpp',
                 'CMake/soh-cvars.cmake', 'CMake/lus-cvars.cmake'):
        old = (baseline / name).read_bytes(); new = (current / name).read_bytes()
        stable_sources[name] = {'equal': old == new, 'sha256': hashlib.sha256(new).hexdigest()}
    mm = {}
    for mode in ('original', 'combo'):
        old = mm_options(baseline, mode, folder); new = mm_options(current, mode, folder)
        mm[mode] = {'before': old, 'after': new,
                    'changedOldOptions': [key for key, value in old.items() if new.get(key) != value]}
    import sys
    sys.path.insert(0, str(current / 'tests/shuffle_parity/boss_keys'))
    import native_boss_key_regression as matrix
    oot_names = {value['cvarExpression'] for value in after.values()}
    oot_names.update(re.findall(r'j\[(CVAR_RANDOMIZER_SETTING\("[^"]+"\))\]', (current / 'soh/soh/OTRGlobals.cpp').read_text()))
    mm_names = {row['cvar'] for row in mm['combo']['after'].values()} | {'gRando.ExcludedChecks', 'gRando.StartingItems'}
    missing = []
    for case in ('all-off', 'all-parity'):
        settings = matrix.settings_for(case)
        for key in settings['oot']:
            suffix = key.removeprefix('gRandoSettings.')
            if 'CVAR_RANDOMIZER_SETTING("' + suffix + '")' not in oot_names:
                missing.append((case, 'oot', key))
        missing.extend((case, 'mm', key) for key in settings['mm'] if key not in mm_names)
    return {'ootOptions': len(before), 'ootDescriptorsBefore': before, 'ootDescriptorsAfter': after, 'changedOldOotDescriptors': changed, 'unchangedFactoriesAndCvarPrefixes': stable_sources,
            'mmCompiledOptions': mm, 'allOffAllParityUnknownCvars': missing}


def native_save_audit(repo, old_snapshot, folder):
    old_ro = old_snapshot['RandoOptionId']['RO_MAX']
    old_checks = old_snapshot['RandoCheckId']['RC_MAX']
    old_inf = old_snapshot['RandoInf']['RANDO_INF_MAX']
    source = r"""
#include "2s2h/BenJsonConversions.hpp"
#include <cassert>
#include <memory>
#include <cstdio>
int main(){
 constexpr size_t oldOptions=OLD_RO,oldChecks=OLD_CHECKS,oldInf=OLD_INF;
 auto source=std::make_unique<RandoSaveInfo>();
 source->finalSeed=20261109;
 source->randoSaveOptions[RO_STARTING_HEALTH]=6;
 source->randoSaveChecks[oldChecks-1].randoItemId=RI_HEART_PIECE;
 source->randoSaveChecks[oldChecks-1].shuffled=true;
 source->randoSaveChecks[oldChecks-1].obtained=true;
 source->randoSaveChecks[oldChecks-1].price=17;
 json legacy=*source;
 legacy.erase("songEvents");legacy.erase("pendingTrapCount");
 auto options=legacy["randoSaveOptions"].get<std::vector<u32>>();options.resize(oldOptions);legacy["randoSaveOptions"]=options;
 auto checks=legacy["randoSaveChecks"].get<std::vector<RandoSaveCheck>>();checks.resize(oldChecks);legacy["randoSaveChecks"]=checks;
 std::vector<u16>flags((oldInf+15)/16,0);flags[0]=1;flags[(oldInf-1)/16]|=1u<<((oldInf-1)%16);legacy["randoInf"]=flags;
 auto loaded=std::make_unique<RandoSaveInfo>();memset(loaded.get(),0xa5,sizeof(*loaded));legacy.get_to(*loaded);
 assert(loaded->randoSaveOptions[RO_STARTING_HEALTH]==6);
 for(size_t i=oldOptions;i<ARRAY_COUNT(loaded->randoSaveOptions);++i)assert(loaded->randoSaveOptions[i]==0);
 assert(loaded->randoSaveChecks[oldChecks-1].randoItemId==RI_HEART_PIECE);
 assert(loaded->randoSaveChecks[oldChecks-1].shuffled&&loaded->randoSaveChecks[oldChecks-1].obtained&&loaded->randoSaveChecks[oldChecks-1].price==17);
 for(size_t i=oldChecks;i<ARRAY_COUNT(loaded->randoSaveChecks);++i){const auto&c=loaded->randoSaveChecks[i];assert(c.randoItemId==RI_UNKNOWN&&!c.shuffled&&!c.eligible&&!c.cycleObtained&&!c.obtained&&!c.skipped&&c.price==0);}
 assert(loaded->randoInf[0]==1);
 assert(loaded->randoInf[(oldInf-1)/16]&(1u<<((oldInf-1)%16)));
 for(size_t i=oldInf;i<RANDO_INF_MAX;++i)assert(!(loaded->randoInf[i/16]&(1u<<(i%16))));
 const auto&nativeDefaults=ComboRando::SongEvents::Defaults(ComboRando::SongEvents::Game::Mm);
 assert(nativeDefaults.size()==ARRAY_COUNT(loaded->songEvents));
 for(size_t i=0;i<nativeDefaults.size();++i)assert(loaded->songEvents[i]==nativeDefaults[i]);
#ifdef COMBO_BUILD
 assert(loaded->pendingTrapCount==0);
#endif
 json future=*loaded;
 future["randoInf"].push_back(65535);future["randoSaveOptions"].push_back(65535);future["randoSaveChecks"].push_back(future["randoSaveChecks"][0]);future.get_to(*loaded);
 assert(loaded->randoSaveOptions[RO_STARTING_HEALTH]==6);
 puts("PASS actual native MM serializer: compiled public option/check/flag extents preserve old entries, zero new tails, native missing-song defaults, ignore future tails");
}
""".replace('OLD_RO', str(old_ro)).replace('OLD_CHECKS', str(old_checks)).replace('OLD_INF', str(old_inf))
    solo_entrances = repo / 'mm/2s2h/Rando/SoloEntrances.cpp'
    native_sources = [str(solo_entrances)] if solo_entrances.exists() else []
    if native_sources:
        # Link the actual codec owner; its runtime helpers use native engine storage.
        source = source.replace('int main(){', 'extern "C" {\nSaveContext gSaveContext{};\n'
                                '#ifdef COMBO_BUILD\nint gComboGameMode = COMBO_GAME_BOTH;\n#endif\n}\nint main(){', 1)
    results = {}
    for mode in ('original', 'combo'):
        cpp = folder / ('native-save-' + mode + '.cpp'); cpp.write_text(source); binary = cpp.with_suffix('')
        flags = ['-std=c++20', '-DF3DEX_GBI_2', '-DGBI_FLOAT_MTX', '-DGBI_S32_VTX', '-DCONTROLLERBUTTONS_T=uint32_t', '-DFMT_HEADER_ONLY']
        if mode == 'combo': flags.append('-DCOMBO_BUILD')
        for path in ('.', 'mm', 'mm/2s2h', 'mm/include', 'mm/include/PR', 'mm/src', 'mm/assets', 'libultraship/include', 'combo'):
            flags.append('-I' + str(repo / path))
        subprocess.run([os.environ.get('CXX', 'g++-14'), *flags, str(cpp), *native_sources, '-o', str(binary)], check=True, cwd=repo)
        results[mode] = subprocess.check_output([str(binary)]).decode().strip()
    return {'compiledPublicExtents': {'options': old_ro, 'checks': old_checks, 'flags': old_inf}, 'nativeSerializer': results}


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--baseline', type=Path, required=True)
    p.add_argument('--current', type=Path, required=True)
    p.add_argument('--output', type=Path, required=True)
    args = p.parse_args()
    assert not args.output.exists(), 'retain earlier audit evidence; use a fresh output'
    args.output.mkdir(parents=True)
    report = {'baseline': str(args.baseline.resolve()), 'current': str(args.current.resolve()), 'snapshots': {}, 'comparisons': {}}
    for label, repo in (('baseline', args.baseline.resolve()), ('current', args.current.resolve())):
        report[label + 'Commit'] = subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=repo).decode().strip()
        report['snapshots'][label] = {}
        for game in ('oot', 'mm', 'fc-oot', 'fc-mm'):
            for mode in ('original', 'combo'):
                folder = args.output / label; folder.mkdir(exist_ok=True)
                report['snapshots'][label][game + '-' + mode] = snapshot(repo, game, mode, folder)
    for key, old in report['snapshots']['baseline'].items():
        report['comparisons'][key] = compare(old, report['snapshots']['current'][key])
    current = report['snapshots']['current']
    for mode in ('original', 'combo'):
        assert {k: current['fc-oot-' + mode][k] for k in ('FleetComboId', 'FcComboItemId', 'NativeCapacity')} == {k: current['fc-mm-' + mode][k] for k in ('FleetComboId', 'FcComboItemId', 'NativeCapacity')}, 'mirrored compiled FC identifiers differ'
        capacity = current['fc-oot-' + mode]['NativeCapacity']['FC_COMBO_OBTAINED_FC_SIZE']
        ids = current['fc-oot-' + mode]['FcComboItemId']
        assert ids['FCI_MAX'] <= capacity == current['fc-oot-' + mode]['NativeCapacity']['ActualNeiFcStoreBytes'] == 512, 'actual compiled FC storage capacity exceeded'
    report['settingsAudit'] = settings_audit(args.baseline.resolve(), args.current.resolve(), args.output)
    report['saveCompatibility'] = native_save_audit(args.current.resolve(), report['snapshots']['baseline']['mm-combo'], args.output)
    report['fcCapacity'] = {'count': current['fc-oot-combo']['FcComboItemId']['FCI_MAX'], 'capacity': 512}
    (args.output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    incompatible = [(key, row) for key, value in report['comparisons'].items() for row in value['incompatibleChanges']]
    print(json.dumps({'compiledModes': len(current), 'stableIdentifiers': sum(sum(x['stableCounts'].values()) for x in report['comparisons'].values()),
                      'incompatibleChanges': incompatible, 'fcCapacity': report['fcCapacity']}))
    assert not report['settingsAudit']['changedOldOotDescriptors'], 'existing native OoT CVar/default descriptors changed'
    assert all(x['equal'] for x in report['settingsAudit']['unchangedFactoriesAndCvarPrefixes'].values()), 'native factory/prefix declaration changed; inspect explicit defaults'
    assert not any(x['changedOldOptions'] for x in report['settingsAudit']['mmCompiledOptions'].values()), 'existing compiled MM CVar/default changed'
    assert not report['settingsAudit']['allOffAllParityUnknownCvars'], 'matrix requests an unknown native setting'
    assert not incompatible, 'previous native identifiers changed; inspect exact compiled report'


if __name__ == '__main__':
    main()
