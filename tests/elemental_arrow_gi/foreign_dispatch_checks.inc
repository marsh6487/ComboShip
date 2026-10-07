{
    // Execute both production foreign dispatchers and the real simple core
    // handlers. A carried shimmer flag must not duplicate intrinsic shimmer.
    for (int profile = 1; profile <= 3; ++profile) for (int assets : {0,1}) for (int effects : {0,1}) {
        Reset(); alt=ownerAlt=assets; enabled=effects; portableSongLists=true;
        ComboForeignDrawInfoOOT oot{};
        oot.ok=true; oot.drawKind=CW_DRAW_KIND_ELEMENTAL_ARROW; oot.neiEffect=profile;
        oot.count=1; oot.xluStart=-1; oot.scale=1; oot.itemShimmer=true;
        oot.dls[0]="__OTR__@oot:objects/object_gi_m_arrow/gGiMagicArrowDL";
        selectedForeignInfoMM=&oot;
        MM_DrawComboForeign(1);
        assert(Drawn()==std::vector<std::string>{oot.dls[0]} && arena.size()==3 && stack.empty());
        Reset(); alt=ownerAlt=assets; enabled=effects; portableSongLists=true;
        ComboForeignDrawInfo mm{};
        mm.ok=true; mm.drawKind=CW_DRAW_KIND_ELEMENTAL_ARROW; mm.neiEffect=profile;
        mm.count=1; mm.xluStart=-1; mm.scale=1; mm.itemShimmer=true;
        mm.dls[0]="__OTR__@mm:objects/object_gi_m_arrow/gGiMagicArrowAmmoDL";
        GetItemEntry entry{}; entry.comboForeignCheck=1;
        selectedForeignInfo=&mm;
        OOT_DrawComboForeign(&play,&entry);
        assert(Drawn()==std::vector<std::string>{mm.dls[0]} && arena.size()==3 && stack.empty());
    }
    selectedForeignInfo=nullptr; selectedForeignInfoMM=nullptr;
    std::cout << "PASS real elemental-arrow foreign dispatch: both owner cores, Alt/toggle parity, exactly one intrinsic shimmer\n";
}
