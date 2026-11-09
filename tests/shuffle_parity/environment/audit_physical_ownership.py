#!/usr/bin/env python3
"""Compare every environmental source with its real native CHECK owner.

This is a source ownership audit, not a native traversal or gameplay test. It
does not trust the donor manifest's authored region/condition as its evidence.
"""
from collections import defaultdict
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[3]
RANDO = ROOT / "mm/2s2h/Rando"


def tokens(source):
    # Preserve offsets and braces belonging to code; comments and string/char
    # literals cannot fabricate a region assignment or a registered CHECK.
    return re.sub(r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'',
                  lambda match: " " * len(match[0]), source, flags=re.S)


def region_owners():
    regions, checks = {}, defaultdict(list)
    for path in sorted((RANDO / "Logic/Regions").glob("*.cpp")):
        source = tokens(path.read_text())
        for match in re.finditer(r"Regions\[(RR_\w+)\]\s*=\s*RandoRegion\s*\{", source):
            end, depth = match.end(), 1
            while depth:
                depth += (source[end] == "{") - (source[end] == "}")
                end += 1
            region = match[1]
            body = source[match.end():end - 1]
            scenes = re.findall(r"\.sceneId\s*=\s*(SCENE_\w+)", body)
            assert region not in regions, f"duplicate native region {region}"
            # A missing/literal-free owner cannot silently count as a match.
            regions[region] = scenes[0] if len(scenes) == 1 else None
            for check in re.findall(r"\bCHECK\(\s*(RC_\w+)\s*,", body):
                checks[check].append(region)
    return regions, checks


def seasonal_equivalence():
    # These are physical seasonal variants of the same outdoor area. Confirm
    # each exception against the native scene canonicalizer, rather than
    # allowing a different cave/interior or all same-name donor regions.
    mapping = {
        "SCENE_10YUKIYAMANOMURA2": "SCENE_10YUKIYAMANOMURA",
        "SCENE_11GORONNOSATO2": "SCENE_11GORONNOSATO",
        "SCENE_17SETUGEN2": "SCENE_17SETUGEN",
    }
    native = tokens((ROOT / "mm/src/code/z_play.c").read_text())
    start = native.index("s16 Play_GetOriginalSceneId(")
    end = native.index("\n}", start)
    native = native[start:end]
    for physical, original in mapping.items():
        assert re.search(r"if\s*\(sceneId\s*==\s*" + physical +
                         r"\)\s*\{\s*return\s+" + original + r"\s*;", native), physical
    return mapping


def main():
    catalog = {}
    for check, scene, region in re.findall(
            r"MM_ENVIRONMENT_CHECK\((RC_\w+),\s*\w+,\s*(SCENE_\w+),\s*(RR_\w+)\)",
            (RANDO / "MMEnvironmentChecks.inc").read_text()):
        assert check not in catalog, check
        catalog[check] = (scene, region)
    for check, scene, region in re.findall(
            r"MM_ICICLE\((RC_\w+),\s*(SCENE_\w+),\s*\d+,\s*\d+,\s*(RR_\w+)\)",
            (RANDO / "MMEnvironmentIcicles.inc").read_text()):
        assert check not in catalog, check
        catalog[check] = (scene, region)
    assert len(catalog) == 211, len(catalog)

    placements = re.findall(
        r"MM_ENVIRONMENT_PLACEMENT\((RC_\w+),\s*\w+,\s*(SCENE_\w+),[^\n]+\)",
        (RANDO / "MMEnvironmentPlacements.inc").read_text())
    assert len(placements) == 279, len(placements)
    assert {check for check, _ in placements} == set(catalog), "source/check coverage differs"
    regions, owners = region_owners()
    seasonal = seasonal_equivalence()
    canonical = lambda scene: seasonal.get(scene, scene)
    failures = []
    for check, (scene, region) in catalog.items():
        if owners[check] != [region]:
            failures.append(f"{check}: native CHECK owners {owners[check]} != [{region}]")
        native_scene = regions.get(region)
        if native_scene is None or canonical(native_scene) != canonical(scene):
            failures.append(f"{check}: catalogue physical {scene}, native {region} owns {native_scene}")
    for check, physical in placements:
        _, region = catalog[check]
        native_scene = regions.get(region)
        if native_scene is None or canonical(native_scene) != canonical(physical):
            failures.append(f"{check}: source physical {physical}, native {region} owns {native_scene}")
    assert not failures, "physical/native ownership mismatch:\n" + "\n".join(failures)
    print("PASS211 checks/all279 physical aliases: exact registered native CHECK owner/scene; "
          "only native winter/spring equivalences")


if __name__ == "__main__":
    main()
