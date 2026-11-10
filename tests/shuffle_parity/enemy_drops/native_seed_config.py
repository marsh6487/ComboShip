"""Source identities only; actual pool/reachability results must come from DLLs."""
from pathlib import Path
import re
ROOT = Path(__file__).resolve().parents[3]
RANDO = ROOT / 'soh/soh/Enhancements/randomizer'

def catalogue():
    rows = re.findall(r'^ENEMY_FIRST_DROP\((\w+),\s*"([^"]+)"\)',
                      (RANDO/'EnemyFirstDropsCatalog.inc').read_text(), re.M)
    source = (RANDO/'item_list.cpp').read_text()
    result = []
    for key, name in rows:
        line = next(line for line in source.splitlines() if 'itemTable[RG_SOUL_ENEMY_'+key+'] =' in line)
        soul = re.search(r'\{\s*"([^"]+)"', line)[1]
        result.append({'key':key, 'check':'Global Enemy Drop: '+name, 'soul':soul})
    assert len(result) == 43
    return result

def layouts():
    # Derive dungeon names and scene ownership from the actual native constructor.
    source = (RANDO/'dungeon.cpp').read_text()
    pairs = re.findall(r'DungeonInfo\(\s*"([^"]+)".*?(SCENE_\w+),\s*\{', source, re.S)
    assert len(pairs) == 12, pairs
    names = dict((scene,name) for name,scene in pairs)
    return names

def expected_checks(mq):
    rows = catalogue()
    names = layouts()
    active = set()
    for file in ('EnemyFirstDropsWitnesses.inc','EnemyFirstDropsAdditionalWitnesses.inc'):
        for key,scene,quest in re.findall(r'^ENEMY_DROP_WITNESS\((\w+),\s*RR_\w+,\s*(SCENE_\w+),\s*(RCQUEST_\w+),', (RANDO/file).read_text(),re.M):
            if quest == 'RCQUEST_BOTH' or (names[scene] in mq) == (quest == 'RCQUEST_MQ'):
                active.add(key)
    return {row['check'] for row in rows if row['key'] in active}

def cases():
    names = list(layouts().values())
    selections = {'vanilla':[], 'mq':names, 'mixed-even':names[::2], 'mixed-odd':names[1::2]}
    for layout, mq in selections.items():
        for enabled,excluded in ((False,False),(True,False),(True,True)):
            expected = expected_checks(mq) if enabled else set()
            excluded_keys = [row['key'] for row in catalogue() if row['check'] in expected] if excluded else []
            yield {'name':layout+('-on-excluded' if excluded else '-on' if enabled else '-off'),
                   'mq':mq, 'expected':sorted(expected), 'excluded':excluded,
                   'settings':{'gRandoSettings.ShuffleEnemyDrops':int(enabled),
                               'gRandoSettings.ShuffleEnemySouls':1,
                               'gRandoSettings.SkijerCustomItems':0,
                               'gRandoSettings.NoLogic':0,
                               'gRandoSettings.AllChecksReachable':1,
                               'gRandoSettings.BaseIceTraps':0,
                               'gRandoSettings.AdditionalIceTraps':0,
                               'gRandoSettings.IceTrapPercent':0,
                               'gRandoSettings.BombchuBag':0,
                               'gRandoSettings.EnableBombchuDrops':0,
                               'gRandoSettings.ExcludedLocations':''.join('RC_ENEMY_DROP_'+key+',' for key in excluded_keys)}}


def native_junk_names():
    """Exact default GetJunkItem candidates, resolved through native item IDs.

    Cases pin BombchuBag/EnableBombchuDrops and trap settings off, so neither
    conditional Bombchu additions nor ShouldJunkItemBeTrap can add candidates.
    Fixed[] exports names/advancement, not RG IDs/categories; native item_list
    binds those names to IDs/categories rather than treating non-advancement as junk.
    """
    pool = (RANDO/'3drando/item_pool.cpp').read_text()
    initializers = [text for text in re.findall(r'JunkPoolItems = \{(.*?)\};',pool,re.S) if 'RG_' in text]
    assert len(initializers)==1, 'native default junk initializer changed'
    ids = re.findall(r'RG_\w+',initializers[0])
    source = (RANDO/'item_list.cpp').read_text()
    result = {}
    for key in ids:
        line = next(line for line in source.splitlines() if 'itemTable['+key+'] =' in line)
        assert 'ITEM_CATEGORY_JUNK' in line, (key,'native junk initializer/category drift')
        assert key != 'RG_ICE_TRAP'
        name = re.search(r'Text\{\s*"([^"]+)"',line)[1]
        assert name not in result
        result[name] = key
    assert result
    return result
