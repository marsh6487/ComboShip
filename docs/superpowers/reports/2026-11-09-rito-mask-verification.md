# Rito Mask implementation and verification

Date: 2026-11-09. Candidate branch: `candidate/rito-mask-20261109`.
Baseline: local R22 `191794e8c7534c30a55ee4ab4554e49356c6ca75`, tree `643dfc57b26b2a39b50b81a81245e1f76b7aecab`.
Published baseline for independent restoration: PR89 `0063920e18145299afc1682aab98407ebd366fc5`.

## Requested result and final placement

Rito is an optional randomizer reward with an authored 3D NEI get-item model, mandatory shimmer, correct icon/name routing, usable native form in either game, a What's New entry and controls in the updated save editors. It adds no solver capability or world-access requirement.

The final placement follows cor's explicit corrections:

| Mode | Inventory placement |
|---|---|
| Solo OoT | Existing bottom-right corner mask wheel (`SLOT_TRADE_CHILD`) |
| Combo, either native host | Existing bottom-right corner mask wheel |
| Solo MM | Existing Bremen mask cell; cycle between independently owned Bremen and Rito |

There is no added inventory page, bank, grid cell or rearrangement. MM's first item page already maps to the OoT layout; the requested Solo MM placement is an explicit mode-specific choice. All original masks remain independently owned.

## Randomizer, presentation and editor behavior

**Shuffle Rito Mask** is default-off, directly below the native mask shuffle controls. The existing shared Combo preference pattern preserves standalone choices and participates in import, snapshots, presets, generation and live widgets. Enabling it contributes one native solo copy; MM Plentiful does not duplicate it. Production Combo fill merges the native contributions into one shared reward.

The append-only identities are OoT item `0xD2`, MM item `0x0225`, `RG_RITO_MASK=730`, `RI_OOT_NEI_RITO_MASK=607`, `FCI_RITO_MASK=465`, `RSK_SHUFFLE_RITO_MASK=365` and `RO_SHUFFLE_RITO_MASK=100`. MM's existing Magic Mirror retains `0x0224`. Existing enum values and save-field offsets remain stable.

The old Deku Stick fallback came from the central OoT icon resolver missing the Rito entry: the custom asset table already contained Rito's art, but the unresolved ID reached the generic first-icon fallback. The resolver now returns the shipped Rito icon directly. MM ships the same icon and name assets, and its full-width item is handled before native byte-ID fallbacks. Pause, HUD/equip, receipt, tracker and editor routes use the Rito identity.

Both updated save editors expose Rito ownership and selection through the canonical native grant/remove helpers. The OoT picker offers it on the corner cell, with an explicit ownership control and selection action. MM's editor uses the real full-width item and mode-specific slot. Removing a live Rito also exits its form, clears its C/D bindings and preserves other masks. The regular randomizer editor follows the existing local-removal policy: it preserves shared obtained/applied counters, so peer MAX values do not immediately regrant it. Combo Vanilla retains the existing one-way shared-unlock policy; an owning peer can restore a locally removed unlock after a switch. Global revocation was not introduced.

Fixed Combo Vanilla acquisition is retained. Its receipt now uses the real Rito randomizer presentation rather than a Clawshot carrier and a flat icon quad.

## Authored model and resources

Root: `__OTR__objects/nei_gi_redesign/rito_mask/gi_dl`. Eight identical GI resources are included for each native host. The original sculpture contains 768 triangles across 13 closed components: ivory shell with real open eyes, gold surrounds, projecting hooked beak, five crest vanes, brow, concave back and mounting pads. It is not an icon billboard or unrelated mask model.

External scale is 1.0; the native matrix contains the quantized internal 0.2 scale. Front faces +Z and crest +Y. Shared frame bounds match the serialized model. Native and foreign descriptors select the standard gold NEI shimmer explicitly. Standalone MM draws the local resource without requiring an OoT manager or an MM cross-resource registration; the existing foreign resolver adds `@mm:` for a receiver, and rendering honors that owner. The effect is procedural and has no hidden donor-texture dependency.

The deterministic recipe, static previews and resource checks are in `tools/rito_mask_gi`. Native factory and packaging evidence is documented in `tools/rito_mask_gi/NATIVE_VALIDATION.md`.

## Ownership, migration and form lifecycle

New files do not receive Rito by enabling its form preference or opening pause. The native grant is the ownership boundary. Existing ownership is retained. Old OoT saves that displayed Rito in Farore's Wind migrate it to the corner wheel; Farore is restored only when its existing legacy ownership bit establishes that it was owned. Current, child and adult C/D bindings migrate without moving unrelated items.

Solo MM stores Bremen selection independently of either mask's ownership. The existing corner cursor is now serialized and validated, including Rito's ninth entry. Old saves default safely, and malformed/wide/non-numeric selector values do not wrap into another selection.

MM now uses its native custom-form lifecycle for Rito's body, charged launch, free glide, hops, bow shots and airborne volley. Exact optional MHR bow/long-name flight packs and the handless bow extraction referenced by the old OoT donor were not present in the accepted inputs. Compatible native animation fallbacks keep the actions available without resampling shared native clips or applying authored-only root lifts. Native bow poses use the right hand. Solo MM uses its own human bow and string resources; OoT can use its built-in adult bow when the optional extraction is absent.

Scene/file transitions forget departed actor/effect/projectile references before any cleanup can touch their old arena. Editor removal and a newly loaded file use different cleanup paths: removal restores the current player's valid saved properties/equipment, whereas an unavailable Rito arriving from an old scene/file drops old transient state and backups without applying them to the new native player. Pre-flash cancellation releases input/pause flags without restoring uninitialized properties. Regranting ownership does not automatically reactivate the removed form.

## Executed verification

| Check | Result |
|---|---|
| Final targeted regression pass | All 14 commands passed; complete Rito randomizer runner, OoT ownership/animation checks, six MM behavior/save/editor scripts, GI checks, deterministic regeneration, existing Vanilla mode/reward/sharing checks |
| Actual-source ownership and migration | Pass with address/undefined-behavior sanitizers; fresh files, all nine corner masks, legacy spell preservation, all age C/D bindings, editor removal, active/pre-flash cancellation, unavailable old-form arrival and unaffected new-save state |
| MM behavior | Pass; Combo corner/Solo Bremen, full-width ID, independent Bremen, grant/remove/regrant, form activation, launch/glide/release/damage, bow decisions, scene/file poison-pointer cleanup, save conversion and updated editor |
| Pool and sharing | Pass; enabled/disabled native pools, fixed cardinality including MM Plentiful, production Combo dedup, no advancement, native grant/remove and actual shared-applier local-removal behavior |
| Settings/UI/presentation | Pass; actual live widget, default/shared/native preference and snapshots, real native/foreign descriptor routing, existing Mirror compatibility and What's New checks |
| OoT native object compilation | Six affected translation units passed, including the pause umbrella that compiles extended inventory, both form units, NEI save, updated editor and fixed receipt adapter |
| MM native object compilation | Six affected translation units passed, including player umbrella, pause, form registry/controller, adult renderer and updated editor |
| Additional native syntax/guards | Affected randomizer, settings, controls, tracker, sync and glue paths passed; original-game guards checked separately |
| GI geometry/serialization | Four tests passed; component closure/orientation, eye apertures, cache/TMEM/resource bounds and exact host/icon/name parity |
| Actual native resource factories | All 20 packed resources loaded across both hosts, including matrix, geometry, converted icon/name and textures |
| Packaging | Actual available OoT/Torch packer preserved all GI bytes, resolved 86 dependency references, converted exact RGBA32 icon and IA4 name; archive CRCs passed |
| Independent source review | All critical/important/minor findings closed after targeted fixes and independent rechecks |

Sanitizer runs disabled only leak detection because this workspace's ptrace environment prevents LSan. Address and undefined-behavior instrumentation remained enabled.

Native compilation reused the existing CMake compilation database and restored SDK, with source roots/PCH text remapped to the isolated candidate. The plain SDK check of `ComboShip.cpp` still encounters the unchanged `ComboFaroresWind.h:28` unqualified `isfinite` error; the exact R22 baseline reproduces it. This is recorded rather than hidden by an unrelated production edit.

## Boundaries of the evidence

This is implemented source, actual native object compilation, packaged-resource loading and behavioral regression evidence. It is not a full executable link or configured in-game acceptance. The MM-owned resource packaging check used the actual available OoT/Torch packer and matching copy-through format; the complete MM ZAPD pipeline was not executed.

Remaining runtime acceptance is specific: native and foreign pickup framing/shimmer in all four host/item combinations, Alt Assets behavior, bow attachment/hand appearance, and flight/camera/collision feel. Static previews use approximate lighting and do not prove shimmer animation. No master promotion is included.

## Recovery

The final recovery archive contains the unchanged accepted R22 recovery ZIP plus a deterministic binary delta to the exact final candidate commit, source snapshots/hashes, restore instructions, static preview and verification records. Restoration is checked with a separate temporary Git index against the exact target tree; the working index is not used. No GIFs, animated previews, private game archives, saves or full runtime binaries belong in that source checkpoint.
