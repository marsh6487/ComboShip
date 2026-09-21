# MM outdoor weather candidate

This candidate adds opt-in persistent/intermittent rain to the MM side of ComboShip. It preserves the existing SoH stack and the streamed music repair at `cc1346ce`. It does not require a placed weather actor or a scene policy. Native weather overrides are outside this first implementation.

## Controls

Open MM's **Enhancements → Audio Editor → Audio Options → MM Outdoor Weather**. The controls are also registered with menu search. Settings are independent of SoH.

| Setting under `gAudioEditor.MMWeather.` | Default | Behavior |
| --- | --- | --- |
| `Enabled` | Off | Add rain in rain-compatible outdoor rooms during ordinary gameplay. |
| `Mode` | 0 / Persistent | 1 / Intermittent alternates showers and dry intervals. |
| `RainColor` | RGB 150, 255, 255 | Tint added rain without recoloring native precipitation. |
| `Overcast` | On | Compose cloudy textures and darker output colors with MM's day-specific palette. |
| `Thunder` | On | Added lightning and concurrent thunder; an active flash finishes when switched off. |
| `RainVolume` | 100% | Rain loop gain, also affected by master and SFX volumes. |
| `ThunderVolume` | 100% | Thunder gain, including shots already playing. |
| `ThunderFrequency` | 100% | 25–200%; higher values shorten intervals. |

Rain fades over one second of simulation time. Intermittent rain starts after a 2–4 second dry interval, sustains a shower for 8–14 seconds, then waits 20–25 seconds between showers. Pause freezes this progression; rendering FPS does not advance it. Scene/room re-entry starts a new cycle. Native rain, snow, Song of Storms, cutscenes, underwater cameras and native fog/lighting overrides suppress the added weather.

## Implementation boundaries

- Added density never writes `EnvironmentContext.precipitation` or `gWeatherMode`. Native day-two scheduling and soft-soil checks keep their existing inputs.
- Overcast composes final rendered sky/light colors without replacing native sky/light configurations. Added flashes have their own timer and bolt storage; they do not take ownership of native `lightningState`, `gLightningStrike` or bolt slots. Added rain/bolts use a separate presentation PRNG.
- MM's native ambience sequence identifies soundfont 2 instrument 18 at C4 for rain, 19 at A2 for low thunder, and 20 at C4 for lightning. This was verified in [the upstream MM ambience sequence](https://github.com/zeldaret/mm/blob/0365728815ec72317d9234e496fc5faa5089b7d0/assets/audio/sequences/seq_1.prg.seq). The adapter uses MM's own font map and resource loader, selects the appropriate instrument note range, and applies native tuning and pitch.
- PCM is decoded once on demand on the game thread. The mixer owns its samples and has a rain loop plus two thunder voices. It adds saturated stereo output before ComboShip's inactive-game mute and does not start/stop native sequence players.
- Play/environment initialization, room changes and teardown clear voices. Audio shutdown joins the worker, clears decoded samples and forces MM to resolve its own resources on re-entry. Missing or unsupported nature samples produce one diagnostic per load attempt; other audio continues.

## Verification and acceptance

Local diagnostics compile the actual environment/play C translation units and the new C++ controller/audio adapter against MM headers, then exercise the production state machine, decoder and mixer. They cover native-state preservation, timing, controls, ownership, scene/pause reset, ADPCM bounds, sample tuning, looping/resampling, clipping, master/SFX mute, live thunder mute, missing samples and the production final-output mute ordering. Existing streamed-ID and scene-randomization diagnostics remain required. The artifact gate runs these checks before Windows/Linux builds. A fresh code review found two rendering integration gaps: the native-only Play draw gate and shared bolt storage. Regressions reproduce those failures on the earlier revision; the candidate includes the caller fix and private bolt ownership with preemption/reset cleanup.

Full build results and in-game acceptance are separate. This document does not claim a rendered or audible runtime pass.

| Runtime check | Expected | Status |
| --- | --- | --- |
| Day-one South Clock Town and Termina Field | Persistent and intermittent rain, continuing BGM/SFX, all live controls | Pending cor's build test |
| Indoor entry/exit, room reload and underwater camera | No added weather in incompatible views; fresh fade on re-entry | Pending |
| Day-two scheduled rain; winter/spring mountain variants | Native weather and fog retain priority | Pending |
| Song of Storms, weather cutscenes and soft soil | Native visual/audio/gameplay behavior preserved | Pending |
| Final Hours, fanfares and streamed music | Music/SFX continue alongside added weather | Pending |
| Pause, save/reset and repeated OoT↔MM switches | No stale weather or voices; settings remain MM-specific | Pending |

## Bundled item compatibility

At cor's request, the build candidate also includes the MM custom magic-pot and heart-model cosmetic compatibility patch from `poc/mm-custom-item-colors-20260921`, based on the same audio-fixed `cc1346ce` baseline. It applies the current `HUD.Magic` or `HUD.Hearts` color to custom item bodies during held-item, pickup, boss-heart and Double Defense drawing. It resolves the first Alt display list before checking custom ownership, preserves heart borders and native models, and clears the added tint after the body draw. No model or texture archive is changed by this code bundle.

`python3 scripts/diagnostics/run_mm_custom_item_color_tests.py` covers 62 emitted-draw-state cases, the affected production C translation units and the Double Defense C++ bridge. It joins the existing weather/audio/shared-item checks in the build gate. Custom pack appearance, live color edits and reset behavior remain pending in-game acceptance alongside the weather matrix above. The separately requested Djipi font O2R is not a code dependency.

Keep this as a combined build POC. Do not promote to the sacred baseline until the build gate and relevant runtime checks are accepted.
