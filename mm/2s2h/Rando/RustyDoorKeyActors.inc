// Forward physical ownership only. Scene-changing interior returns never require a key.
// Physical source scene and transition actor index, before entrance override.
    switch (scene)
    {
    case SCENE_20SICHITAI:
    case SCENE_20SICHITAI2:
        switch (id)
        {
        case 3: return RI_RUSTY_KEY_TOURIST_INFORMATION;
        case 4: return RI_RUSTY_KEY_POTION_SHOP;
        }
        break;
    case SCENE_ICHIBA:
        switch (id)
        {
        case 0: return RI_RUSTY_KEY_POST_OFFICE;
        case 1: return RI_RUSTY_KEY_SWORDSMAN_SCHOOL;
        case 2: return RI_RUSTY_KEY_LOTTERY;
        case 3: return RI_RUSTY_KEY_BOMB_SHOP;
        case 4: return RI_RUSTY_KEY_TRADING_POST;
        case 5: return RI_RUSTY_KEY_CURIOSITY_SHOP;
        }
        break;
    case SCENE_ALLEY: // Laundry Pool: forward entry to Kafei's hideout.
        return RI_RUSTY_KEY_KAFEI_HIDEOUT;
    // Unrelated South Clock Town decorative doors are not the East Town gallery.
    case SCENE_24KEMONOMITI:
        return RI_RUSTY_KEY_SWAMP_ARCHERY;
    case SCENE_00KEIKOKU:
        return RI_RUSTY_KEY_OBSERVATORY;
    case SCENE_10YUKIYAMANOMURA2:
    case SCENE_10YUKIYAMANOMURA:
        return RI_RUSTY_KEY_BLACKSMITH;
    case SCENE_IKANA:
        return RI_RUSTY_KEY_MUSIC_HOUSE;
    case SCENE_30GYOSON:
        return RI_RUSTY_KEY_LABORATORY;
    case SCENE_BOTI:
        return RI_RUSTY_KEY_DAMPE_HOUSE;
    case SCENE_HAKASHITA:
        return RI_RUSTY_KEY_BENEATH_GRAVEYARD;
    case SCENE_SONCHONOIE:
        switch (id)
        {
        case 0: return RI_RUSTY_KEY_MAYOR_RESIDENCE_OFFICE;
        case 1: return RI_RUSTY_KEY_MAYOR_RESIDENCE_SALON;
        case 2: return RI_RUSTY_KEY_MAYOR_RESIDENCE_KAFEI;
        }
        break;
    case SCENE_F01:
        switch (id)
        {
        case 0: return RI_RUSTY_KEY_RANCH_HOUSE;
        case 1: return RI_RUSTY_KEY_RANCH_BARN;
        case 2: return RI_RUSTY_KEY_DOG_RACETRACK;
        case 3: return RI_RUSTY_KEY_CUCCO_SHACK;
        }
        break;
    case SCENE_OMOYA:
        switch (id)
        {
        case 2: return RI_RUSTY_KEY_RANCH_HOUSE_ROOM;
        }
        break;
    case SCENE_33ZORACITY:
        switch (id)
        {
        case 0: return RI_RUSTY_KEY_ZORA_LULU_ROOM;
        case 1: return RI_RUSTY_KEY_ZORA_EVAN_ROOM;
        case 2: return RI_RUSTY_KEY_ZORA_JAPAS_ROOM;
        case 3: return RI_RUSTY_KEY_ZORA_TIJO_ROOM;
        case 4: return RI_RUSTY_KEY_ZORA_SHOP;
        }
        break;
    case SCENE_YADOYA:
        switch (id)
        {
        case 0: return RI_RUSTY_KEY_GRANDMA_ROOM;
        case 1: return RI_RUSTY_KEY_STOCK_POT_INN_STAFF_ROOM;
        case 3: return RI_RUSTY_KEY_STOCK_POT_INN_DORMITORY;
        }
        break;
    case SCENE_TOWN:
        switch (id)
        {
        case 0: return RI_RUSTY_KEY_STOCK_POT_INN;
        case 1: return RI_RUSTY_KEY_STOCK_POT_INN_ROOF;
        case 2: return RI_RUSTY_KEY_TREASURE_CHEST_GAME;
        case 3: return RI_RUSTY_KEY_TOWN_ARCHERY;
        case 4: return RI_RUSTY_KEY_HONEY_DARLING;
        case 5: return RI_RUSTY_KEY_MAYOR_RESIDENCE;
        case 6: return RI_RUSTY_KEY_MILK_BAR;
        }
        break;
    }
