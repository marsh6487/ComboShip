"""Exercise production SW97 cast limbs and isolated tornado dust textures."""
from pathlib import Path
import os
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
SW97 = ROOT / "soh/expansions/sw97"
SPELLS = ("Fire", "Ice", "Light", "Dark", "Soul", "Wind")


def check_actor_registration(build, cc, common):
    # Compile the native structs and the size definitions from the router in C, the
    # language used to build the actors. The actual C++ registration calls a recording ActorDB.
    layouts = ['#include "global.h"', '#include <string.h>']
    for spell in SPELLS:
        layout = (SW97 / f"actors/spells/z_magic_{spell.lower()}.inc.c").read_text()
        layouts.extend(re.findall(r"^enum \{ SW97_.*? \};|^struct Magic\w+;|^typedef void \(\*Magic\w+Func\).*?;",
                                  layout, re.M))
        layouts.append(re.search(rf"typedef struct Magic{spell} \{{.*?\}} Magic{spell};", layout, re.S)[0])
    router = (SW97 / "sw97_router.c").read_text()
    layouts.extend(re.findall(r"^const size_t gSw97Magic\w+InstanceSize = [^;]+;", router, re.M))
    layouts.append("size_t Sw97Test_ActorSize(const char* name) {")
    for spell in SPELLS:
        layouts.append(f'    if (strcmp(name, "Sw97_Magic_{spell}") == 0) return sizeof(Magic{spell});')
    layouts.append("    return 0;\n}")
    source = build / "sw97-layouts.c"
    source.write_text("\n".join(layouts))
    native = build / "sw97-layouts.o"
    subprocess.run([cc, "-std=gnu2x", *common, "-c", str(source), "-o", str(native)], cwd=ROOT, check=True)

    registration = SW97 / "sw97_init.cpp"
    callbacks = re.findall(r"^extern void (Sw97_\w+)\(Actor\* thisx, PlayState\* play\);", registration.read_text(), re.M)
    fixture = build / "sw97-actor-db.cpp"
    fixture.write_text('''#include "soh/ActorDB.h"
#include <cstdio>

extern "C" size_t Sw97Test_ActorSize(const char* name);
void Sw97_RegisterActors();
static int checked;
static int errors;
ActorDB::ActorDB() = default;
ActorDB::Entry::Entry() = default;
static ActorDB recordingDb;
ActorDB* ActorDB::Instance = &recordingDb;

ActorDB::Entry& ActorDB::AddEntry(const ActorDBInit& init) {
    static Entry entry;
    size_t expected = Sw97Test_ActorSize(init.name.c_str());
    if (expected != 0) {
        ++checked;
        if (init.instanceSize != expected) {
            std::fprintf(stderr, "%s: allocates %zu bytes for a %zu-byte actor\\n",
                         init.name.c_str(), init.instanceSize, expected);
            ++errors;
        }
    }
    return entry;
}
int main() {
    Sw97_RegisterActors();
    if (checked != 6 || errors != 0) return 1;
    std::puts("PASS: native ActorDB allocation sizes for all six SW97 spells");
}
''' + "\n".join(f'extern "C" void {name}(Actor*, PlayState*) {{}}' for name in callbacks))
    binary = build / "sw97-register-actors"
    subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", "-fpermissive", *common,
                    str(registration), str(fixture), str(native), "-o", str(binary)], cwd=ROOT, check=True)
    subprocess.run([str(binary)], check=True)


def main():
    cc = os.environ.get("CC", "cc")
    common = ["-DNDEBUG", "-DF3DEX_GBI_2", "-DLOG_LEVEL_GAME_PRINTS=0"]
    for path in ("soh/include", "soh/src", "soh/assets", "soh", "libultraship/include"):
        common.append("-I" + str(ROOT / path))
    for path in ("CMake/soh-cvars.cmake", "CMake/lus-cvars.cmake"):
        for key, value in re.findall(r'set\((CVAR_PREFIX_\w+)\s+"?([^\s"\)]+)', (ROOT / path).read_text()):
            common.append(f'-D{key}="{value}"')
    with tempfile.TemporaryDirectory(prefix="medallion-cast-textures-") as temp:
        if len(sys.argv) == 1 or sys.argv[1] == "registration":
            check_actor_registration(Path(temp), cc, common)
            if len(sys.argv) != 1:
                return
        binary = Path(temp) / "test"
        subprocess.run([cc, *common, "-std=gnu2x", "-O1", "-g", "-Werror=implicit-function-declaration",
                        "-Wno-incompatible-pointer-types", "-Wno-int-conversion", "-Wno-discarded-qualifiers",
                        "-ffunction-sections", "-fdata-sections",
                        "soh/tests/medallion_cast_textures_test.c",
                        "soh/src/code/code_8006C510.c",
                        "soh/src/code/z_fcurve_data_skelanime.c",
                        "soh/src/code/z_effect_soft_sprite_old_init.c",
                        "-Wl,--gc-sections", "-lm", "-o", str(binary)], cwd=ROOT, check=True)
        subprocess.run([str(binary), *sys.argv[1:]], check=True)


if __name__ == "__main__":
    main()
