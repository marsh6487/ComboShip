/* Remake-only, per-host audio bridge. All world/context calls run on the game
 * thread; MixInto consumes values and immutable PCM on the audio thread. */
#ifndef COMBO_AUDIO_DINS_FIRE_SFX_H
#define COMBO_AUDIO_DINS_FIRE_SFX_H
#include <stdint.h>

struct PlayState;
struct Actor;

#if defined(MM_BUILD_DLL) || defined(CW_DINS_HOST_MM)
#define DinsFireSfx_Begin MM_DinsFireSfx_Begin
#define DinsFireSfx_Attach MM_DinsFireSfx_Attach
#define DinsFireSfx_Contact MM_DinsFireSfx_Contact
#define DinsFireSfx_Release MM_DinsFireSfx_Release
#define DinsFireSfx_UpdateActor MM_DinsFireSfx_UpdateActor
#define DinsFireSfx_CancelCast MM_DinsFireSfx_CancelCast
#define DinsFireSfx_Stop MM_DinsFireSfx_Stop
#define DinsFireSfx_UpdateContext MM_DinsFireSfx_UpdateContext
#define DinsFireSfx_MixInto MM_DinsFireSfx_MixInto
#else
#define DinsFireSfx_Begin OOT_DinsFireSfx_Begin
#define DinsFireSfx_Attach OOT_DinsFireSfx_Attach
#define DinsFireSfx_Contact OOT_DinsFireSfx_Contact
#define DinsFireSfx_Release OOT_DinsFireSfx_Release
#define DinsFireSfx_UpdateActor OOT_DinsFireSfx_UpdateActor
#define DinsFireSfx_CancelCast OOT_DinsFireSfx_CancelCast
#define DinsFireSfx_Stop OOT_DinsFireSfx_Stop
#define DinsFireSfx_UpdateContext OOT_DinsFireSfx_UpdateContext
#define DinsFireSfx_MixInto OOT_DinsFireSfx_MixInto
#endif

#ifdef __cplusplus
extern "C" {
#endif
/* Nonzero generation handle; zero is always a silent no-op. */
uint32_t DinsFireSfx_Begin(struct PlayState* play, float x, float y, float z);
/* Only an unbound charge in the same play context can attach a new actor. */
uint32_t DinsFireSfx_Attach(struct PlayState* play, const struct Actor* actor);
void DinsFireSfx_Contact(uint32_t handle);
void DinsFireSfx_Release(uint32_t handle);
/* Negative fireFrame updates the pending anchor without starting fire audio. */
void DinsFireSfx_UpdateActor(uint32_t handle, float x, float y, float z, int fireFrame);
/* Action changes cancel only a pre-release charge; a released actor owns it. */
void DinsFireSfx_CancelCast(uint32_t handle);
void DinsFireSfx_Stop(uint32_t handle);
/* Called even on paused Play_Update frames, before actor-update gating. */
void DinsFireSfx_UpdateContext(struct PlayState* play);
/* Interleaved stereo s16 at 32000 Hz; frames is not the scalar sample count. */
void DinsFireSfx_MixInto(int16_t* output, uint32_t frames);
#ifdef __cplusplus
}
#endif
#endif
