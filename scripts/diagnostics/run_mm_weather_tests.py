"""Compile and exercise MM weather production code without game assets."""
import os
import pathlib
import re
import shlex
import subprocess
import tempfile
import sys

sys.dont_write_bytecode = True
from run_mm_audio_runtime_test import function_body

ROOT = pathlib.Path(__file__).resolve().parents[2]


def main():
    with tempfile.TemporaryDirectory(prefix="mm-weather-") as temporary:
        build = pathlib.Path(temporary)
        compiler = shlex.split(os.environ.get("CXX", "c++"))
        extra = shlex.split(os.environ.get("MM_WEATHER_TEST_CXXFLAGS", ""))
        flags = [*extra, "-std=c++20", "-Wall", "-Wextra", "-Werror", "-I" + str(ROOT / "mm")]
        executable = build / "weather_state_test"
        subprocess.run([*compiler, *flags,
                        str(ROOT / "mm/tests/weather_state_test.cpp"),
                        str(ROOT / "mm/2s2h/Enhancements/Audio/MMWeatherState.cpp"),
                        "-o", str(executable)], check=True)
        subprocess.run([str(executable)], check=True)
        executable = build / "weather_mixer_test"
        subprocess.run([*compiler, *flags,
                        str(ROOT / "mm/tests/weather_mixer_test.cpp"),
                        str(ROOT / "mm/2s2h/Enhancements/Audio/MMWeatherMixer.cpp"),
                        "-o", str(executable)], check=True)
        subprocess.run([str(executable)], check=True)
        game_flags = ["-DF3DEX_GBI_2", "-DCOMBO_BUILD", "-DMM_BUILD_DLL",
                      "-DCONTROLLERBUTTONS_T=uint32_t", "-DNON_EQUIVALENT", "-DNON_MATCHING"]
        game_flags += ["-I" + str(ROOT / path) for path in
                       ("mm/include", "mm/include/PR", "mm/src", "mm/2s2h", "mm/assets",
                        "libultraship/include", "libultraship/src", "combo")]
        for source in ("mm/src/code/z_kankyo.c", "mm/src/code/z_play.c"):
            result = subprocess.run([*shlex.split(os.environ.get("CC", "cc")), "-std=gnu17",
                                     "-fsyntax-only", "-DNDEBUG", "-Wno-int-conversion",
                                     "-Wno-incompatible-pointer-types", "-I" + str(ROOT / "mm"),
                                     *game_flags, str(ROOT / source)], capture_output=True, text=True)
            if result.returncode:
                raise RuntimeError(result.stdout + result.stderr)
            print("PASS real-header weather syntax:", source, flush=True)
        executable = build / "weather_bridge_test"
        environment = (ROOT / "mm/src/code/z_kankyo.c").read_text()
        play = (ROOT / "mm/src/code/z_play.c").read_text()
        gate = re.search(r"if \([^;{}]*precipitation\[PRECIP_RAIN_CUR\][^;{}]*\)\s*\{\s*"
                         r"Environment_DrawRain\(this, &this->view, gfxCtx\);\s*\}", play)
        if gate is None:
            raise RuntimeError("Cannot find actual Play rain draw gate")
        (build / "weather_draw_gate.inc").write_text(
            "void DrawWeatherFromPlay(PlayState* play) {\nGraphicsContext* gfxCtx = play->state.gfxCtx;\n" +
            gate[0].replace("this", "play") + "\n}\n")
        structs = environment[environment.index("typedef enum {"):environment.index("// Variables are put")]
        functions = []
        for name in ("Environment_AddLightningBolts", "MMWeather_StartBolt", "MMWeather_ClearBolts"):
            if re.search(r"^void " + name + r"\(", environment, re.M):
                start, end = function_body(environment, name)
                functions.append(environment[start:end])
        (build / "weather_bolts.inc").write_text(
            structs + "\nLightningBolt sLightningBolts[3];\n"
            "static LightningBolt sMMWeatherLightningBolt = { .state = LIGHTNING_BOLT_INACTIVE };\n" +
            "\n".join(functions))
        result = subprocess.run([*compiler, *flags, *game_flags, "-Wno-error", "-I" + str(build),
                        str(ROOT / "mm/tests/weather_bridge_test.cpp"),
                        str(ROOT / "mm/2s2h/Enhancements/Audio/MMWeather.cpp"),
                        str(ROOT / "mm/2s2h/Enhancements/Audio/MMWeatherState.cpp"),
                        "-o", str(executable)], capture_output=True, text=True)
        if result.returncode:
            raise RuntimeError(result.stdout + result.stderr)
        subprocess.run([str(executable)], check=True)
        executable = build / "weather_audio_test"
        port = (ROOT / "mm/2s2h/BenPort.cpp").read_text()
        start = port.index("        MMWeatherAudio_Mix(audio_buffer,")
        end = port.index("        audio.processing = false;", start)
        (build / "weather_output.inc").write_text(
            "void SubmitWeatherAudio(int16_t* audio_buffer, unsigned int num_audio_samples) {\n" +
            port[start:end] + "}\n")
        result = subprocess.run([*compiler, *flags, *game_flags, "-Wno-error", "-pthread",
                                "-I" + str(build),
                                str(ROOT / "mm/tests/weather_audio_test.cpp"),
                                str(ROOT / "mm/2s2h/Enhancements/Audio/MMWeatherAudio.cpp"),
                                str(ROOT / "mm/2s2h/Enhancements/Audio/MMWeatherMixer.cpp"),
                                "-o", str(executable)], capture_output=True, text=True)
        if result.returncode:
            raise RuntimeError(result.stdout + result.stderr)
        subprocess.run([str(executable)], check=True)


if __name__ == "__main__":
    main()
