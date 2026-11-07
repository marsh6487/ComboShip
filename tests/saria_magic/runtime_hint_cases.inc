static void ResetHintStorage() {
    auto* ctx = Context::GetInstance();
    ctx->placements = {};
    ctx->hint = Hint();
    ctx->added = 0;
    ComboRando::g_comboForeignJson.clear();
    sSariaMagicReachabilityVerified = false;
    sEarlySariaMagicCheck = RC_UNKNOWN_CHECK;
    sEarlySariaMagicSeed = 0;
    revealed = 0;
    loadedEn.clear(); loadedDe.clear(); loadedFr.clear();
    registered.clear();
    InitTemplates();
    RegisterSariaCallbacks();
}
static void Open(uint16_t id) {
    assert(registered.contains(id) && "effective stat fallback must register the real text callback");
    bool fromTable = true;
    registered.at(id)(&id, &fromTable);
    assert(!fromTable);
}
extern "C" void SariaFixture_OpenNativeHint(uint16_t id) {
    assert(SariaFixture_GetClassic() == 0 && SariaFixture_GetStat() == 1);
    ResetHintStorage();
    auto* ctx = Context::GetInstance();
    // No allLocations/graph-area pool exists in this cold-load boundary.
    ctx->placements[1].item = RG_MAGIC_STAT_UPGRADE;
    ctx->placements[2].item = RG_MAGIC_STAT_UPGRADE;
    const auto rng = rando_state;
    Open(id);
    assert(loadedEn.find("Hyrule Field") != std::string::npos && loadedEn.find("[[1]]") == std::string::npos);
    assert(loadedDe.find("Hyrule Field") != std::string::npos && loadedFr.find("Hyrule Field") != std::string::npos);
    assert(revealed == 1 && ctx->added == 1 && rando_state == rng);
    for (const auto& item : ctx->placements)
        assert(!item.hinted && !item.accessible);
    assert(ctx->placements[1].item == RG_MAGIC_STAT_UPGRADE && ctx->placements[2].item == RG_MAGIC_STAT_UPGRADE);
    Open(TEXT_SARIAS_SONG_FACE_TO_FACE);
    assert(loadedEn.find("Hyrule Field") != std::string::npos && loadedType == TEXTBOX_TYPE_BLUE);
    assert(ctx->added == 1 && rando_state == rng && SariaFixture_GetClassic() == 0);
    const auto rawTalk = ctx->hint.GetHintMessage(MF_RAW, 0);
    const auto rawSong = ctx->hint.GetHintMessage(MF_RAW, 1);
    assert(rawTalk.GetEnglish(MF_RAW).find("%g") != std::string::npos &&
           rawTalk.GetEnglish(MF_RAW).find("%r") != std::string::npos &&
           rawTalk.GetEnglish(MF_RAW).find('#') == std::string::npos);
    // SaveRandomizer stores raw localized MESSAGE text; LoadRandomizer rebuilds
    // plain CustomMessage values, dropping the separate color palette/type.
    ctx->hint = Hint(RH_SARIA_HINT, {
        CustomMessage(rawTalk.GetEnglish(MF_RAW), rawTalk.GetGerman(MF_RAW), rawTalk.GetFrench(MF_RAW)),
        CustomMessage(rawSong.GetEnglish(MF_RAW), rawSong.GetGerman(MF_RAW), rawSong.GetFrench(MF_RAW))});
    Open(TEXT_SARIAS_SONG_FACE_TO_FACE);
    assert(loadedEn.find("\x05\x42") != std::string::npos && loadedEn.find("\x05\x41") != std::string::npos);
    assert(loadedType == TEXTBOX_TYPE_BLUE && loadedEn.back() == '\x02');
    std::cout << "PASS actual Saria actor text -> registered callbacks -> lazy native saved placement, talk/song/locales, immutable RNG/settings/locations\n";
}
extern "C" void SariaFixture_VerifyRuntimeControls(void) {
    SariaFixture_SetOptions(0, 1);
    ResetHintStorage();
    auto* ctx = Context::GetInstance();
    ComboRando::g_comboForeignJson = R"({"hints":{"ootItemAreas":{"Magic Stat Upgrade":"Snowhead Temple"}}})";
    Open(TEXT_SARIA_SFM);
    assert(loadedEn.find("Snowhead Temple") != std::string::npos && revealed == 1);
    assert(ctx->added == 1);
    // Existing enabled authored/composed rows are never replaced.
    ctx->hint = Hint(RH_SARIA_HINT, {CustomMessage("Authored talk"), CustomMessage("Authored song")});
    Open(TEXT_SARIA_SFM);
    assert(loadedEn == "Authored talk\x02" && ctx->added == 1);
    ResetHintStorage();
    const auto rng = rando_state;
    for (const char* malformed : {"{", R"({"hints":{"ootItemAreas":null}})",
                                  R"({"hints":{"ootItemAreas":{"Magic Stat Upgrade":7}}})",
                                  R"({"hints":{"ootItemAreas":{"Magic Stat Upgrade":""}}})"}) {
        ComboRando::g_comboForeignJson = malformed;
        Open(TEXT_SARIA_SFM);
        assert(!loadedEn.empty() && revealed == 0 && ctx->added == 0 && rando_state == rng);
    }
    ComboRando::g_comboForeignJson.clear();
    Open(TEXT_SARIA_SFM);
    assert(!loadedEn.empty() && loadedEn.find("cannot sense") != std::string::npos);
    assert(loadedEn.back() == '\x02' && loadedDe.back() == '\x02' && loadedFr.back() == '\x02');
    Open(TEXT_SARIAS_SONG_FACE_TO_FACE);
    assert(loadedType == TEXTBOX_TYPE_BLUE && loadedEn.back() == '\x02' && loadedEn[loadedEn.size() - 2] == '\x0B');
    assert(revealed == 0 && ctx->added == 0 && rando_state == rng);
    // A matching complete search proving no upgrade reachable beats the
    // legacy placement fallback, even with an actual unreachable upgrade.
    ctx->placements[2].item = RG_MAGIC_STAT_UPGRADE;
    sSariaMagicReachabilityVerified = true;
    sEarlySariaMagicSeed = ctx->GetSeed();
    Open(TEXT_SARIA_SFM);
    assert(revealed == 0 && ctx->added == 0);
    // A stale cache from a different seed cannot suppress the current row.
    ++sEarlySariaMagicSeed;
    Open(TEXT_SARIA_SFM);
    assert(loadedEn.find("Hyrule Field") != std::string::npos && revealed == 1);
    ResetHintStorage();
    ctx->placements[1].item = ctx->placements[2].item = RG_MAGIC_STAT_UPGRADE;
    sEarlySariaMagicCheck = 1;
    sEarlySariaMagicSeed = ctx->GetSeed();
    sSariaMagicReachabilityVerified = true;
    Open(TEXT_SARIA_SFM);
    assert(loadedEn.find("Kokiri Forest") != std::string::npos);
    SariaFixture_SetOptions(0, 0);
    ResetHintStorage();
    assert(registered.empty() && !EnsureSariaMagicHint() && ctx->added == 0);
    SariaFixture_SetOptions(1, 0);
    ResetHintStorage();
    assert(registered.contains(TEXT_SARIA_SFM));
    ctx->hint = Hint(RH_SARIA_HINT, {CustomMessage("Classic meter"), CustomMessage("Classic song")});
    Open(TEXT_SARIA_SFM);
    assert(loadedEn == "Classic meter\x02" && ctx->added == 0);
    SariaFixture_SetOptions(0, 1);
    SariaFixture_SetRandomizer(0);
    ResetHintStorage();
    assert(registered.empty());
    SariaFixture_SetRandomizer(1);
    std::cout << "PASS cold cross-world area, preserved prebuilt hints, no-target/no-reveal, verified-none/stale-cache and classic-off/stat-off controls\n";
}
