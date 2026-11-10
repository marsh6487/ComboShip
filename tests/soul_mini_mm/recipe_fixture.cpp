static void q(const char* s) {if(!s) std::cout<<"null"; else std::cout<<'"'<<s<<'"';}
static void dump(const char* name, const char* category, const CwItemAnimDrawInfo& d, bool first) {
    if(!first) std::cout<<",\n";
    std::cout<<"{\"owner\":\"mm\",\"id\":";q(name);std::cout<<",\"category\":";q(category);
    std::cout<<",\"skelPath\":";q(d.skelPath);std::cout<<",\"animPath\":";q(d.animPath);
    std::cout<<",\"posePath\":";q(d.posePath);std::cout<<",\"limbCount\":"<<d.limbCount<<",\"scale\":"<<d.scale;
    std::cout<<",\"nonFlexSkeleton\":"<<d.nonFlexSkeleton<<",\"bindPose\":"<<d.bindPose;
    std::cout<<",\"hiddenLimb\":"<<d.hiddenLimb<<",\"hiddenLimbMask\":"<<d.hiddenLimbMask;
    std::cout<<",\"matrixCount\":"<<d.matrixCount<<",\"matrixSegment\":"<<d.matrixSegment;
    std::cout<<",\"texAnimPath\":";q(d.texAnimPath);std::cout<<",\"texAnimPath2\":";q(d.texAnimPath2);
    std::cout<<",\"texAnimStep\":"<<d.texAnimStep<<",\"texAnimStep2\":"<<d.texAnimStep2;
    std::cout<<",\"segments\":[";
    for(int i=0;i<d.segCount;++i){if(i)std::cout<<',';auto& s=d.segs[i];std::cout<<"{\"segment\":"<<s.segment<<",\"kind\":"<<s.kind<<",\"onOpa\":"<<s.onOpa<<",\"onXlu\":"<<s.onXlu<<",\"path\":";q(s.path);std::cout<<'}';}
    std::cout<<"],\"colorSegments\":[";
    for(int i=0;i<d.colorSegmentCount;++i){if(i)std::cout<<',';std::cout<<d.colorSegments[i].segment;}
    std::cout<<"],\"limbDLs\":[";
    for(int i=0;i<d.limbDLCount;++i){if(i)std::cout<<',';auto& l=d.limbDLs[i];std::cout<<"{\"limbIndex\":"<<l.limbIndex<<",\"hide\":"<<l.hide<<",\"postSelf\":"<<l.postSelf<<",\"dlPath\":";q(l.dlPath);std::cout<<",\"postDlPath\":";q(l.postDlPath);std::cout<<'}';}
    std::cout<<"],\"parts\":[";
    for(int i=0;i<d.partCount;++i){if(i)std::cout<<',';auto& p=d.parts[i];std::cout<<"{\"dlPath\":";q(p.dlPath);std::cout<<",\"alternateDlPath\":";q(p.alternateDlPath);std::cout<<",\"xlu\":"<<p.xlu<<",\"materialProfile\":"<<p.materialProfile<<",\"framePaths\":[";for(int f=0;f<p.frameCount;++f){if(f)std::cout<<',';q(p.framePaths[f]);}std::cout<<"]}";}
    std::cout<<"],\"flameDlPath\":";q(d.flameDlPath);std::cout<<",\"fallbackTokenPath\":";q(d.fallbackTokenPath);std::cout<<'}';
}
int main(int argc,char**) {
    const bool dumping=argc>1;
    if(dumping)std::cout<<"[\n";
    bool first=true;
    for(auto& e:expected) {
        CwItemAnimDrawInfo d{};
        assert(MM_FillNpcSoulAnim(e.id,&d)==1);
        assert(d.skelPath && std::strcmp(d.skelPath,e.skel)==0);
        assert(d.limbCount==e.limbs && d.opa==1 && d.scale>0.f);
        assert((!e.anim && d.bindPose && d.posePath) || (e.anim && d.animPath && !std::strcmp(d.animPath,e.anim)));
        assert(d.stateDependent && d.fallbackTokenPath && d.flameDlPath);
        if(dumping){dump(e.name,"npc",d,first);first=false;}
        // Both toggles, in the same logical frame, independently affect only their category.
        ComboSoulModels::enemy=true;
        d={};assert(MM_FillNpcSoulAnim(e.id,&d)==1);
        ComboSoulModels::npc=true;
        d={};assert(MM_FillNpcSoulAnim(e.id,&d)==0);
        ComboSoulModels::enemy=false;d={};assert(MM_FillNpcSoulAnim(e.id,&d)==0);
        ComboSoulModels::npc=false;
    }
    for(int i=0;i<47;++i) {
        auto id=static_cast<RandoItemId>(i);CwItemAnimDrawInfo d{};
        assert(MM_FillEnemySoulAnim(id,&d)==1);
        assert((d.skelPath && d.animPath && d.limbCount>0) || d.partCount>0);
        assert(d.stateDependent && d.fallbackTokenPath && d.flameDlPath);
        assert(d.segCount<=CW_ANIM_MAX_SEGS && d.partCount<=CW_ANIM_MAX_PARTS && d.matrixCount<=CW_ANIM_MAX_MATRICES);
        if(id==RI_SOUL_ENEMY_BAD_BAT)assert(d.parts[2].frameCount==9);
        if(id==RI_SOUL_ENEMY_LIKE_LIKE)assert(d.matrixCount==4 && d.matrixSegment==12);
        if(id==RI_SOUL_ENEMY_GOMESS)assert(d.texAnimPath && d.texAnimPath2 && d.texAnimStep==24);
        if(id==RI_SOUL_ENEMY_DEXIHAND)assert(d.partCount==2 && d.skeletonTranslate[1]==2500.f);
        if(id==RI_SOUL_ENEMY_IRON_KNUCKLE)assert(d.segCount==3 && d.limbDLs[0].postXlu);
        if(id==RI_SOUL_ENEMY_FREEZARD)assert(d.parts[0].materialProfile==CW_ANIM_MATERIAL_FREEZARD);
        if(dumping){dump(enemyNames[i],"enemy",d,first);first=false;}
        ComboSoulModels::npc=true;d={};assert(MM_FillEnemySoulAnim(id,&d)==1);
        ComboSoulModels::enemy=true;d={};assert(MM_FillEnemySoulAnim(id,&d)==0);
        ComboSoulModels::npc=false;d={};assert(MM_FillEnemySoulAnim(id,&d)==0);
        ComboSoulModels::enemy=false;
    }
    // Execute the actual native ID helper and enemy/boss dispatcher.
    for(int i=0;i<47;++i) {
        auto id=static_cast<RandoItemId>(i); soulDrawMap[id]=[]{++legacy;};
        const int oldRender=rendered,oldToken=tokens,oldLegacy=legacy;
        DrawSoul(id);assert(rendered==oldRender+1 && tokens==oldToken && legacy==oldLegacy);
        ComboSoulModels::enemy=true;DrawSoul(id);assert(tokens==oldToken+1 && rendered==oldRender+1);
        ComboSoulModels::enemy=false;failRenderer=true;DrawSoul(id);assert(tokens==oldToken+2);
        failRenderer=false;assert(pushes==pops);
    }
    soulDrawMap[RI_SOUL_BOSS_GOHT]=[]{++legacy;};const int oldLegacy=legacy;
    ComboSoulModels::enemy=true;DrawSoul(RI_SOUL_BOSS_GOHT);assert(legacy==oldLegacy+1);
    ComboSoulModels::enemy=false;
    for(auto& e:expected) {
        const int oldRender=rendered;
        assert(ComboDrawNativeMmSoul(e.id)==1 && rendered==oldRender+1);
        ComboSoulModels::npc=true;assert(ComboDrawNativeMmSoul(e.id)==0 && rendered==oldRender+1);
        ComboSoulModels::npc=false;
    }
    assert(pushes==pops);
    CwItemAnimDrawInfo d{};assert(!MM_FillNpcSoulAnim(RI_UNKNOWN,&d));assert(!MM_FillEnemySoulAnim(RI_UNKNOWN,&d));
    if(dumping)std::cout<<"\n]\n";else std::cout<<"PASS: production MM 53 NPC + 47 enemy recipes; independent same-frame category toggles; special geometry/material contracts; actual native ID dispatch/fallback/boss preservation\n";
}
