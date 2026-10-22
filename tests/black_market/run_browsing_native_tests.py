#!/usr/bin/env python3
"""Exercise the actual keeper interaction and camera adapters with native types."""
from pathlib import Path
import os, re, subprocess, sys, tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/diagnostics"))
from run_time_pedestal_tests import block_from

with tempfile.TemporaryDirectory(prefix="market-browsing-") as temporary:
    for host in ("mm", "soh"):
        source = (ROOT / host / ("2s2h" if host == "mm" else "soh") /
                  "Enhancements/Gameplay" / ("BlackMarket.cpp" if host == "mm" else
                  "ComboBlackMarketNative.cpp")).read_text()
        declarations = re.findall(r'constexpr u16 kBrowseText[^;]+;', source)
        declarations += re.findall(r'constexpr Vec3f kShelves\[8\] = \{.*?\};', source, re.S)
        native = (ROOT / "combo/BlackMarketBrowsingNative.h").read_text()
        signatures = ["void PlayBrowseCursorSound(", "bool BrowsingMessage(", "void EndBrowsing(", "bool BeginBrowsing(",
                      "void UpdateBrowsingCamera(PlayState* play) {", "void MonitorBrowsing(", "void BrowseInput(",
                      "void DrawBrowseCursor("]
        bodies = [block_from(native, native.index(s)) for s in signatures]
        bodies.append(block_from(source, source.index("void KeeperUpdate(")))
        actor = (ROOT / host / "src/code/z_actor.c").read_text()
        skin = (ROOT / host / "src/code/z_skin_matrix.c").read_text()
        projection = "Actor_ProjectPos" if host == "soh" else "Actor_GetProjectedPos"
        # The old fixture omitted cursor drawing. Keep the actual focus-position
        # semantics and matrix multiplication so an origin cursor cannot pass.
        bodies[:0] = [block_from(skin, skin.index("void SkinMatrix_Vec3fMtxFMultXYZW(")),
                      block_from(actor, actor.index("void " + projection + "(")),
                      block_from(actor, actor.index("void Actor_GetScreenPos("))]
        shop = (ROOT / host / "src/overlays/actors/ovl_En_Ossan/z_en_ossan.c").read_text()
        cursor = block_from(shop, shop.index("void EnOssan_UpdateCursorPos("))
        bodies.insert(3, re.sub(r"\bthis\b", "current", cursor.replace("EnOssan_UpdateCursorPos", "NativeCursorPos", 1)))
        joystick = block_from(shop, shop.index("void EnOssan_UpdateJoystickInputState("))
        bodies.insert(4, re.sub(r"\bthis\b", "current", joystick.replace("EnOssan_UpdateJoystickInputState", "NativeJoystick", 1)))
        dpad = 'u16 dLeft = BTN_DLEFT; u16 dRight = BTN_DRIGHT;'
        if host == "soh":
            begin = shop.index("u16 dLeft = BTN_DLEFT;")
            dpad = shop[begin:shop.index("// Stick Left", begin)]
        bodies.insert(5, 'int NativeDpadIntent(u16 buttons) { ' + dpad +
                      ' return (buttons & dLeft) ? -1 : (buttons & dRight) ? 1 : 0; }')
        fixture = (ROOT / "tests/black_market/browsing_native_test.cpp").read_text()
        graph = (ROOT / host / "src/code/graph.c").read_text()
        debug = "\n".join(block_from(graph, graph.index(signature))
                          for signature in ("void Graph_OpenDisps(", "void Graph_CloseDisps("))
        fixture = fixture.replace("/* NATIVE DEBUG */", debug)
        camera = (ROOT / host / "src/code/z_camera.c").read_text()
        play = (ROOT / host / "src/code/z_play.c").read_text()
        math = (ROOT / host / "src/code/sys_math3d.c").read_text()
        lib = (ROOT / host / "src/code/z_lib.c").read_text()
        setter = "Camera_SetParam" if host == "soh" else "Camera_SetViewParam"
        at_eye = "Play_CameraSetAtEye" if host == "soh" else "Play_SetCameraAtEye"
        fov = "Play_CameraSetFov" if host == "soh" else "Play_SetCameraFov"
        camera_bodies = [block_from(camera, camera.index("s32 " + setter + "(")),
                         block_from(lib, lib.index("void Math_Vec3f_Diff(")),
                         block_from(lib, lib.index("f32 Math_Vec3f_DistXYZ(")),
                         block_from(math, math.index("f32 Math3D_Vec3f_DistXYZ(")),
                         block_from(play, play.index("s32 " + at_eye + "(")).replace(at_eye, "NativeAtEye", 1),
                         block_from(play, play.index("s32 " + fov + "(")).replace(fov, "NativeFov", 1)]
        fixture = fixture.replace("/* NATIVE CAMERA */", re.sub(r"\bthis\b", "current", "\n".join(camera_bodies)))
        fixture = fixture.replace("/* PRODUCTION */", "\n".join(declarations + bodies))
        cpp = Path(temporary) / (host + ".cpp"); cpp.write_text(fixture)
        binary = Path(temporary) / host
        paths = [host, host + "/include", host + "/include/PR", host + "/src", host + "/assets",
                 host + ("/2s2h" if host == "mm" else "/soh"), "libultraship/include", "combo"]
        subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", "-w", "-DCOMBO_BUILD", "-DF3DEX_GBI_2",
                        *(["-DOOT_BROWSING_TEST", "-DBLACK_MARKET_BROWSING_OOT",
                           '-DCVAR_PREFIX_ENHANCEMENT="gEnhancements"'] if host == "soh" else []), "-g",
                        "-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-no-pie",
                        "-I" + str(ROOT), *["-I" + str(ROOT / p) for p in paths], str(cpp), "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True, env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"})
