# Soul receipt flavor and readable hint emphasis

Base: `d3cf01741084c188eb0a01a263018fa3de23fbe1`; cumulative November 17 unpublished bugfix candidate.

Only boss souls had the existing descriptive/colored receipt override. Ordinary enemy, NPC, animal and miscellaneous soul identities now use an exact shared catalog at the receipt boundary. Enemy names are red, NPC names purple, animal names green, and misc names yellow. Brief descriptions explain that the associated enemies, people or creatures can return. Gold Skulltula and Business Scrub souls have their own short lines. No location, new progression requirement, or NPC-specific quest claim is invented.

The helper recognizes the authoritative shared soul catalog plus the original 47 MM enemy identities. Unrecognized names remain on their existing route. Boss soul text/color overrides remain intact. Native OoT preserves its existing localized item names and custom icon; native/imported MM and imported OoT use the canonical name, with EN/DE/FR sentence text. The existing foreign-trap branch still takes precedence, so this does not expose a disguised trap. No grants, soul ownership, save state or item lookup identities change.

Native MM free, paid and Moon gossip already used colored item/location spans and the automatic native encoder. They are retained. Plain saved placement sentences in the direct-message OoT path now gain green item/yellow location emphasis only for formatted display. MM's foreign-gossip formatter also handles that plain placement grammar when no OoT suffix is present. Already colored prose, unknown sentence forms and obscure hints remain unchanged. Saved raw messages, hint generation, weights, RNG, tracker identity and translation wording are preserved. Native encoded control commands and authored hashtag spans reject plain-sentence decoration. OoT updates message text in place, retaining all presentation metadata rather than reconstructing a message.

Verification completed:
- The real MM receipt builder catalog sweep covers all 106 ordinary native soul rows across three locales, native/imported equality, retained icon, no donor read and descriptive text. Exact native enum/catalog membership prevents silent missing entries.
- The actual OoT donor/native receipt fixture passes the new ordinary-soul samples and its existing boss, native/imported, trap, locale, icon, tutorial and save-isolation checks.
- `tests/cross_hints/run_gossip_routing_tests.py --sanitize` passes native OoT/MM stone consumers, existing native colors, saved-pool/RNG preservation, and the new plain-placement grammar/idempotence controls.
- Full `hint.cpp` and `Messages/ItemMessages.cpp` translation-unit syntax checks pass with actual game/libultraship headers and pinned dependencies.
- The complete `scripts/diagnostics/run_mm_item_receipt_tests.py --sanitizers` aggregate gate passes, including its real OoT donor-to-MM receipt link, queued native/imported awards, traps, sprites, translated tutorials, altars and settings. The extracted altar-message service mock includes the native in-place replacement boundary. A focused ASan/UBSan control also confirms native encoded color commands survive both plain and foreign hint decoration unchanged.

LeakSanitizer is disabled because the managed process runs under tracing; address and undefined-behavior sanitizers remain enabled. No full executable, rendered textbox, exact active pack or gameplay acceptance is claimed.
