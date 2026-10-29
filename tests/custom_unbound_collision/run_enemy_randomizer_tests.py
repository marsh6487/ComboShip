#!/usr/bin/env python3
"""Run actual OoT Enemy Randomizer policy and spawn hook bodies on native types."""
from pathlib import Path
import os
import subprocess
import tempfile

from run_tests import ROOT, flags, function


def main():
    source = (ROOT / "soh/soh/Enhancements/ExtraModes/EnemyRandomizer.cpp").read_text()
    begin = source.index("typedef struct EnemyEntry {")
    end = source.index("void FixClubMoblinScale(")
    hook = source.index("COND_VB_SHOULD(VB_SPAWN_ACTOR_ENTRY,")
    begin_hook = source.index("{", hook)
    end_hook, depth = begin_hook + 1, 1
    while depth:
        depth += (source[end_hook] == "{") - (source[end_hook] == "}")
        end_hook += 1
    production = (source[begin:end] + "\n" + function(source, "EnemyRando_SetColliderPosTp") + "\n" +
                  function(source, "EnemyRando_SetColliderPosSkb") +
                  "\nstatic void ProductionSpawnEntry(bool* should, ...) {\nva_list args; va_start(args, should);\n" +
                  source[begin_hook + 1:end_hook - 1] + "\nva_end(args);\n}\n")
    with tempfile.TemporaryDirectory(prefix="custom-wide-enemy-") as directory:
        work = Path(directory)
        (work / "enemy_randomizer_production.inc").write_text(production)
        binary = work / "enemy"
        subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", *flags("soh"), "-I" + directory,
                        "-fsanitize=float-cast-overflow", "-no-pie",
                        str(Path(__file__).with_name("enemy_randomizer_test.cpp")),
                        "-Wl,--gc-sections", "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True, env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"})
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
