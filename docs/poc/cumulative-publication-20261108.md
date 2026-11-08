# November 8 cumulative publication candidate

Authorized by cor for public candidate publication, draft PR and necessary integration/CI repairs; no merge or accepted-master promotion.

## Provenance and preservation

Restore POC6 archive `ComboShip_Cumulative_Action_Pool_Fix_POC6_Source_20261108.zip` (SHA-256 `14ed6f3f62627a2c3e18530bf70dbb31b60d73268c914ba097f5d3ac0dfa455d`) over public PR87 head `1415eecde372a5bcf3f4b8b9d650d77f96e3afd9`. The checkpoint installer accepted all 98 paths before writing; no source conflict was overridden. Both previous CI fixture repairs remain inherited. CI-covered production files are formatted with clang-format 14.0.6. New asset-independent regressions are registered in the existing required groups without dropping prior tasks.

## Included source

- Both-game True Hidden UI control ownership and visible startup/navigation screens.
- OoT Debug Warp night-music eligibility.
- OoT first shovel handoff and MM hidden-grotto type/collider handling.
- Both-game snow walking sounds and frozen-water sound/ripple behavior; MM saved Winter ownership.
- Lost Woods time-pedestal Story Skip parity, retaining equipment, camera and local return.
- MM invisible worn/persistent masks and Couple health/magic recovery, plus shared new-option accent.
- MM fractional/native magic and hallmark upgrade hint selectors.
- MM age-specific optional Marin voices, native OoT adult fallback, native custom-item voice routing and PCM mixer.
- OoT native Deku/FD blinking and compatible optional banks.
- Material-identity Winter ground/wall whitening in Death Mountain Trail, Gerudo Valley and Gerudo Fortress, with protected mixed atlases.
- Disabled Open Chests pool leak correction retaining enabled/progressive/Climb settings.

## Fresh local checks

Passed: 25 CI policy tests; action-pool sanitizer regression (720 fills and 24 native abundance/action cases); native MM mask/recovery and magic hint suites; 20 shovel/Winter feedback configurations; 736 native UI control dispatches; pedestal and sword-source suites; native blink helper/pixels; Winter terrain sanitizer suite including 309 material cohorts and 256 renderer boundaries; age-voice mixer sanitizer/thread stress; whitespace checks.

These are CPU/source/fixture checks. Asset-dependent audio/listening and native texture appearance proof from earlier checkpoints are not rerun here. Complete local application configuration is blocked because CMake is absent; no local complete compilation/link or configured game execution is claimed. Platform CI provides a separate build/regression gate.

## Required runtime checks and exclusions

Rebuild the executable and all engine-sharing modules together: transient Texture layout changes. Test both ages and optional voice-pack presence/absence, UI inventory paging/startup, shovel first use, frozen temple pools, pedestal settings, mask effects and all three Winter scenes. Unsupported custom eye layouts/new terrain identities need their own classification/banks. Native OoT adult provider preserves donor recordings but does not emulate exact native sequence envelopes/rests. Solo MM needs mounted native OoT companion resources for this adult fallback.

Severe Black Market slowdown and camera regression remain open. No additional daytime hint-music repair or legacy MM mask-menu cleanup is claimed. Asset-only models, hands, animations, texture/icon packs, optional Marin recordings, saves, gameplay logs and credentials are excluded from publication. Existing seeds and asset packs are not rewritten.
