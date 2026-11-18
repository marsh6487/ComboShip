# Prepublication bugfix candidate

This is an isolated source candidate based on the immutable public ComboShip
commit `d3cf01741084c188eb0a01a263018fa3de23fbe1`. It has not been pushed,
merged, installed, built as a full game executable, or accepted in gameplay.
The unavailable seasonal outfit/run source `2d89cbf63b9a666a941628abae505a8876662b3c`
is not included or claimed recovered. These fixes must be integrated with that
work, once recovered or deliberately recreated, before the intended combined
publication. Run-animation acceptance remains a separate prerequisite.

## Current scope

| Report | Source outcome | Evidence limit |
| --- | --- | --- |
| Mounted Great Bay/OoT/MM Pirates Fortress softlock | Clear suspended MM mount state before player reconstruction; restrict native horse spawning to supported destinations/forms; handle failed allocation | Native baseline-control/sanitizer tests pass; exact seed route needs gameplay |
| Gold Skulltula and Business Scrub misc soul GIs | Added native actor recipes, native gold-limb substitutions, resource preflight and safe fallback in both hosts | Detailed models were previously absent; exact selected-pack garbage cause remains unverified |
| MM Black Market Treasure Gal interactions | Native gal interaction clips and form-specific original message SFX cues linked to shop phases | Native action/cue and full native shop-camera fixtures pass; audible/visual runtime review pending |
| Summer drained-lake shortcut crash | Repaired a definite Zora Domain WaterBox camera initializer (8452 became intended4) and bounded camera lookup | Baseline fault reproduced under sanitizers; exact reported shortcut crash attribution unconfirmed |
| PAK models stretch across scene | Decode old N64 vertices and fixed matrices into retained native buffers in both hosts, including overlapping loads | Native translator baseline fails; fixed legacy/wide host tests pass; specific user PAKs need runtime |
| Starting sticks/nuts are full | Uploaded seed explicitly enables MM Consumables Full; clarified its shared-inventory effect | Intentional enabled setting preserved; turn it off for future seeds to farm starting ammo |
| Impa reward question mark | Replaced unsupported em-dash attribution glyph with ASCII in both startup toasts | Native receipt tests pass; this addresses toast text, not an unrelated3D question-mark GI |
| MM check contents after time reset | Permanent soul flags now govern appended soul obtainability; Chuchu visual conversion receives its actual check ID | Native ownership/cycle/preview tests pass; exact save replay pending |
| Collected-souls QoL | MM pause-map L overlay lists111 native souls, owned/missing, including NPC/shopkeepers | Actual-header syntax and authoritative-flag/catalog tests pass; controller/UI runtime pending |
| Soul receipt and gossip readability | Concise family descriptions and soul/item/location emphasis; details recorded by the text workstream | Text/control-code/catalog checks; no translation or hint-clarity changes by inference |
| MM Switch Hook first-person freezes/snaps back | Fixed native weapon-aim entry and retention, consumed C-Up toggle and lifecycle/cancellation cleanup; no magic requirement or charge change | Actual uploaded runtime45dbd690 matches source. Native baseline exits aiming on frame0; fixed native aim/launch/cancel tests pass. Configured gameplay and all ordinary selection swaps remain unverified |
| MM Beetle camera and mirrored X input | Restore prior owned camera context, guard recycled/exhausted slots, reset transient state, remove fake boomerang state and match OoT steering | Native camera/steering fixtures and full item TU syntax; exact swamp-house adult/child runtime pending |
| Sheikah Slate refuses normal stow | Native put-away recognizes Slate in both hosts, clears held/modal state and latches ext-button release while preserving placed rune effects | Actual native put-away/classifier plus Slate cleanup fixtures pass all4 host/build variants; exact controller gameplay pending |

Song of Time clears MM's per-cycle collection status so repeat visits can give
renewable drops. Permanent check receipts and owned souls remain. Already owned
one-time rewards should convert to their repeat reward; they should not appear
as unclaimed original souls. Merely advancing to day two is not this reset.

The per-change `docs/poc/` records describe exact baseline controls, test
commands and service boundaries. Passing those checks is source/component
verification and must not be described as a running-game pass.

Separate pre-existing recovery inputs remain separate unless explicitly merged:
`ComboShip_EquipSwap_Restoration_Source_20261117.zip` and
`ComboShip-ammo-oracle-source-20261114.zip`. This candidate does not overwrite or
silently reapply those older source overlays. Preserve them for cumulative
integration along with the missing seasonal source.

Weighted player skinning was discussed as a separate proposed feature. Both
hosts contain native weighted Skin code; the current player uses Flex drawing.
No weighted player implementation or model conversion belongs to this bugfix
candidate. A future bounded weighted-shoulder proof should preserve the existing
rig, then receive a full-motion preview review before any model archive.

Authored Prelude wall/bomb actor edits are explicitly excluded from this code
candidate at cor's request. Their data, recipes and checks are preserved only
in the separate authored-actor cleanup working deliverable.
