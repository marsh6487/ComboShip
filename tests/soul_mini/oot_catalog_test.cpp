#include <cassert>
#include <cstdio>
#include <cstring>
#include <set>
#include <string>
#include <vector>

static int enemySimple = 0, npcSimple = 0;
int CVarGetInteger(const char* key, int fallback) {
    if (std::strstr(key, "SimplerEnemySoulModels")) return enemySimple;
    if (std::strstr(key, "SimplerNpcSoulModels")) return npcSimple;
    return fallback;
}
enum {
#define SOUL(game, family, key, name) SOUL_##game(family, key, name)
#define SOUL_OOT(family, key, name) RG_SOUL_##family##_##key,
#define SOUL_MM(family, key, name)
#include "combo/rando/SoulShuffleCatalog.inc"
#undef SOUL_MM
#undef SOUL_OOT
#undef SOUL
};
#include "combo/menu/ComboSoulMiniOOT.h"

struct Identity {int id; const char* name; const char* category;};
static constexpr Identity Identities[] = {
#define SOUL(game, family, key, name) SOUL_##game(family, key, name)
#define SOUL_OOT(family, key, name) {RG_SOUL_##family##_##key, "RG_SOUL_" #family "_" #key, #family},
#define SOUL_MM(family, key, name)
#include "combo/rando/SoulShuffleCatalog.inc"
#undef SOUL_MM
#undef SOUL_OOT
#undef SOUL
};
static void path(const char* p) { if(p) std::printf("\"%s\"",p); else std::printf("null"); }
int main(int argc, char**) {
    using namespace ComboSoulMiniOOT;
    assert(sizeof(Catalog)/sizeof(Catalog[0]) == 89);
    CwItemAnimDrawInfo info{};
    assert(!Describe(-1, &info)); assert(!Describe(RG_SOUL_NPC_SARIA, nullptr));
    std::set<int> unique;
    unsigned enemies=0,npcs=0,composites=0;
    const bool dump = argc > 1;
    if(dump) std::puts("[");
    bool first=true;
    for(const auto& identity: Identities) {
        if(std::strcmp(identity.category,"ENEMY") && std::strcmp(identity.category,"NPC")) {
            assert(!Find(identity.id)); continue;
        }
        assert(unique.insert(identity.id).second);
        const bool npc = !std::strcmp(identity.category,"NPC");
        npc ? ++npcs : ++enemies;
        enemySimple = npcSimple = 0;
        assert(Describe(identity.id,&info)); assert(info.stateDependent);
        assert(info.scale > 0 && info.scale <= 1.0f);
        assert(info.skelPath || info.partCount);
        assert(!info.skelPath || info.limbCount > 1);
        assert(!info.skelPath || info.animPath || info.bindPose);
        assert(info.segCount <= CW_ANIM_MAX_SEGS && info.limbDLCount <= CW_ANIM_MAX_LIMB_DLS);
        assert(info.partCount <= CW_ANIM_MAX_PARTS && info.matrixCount <= CW_ANIM_MAX_MATRICES);
        assert(info.colorSegmentCount <= 6);
        assert(info.fallbackTokenPath && info.flameDlPath && info.flameHasSeg);
        assert(info.skelPath || ++composites);
        enemySimple = 1;
        assert(!!Describe(identity.id,&info) == npc);
        npcSimple = 1;
        assert(!Describe(identity.id,&info));
        enemySimple = 0;
        assert(!!Describe(identity.id,&info) == !npc);
        npcSimple = 0;
        assert(Describe(identity.id,&info));
        if(identity.id == RG_SOUL_ENEMY_FREEZARD) {
            // Native EnFz_Draw's intact ice pass must retain transparency,
            // scrolling layers and both constants consumed by its combiner.
            assert(info.partCount == 1 && info.opa == 1);
            const auto& ice = info.parts[0];
            assert(ice.xlu == 1 && ice.materialProfile == CW_ANIM_MATERIAL_FREEZARD);
            assert(ice.hasPrimColor && ice.hasEnvColor && ice.primLodFrac == 128);
            const uint8_t prim[] = {155,255,255,255}, env[] = {200,200,200,255};
            assert(!std::memcmp(ice.primColor,prim,sizeof(prim)));
            assert(!std::memcmp(ice.envColor,env,sizeof(env)));
            assert(info.segCount == 1);
            const auto& scroll = info.segs[0];
            assert(scroll.segment == 8 && scroll.kind == CW_ANIM_SEG_TEXSCROLL);
            assert(scroll.onXlu == 1 && scroll.onOpa == 0);
            assert(scroll.width1 == 32 && scroll.height1 == 32 && scroll.width2 == 32 && scroll.height2 == 32);
            assert(scroll.yStep1 == 1 && scroll.yStep2 == 2 && scroll.yMask1 == 0x7F && scroll.yMask2 == 0x7F);
        }
        if(!dump) continue;
        if(!first) std::puts(","); first=false;
        std::printf("{\"owner\":\"oot\",\"id\":\"%s\",\"category\":\"%s\",\"skelPath\":",identity.name,identity.category);
        path(info.skelPath); std::printf(",\"animPath\":");path(info.animPath);
        std::printf(",\"posePath\":");path(info.posePath);
        std::printf(",\"limbCount\":%d,\"nonFlexSkeleton\":%d,\"scale\":%.6f,\"centerY\":%.3f,\"opa\":%d,\"xlu\":%d,\"bindPose\":%d,\"matrixCount\":%d,\"matrixSegment\":%d,\"segments\":[",info.limbCount,info.nonFlexSkeleton,info.scale,info.translatePre[1],info.opa,info.xlu,info.bindPose,info.matrixCount,info.matrixSegment);
        for(int i=0;i<info.segCount;i++) {
            if(i)std::printf(","); auto& s=info.segs[i];
            std::printf("{\"segment\":%d,\"kind\":%d,\"path\":",s.segment,s.kind);path(s.path);std::printf("}");
        }
        for(int i=0;i<info.colorSegmentCount;i++) {
            if(i || info.segCount)std::printf(",");
            std::printf("{\"segment\":%d,\"kind\":\"color\"}",info.colorSegments[i].segment);
        }
        std::printf("],\"colorSegments\":[");
        for(int i=0;i<info.colorSegmentCount;i++) {
            if(i)std::printf(",");auto& c=info.colorSegments[i];
            std::printf("{\"segment\":%d,\"prim\":[%d,%d,%d,%d],\"env\":[%d,%d,%d,%d]}",c.segment,c.prim[0],c.prim[1],c.prim[2],c.prim[3],c.env[0],c.env[1],c.env[2],c.env[3]);
        }
        std::printf("],\"hiddenLimb\":%d,\"hiddenLimbMask\":%llu,\"limbOverrides\":[",info.hiddenLimb,(unsigned long long)info.hiddenLimbMask);
        for(int i=0;i<info.limbDLCount;i++) {
            if(i)std::printf(","); auto& l=info.limbDLs[i];
            std::printf("{\"limbIndex\":%d,\"hide\":%d,\"postSelf\":%d,\"dlPath\":",l.limbIndex,l.hide,l.postSelf);path(l.dlPath);std::printf(",\"postDlPath\":");path(l.postDlPath);std::printf("}");
        }
        std::printf("],\"parts\":[");
        for(int i=0;i<info.partCount;i++) {
            if(i)std::printf(",");auto& p=info.parts[i];
            std::printf("{\"dlPath\":");path(p.dlPath);std::printf(",\"alternateDlPath\":");path(p.alternateDlPath);
            std::printf(",\"xlu\":%d,\"billboard\":%d,\"beforeSkeleton\":%d,\"materialProfile\":%d,\"framePaths\":[",p.xlu,p.billboard,p.beforeSkeleton,p.materialProfile);
            for(int j=0;j<p.frameCount;j++){if(j)std::printf(",");path(p.framePaths[j]);}
            std::printf("],\"transform\":{\"translate\":[%f,%f,%f],\"rotate\":[%d,%d,%d],\"scale\":[%f,%f,%f]}}",p.transform.translate[0],p.transform.translate[1],p.transform.translate[2],p.transform.rotate[0],p.transform.rotate[1],p.transform.rotate[2],p.transform.scale[0],p.transform.scale[1],p.transform.scale[2]);
        }
        std::printf("],\"flameDlPath\":");path(info.flameDlPath);
        std::printf(",\"fallbackTokenPath\":");path(info.fallbackTokenPath);
        std::printf(",\"texAnimPath\":");path(info.texAnimPath);
        std::printf(",\"texAnimPath2\":");path(info.texAnimPath2);
        std::printf(",\"paths\":[");
        std::vector<const char*> paths{info.skelPath,info.animPath,info.posePath,info.texAnimPath,info.texAnimPath2,info.flameDlPath,info.fallbackTokenPath};
        for(int i=0;i<info.segCount;i++)paths.push_back(info.segs[i].path);
        for(int i=0;i<info.limbDLCount;i++){paths.push_back(info.limbDLs[i].dlPath);paths.push_back(info.limbDLs[i].postDlPath);}
        for(int i=0;i<info.partCount;i++){paths.push_back(info.parts[i].dlPath);paths.push_back(info.parts[i].alternateDlPath);}
        bool firstPath=true;
        for(auto p:paths)if(p){if(!firstPath)std::printf(",");firstPath=false;path(p);}
        std::printf("]}");
    }
    assert(enemies==43 && npcs==46 && composites==7);
    if(dump)std::puts("\n]");
    else std::puts("PASS 89 live descriptors, unique coverage, all four preference combinations, bounded static/palette/material recipes");
}
