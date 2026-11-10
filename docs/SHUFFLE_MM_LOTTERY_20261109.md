# MM separate lottery nights — 2026-11-09 candidate

Implemented default-off `RO_SHUFFLE_LOTTERY_NIGHTS` (`gRando.Options.RO_SHUFFLE_LOTTERY_NIGHTS`). The real Item Pool page exposes **Separate Lottery Nights** with the normal gold What's New scope, disabled-state handling and a +2 count. No OoT clock changes are included.

| Mode | Check/reward behavior |
|---|---|
| Off | Existing `RC_CLOCK_TOWN_WEST_LOTTERY` remains the one reward available on any native lottery day; extra checks do not enter the pool, tracker or exclusion rows/counts. |
| On | Existing check is night one; appended `RC_CLOCK_TOWN_WEST_LOTTERY_NIGHT_2` and `_NIGHT_3` each have one independent permanent reward. Two purple rupees supply the extra locations. |

Existing check/item/option IDs are preserved. Exclusion keys are those exact RC strings; readable names come from the existing enum-name converter. New checks retain the original NPC category and lottery-shop scene. The existing name-based exclusion adapter preserves exclusions across configuration reloads; disabling the option hides extra nights without deleting those preferences. Excluded junk checks use the existing unshuffled vanilla-item behavior and supply no item to the pool.

The native `EnKujiya` actor still charges 10 rupees, accepts a ticket during 06:00–18:00, indexes winning numbers using `CURRENT_DAY`, consumes the matching day's ticket week flag, and invokes the existing win hook only after a winning comparison. Ticket flags are `WEEKEVENTREG_33_10`, `_33_20` and `_33_40`. Both ticket-offer and victory messages resolve the current day's actual check/item, including the correct check passed to the native item-name service for foreign-item support. Native dialogue, message formatting, fee and number selection are unchanged.

Separate mode queues only an unclaimed check; obtained/eligible state prevents re-awarding on another interaction, scene reload or Song of Time. The native queue owns receipt processing and permanent `obtained` state; the existing cycle-save hook clears `cycleObtained` without clearing permanent ownership. Legacy mode retains its previous queue behavior.

Glitchless logic uses three positive `RE_LOTTERY_TICKET_DAY_1/2/3` purchase-access events. Each requires the matching literal daytime and the existing paid-check wallet requirement. These are monotonic solver access facts, not runtime ticket flags. A separate reward CHECK requires its own ticket event and the actual matching evening interval 18:00 inclusive to 23:00 exclusive. Native `BETWEEN` applies clock ownership; acquiring a day clock alone or a night clock alone cannot fabricate a ticket/reward. This prevents a night reward satisfying a midday deadline. Existing West Clock Town entry restrictions already close entry at 23:00; the shop now has matching stay restrictions. Off-mode reward predicates retain the original expression.

There is no lottery-owner entry in the native soul catalogue or actor mapping. No unrelated soul requirement was invented.

Verification: `tests/shuffle_parity/time/run_lottery_tests.py --sanitize` builds/runs original and Combo fixtures using actual native save types, full production pools/catalogues, callback bodies, JSON conversion, cycle hooks, exclusion writer/reader and stored Central CHECK/EVENT expressions. It checks every exclusion subset, exact additional supply, permanent receipts, all 64 clock masks in Random/Ascending/Descending modes, each day and evening boundary, missing tickets, base-wallet gating, clock-shuffle-off behavior and legacy IDs. Hook registration, font rendering and item-name lookup are controlled services in the callback fixture; filesystem configuration persistence is represented by a JSON-backed Config boundary. UI assertions are source integration checks.

Complete `EnKujiya.cpp`, `Central.cpp` and `Menu.cpp` translation units pass `g++-14 -fsyntax-only` in original and Combo modes with the real dependency headers and the project's JSON precompiled-header dependency explicitly included. `run_lottery_syntax.py` reproduces these probes. No fake headers or warning downgrades were used.

This is an implemented and native-build-verified candidate. No configured game loop, actual ticket playthrough, rendered menu/message, foreign receipt, full application link or seed traversal was demonstrated. In-game acceptance must check all three nights, numbers/fees, native winning and losing dialogue, save/reload before and after receipt, Song of Time, disabled legacy seeds, exclusions and original/Combo configurations. No sacred baseline promotion is implied.

The real exclusion list dynamically labels the preserved original check “(Night 1)” when Separate Lottery Nights is on; filtering uses the same label. The native tracker’s existing original-check label remains generic, while appended nights have explicit generated names. Persistent names/IDs are unchanged.
