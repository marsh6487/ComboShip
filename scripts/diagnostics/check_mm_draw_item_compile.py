"""Compile MM's complete production item drawer before the expensive game builds."""
import os
from pathlib import Path
import re
import shlex
import subprocess
import tempfile
import urllib.request

ROOT = Path(__file__).resolve().parents[2]


def check_draw_item(source=ROOT / "mm/2s2h/Rando/DrawItem.cpp"):
    with tempfile.TemporaryDirectory(prefix="mm-draw-item-") as directory:
        work = Path(directory)
        # Use the project's real dependency pins, not stand-ins for engine headers.
        dependencies = (ROOT / "libultraship/cmake/dependencies/common.cmake").read_text()
        includes = []
        for name, repository, files in (
            ("ImGui", "ocornut/imgui", ("imgui.h", "imgui_internal.h", "imconfig.h")),
            ("ThreadPool", "bshoshany/thread-pool", ("include/BS_thread_pool.hpp",)),
        ):
            tag = re.search(r"FetchContent_Declare\(\s*" + name + r"\b.*?GIT_TAG\s+(\S+)",
                            dependencies, re.S)[1]
            location = os.environ.get(name.upper() + "_INCLUDE_DIR")
            if location is None:
                location = work / name
                location.mkdir()
                for file in files:
                    url = f"https://raw.githubusercontent.com/{repository}/{tag}/{file}"
                    with urllib.request.urlopen(url, timeout=30) as response:
                        (location / Path(file).name).write_bytes(response.read())
            includes.append("-I" + str(location))
        # These ordinary includes supply the production PCH prerequisites. In
        # particular, no NEI renderer declaration is injected into the drawer.
        headers = ("libultraship/bridge/consolevariablebridge.h", "ship/Context.h",
                   "ship/resource/ResourceManager.h", "ship/window/Window.h",
                   "2s2h/GameInteractor/GameInteractor.h", "variables.h", "functions.h",
                   "macros.h", "z64.h")
        wrapper = work / "draw_item.cpp"
        wrapper.write_text("".join(f"#include <{header}>\n" for header in headers) +
                           f'#include "{Path(source).resolve().as_posix()}"\n')
        flags = ["-std=c++20", "-fsyntax-only", "-fpermissive", "-DCOMBO_BUILD",
                 "-DMM_BUILD_DLL", "-DF3DEX_GBI_2", "-DNDEBUG", "-DLOG_LEVEL_GAME_PRINTS=6"]
        flags += ["-I" + str(ROOT / path) for path in
                  ("mm", "mm/2s2h", "mm/include", "mm/include/PR", "mm/src", "mm/assets",
                   "mm/mods", "libultraship/include", "ZAPDTR/ZAPD/resource/type", "combo", "combo/menu")]
        flags += shlex.split(subprocess.check_output(["pkg-config", "--cflags", "sdl2", "spdlog"], text=True))
        subprocess.run([os.environ.get("CXX", "c++"), *shlex.split(os.environ.get("CPPFLAGS", "")),
                        *flags, *includes, str(wrapper)], check=True)
        print("MM production DrawItem.cpp compile check passed", flush=True)


if __name__ == "__main__":
    check_draw_item()
