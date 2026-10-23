#!/usr/bin/env python3
"""Exercise the actual keeper interaction and camera adapters with native types."""
from pathlib import Path
import os, re, subprocess, sys, tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/diagnostics"))
from run_time_pedestal_tests import block_from


def native_camera_path(host, camera, play):
    """Execute the real manual-camera path; debug hooks are unused boundaries."""
    data = (ROOT / host / 'src/code/z_camera_data.inc').read_text()
    prefix = data[:data.index('} CameraSetting;') + len('} CameraSetting;')]
    array = lambda signature: block_from(data, data.index(signature)) + ';\n'
    function = lambda signature: block_from(camera, re.search(re.escape(signature) + r'[^;{]*\{', camera).start())
    mode_values = 'sSetFree0ModeNormalData' if host == 'soh' else 'sSetCutscenesModeNormalData'
    prefix += '\n#define CAM_SETTING_MODE_ENTRY(func, data) { func, ARRAY_COUNT(data), data }\n'
    prefix += array('CameraModeValue ' + mode_values + '[')
    prefix += array('CameraMode sCamSetFree0Modes[')
    setting = re.search(r'^\s*(\{[^\n]*sCamSetFree0Modes[^\n]*\}),', data, re.M).group(1)
    prefix += 'CameraSetting sCameraSettings[CAM_SET_MAX]{};\n'
    prefix += 'void InitNativeCameraTable() { sCameraSettings[CAM_SET_FREE0] = ' + setting + '; }\n'
    prefix += '#define osSyncPrintf(...) ((void)0)\n'
    prefix += 'f32 Player_GetHeight(Player*) { assert(false && "manual subcamera has no player binding"); return 0; }\n'
    if host == 'soh':
        prefix += '\n'.join((
            'static GameInfo cameraRegisters{}; GameInfo* gGameInfo = &cameraRegisters;',
            'SaveContext gSaveContext{};',
            'static s32 sInitRegs = 1, sDbgModeIdx = -1, sCameraInterfaceFlags = 1;',
            'static s32 sCameraInterfaceAlpha, sCameraShrinkWindowVal, D_8011D3F0;',
            'static s16 sNextUID; PlayState* D_8015BD7C; DbCamera D_8015BD80;',
            'void DbCamera_Reset(Camera*, DbCamera*) {}',
            'void DbCamera_Init(DbCamera*, Camera*) {}',
            '#define PCT(x) ((x) * .01f)',
            '#define NEXTSETTING ((values++)->val)',
            '#define RELOAD_PARAMS (camera->animState == 0 || camera->animState == 10 || camera->animState == 20 || R_RELOAD_CAM_PARAMS)',
            'char sCameraSettingNames[CAM_SET_MAX][12]{};',
            's32 Camera_RequestModeImpl(Camera*, s16, u8) { assert(false && "FREE0 request must retain initialized normal mode"); return -1; }',
            'void Camera_CopyDataToRegs(Camera*, s16) { assert(false); }',
            's32 Camera_CopyPREGToModeValues(Camera*) { assert(false && "debug camera parameter reload is disabled"); return 0; }',
        )) + '\n'
        prefix += array('s16 sOREGInit[') + 's16 sOREGInitCnt = ARRAY_COUNT(sOREGInit);\n'
        prefix += array('s16 sCamDataRegsInit[') + 's16 sCamDataRegsInitCount = ARRAY_COUNT(sCamDataRegsInit);\n'
        signatures = ('void func_80043ABC(', 's16 Camera_ChangeSettingFlags(', 's32 Camera_RequestSetting(')
        wrapper = 'Play_CameraChangeSetting'
    else:
        prefix += '\n'.join((
            'PlayState* sCamPlayState; s16 sCameraNextUID;',
            's32 sCameraInterfaceFlags, sCameraHudVisibility, sCameraLetterboxSize, sCameraInitSceneTimer;',
            '#define __osMemset std::memset',
            '#define RELOAD_PARAMS(camera) ((camera)->animState == 0 || (camera)->animState == 10 || (camera)->animState == 20)',
            '#define GET_NEXT_RO_DATA(values) ((values++)->val)',
            '#define CAM_CHANGE_SETTING_0 (1 << 0)',
            '#define CAM_CHANGE_SETTING_1 (1 << 1)',
            '#define CAM_CHANGE_SETTING_2 (1 << 2)',
            '#define CAM_CHANGE_SETTING_3 (1 << 3)',
            '#define GameInteractor_ExecuteOnCameraChangeSettingsFlags(...) assert(false)',
            's32 Camera_ChangeModeFlags(Camera*, s16, u8) { assert(false && "FREE0 request must retain initialized normal mode"); return -1; }',
            'f32 Camera_GetFocalActorHeight(Camera*) { assert(false && "manual subcamera has no focal actor binding"); return 0; }',
        )) + '\n'
        signatures = ('s16 Camera_SetStateFlag(', 's16 Camera_UnsetStateFlag(', 's32 func_800DF498(',
                      'void Camera_ResetActionFuncState(', 'void Camera_SetUpdateRatesFastPitch(',
                      'void Camera_SetFocalActorAtOffset(', 's16 Camera_ChangeSettingFlags(', 's32 Camera_ChangeSetting(')
        wrapper = 'Play_ChangeCameraSetting'
    prefix += '\n'.join(function(signature) for signature in signatures)
    prefix += '\n' + function('void Camera_Init(').replace('Camera_Init', 'NativeCameraInit', 1)
    olig = (ROOT / host / 'src/code/z_olib.c').read_text()
    prefix += '\n' + block_from(olig, olig.index('f32 OLib_Vec3fDist('))
    prefix += '\n' + function('s32 Camera_Unique6(')
    prefix += '\n' + block_from(play, play.index('s32 ' + wrapper + '(')).replace(wrapper, 'NativeSetting', 1)
    return prefix


with tempfile.TemporaryDirectory(prefix="market-browsing-") as temporary:
    for host in ("mm", "soh"):
        source = (ROOT / host / ("2s2h" if host == "mm" else "soh") /
                  "Enhancements/Gameplay" / ("BlackMarket.cpp" if host == "mm" else
                  "ComboBlackMarketNative.cpp")).read_text()
        declarations = re.findall(r'constexpr u16 kBrowseText[^;]+;', source)
        declarations += re.findall(r'constexpr Vec3f kShelves\[8\] = \{.*?\};', source, re.S)
        native = (ROOT / "combo/BlackMarketBrowsingNative.h").read_text()
        signatures = ["void PlayBrowseCursorSound(", "bool BrowsingMessage(", "void ContinueBrowsingText(", "void EndBrowsing(", "bool BeginBrowsing(",
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
        camera_bodies = [native_camera_path(host, camera, play),
                         block_from(camera, camera.index("s32 " + setter + "(")),
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
