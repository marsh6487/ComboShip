// The producer and the MM host draw body run against the renderer's existing real-header fixture.
CwItemDrawInfo exported{};
assert(OOT_GetNeiGiDrawInfo(nullptr, &exported) == 0);
assert(OOT_GetNeiGiDrawInfo("unknown", &exported) == 0);
int authored = 0;
for (const auto& item : kPresentations) {
    if (!item.opaque) continue;
    Reset();
    files.insert(item.opaque);
    if (item.translucent) files.insert(item.translucent);
    GetItemEntry bound{}; bound.drawFunc = item.draw;
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
    Reset(); // missing archives: let the original native item render
    exported = {};
    assert(NeiGi_DescribeEntry(&bound, &exported) == 0);
    if (item.translucent) {
        files.insert(item.opaque);
        assert(NeiGi_DescribeEntry(&bound, &exported) == 0); // incomplete split pass never queues half a GI
    }
    ++authored;
}
assert(authored >= 21);
std::cout << "PASS real owner descriptors and MM host draw: all 21 meshes, effects, split passes, concrete Somaria flame, missing-resource fallback\n";
Reset();
