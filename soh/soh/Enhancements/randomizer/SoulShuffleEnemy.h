#pragma once
#include "randomizerEnums.h"
#include "z64actor_enum.h"
#include <stdint.h>

namespace Rando::SoulShuffle {
// Native actor IDs, never donor numeric IDs. Boss children, inert statues,
// composer ghosts and child Skull Kid roles retain their native behavior.
inline RandomizerGet EnemyForActor(int actorId, uint16_t params, bool adult, bool gohmaArena) {
    switch (actorId) {
        case ACTOR_EN_GOMA:
            return gohmaArena ? RG_NONE : RG_SOUL_ENEMY_GOHMA_LARVA;
        case ACTOR_EN_AM:
            return params == 0 ? RG_NONE : RG_SOUL_ENEMY_ARMOS;
        case ACTOR_EN_POH:
            return params == 2 || params == 3 ? RG_NONE : RG_SOUL_ENEMY_POE;
        case ACTOR_EN_SKJ:
            return adult ? RG_SOUL_ENEMY_SKULL_KID : RG_NONE;
        case ACTOR_EN_SW:
            return params & 0xE000 ? RG_SOUL_MISC_GS : RG_SOUL_ENEMY_SKULLWALLTULA;
        case ACTOR_EN_TEST:
            return RG_SOUL_ENEMY_STALFOS;
        case ACTOR_EN_OKUTA:
        case ACTOR_EN_BIGOKUTA:
            return RG_SOUL_ENEMY_OCTOROK;
        case ACTOR_EN_WALLMAS:
            return RG_SOUL_ENEMY_WALLMASTER;
        case ACTOR_EN_DODONGO:
            return RG_SOUL_ENEMY_DODONGO;
        case ACTOR_EN_FIREFLY:
            return RG_SOUL_ENEMY_KEESE;
        case ACTOR_EN_TITE:
            return RG_SOUL_ENEMY_TEKTITE;
        case ACTOR_EN_REEBA:
            return RG_SOUL_ENEMY_LEEVER;
        case ACTOR_EN_PEEHAT:
            return RG_SOUL_ENEMY_PEAHAT;
        case ACTOR_EN_ZF:
            return RG_SOUL_ENEMY_LIZALFOS_DINOLFOS;
        case ACTOR_EN_BUBBLE:
            return RG_SOUL_ENEMY_SHABOM;
        case ACTOR_EN_DODOJR:
            return RG_SOUL_ENEMY_BABY_DODONGO;
        case ACTOR_EN_TORCH2:
            return RG_SOUL_ENEMY_DARK_LINK;
        case ACTOR_EN_BILI:
        case ACTOR_EN_VALI:
            return RG_SOUL_ENEMY_BIRI_BARI;
        case ACTOR_EN_TP:
            return RG_SOUL_ENEMY_TAILPASARN;
        case ACTOR_EN_BA:
            return RG_SOUL_ENEMY_PARASITE;
        case ACTOR_EN_ST:
            return RG_SOUL_ENEMY_SKULLTULA;
        case ACTOR_EN_BW:
            return RG_SOUL_ENEMY_TORCH_SLUG;
        case ACTOR_EN_MB:
            return RG_SOUL_ENEMY_MOBLIN;
        case ACTOR_EN_DEKUBABA:
        case ACTOR_EN_KAREBABA:
            return RG_SOUL_ENEMY_DEKU_BABA;
        case ACTOR_EN_DEKUNUTS:
        case ACTOR_EN_HINTNUTS:
            return RG_SOUL_ENEMY_DEKU_SCRUB;
        case ACTOR_EN_BB:
            return RG_SOUL_ENEMY_BUBBLE;
        case ACTOR_EN_VM:
            return RG_SOUL_ENEMY_BEAMOS;
        case ACTOR_EN_FLOORMAS:
            return RG_SOUL_ENEMY_FLOORMASTER;
        case ACTOR_EN_PO_SISTERS:
        case ACTOR_EN_PO_FIELD:
            return RG_SOUL_ENEMY_POE;
        case ACTOR_EN_RD:
            return RG_SOUL_ENEMY_REDEAD_GIBDO;
        case ACTOR_EN_FD:
            return RG_SOUL_ENEMY_FLARE_DANCER;
        case ACTOR_EN_DH:
        case ACTOR_EN_DHA:
            return RG_SOUL_ENEMY_DEAD_HAND;
        case ACTOR_EN_SB:
            return RG_SOUL_ENEMY_SHELL_BLADE;
        case ACTOR_EN_RR:
            return RG_SOUL_ENEMY_LIKE_LIKE;
        case ACTOR_EN_NY:
            return RG_SOUL_ENEMY_SPIKE;
        case ACTOR_EN_ANUBICE:
            return RG_SOUL_ENEMY_ANUBIS;
        case ACTOR_EN_IK:
            return RG_SOUL_ENEMY_IRON_KNUCKLE;
        case ACTOR_EN_TUBO_TRAP:
            return RG_SOUL_ENEMY_FLYING_POT;
        case ACTOR_EN_FZ:
            return RG_SOUL_ENEMY_FREEZARD;
        case ACTOR_EN_WEIYER:
        case ACTOR_EN_EIYER:
            return RG_SOUL_ENEMY_STINGER;
        case ACTOR_EN_WF:
            return RG_SOUL_ENEMY_WOLFOS;
        case ACTOR_EN_SKB:
            return RG_SOUL_ENEMY_STALCHILD;
        case ACTOR_EN_CROW:
            return RG_SOUL_ENEMY_GUAY;
        case ACTOR_EN_GELDB:
            return RG_SOUL_ENEMY_THIEVES;
        default:
            return RG_NONE;
    }
}

// This is ANDed with native weapons/tricks in the real CanKillEnemy body.
inline RandomizerGet EnemyForLogic(RandomizerEnemy enemy) {
    switch (enemy) {
        case RE_GOLD_SKULLTULA:
            return RG_SOUL_MISC_GS;
        case RE_BIG_SKULLTULA:
            return RG_SOUL_ENEMY_SKULLTULA;
        case RE_DODONGO:
            return RG_SOUL_ENEMY_DODONGO;
        case RE_LIZALFOS:
            return RG_SOUL_ENEMY_LIZALFOS_DINOLFOS;
        case RE_GOHMA_LARVA:
            return RG_SOUL_ENEMY_GOHMA_LARVA;
        case RE_KEESE:
            return RG_SOUL_ENEMY_KEESE;
        case RE_FIRE_KEESE:
            return RG_SOUL_ENEMY_KEESE;
        case RE_GUAY:
            return RG_SOUL_ENEMY_GUAY;
        case RE_MAD_SCRUB:
            return RG_SOUL_ENEMY_DEKU_SCRUB;
        case RE_BLUE_BUBBLE:
            return RG_SOUL_ENEMY_BUBBLE;
        case RE_POE:
            return RG_SOUL_ENEMY_POE;
        case RE_DEAD_HAND:
            return RG_SOUL_ENEMY_DEAD_HAND;
        case RE_DEKU_BABA:
            return RG_SOUL_ENEMY_DEKU_BABA;
        case RE_WITHERED_DEKU_BABA:
            return RG_SOUL_ENEMY_DEKU_BABA;
        case RE_LIKE_LIKE:
            return RG_SOUL_ENEMY_LIKE_LIKE;
        case RE_STALFOS:
            return RG_SOUL_ENEMY_STALFOS;
        case RE_IRON_KNUCKLE:
            return RG_SOUL_ENEMY_IRON_KNUCKLE;
        case RE_FLARE_DANCER:
            return RG_SOUL_ENEMY_FLARE_DANCER;
        case RE_WOLFOS:
            return RG_SOUL_ENEMY_WOLFOS;
        case RE_FLOORMASTER:
            return RG_SOUL_ENEMY_FLOORMASTER;
        case RE_REDEAD:
            return RG_SOUL_ENEMY_REDEAD_GIBDO;
        case RE_MEG:
            return RG_SOUL_ENEMY_POE;
        case RE_ARMOS:
            return RG_SOUL_ENEMY_ARMOS;
        case RE_GREEN_BUBBLE:
            return RG_SOUL_ENEMY_BUBBLE;
        case RE_DINOLFOS:
            return RG_SOUL_ENEMY_LIZALFOS_DINOLFOS;
        case RE_TORCH_SLUG:
            return RG_SOUL_ENEMY_TORCH_SLUG;
        case RE_FREEZARD:
            return RG_SOUL_ENEMY_FREEZARD;
        case RE_SHELL_BLADE:
            return RG_SOUL_ENEMY_SHELL_BLADE;
        case RE_SPIKE:
            return RG_SOUL_ENEMY_SPIKE;
        case RE_WHITE_WOLFOS:
            return RG_SOUL_ENEMY_WOLFOS;
        case RE_STINGER:
            return RG_SOUL_ENEMY_STINGER;
        case RE_BIG_OCTO:
            return RG_SOUL_ENEMY_OCTOROK;
        case RE_GERUDO_WARRIOR:
            return RG_SOUL_ENEMY_THIEVES;
        case RE_GIBDO:
            return RG_SOUL_ENEMY_REDEAD_GIBDO;
        case RE_DARK_LINK:
            return RG_SOUL_ENEMY_DARK_LINK;
        case RE_ANUBIS:
            return RG_SOUL_ENEMY_ANUBIS;
        case RE_BEAMOS:
            return RG_SOUL_ENEMY_BEAMOS;
        case RE_WALLMASTER:
            return RG_SOUL_ENEMY_WALLMASTER;
        case RE_PURPLE_LEEVER:
            return RG_SOUL_ENEMY_LEEVER;
        case RE_TENTACLE:
            return RG_SOUL_ENEMY_PARASITE;
        case RE_BARI:
            return RG_SOUL_ENEMY_BIRI_BARI;
        case RE_SHABOM:
            return RG_SOUL_ENEMY_SHABOM;
        case RE_OCTOROK:
            return RG_SOUL_ENEMY_OCTOROK;
        case RE_WALLTULA:
            return RG_SOUL_ENEMY_SKULLWALLTULA;
        default:
            return RG_NONE; // Bosses and noncombat guards retain existing predicates.
    }
}
} // namespace Rando::SoulShuffle
