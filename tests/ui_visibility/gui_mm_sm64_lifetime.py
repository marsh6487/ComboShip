#!/usr/bin/env python3
"""Execute MM's actual HUD update and health draw block across suppressed draws."""
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / "mm/2s2h/expansions/sm64/Sm64CapsHud.cpp"


def block(source, start):
    begin = source.index("{", start)
    end, depth = begin + 1, 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[start:end]


prelude = r'''
#include <cassert>
#include <cstdint>
#include <memory>
#include <string>
#include <iostream>
using ImTextureID = uintptr_t;
using ImU32 = uint32_t;
using f32 = float;
struct ImVec2 { float x, y; ImVec2(float a, float b) : x(a), y(b) {} };
#define IM_COL32(r,g,b,a) uint32_t(a)
namespace ImGui {
struct IO { float DeltaTime = .25f; };
IO& GetIO() { static IO io; return io; }
void* GetCurrentContext() { return (void*)1; }
}
struct Vec3f { float x, y, z; };
struct Player { struct { struct { Vec3f pos; } world; } actor; };
struct PlayState {
    struct { int state = 0; } pauseCtx;
    struct { struct { Player* first; } actorLists[1]; } actorCtx;
};
constexpr int ACTORCAT_PLAYER = 0;
static Player player;
static PlayState play;
static PlayState* gPlayState = &play;
static int wedges = 8;
static bool enabled = true;
int Sm64Mario_GetHealthWedges() { return wedges; }
int CVarGetInteger(const char*, int) { return enabled; }
void Actor_GetProjectedPos(PlayState*, Vec3f*, Vec3f* out, float* w) { *out = {0,0,0}; *w = 1; }
namespace Ship {
struct GuiWindow {
    virtual void InitElement() = 0;
    virtual void DrawElement() = 0;
    virtual void UpdateElement() = 0;
    virtual void Draw() = 0;
};
struct Gui { bool menu = false; bool GetMenuOrMenubarVisible() { return menu; } };
struct Window { std::shared_ptr<Gui> gui = std::make_shared<Gui>(); auto GetGui() { return gui; } };
struct Context {
    std::shared_ptr<Window> window = std::make_shared<Window>();
    static auto GetRawInstance() { static Context context; return &context; }
    auto GetWindow() { return window; }
};
}
static unsigned healthImages;
struct DrawList {
    void AddImage(ImTextureID, ImVec2, ImVec2, ImVec2, ImVec2, ImU32 alpha) {
        if (alpha) ++healthImages;
    }
};
struct FastGui {
    ImTextureID GetTextureByName(const std::string&) { return 1; }
    ImVec2 GetTextureSize(const std::string&) { return ImVec2(32,32); }
};
/* PRODUCTION_CLASS */;
void Sm64CapsHudWindow::Draw() {
    const ImVec2 disp(640, 480);
    const float s = 1;
    DrawList list;
    FastGui gui;
    auto dl = &list;
    auto fastGui = &gui;
    /* PRODUCTION_HEALTH_DRAW */
}
'''

test = r'''
int main() {
    play.actorCtx.actorLists[0].first = &player;
    Sm64CapsHudWindow hud;
    hud.UpdateElement();
    hud.Draw();
    assert(healthImages == 0); // Initialization is not a health-change notification.
    wedges = 7;
    hud.UpdateElement();
    hud.Draw();
    assert(healthImages == 1);
    // True Hidden suppresses Draw but the GUI still calls UpdateElement.
    for (int frame = 0; frame < 14; ++frame) hud.UpdateElement();
    hud.Draw();
    assert(healthImages == 1); // The expired meter must not replay on reveal.
    wedges = 6;
    hud.UpdateElement();
    for (int draw = 0; draw < 20; ++draw) hud.Draw();
    assert(healthImages == 21); // Rendering itself must never age the timer.
    // A health change wholly inside hidden mode expires without a future toast.
    wedges = 5;
    for (int frame = 0; frame < 14; ++frame) hud.UpdateElement();
    hud.Draw();
    assert(healthImages == 21);
    std::cout << "PASS MM SM64 health meter lifetime: hidden update, no stale replay, no draw-time aging\n";
}
'''

source = SOURCE.read_text()
actor_class = block(source, source.index("class Sm64CapsHudWindow final"))
health_start = source.index("{", source.index("// SM64 power-meter HP dial"))
health = block(source, health_start)
fixture = prelude.replace("/* PRODUCTION_CLASS */", actor_class)
fixture = fixture.replace("/* PRODUCTION_HEALTH_DRAW */", health) + test
with tempfile.TemporaryDirectory(prefix="mm-sm64-ui-lifetime-") as directory:
    path = Path(directory) / "test.cpp"
    path.write_text(fixture)
    binary = Path(directory) / "test"
    subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", "-g", "-O1", "-fsanitize=address,undefined",
                    "-fno-sanitize-recover=all", "-fno-pie", "-no-pie", str(path), "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True, env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"})
