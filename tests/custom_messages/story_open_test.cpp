// Actual story registration, GameInteractor registry, formatter and native
// message open/decode. Graphics, audio and unrelated receipt services are the
// boundaries; no game or save is run.
#include "global.h"
#include "message_data_static.h"
#include "message_data_fmt.h"
#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include "soh/Enhancements/game-interactor/GameInteractor_Hooks.h"
#include "soh/ShipInit.hpp"
#include "soh/util.h"
#include "soh/z_message_OTR.h"
extern "C" {
#include "src/overlays/actors/ovl_En_Viewer/static_story_dialogue.h"
void Message_OpenText(PlayState *, u16);
void Message_Decode(PlayState *);
void Message_FindMessage(PlayState *, u16);
void Message_FindMessageJPN(PlayState *, u16);
void Message_FindCreditsMessage(PlayState *, u16);
}
#include <cassert>
#include <cstdio>
#include <cstring>
#include <string>
#include <array>
#include <deque>
#include <unordered_map>
#include <vector>
#include "soh/resource/type/Text.h"

SaveContext gSaveContext{};
u32 gBitFlags[32]{};
u8 gEquipShifts[4]{};
static GameInfo registers{};
GameInfo *gGameInfo = &registers;
static PlayState play;
PlayState *gPlayState = &play;
static GraphicsContext graphics;
static Gfx commands[512];
static char textbox[4096];
static GameInteractor interactor;
GameInteractor *GameInteractor::Instance = &interactor;
static const char vanilla[] = "Native table.\x02";
static const char legacyCredits[] = "Native credits.\x02";
static const u16 legacyJapanese[] = {MESSAGE_END_JPN};
static MessageTableEntry table[] = {
    {0x1234, 0x02, vanilla, sizeof(vanilla) - 1},
    {0x0500, 0x02, legacyCredits, sizeof(legacyCredits) - 1},
    {0xFFFF, 0x02, vanilla + sizeof(vanilla) - 1, 0},
};
static MessageTableEntry japaneseTable[] = {
    {0x1234, 0x02, reinterpret_cast<const char*>(legacyJapanese), sizeof(legacyJapanese)},
    {0xFFFF, 0, nullptr, 0},
};
/* NATIVE_UNBOUND_MESSAGES */
extern "C" {
MessageTableEntry *sNesMessageEntryTablePtr = table;
MessageTableEntry *sGerMessageEntryTablePtr = table;
MessageTableEntry *sFraMessageEntryTablePtr = table;
MessageTableEntry *sJpnMessageEntryTablePtr = japaneseTable;
MessageTableEntry *sStaffMessageEntryTablePtr = table;
void Interface_ChangeHudVisibilityMode(u16) {}
void Interface_SetDoAction(PlayState *, u16) {}
s32 Flags_GetEventChkInf(s32) { return 0; }
size_t GetEquipNowMessage(char *, char *, size_t) {
  assert(false);
  return 0;
}
int32_t CVarGetInteger(const char *, int32_t fallback) { return fallback; }
bool GameInteractor_Should(GIVanillaBehavior, uint32_t result, ...) {
  return result;
}
}
#ifdef COMBO_BUILD
// ShipInit's RM guard is outside the message/registration boundary.
Ship::OwnRMScope::OwnRMScope(const char *) {}
Ship::OwnRMScope::~OwnRMScope() = default;
#endif

/* NATIVE_COPY */
extern "C" {
/* NATIVE_OPEN_HOOK */
}

static void ResetMessage() {
  play.msgCtx = {};
  play.msgCtx.textboxSegment = textbox;
  graphics.polyOpa.p = commands;
  play.state.gfxCtx = &graphics;
}

static std::string Decode() {
  graphics.polyOpa.p = commands;
  Message_Decode(&play);
  const auto &context = play.msgCtx;
  assert(context.decodedTextLen < ARRAY_COUNT(context.msgBufDecoded));
  assert(context.msgBufDecoded[context.decodedTextLen] == '\x02');
  std::string body;
  for (unsigned i = 0; i < context.decodedTextLen; ++i) {
    const char c = context.msgBufDecoded[i];
    body += c == '\x01' ? ' ' : c;
  }
  return body;
}

static void CheckCustom(u16 id, const char* expectedText = nullptr) {
  ResetMessage();
  bool fromTable = true;
  GameInteractor_ExecuteOnOpenText(&id, &fromTable);
  if (fromTable) {
    std::fprintf(stderr,
                 "FAIL registered story text 0x%04X falls through to native "
                 "message table\n",
                 id);
    std::abort();
  }
  // Exercise the entire native opening path with poisoned table metadata.
  // A missing hook must never reach its absent-ID/null-offset fallback.
  play.msgCtx.font.msgOffset = 0;
  play.msgCtx.font.msgLength = 777;
  play.msgCtx.msgLength = 777;
  graphics.polyOpa.p = commands;
  Message_StartTextbox(&play, id, nullptr);
  assert(play.msgCtx.textId == id && play.msgCtx.msgMode == MSGMODE_TEXT_START);
  assert(play.msgCtx.msgLength > 0 &&
         play.msgCtx.msgLength < sizeof(play.msgCtx.font.msgBuf));
  assert(play.msgCtx.font.msgLength == play.msgCtx.msgLength);
  const std::string expected =
      expectedText != nullptr ? expectedText : StaticStoryDialogue_GetText(id, gSaveContext.language);
  assert(Decode() == expected);
  Message_CloseTextbox(&play);
  assert(play.msgCtx.msgMode == MSGMODE_TEXT_CLOSING &&
         play.msgCtx.stateTimer == 2);
  graphics.polyOpa.p = commands;
  Message_ContinueTextbox(&play, id);
  assert(play.msgCtx.msgMode == MSGMODE_TEXT_CONTINUING &&
         play.msgCtx.stateTimer == 3);
  assert(Decode() == expected);
}

static void CheckEmptyMessage(bool japanese) {
  const auto& font = play.msgCtx.font;
  assert(font.msgOffset);
  if (japanese) {
    u16 token;
    assert(font.msgLength == sizeof(token));
    std::memcpy(&token, reinterpret_cast<const void*>(font.msgOffset), sizeof(token));
    assert(token == MESSAGE_END_JPN);
  } else {
    assert(font.msgLength == 1 && *reinterpret_cast<const u8*>(font.msgOffset) == MESSAGE_END);
  }
}

static void CheckEmptyCopy(bool japanese) {
  if (japanese)
    assert(play.msgCtx.font.msgLength == sizeof(u16) && play.msgCtx.font.msgBufWide[0] == MESSAGE_END_JPN);
  else
    assert(play.msgCtx.font.msgLength == 1 && static_cast<u8>(play.msgCtx.font.msgBuf[0]) == MESSAGE_END);
}

static void CheckLegacyReaders() {
  struct Reader {
    const char* name;
    void (*find)(PlayState*, u16);
    u16 id;
    const char* text;
    u32 size;
    u8 language;
  };
  const Reader readers[] = {
      {"normal", Message_FindMessage, 0x1234, vanilla, sizeof(vanilla) - 1, LANGUAGE_ENG},
      {"JPN", Message_FindMessageJPN, 0x1234, reinterpret_cast<const char*>(legacyJapanese), sizeof(legacyJapanese), LANGUAGE_JPN},
      {"credits", Message_FindCreditsMessage, 0x0500, legacyCredits, sizeof(legacyCredits) - 1, LANGUAGE_ENG},
  };
  bool foundAll = true;
  for (const auto& reader : readers) {
    ResetMessage();
    gSaveContext.language = reader.language;
    play.msgCtx.font.msgOffset = 0xBAD;
    play.msgCtx.font.msgLength = 777;
    reader.find(&play, reader.id);
    const bool found = play.msgCtx.font.msgOffset == reinterpret_cast<uintptr_t>(reader.text) &&
                       play.msgCtx.font.msgLength == reader.size;
    if (!found)
      std::fprintf(stderr, "FAIL legacy %s Find: offset=%zx length=%u\n", reader.name,
                   size_t(play.msgCtx.font.msgOffset), play.msgCtx.font.msgLength);
    foundAll &= found;
  }
  assert(foundAll && "all native readers must locate entries in an unregistered legacy table");
  for (const auto& reader : readers) {
    ResetMessage();
    gSaveContext.language = reader.language;
    Message_OpenText(&play, reader.id);
    assert(play.msgCtx.font.msgLength == reader.size &&
           std::memcmp(play.msgCtx.font.msgBuf, reader.text, reader.size) == 0);
  }
  for (u8 language : {LANGUAGE_GER, LANGUAGE_FRA}) {
    auto*& selected = language == LANGUAGE_GER ? sGerMessageEntryTablePtr : sFraMessageEntryTablePtr;
    selected = nullptr;
    gSaveContext.language = language;
    Message_FindMessage(&play, 0x1234);
    assert(play.msgCtx.font.msgOffset == reinterpret_cast<uintptr_t>(vanilla));
    selected = table;
  }
  for (const auto& reader : readers) {
    ResetMessage();
    gSaveContext.language = reader.language;
    reader.find(&play, 0x7777);
    if (reader.find == Message_FindCreditsMessage) {
      CheckEmptyMessage(false);
    } else {
      assert(play.msgCtx.font.msgOffset == reinterpret_cast<uintptr_t>(reader.text) &&
             play.msgCtx.font.msgLength == reader.size);
    }
  }
  MessageTableEntry sentinel[] = {{0xFFFF, 0, nullptr, 0}};
  for (auto* absent : {static_cast<MessageTableEntry*>(nullptr), sentinel}) {
    sNesMessageEntryTablePtr = sGerMessageEntryTablePtr = sFraMessageEntryTablePtr =
        sJpnMessageEntryTablePtr = sStaffMessageEntryTablePtr = absent;
    for (const auto& reader : readers) {
      ResetMessage();
      gSaveContext.language = reader.language;
      reader.find(&play, reader.id);
      const bool japanese = reader.find == Message_FindMessageJPN;
      CheckEmptyMessage(japanese);
      Message_OpenText(&play, reader.id);
      CheckEmptyCopy(japanese);
    }
  }
  sNesMessageEntryTablePtr = sGerMessageEntryTablePtr = sFraMessageEntryTablePtr =
      sStaffMessageEntryTablePtr = table;
  sJpnMessageEntryTablePtr = japaneseTable;
  gSaveContext.language = LANGUAGE_ENG;
  puts("PASS normal/JPN/credits native Find and OpenText: unregistered tables, notfound, PAL fallback, NULL and sentinel tables");
}

int main() {
  ShipInit::InitAll();
  CheckLegacyReaders();
  // Explicit IDs/copy ensure missing Zelda catalog or registration fails decisively.
  for (u8 language : {LANGUAGE_ENG, LANGUAGE_GER, LANGUAGE_FRA, LANGUAGE_JPN}) {
    gSaveContext.language = language;
    CheckCustom(0x8F2B, "Do you hear it? The stones are humming... as if they remember your footsteps.");
    CheckCustom(0x8F2C,
                "Every path leaves an echo in this chamber. Some wake to a song... others to a promise fulfilled.");
    CheckCustom(0x8F2D, "A fairy boy... I thought the forest kept its secrets. Did your fairy guide you here?");
    for (size_t entry = 0; const u16 id = StaticStoryDialogue_GetTextIdAt(entry); ++entry)
      CheckCustom(id);
    bool fromTable = true;
    u16 ordinary = 0x1234;
    GameInteractor_ExecuteOnOpenText(&ordinary, &fromTable);
    assert(fromTable && ordinary == 0x1234);
  }
  // Re-running initialization must retain one delivery per ID.
  ShipInit::InitAll();
  gSaveContext.language = LANGUAGE_ENG;
  for (size_t entry = 0; const u16 id = StaticStoryDialogue_GetTextIdAt(entry); ++entry) {
    CheckCustom(id);
    assert(GameInteractor::RegisteredGameHooks<
               GameInteractor::OnOpenText>::functionsForID[id]
               .size() == 1);
  }
  ResetMessage();
  Message_StartTextbox(&play, 0x1234, nullptr);
  assert(Decode() == "Native table.");
  assert(OTRMessage_Find(table, 0x1234) == nullptr &&
         "unregistered legacy tables must retain the native fallback");
  for (size_t language = 0; language < sTables.size(); ++language) {
    auto& owned = sTables[language];
    owned.Set(0x1234, 0x02, language == MSG_JPN ?
        std::string(reinterpret_cast<const char*>(legacyJapanese), sizeof(legacyJapanese)) : "Owned table.");
    owned.Set(0x0500, 0x02, "Owned credits.");
    owned.Set(0x8F2B, 0x02, "Registry shadow must not replace the story hook.");
    owned.Finalize();
  }
  sNesMessageEntryTablePtr = sTables[MSG_NES].entries.data();
  sGerMessageEntryTablePtr = sTables[MSG_GER].entries.data();
  sFraMessageEntryTablePtr = sTables[MSG_FRA].entries.data();
  sJpnMessageEntryTablePtr = sTables[MSG_JPN].entries.data();
  sStaffMessageEntryTablePtr = sTables[MSG_STAFF].entries.data();
  assert(OTRMessage_Find(sNesMessageEntryTablePtr, 0x9999) == nullptr);
  for (u8 language : {LANGUAGE_ENG, LANGUAGE_GER, LANGUAGE_FRA, LANGUAGE_JPN}) {
    gSaveContext.language = language;
    CheckCustom(0x8F2B, "Do you hear it? The stones are humming... as if they remember your footsteps.");
    ResetMessage();
    if (language == LANGUAGE_JPN) {
      Message_FindMessageJPN(&play, 0x1234);
      const auto* entry = OTRMessage_Find(sJpnMessageEntryTablePtr, 0x1234);
      assert(entry && play.msgCtx.font.msgOffset == reinterpret_cast<uintptr_t>(entry->segment) &&
             play.msgCtx.font.msgLength == entry->msgSize);
      Message_OpenText(&play, 0x1234);
      assert(play.msgCtx.font.msgBufWide[0] == MESSAGE_END_JPN);
    } else {
      Message_StartTextbox(&play, 0x1234, nullptr);
      assert(Decode() == "Owned table.");
    }
  }
  ResetMessage();
  Message_FindCreditsMessage(&play, 0x0500);
  const auto* credits = OTRMessage_Find(sStaffMessageEntryTablePtr, 0x0500);
  assert(credits && play.msgCtx.font.msgOffset == reinterpret_cast<uintptr_t>(credits->segment) &&
         play.msgCtx.font.msgLength == credits->msgSize);
  Message_OpenText(&play, 0x0500);
  assert(std::memcmp(play.msgCtx.font.msgBuf, "Owned credits.\x02", sizeof("Owned credits.\x02") - 1) == 0);
  for (int path = 0; path < 3; ++path) {
    ResetMessage();
    const bool japanese = path == 1, staff = path == 2;
    gSaveContext.language = japanese ? LANGUAGE_JPN : LANGUAGE_ENG;
    auto find = japanese ? Message_FindMessageJPN : staff ? Message_FindCreditsMessage : Message_FindMessage;
    find(&play, 0x7777);
    if (staff)
      CheckEmptyMessage(false);
    else {
      const auto& first = sTables[japanese ? MSG_JPN : MSG_NES].entries[0];
      assert(play.msgCtx.font.msgOffset == reinterpret_cast<uintptr_t>(first.segment) &&
             play.msgCtx.font.msgLength == first.msgSize);
    }
    Message_OpenText(&play, staff ? 0x0501 : 0x7777);
    if (staff)
      CheckEmptyCopy(false);
    else if (japanese)
      assert(play.msgCtx.font.msgBufWide[0] == MESSAGE_END_JPN);
    else
      assert(std::memcmp(play.msgCtx.font.msgBuf, "Owned table.\x02", sizeof("Owned table.\x02") - 1) == 0);
  }
  for (auto& owned : sTables) {
    owned = MessageTable{};
    owned.Finalize();
  }
  sNesMessageEntryTablePtr = sTables[MSG_NES].entries.data();
  sGerMessageEntryTablePtr = sTables[MSG_GER].entries.data();
  sFraMessageEntryTablePtr = sTables[MSG_FRA].entries.data();
  sJpnMessageEntryTablePtr = sTables[MSG_JPN].entries.data();
  sStaffMessageEntryTablePtr = sTables[MSG_STAFF].entries.data();
  for (int path = 0; path < 3; ++path) {
    ResetMessage();
    const bool japanese = path == 1, staff = path == 2;
    gSaveContext.language = japanese ? LANGUAGE_JPN : LANGUAGE_ENG;
    auto find = japanese ? Message_FindMessageJPN : staff ? Message_FindCreditsMessage : Message_FindMessage;
    find(&play, staff ? 0x0500 : 0x1234);
    CheckEmptyMessage(japanese);
    Message_OpenText(&play, staff ? 0x0500 : 0x1234);
    CheckEmptyCopy(japanese);
  }
  puts("PASS owned normal/JPN/credits tables: hits, absent ids and empty finalized tables use native termination encoding");
  puts("PASS story registration -> native opening/decoding/closing/reopening: "
       "plain child/chamber Zelda and all catalog IDs, "
       "four languages, repeat registration, legacy fallback and actual owned-table lookup with story precedence "
       "(ASan/UBSan)");
}
