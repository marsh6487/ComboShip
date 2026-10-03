// The producer and the MM host draw body run against the renderer's existing real-header fixture.
CwItemDrawInfo exported{};
assert(OOT_GetNeiGiDrawInfo(nullptr, &exported) == 0);
assert(OOT_GetNeiGiDrawInfo("unknown", &exported) == 0);
// Season descriptors never require a rod or authored season mesh, and MM's
// generic native bridge must accept their zero-display-list weather recipes.
const CustomDrawFunc seasonCallbacks[] = {Randomizer_DrawSeasonSpring, Randomizer_DrawSeasonSummer,
                                         Randomizer_DrawSeasonAutumn, Randomizer_DrawSeasonWinter};
const char* seasonNames[] = {"season_spring", "season_summer", "season_autumn", "season_winter"};
for (int season = 0; season < 4; ++season) {
    Reset();
    GetItemEntry weather{};
    weather.drawFunc = seasonCallbacks[season];
    exported = {};
    assert(NeiGi_DescribeEntry(&weather, &exported) == 1);
    assert(exported.drawKind == CW_DRAW_KIND_SEASON_GI && exported.neiEffect == season + 1);
    assert(exported.dlistCount == 0 && exported.opCount == 0 && exported.xluStartIndex == -1);
    CwItemDrawInfo native{};
    assert(OOT_GetNeiGiDrawInfo(seasonNames[season], &native) == 1);
    assert(native.drawKind == exported.drawKind && native.neiEffect == exported.neiEffect);
    assert(native.dlistCount == 0 && native.opCount == 0);
    MM_DrawNeiGi(native);
    assert(!arena.empty() && gfx.polyXlu.p > xlu);
    assert(Drawn().empty() && gfx.polyOpa.p == opa && fallback == 0);
    assert(stack.empty() && interpolation == 0 && matrix == 1 && loads == 0);
}
Reset();
files.insert("__OTR__objects/nei_gi_redesign/rod_of_seasons/gi_dl");
assert(OOT_GetNeiGiDrawInfo("rod_of_seasons", &exported) == 1);
assert(exported.drawKind == CW_DRAW_KIND_NEI_GI && exported.neiEffect == int(Kind::SeasonCycle));
MM_DrawNeiGi(exported);
assert(Drawn() == std::vector<std::string>{"__OTR__objects/nei_gi_redesign/rod_of_seasons/gi_dl"});
assert(!arena.empty() && stack.empty() && interpolation == 0 && loads == 0);
// Inactive OoT owns its own Alt mode, independently of the active MM context.
Reset();
files.insert("alt/__OTR__objects/nei_gi_redesign/fire_rod/gi_dl");
ownerAlt = true;
alt = false;
assert(OOT_GetNeiGiDrawInfo("fire_rod", &exported) == 1);
ownerAlt = false;
alt = true;
assert(OOT_GetNeiGiDrawInfo("fire_rod", &exported) == 0);
int authored = 0;
for (const auto& item : kPresentations) {
    if (!item.opaque) continue;
    Reset();
    files.insert(item.opaque);
    if (item.translucent) files.insert(item.translucent);
    GetItemEntry bound{}; bound.drawFunc = item.draw; bound.drawItemId=item.identity; bound.gid=item.nativeGid;
    exported = {};
    assert(NeiGi_DescribeEntry(&bound, &exported) == 1);
    assert(exported.drawKind == CW_DRAW_KIND_NEI_GI);
    assert(exported.neiEffect == static_cast<int>(item.effect));
    assert(exported.scale == item.scale);
    assert(exported.neiSomariaUpgrade == (item.draw == Randomizer_DrawCaneSomariaUpgrade));
    assert(std::strcmp(exported.dlists[0], item.opaque) == 0);
    const std::string slug = std::string(item.opaque).substr(std::strlen("__OTR__objects/nei_gi_redesign/"));
    CwItemDrawInfo native{};
    assert(OOT_GetNeiGiDrawInfo(slug.substr(0, slug.find('/')).c_str(), &native) == 1);
    assert(native.neiSomariaUpgrade == 0); // native MM chooses concrete skill, never OoT inventory
    MM_DrawNeiGi(exported);
    const auto paths = Drawn();
    assert(std::count(paths.begin(), paths.end(), item.opaque) == 1);
    if (item.translucent) assert(std::count(paths.begin(), paths.end(), item.translucent) == 1);
    assert(stack.empty() && interpolation == 0 && matrix == 1);
    if (NeiGi::IsRod(item.effect) || NeiGi::IsSpell(item.effect)) assert(!arena.empty());
    if (exported.neiSomariaUpgrade) assert((flameColors == std::vector<std::array<unsigned,3>>{{255,60,60}}));
    // Every new binding also shares the old NEI shimmer in the MM host.
    const auto offMeshes = arena;
    const auto offPaths = paths;
    Reset(); enabled = 1;
    MM_DrawNeiGi(exported);
    assert(Drawn() == offPaths);
    assert(arena.size() == offMeshes.size() + (exported.itemShimmer ? 0 : 1));
    const auto shown = arena;
    arena.clear();
    NeiGi_DrawMesh(&play, NeiGi::SampleShimmer(play.gameplayFrames, true,
                       NeiGi_CameraBasis(&play), item.effect));
    assert(arena.size() == 1);
    const auto &want = arena.front();
    size_t matching = 0;
    for (const auto &got : shown)
      if (got.size() == want.size() &&
          !std::memcmp(got.data(), want.data(), want.size() * sizeof(Vtx))) ++matching;
    assert(matching == 1);
    assert(stack.empty() && interpolation == 0 && matrix == 1);
    Reset(); // missing archives: let the original native item render
    exported = {};
    assert(NeiGi_DescribeEntry(&bound, &exported) == 0);
    if (item.translucent) {
        files.insert(item.opaque);
        assert(NeiGi_DescribeEntry(&bound, &exported) == 0); // incomplete split pass never queues half a GI
    }
    ++authored;
}
assert(authored == 64);
std::cout << "PASS real owner descriptors and MM host draw: all authored bindings, effects, split passes, concrete Somaria flame, missing-resource fallback\n";
Reset();

// The same callback represents six concrete wand awards; identity must select
// the new mesh and effect, independent of another game's inventory.
Reset(); files.insert("__OTR__objects/nei_gi_redesign/water_rod/gi_dl");
GetItemEntry water{}; water.drawFunc=Randomizer_DrawElementalWand; water.drawItemId=RG_WAND_WATER_ROD;
CwItemDrawInfo waterInfo{};
assert(NeiGi_DescribeEntry(&water,&waterInfo)==1);
assert(!strcmp(waterInfo.dlists[0],"__OTR__objects/nei_gi_redesign/water_rod/gi_dl"));
assert(waterInfo.neiEffect==int(Kind::Water));
Reset();

// Selected sword availability is owned by OoT even while MM is active.
Reset();
files.insert("__OTR__objects/nei_gi_redesign/master_sword/gi_dl");
files.insert("__OTR__alt/objects/object_custom_equip/gCustomMasterSwordDL");
ownerAlt=true; alt=false;
assert(OOT_GetNeiGiDrawInfo("master_sword",&exported)==0);
ownerAlt=false; alt=true;
assert(OOT_GetNeiGiDrawInfo("master_sword",&exported)==1);
Reset();
