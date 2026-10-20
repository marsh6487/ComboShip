# Elemental sword UI, independent cheats, and finish presentation

Baseline: published PR45 source `613802447cf8a4fbc3e058ba084f9e3dd7b9f7ae`.
Candidate branch: `poc/elemental-sword-ui-deaths-20261020`.

cor approved the design in chat on October 20, 2026, then clarified that
an element-immune enemy must still take the sword strike's normal damage.
The supplied selector screenshot also establishes that new layout and smooth
typography are required in addition to replacing the arrow icons. A second
screenshot reports incomplete Biggoron frost casing; its diagnosis is included.

## Preservation and scope

- Fire/Ice/Light arrow acquisition alone unlocks the corresponding normal
  A-button equipment choice. Each sword retains its saved element and explicit
  None, including progressive inheritance. The existing save layout is unchanged.
- Existing damage checkbox identities/locations become independent cheat
  overrides, usable without arrows, Din resources, or Alternate Assets. Enabling
  one clears the others. Disabling it restores the saved normal choice. Legacy
  preferences containing several enabled flags resolve deterministically to
  Light, then Ice, then Fire, without stacking. Any subsequent checkbox toggle
  clears the other override flags, so turning the active cheat off cannot reveal
  a hidden legacy cheat.
- Normal sword damage, multipliers, cutting, drops, soul gates and zero-magic
  swings remain authoritative, including when the elemental row is immune/inert.
- Selector typography, layout and dedicated fire/ice/light artwork are replaced.
  The same artwork marks the effective B-button sword enchantment.
- Native enemy finish effects are reused where available. Added presentation
  must never delay actor destruction, progression, or cleanup. Exact body
  encasement before an actor's immediate disappearance cannot be promised by a
  global health hook; copied effect anchors must not retain freed actor pointers.
- Concurrent native forest and transformation-camera candidates are preserved
  separately, not silently included in this candidate.

## Evidence so far

The access regression failed on the unchanged core at arrow ownership with
checkboxes off. The override-menu regression failed with multiple effects still
active. After implementation, all four real-header core/combat variants pass
(solo OoT/MM and Combo OoT/MM) with address/undefined-behavior sanitizer checks.
The actual checkbox helper passes last-enabled/off/unrelated-option controls,
including turning off preferences containing several historical enabled flags.
The first-arrow receipt regression failed with the switches off, then the real
native/custom pickup/queue/decoder fixtures pass after decoupling tutorials from
cheat switches. Both hosts' existing save roundtrip/malformed-data tests pass.

Environment: dependencies come from the earlier native-camera dependency cache
using CPLUS_INCLUDE_PATH; the first broad baseline command stopped because its
historical nlohmann include path no longer exists. No production failure was
inferred from that missing dependency. Leak sanitizer is disabled for this
container; address and undefined-behavior checks remain enabled.

UI, kill presentation, Biggoron correction and combined verification are still
in progress at this checkpoint. No configured gameplay, audio, Windows/full
application build, user acceptance or master promotion is claimed.
