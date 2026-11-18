#include <cassert>
#include <cstdint>
#include <cstdio>
#include <initializer_list>
using s16 = int16_t;
using s32 = int32_t;
/* NATIVE_ENUMS */
struct InterfaceContext {
  int magicConsumptionTimer{}, unk_230{};
};
struct PlayState {
  InterfaceContext interfaceCtx;
  struct {
    bool lensActive{};
  } actorCtx;
} play;
struct {
  bool isMagicAcquired{};
  int magic{}, magicCapacity{}, magicState{}, magicTarget{}, magicToConsume{};
  struct {
    struct {
      struct {
        bool isMagicAcquired{};
        int magic{};
      } playerData;
    } saveInfo;
  } save;
} gSaveContext;
bool chateau = false, permanent = false;
int errors = 0;
#define CHECK_WEEKEVENTREG(x) (chateau)
#define WEEKEVENTREG_DRANK_CHATEAU_ROMANI 1
#define RAND_INF_HAS_INFINITE_MAGIC_METER 1
#define R_MAGIC_CONSUME_TIMER_GIANTS_MASK 10
constexpr int NA_SE_SY_ERROR = 0;
int gSfxDefaultPos, gSfxDefaultFreqAndVolScale, gSfxDefaultReverb;
bool Flags_GetRandomizerInf(int) { return chateau || permanent; }
bool RandoCapacity_InfiniteMagic() { return permanent; }
template <class... T> void Audio_PlaySfx(T...) { ++errors; }
template <class... T> void Audio_PlaySfxGeneral(T...) { ++errors; }
/* NATIVE_REQUEST */
int Request(int amount, int type) {
#ifdef HOST_MM
  return Magic_Consume(&play, amount, type);
#else
  return Magic_RequestChange(&play, amount, type);
#endif
}
void Reset(int capacity, int current, bool owned) {
  gSaveContext = {};
  gSaveContext.magicCapacity = capacity;
  gSaveContext.magic = current;
  gSaveContext.save.saveInfo.playerData.magic = current;
  gSaveContext.isMagicAcquired =
      gSaveContext.save.saveInfo.playerData.isMagicAcquired = owned;
  gSaveContext.magicState = MAGIC_STATE_IDLE;
  errors = 0;
  play = {};
}
int main() {
  for (bool permanentPolicy : {false, true})
    for (int current : {0, 12}) {
      chateau = !permanentPolicy;
      permanent = permanentPolicy;
      for (int type : {/* REQUEST_TYPES */}) {
        Reset(12, current, true);
        assert(Request(24, type));
        assert(errors == 0);
        assert(gSaveContext.magic == current &&
               gSaveContext.save.saveInfo.playerData.magic == current);
        assert(gSaveContext.magicToConsume == 0);
#ifndef HOST_MM
        assert(gSaveContext.magicTarget >= 0);
#endif
      }
      Reset(12, current, true);
      gSaveContext.magicState = MAGIC_STATE_FILL;
      assert(!Request(24, MAGIC_CONSUME_NOW) && errors == 1);
      assert(gSaveContext.magicState == MAGIC_STATE_FILL);
      Reset(0, 0, false);
      assert(!Request(24, MAGIC_CONSUME_NOW) && errors == 0);
    }
  chateau = permanent = false;
  Reset(12, 12, true);
  assert(!Request(24, MAGIC_CONSUME_NOW) && errors == 1);
  chateau = false;
  Reset(48, 48, true);
  assert(Request(24, MAGIC_CONSUME_NOW));
#ifdef HOST_MM
  assert(gSaveContext.magicToConsume == 24);
#else
  assert(gSaveContext.magicTarget == 24);
  chateau = true;
  Reset(48, 12, true);
  assert(Request(8, MAGIC_ADD) && gSaveContext.magicTarget == 20);
#endif
  std::puts(
      "PASS native Chateau/infinite admission with zero/fractional current "
      "magic, owned-meter and ordinary/refill behavior preserved");
}
