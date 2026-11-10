// Forward physical ownership only. Scene-changing interior returns never require a key.
// Physical source scene and transition actor index, before entrance override.
    switch (scene)
    {
    case SCENE_MARKET_ENTRANCE_RUINS:
    case SCENE_MARKET_ENTRANCE_DAY:
    case SCENE_MARKET_ENTRANCE_NIGHT:
        return RG_GUARD_HOUSE_KEY;
    case SCENE_BACK_ALLEY_DAY:
    case SCENE_BACK_ALLEY_NIGHT:
        switch (id)
        {
        case 0: return RG_RICHARDS_HOUSE_KEY;
        case 1: return RG_ALLEY_HOUSE_KEY;
        case 2: return RG_BOMBCHU_SHOP_KEY;
        }
        break;
    // Castle uses the actual BgSpot15Saku collision gate, not a checkable En_Door proxy.
    case SCENE_TREASURE_BOX_SHOP:
        return RG_TREASURE_CHEST_GAME_BUILDING_KEY;
    case SCENE_MARKET_DAY:
    case SCENE_MARKET_NIGHT:
        switch (id)
        {
        case 0: return RG_TREASURE_CHEST_GAME_BUILDING_KEY;
        case 1: return RG_MARKET_POTION_SHOP_KEY;
        case 2: return RG_MARKET_SHOOTING_GALLERY_KEY;
        case 3: return RG_MASK_SHOP_KEY;
        case 4: return RG_MARKET_BAZAAR_KEY;
        case 5: return RG_BOMBCHU_BOWLING_KEY;
        }
        break;
    case SCENE_LAKE_HYLIA:
        switch (id)
        {
        case 0: return RG_HYLIA_LAB_KEY;
        case 1: return RG_FISHING_HOLE_KEY;
        }
        break;
    case SCENE_LON_LON_RANCH:
        switch (id)
        {
        case 0: return RG_BACK_TOWER_KEY;
        case 1: return RG_STABLES_KEY;
        case 2: return RG_TALONS_HOUSE_KEY;
        }
        break;
    case SCENE_LON_LON_BUILDINGS:
        switch (id)
        {
        case 2: return RG_RUSTY_KEY_RANCH_HOUSE_ROOM;
        }
        break;
    case SCENE_GRAVEYARD:
        return RG_DAMPES_HUT_KEY;
        case SCENE_KAKARIKO_VILLAGE:
        switch (id)
        {
            case 0: return RG_WINDMILL_KEY;
            case 1: return RG_IMPAS_HOUSE_KEY;
            case 2: return RG_BOSS_HOUSE_KEY;
            case 3: return RG_GRANNYS_POTION_SHOP_KEY;
            case 4: return RG_KAK_SHOOTING_GALLERY_KEY;
            case 5: return RG_SKULLTULA_HOUSE_KEY;
            case 6: return RG_KAK_BAZAAR_KEY;
            case 7: return RG_KAK_POTION_SHOP_KEY;
            case 8: return RG_RUSTY_KEY_ADULT_POTION_SHOP_BACK;
        }
        break;
    }
