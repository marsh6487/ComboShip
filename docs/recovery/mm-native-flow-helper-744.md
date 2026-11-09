# MM native save-flow fixture closure

The registered systems Solo Games row reached `mm_native_flow_test.py` after the previous native behavior closure, then failed because the extracted production generation/import bodies now use `VanillaCompatibility` and `SongEvents` while the fixture still declared a reduced synthetic save layout.

This test-only repair uses the actual owning native headers and save/options/item/check identities, links the complete production SongEvents.cpp, and runs creation under both original and Combo guards. The actual generation, file creation, spoiler application, normalization and song import bodies remain under test. Placement/fill, seed randomness, filesystem path lookup, notifications and audio remain controlled service boundaries; this fixture does not claim full placement or gameplay runtime coverage. Real equipment mask data and native flag bodies compile as C, and the actual GameInteractor flag notification/hook dispatcher executes against a registered native hook. The existing reset and owl-save boundary assertions are retained.

Verification: with the established native dependency environment and GCC14, `ASAN_OPTIONS=detect_leaks=0 python3 -B tests/solo_games/mm_native_flow_test.py --sanitize` passed create-original, create-combo, reset-combo and owl-combo. AddressSanitizer and UndefinedBehaviorSanitizer remain enabled. The complete registered Solo Games row is rerun separately after this bounded checkpoint.

No production behavior or required regression registration changed.

Independent review followup: the fixture now extracts the exact production GeneratePools policy prefix, including conditional ClearImportedMapping and preview provenance, before the controlled placement boundary. Added actual spoiler-import valid/missing/invalid mapping controls, preview-preserves-import and new-generation-clears-import oracle assertions, plus Vanilla incompatible-option/night-check normalization with persistent receipt/item/price/skip preservation. Actual SetStartingItemsInSave and GetStartingItemsFromSave bodies now persist and verify the supplied config/spoiler starting items. Both creation build guards and reset/owl controls pass ASan+UBSan again.

The actual complete registered systems Solo Games row (103.50 seconds) progressed through these MM cases and native OoT/network/save/seed loading, then exposed a separate retained OoT settings fixture failure: extracted ParseJson/SOH_RestoreRandoSettings lack RustyDoorKeys declarations. That row is not claimed green by this checkpoint.
