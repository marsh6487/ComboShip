#!/usr/bin/env python3
"""Check Solo overworld portals against both directions of the native MM graph."""
import os
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
header = ROOT / "combo/rando/SoloEntranceMmOverworld.h"
assert header.exists(), "Solo MM has no native overworld entrance catalogue"

# These are outdoor regions and their open connecting passages. Door interiors,
# temple entrances, owl/spawn travel and authored ComboShip areas have other owners.
world = set("""
TERMINA_FIELD EAST_CLOCK_TOWN NORTH_CLOCK_TOWN SOUTH_CLOCK_TOWN WEST_CLOCK_TOWN LAUNDRY_POOL
MILK_ROAD ROMANI_RANCH GORMAN_TRACK ROAD_TO_SOUTHERN_SWAMP SOUTHERN_SWAMP_POISONED
WOODS_OF_MYSTERY DEKU_PALACE WOODFALL PATH_TO_MOUNTAIN_VILLAGE MOUNTAIN_VILLAGE_WINTER
PATH_TO_GORON_VILLAGE_WINTER GORON_VILLAGE_WINTER PATH_TO_SNOWHEAD SNOWHEAD GORON_RACETRACK
GREAT_BAY_COAST ZORA_CAPE PINNACLE_ROCK WATERFALL_RAPIDS ZORA_HALL ROAD_TO_IKANA
IKANA_GRAVEYARD IKANA_CANYON STONE_TOWER STONE_TOWER_INVERTED
""".split())
native, region_scenes = {}, {}
for path in sorted((ROOT / "mm/2s2h/Rando/Logic/Regions").glob("*.cpp")):
    source = path.read_text()
    regions = list(re.finditer(r"Regions\[(RR_\w+)\]\s*=\s*RandoRegion\s*\{", source))
    for i, region in enumerate(regions):
        body = source[region.end():regions[i + 1].start() if i + 1 < len(regions) else len(source)]
        for a, sa, b, sb in re.findall(
                r"EXIT\(ENTRANCE\((\w+),\s*(\d+)\),\s*ENTRANCE\((\w+),\s*(\d+)\)", body):
            if a in world and b in world:
                native[((a, int(sa)), (b, int(sb)))] = region[1]
                region_scenes[region[1]] = re.search(r"\.sceneId\s*=\s*(SCENE_\w+)", body)[1]
assert len(native) == 80, "Review changed native overworld coverage before changing the catalogue"
assert all((b, a) in native for a, b in native), "Native pair is missing its return"

scenes = (ROOT / "mm/include/z64scene.h").read_text()
types = (ROOT / "mm/2s2h/Rando/Types.h").read_text()
constants = "\n".join("constexpr int " + name + "=" + value + ";" for value, name in re.findall(
    r"/\*\s*(0x[0-9A-Fa-f]+)\s*\*/\s*(ENTR_SCENE_\w+)", scenes))
constants += "\nenum RandoRegionId {" + ",".join(dict.fromkeys(re.findall(r"\bRR_\w+", types))) + "};\n"
constants += "#define ENTRANCE(scene,spawn) (((ENTR_SCENE_##scene&0x7F)<<9)|((spawn&0x1F)<<4))\n"
constants += "\n".join("constexpr int " + name + "=" + value + ";" for value, name in re.findall(
    r"/\*\s*(0x[0-9A-Fa-f]+)\s*\*/\s*DEFINE_SCENE\([^,]+,\s*(SCENE_\w+)",
    (ROOT / "mm/include/tables/scene_table.h").read_text()))
instant = re.search(r"/\*\s*(\d+)\s*\*/\s*TRANS_TYPE_INSTANT,",
                    (ROOT / "mm/include/z64transition.h").read_text())[1]
constants += "\nconstexpr int TRANS_TYPE_INSTANT = " + instant + ";\n"
expected = []
for (a, b), source in sorted(native.items()):
    reverse_source = native[(b, a)]
    expected.append("{{ENTRANCE(%s,%d),ENTRANCE(%s,%d)},{%s,%s}}" % (
        *a, *b, reverse_source, source))

program = r'''
#include <cassert>
#include <cstring>
#include <iostream>
#include <map>
#include <set>
''' + constants + r'''
#include "combo/rando/CrossEntranceMmCatalogue.h"
#include "combo/rando/SoloEntranceMmOverworld.h"
int main() {
    using namespace ComboRando::CrossDungeon;
    using namespace ComboSoloMmOverworld;
    const std::map<std::pair<int,int>,std::pair<RandoRegionId,RandoRegionId>> expected = {
''' + ",\n".join(expected) + r'''
    };
    const std::map<RandoRegionId,int> nativeScenes = {
''' + ",\n".join("{%s,%s}" % pair for pair in sorted(region_scenes.items())) + r'''
    };
    for (const auto& portal : Portals()) {
        assert(portal.scene == nativeScenes.at(portal.region));
        assert(portal.sourceScene == nativeScenes.at(portal.sourceRegion));
        assert(AllowsTransition(portal.entrance, portal.sourceScene, 0xFFEF, 0));
        assert(AllowsTransition(portal.exit, portal.scene, 0xFFEF, 0));
        assert(!AllowsTransition(portal.entrance, -1, 0xFFEF, 0));
        assert(!AllowsTransition(portal.exit, -1, 0xFFEF, 0));
    }
    std::set<std::pair<int,int>> seen;
    std::vector<Dungeon> overworld;
    Append(overworld);
    assert(overworld.size() == 40);
    for (const auto& portal : overworld) {
        assert(portal.pool == Pool::Overworld && portal.entrance.game == 1 && portal.exit.game == 1);
        assert(portal.exit == portal.returnTo && portal.hasReverse && !portal.expanded);
        const auto key = std::make_pair(portal.entrance.index, portal.exit.index);
        const auto found = expected.find(key);
        assert(found != expected.end() && "Overworld route must be an actual native reversible portal");
        assert(RegionForEntry(key.first) == found->second.first);
        assert(RegionForReturn(key.second) == found->second.second);
        assert(SourceLabel(key.first) && std::strlen(SourceLabel(key.first)) > 10);
        assert(Canonicalize(key.first) == key.first && Canonicalize(key.second) == key.second);
        assert(seen.insert(key).second);
        assert(seen.insert({key.second,key.first}).second);
    }
    assert(seen.size() == expected.size());
    PoolOptions all;
    all.dungeons=all.expanded=all.bosses=all.interiors=all.shops=all.grottos=true;
    auto catalogue = ComboCrossMmCatalogue::Catalogue(all);
    Append(catalogue);
    Validate(catalogue); // No duplicate ownership with interiors, grottos, temples or bosses.
    assert(catalogue.size() == 124);
    assert(Canonicalize(ENTRANCE(SOUTHERN_SWAMP_CLEARED,0)) == ENTRANCE(SOUTHERN_SWAMP_POISONED,0));
    assert(Canonicalize(ENTRANCE(SOUTHERN_SWAMP_CLEARED,1)) == ENTRANCE(SOUTHERN_SWAMP_POISONED,1));
    assert(Canonicalize(ENTRANCE(SOUTHERN_SWAMP_CLEARED,8)) == ENTRANCE(SOUTHERN_SWAMP_POISONED,8));
    assert(Canonicalize(ENTRANCE(MOUNTAIN_VILLAGE_SPRING,2)) == ENTRANCE(MOUNTAIN_VILLAGE_WINTER,2));
    assert(Canonicalize(ENTRANCE(MOUNTAIN_VILLAGE_SPRING,1)) == ENTRANCE(MOUNTAIN_VILLAGE_WINTER,1));
    assert(Canonicalize(ENTRANCE(GORON_VILLAGE_SPRING,0)) == ENTRANCE(GORON_VILLAGE_WINTER,0));
    assert(Canonicalize(ENTRANCE(PATH_TO_GORON_VILLAGE_SPRING,1)) == ENTRANCE(PATH_TO_GORON_VILLAGE_WINTER,1));
    // Boss warp, authored forest, owl spawn, virtual grotto and unknown IDs are not ordinary portals.
    for (int index : {ENTRANCE(MOUNTAIN_VILLAGE_SPRING,7), ENTRANCE(MOUNTAIN_VILLAGE_SPRING,8),
                      ENTRANCE(MOUNTAIN_VILLAGE_SPRING,0), ENTRANCE(SOUTH_CLOCK_TOWN,0), 0xF01A, -1})
        assert(Canonicalize(index) == index);
    assert(RegionForEntry(ENTRANCE(SOUTHERN_SWAMP_CLEARED,0)) == RR_SOUTHERN_SWAMP_NORTH);
    assert(RegionForReturn(ENTRANCE(MOUNTAIN_VILLAGE_SPRING,2)) == RR_MOUNTAIN_VILLAGE);
    assert(RegionForEntry(-1) == RR_MAX && RegionForReturn(-1) == RR_MAX && !SourceLabel(-1));
    // The same destination index also appears in scripted native reloads. It
    // must only act as a shuffled portal from the actual native source scene.
    assert(AllowsTransition(ENTRANCE(ROMANI_RANCH,0), SCENE_ROMANYMAE, 0xFFEF, 0));
    assert(!AllowsTransition(ENTRANCE(ROMANI_RANCH,0), SCENE_F01, 0xFFF0, 0));
    assert(!AllowsTransition(ENTRANCE(ROMANI_RANCH,0), SCENE_F01, 0xFFF3, 0));
    assert(!AllowsTransition(ENTRANCE(ROMANI_RANCH,0), SCENE_F01, 0xFFF5, 0));
    assert(AllowsTransition(ENTRANCE(IKANA_CANYON,14), SCENE_IKANA, 0xFFEF, 0));
    assert(!AllowsTransition(ENTRANCE(IKANA_CANYON,14), SCENE_IKANA, 0, TRANS_TYPE_INSTANT));
    assert(AllowsTransition(ENTRANCE(STONE_TOWER_INVERTED,0), SCENE_F40, 0, 64));
    assert(AllowsTransition(ENTRANCE(STONE_TOWER,1), SCENE_F41, 0, 64));
    assert(!AllowsTransition(ENTRANCE(STONE_TOWER_INVERTED,0), SCENE_F41, 0, 64));
    assert(AllowsTransition(ENTRANCE(PATH_TO_GORON_VILLAGE_SPRING,0), SCENE_10YUKIYAMANOMURA2, 0xFFEF, 0));
    assert(AllowsTransition(ENTRANCE(WOODS_OF_MYSTERY,0), SCENE_20SICHITAI, 0xFFEF, 0));
    assert(!AllowsTransition(ENTRANCE(WOODS_OF_MYSTERY,0), 0x7fff, 0xFFEF, 0));
    assert(AllowsTransition(ENTRANCE(TERMINA_FIELD,0), SCENE_ICHIBA, 0xFFEF, 0));
    assert(!AllowsTransition(ENTRANCE(TERMINA_FIELD,0), SCENE_ICHIBA, 0xFFF7, 0));
    std::cout << "PASS 40 native overworld pairs, exact regions, complete coverage, seasonal aliases and reserved travel\n";
}
'''
with tempfile.TemporaryDirectory(prefix="solo-mm-overworld-") as directory:
    source, binary = Path(directory) / "test.cpp", Path(directory) / "test"
    source.write_text(program)
    subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", "-Wall", "-Wextra", "-Werror",
                    "-I", str(ROOT), str(source), "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
