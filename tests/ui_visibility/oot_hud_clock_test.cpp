/* Production visual-clock functions, with the host context/window/ImGui clock
 * and health/network services as controlled external boundaries. No renderer.
 */
#include "tests/test_require.h"
#include "z64.h"
#include <cstring>
#include <memory>
static bool hidden = true, marioEnabled = true, menuVisible,
            contextReady = true;
static int wedges = 8;
static unsigned healthReads;
PlayState *gPlayState;
namespace Ship {
struct Gui {
  virtual ~Gui() = default;
};
struct Window {
  std::shared_ptr<Gui> GetGui();
};
struct Context {
  static Context *GetRawInstance() {
    static Context context;
    return &context;
  }
  Window *GetWindow() {
    static Window window;
    return &window;
  }
};
} // namespace Ship
namespace Fast {
struct Fast3dGui : Ship::Gui {
  bool GetMenuOrMenubarVisible() { return menuVisible; }
};
} // namespace Fast
std::shared_ptr<Ship::Gui> Ship::Window::GetGui() {
  static auto gui = std::make_shared<Fast::Fast3dGui>();
  return gui;
}
namespace ImGui {
struct IO {
  float DeltaTime = 0.1f;
};
void *GetCurrentContext() { return contextReady ? (void *)1 : nullptr; }
IO &GetIO() {
  static IO io;
  return io;
}
} // namespace ImGui
int CVarGetInteger(const char *name, int fallback) {
  return std::strcmp(name, "gSm64Mario") == 0 ? marioEnabled : fallback;
}
int Sm64Mario_GetHealthWedges() {
  ++healthReads;
  return wedges;
}
class Sm64CapsHudWindow {
public:
  int previousWedges = -1;
  float showTimeLeft = 0;
  bool healthInitialized = false;
  void UpdateElement();
};
/* PRODUCTION_SM64_UPDATE */
constexpr int HARPOON_MODE_PROP_HUNT = 2;
struct Harpoon {
  static Harpoon *Instance;
  bool isConnected = true;
  int activeGameMode = 0;
};
Harpoon *Harpoon::Instance;
namespace HarpoonTriforceThief {
struct HudState {
  int preRoundCountdownFrames = 0;
  int goFlashFrames = 0;
} sLocal;
static bool loaded = true, selecting, inRound = true;
bool IsLoaded() { return loaded; }
bool IsInMapSelect() { return selecting; }
bool IsInRound() { return inRound; }
/* PRODUCTION_TRIFORCE_TICK */
} // namespace HarpoonTriforceThief
namespace HarpoonHud {
struct Window {
  void UpdateElement();
};
/* PRODUCTION_HARPOON_UPDATE */
} // namespace HarpoonHud
int main() {
  static PlayState play;
  gPlayState = &play;
  Sm64CapsHudWindow sm64;
  sm64.UpdateElement();
  REQUIRE(sm64.healthInitialized && sm64.previousWedges == 8 &&
          sm64.showTimeLeft == 0);
  wedges = 6;
  sm64.UpdateElement();
  REQUIRE(sm64.previousWedges == 6 && sm64.showTimeLeft > 2.89f &&
          sm64.showTimeLeft < 2.91f);
  for (unsigned frame = 0; frame < 30; ++frame)
    sm64.UpdateElement();
  REQUIRE(sm64.showTimeLeft <= 0);
  hidden = false; /* Revealing UI after hidden time cannot replay an expired HP
                     notification. */
  sm64.UpdateElement();
  REQUIRE(sm64.showTimeLeft <= 0);
  wedges = 7;
  sm64.UpdateElement();
  float timeLeft = sm64.showTimeLeft;
  play.pauseCtx.state = 6;
  sm64.UpdateElement();
  REQUIRE(sm64.showTimeLeft == timeLeft);
  play.pauseCtx.state = 0;
  menuVisible = true;
  sm64.UpdateElement();
  REQUIRE(sm64.showTimeLeft == timeLeft);
  menuVisible = false;
  marioEnabled = false;
  unsigned before = healthReads;
  sm64.UpdateElement();
  REQUIRE(healthReads == before && sm64.showTimeLeft == timeLeft);
  Harpoon connection;
  Harpoon::Instance = &connection;
  HarpoonHud::Window harpoon;
  HarpoonTriforceThief::sLocal.goFlashFrames = 20;
  hidden = true;
  for (unsigned frame = 0; frame < 20; ++frame)
    harpoon.UpdateElement();
  REQUIRE(HarpoonTriforceThief::sLocal.goFlashFrames == 0);
  harpoon.UpdateElement();
  REQUIRE(HarpoonTriforceThief::sLocal.goFlashFrames == 0);
  HarpoonTriforceThief::sLocal.goFlashFrames = 10;
  HarpoonTriforceThief::sLocal.preRoundCountdownFrames = 1;
  harpoon.UpdateElement();
  REQUIRE(HarpoonTriforceThief::sLocal.goFlashFrames == 10);
  HarpoonTriforceThief::sLocal.preRoundCountdownFrames = 0;
  connection.isConnected = false;
  harpoon.UpdateElement();
  REQUIRE(HarpoonTriforceThief::sLocal.goFlashFrames == 10);
  connection.isConnected = true;
  connection.activeGameMode = HARPOON_MODE_PROP_HUNT;
  harpoon.UpdateElement();
  REQUIRE(HarpoonTriforceThief::sLocal.goFlashFrames == 10);
  connection.activeGameMode = 0;
  HarpoonTriforceThief::inRound = false;
  harpoon.UpdateElement();
  REQUIRE(HarpoonTriforceThief::sLocal.goFlashFrames == 10);
  HarpoonTriforceThief::selecting = true;
  harpoon.UpdateElement();
  REQUIRE(HarpoonTriforceThief::sLocal.goFlashFrames == 9);
  puts("PASS OoT HUD clocks: hidden HP/GO notifications expire through GUI "
       "updates; mode/pause/menu gates preserved");
}
