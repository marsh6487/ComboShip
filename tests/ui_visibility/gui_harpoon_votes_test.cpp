// Actual map-select class methods. Network sends/round starts are boundaries;
// headless ImGui executes the retained Draw path to catch duplicate input work.
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <imgui.h>
#include <imgui_internal.h>
#include <iostream>
#include <map>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

using s32 = int32_t;
using s8 = int8_t;
enum HarpoonGameState { HARPOON_STATE_LOBBY, HARPOON_STATE_MAP_SELECT };
enum HarpoonMapSelectMode {
  MAP_SELECT_HOST_CHOOSES,
  MAP_SELECT_EVERYONE_CHOOSES,
  MAP_SELECT_RANDOM
};
constexpr unsigned BTN_A = 0x8000;
#define CHECK_BTN_ALL(value, mask) (((value) & (mask)) == (mask))
struct Input {
  struct {
    s8 stick_x = 0, stick_y = 0;
  } cur;
  struct {
    unsigned button = 0;
  } press;
};
struct PlayState {
  struct {
    Input input[1];
  } state;
};
static PlayState play;
static PlayState *gPlayState = &play;

struct Client {
  bool online = true, self = true, hasVoted = false;
  s32 mapSelectIndex = 0;
  struct {
    uint8_t r = 255, g = 255, b = 255;
  } color;
};
struct Harpoon {
  static inline std::shared_ptr<Harpoon> Instance;
  bool isConnected = true;
  HarpoonGameState gameState = HARPOON_STATE_MAP_SELECT;
  std::string currentRoomGameMode = TEST_MODE;
  uint32_t ownClientId = 1, hostClientId = 2;
  s32 selectedMapIndex = 0;
  HarpoonMapSelectMode mapSelectMode = MAP_SELECT_EVERYONE_CHOOSES;
  std::map<uint32_t, Client> clients = {{1, {}}};
  std::vector<nlohmann::json> sent;
  void SendJsonToRemote(nlohmann::json packet) {
    sent.push_back(std::move(packet));
  }
};
static unsigned starts;
static s32 startMap = -1;
namespace HarpoonPropHunt {
void HostStartRound(s32 index) {
  ++starts;
  startMap = index;
}
} // namespace HarpoonPropHunt
namespace HarpoonTriforceThief {
void HostConfirmMap(s32 index) {
  ++starts;
  startMap = index;
}
} // namespace HarpoonTriforceThief
namespace Ship {
class GuiWindow {
public:
  virtual ~GuiWindow() = default;
  virtual void InitElement() = 0;
  virtual void DrawElement() = 0;
  virtual void UpdateElement() = 0;
  virtual void Draw() = 0;
};
struct Gui {
  virtual ~Gui() = default;
};
struct Window {
  std::shared_ptr<Gui> gui;
  auto GetGui() { return gui; }
};
struct Context {
  std::shared_ptr<Window> window = std::make_shared<Window>();
  static Context *GetRawInstance() {
    static Context context;
    return &context;
  }
  auto GetWindow() { return window; }
};
} // namespace Ship
namespace Fast {
struct Fast3dGui : Ship::Gui {
  ImTextureID GetTextureByName(const char *) { return (ImTextureID)0; }
};
} // namespace Fast
void LoadTextures() { assert(false); }
void LoadTTTextures() {
  assert(false);
}

/* PRODUCTION_GLOBALS */
/* PRODUCTION_CLASS */;

static void draw(MapSelectWindow &window) {
  ImGui::NewFrame();
  window.Draw();
  ImGui::Render();
}

int main() {
  ImGui::CreateContext();
  auto &io = ImGui::GetIO();
  io.DisplaySize = ImVec2(640, 480);
  io.DeltaTime = 1.0f / 60.0f;
  io.IniFilename = nullptr;
  io.Fonts->Build();
  Ship::Context::GetRawInstance()->window->gui =
      std::make_shared<Fast::Fast3dGui>();
  Harpoon::Instance = std::make_shared<Harpoon>();
  auto harpoon = Harpoon::Instance;
  MapSelectWindow window;
  TexturesLoaded = true;
  // First-entry reset still runs even if all Draw calls are suppressed.
  HasVoted = true;
  play.state.input[0].cur.stick_x = 50;
  window.UpdateElement();
  assert(!HasVoted && harpoon->selectedMapIndex == 1 && StickDebounce == 9);
  assert(harpoon->sent.size() == 1 &&
         harpoon->sent.back()["event_name"] == TEST_CURSOR);
  assert(harpoon->sent.back()["type"] == "ROOM.BROADCAST_EVENT" &&
         harpoon->sent.back()["data"]["mapIndex"] == 1);
  // Revealing the UI must render without another stick tick or cursor packet.
  draw(window);
  draw(window);
  assert(StickDebounce == 9 && harpoon->selectedMapIndex == 1 &&
         harpoon->sent.size() == 1);
  for (int tick = 0; tick < 8; ++tick)
    window.UpdateElement();
  assert(StickDebounce == 1 && harpoon->selectedMapIndex == 1 &&
         harpoon->sent.size() == 1);
  window.UpdateElement();
  assert(StickDebounce == 9 && harpoon->selectedMapIndex == 2 &&
         harpoon->sent.size() == 2);
  // Peer A-alone vote works while hidden, updates local voting state once.
  play.state.input[0].cur.stick_x = 0;
  play.state.input[0].press.button = BTN_A;
  window.UpdateElement();
  assert(HasVoted && harpoon->sent.size() == 3 &&
         harpoon->sent.back()["event_name"] == TEST_VOTE);
  assert(harpoon->sent.back()["data"]["mapIndex"] == 2);
  assert(harpoon->clients.at(1).hasVoted &&
         harpoon->clients.at(1).mapSelectIndex == 2);
  draw(window);
  window.UpdateElement();
  draw(window);
  assert(HasVoted && harpoon->sent.size() == 3); // No draw reset/double vote.
  // Keep the existing authority gates: peers spectate HOST_CHOOSES.
  HasVoted = false;
  StickDebounce = 0;
  harpoon->mapSelectMode = MAP_SELECT_HOST_CHOOSES;
  play.state.input[0].cur.stick_x = 50;
  window.UpdateElement();
  assert(harpoon->selectedMapIndex == 2 && harpoon->sent.size() == 3 &&
         starts == 0);
  harpoon->ownClientId = harpoon->hostClientId;
  play.state.input[0].cur.stick_x = 0;
  window.UpdateElement();
  assert(starts == 1 && startMap == 2);
  draw(window);
  draw(window);
  assert(starts == 1); // Host confirm also belongs only to Update.
  // Disconnected, wrong phase, wrong room mode and missing play state remain
  // inert.
  harpoon->isConnected = false;
  window.UpdateElement();
  harpoon->isConnected = true;
  harpoon->gameState = HARPOON_STATE_LOBBY;
  window.UpdateElement();
  harpoon->gameState = HARPOON_STATE_MAP_SELECT;
  harpoon->currentRoomGameMode = "another_mode";
  window.UpdateElement();
  harpoon->currentRoomGameMode = TEST_MODE;
  gPlayState = nullptr;
  window.UpdateElement();
  assert(starts == 1 && harpoon->sent.size() == 3);
  Harpoon::Instance.reset();
  window.UpdateElement();
  Ship::Context::GetRawInstance()->window->gui.reset();
  ImGui::DestroyContext();
  std::cout << "PASS " << TEST_MODE
            << " hidden vote input: navigation, debounce, A vote/confirm, "
               "guards, no draw double-tick\n";
}
