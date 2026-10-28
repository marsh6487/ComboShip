#!/usr/bin/env python3
"""Exercise Unbound's native width and layered scene-format boundaries."""
from pathlib import Path
import os
import re
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]


def function(source, signature):
    start = source.index(signature)
    opening = source.index("{", start)
    depth, end = 1, opening + 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[start:end]


def compile_run(test, body, name):
    with tempfile.TemporaryDirectory(prefix="unbound-" + name + "-") as tmp:
        src = Path(tmp) / (name + ".cpp")
        src.write_text(body)
        exe = Path(tmp) / name
        includes = [ROOT, ROOT / "soh/include", ROOT / "soh", ROOT / "libultraship/include"]
        if os.environ.get("UNBOUND_JSON_INCLUDE"):
            includes.insert(0, Path(os.environ["UNBOUND_JSON_INCLUDE"]))
        command = ["g++", "-std=c++20", "-DF3DEX_GBI_2"]
        for include in includes:
            command += ["-I", str(include)]
        subprocess.run(command + [str(src), "-o", str(exe)], check=True)
        run = subprocess.run([str(exe)], capture_output=True, text=True)
        test.assertEqual(run.returncode, 0, run.stderr)


class NativeWidths(unittest.TestCase):
    def test_dyna_growth_never_allocates_the_static_scene_sentinel(self):
        source = (ROOT / "soh/src/code/z_bgcheck.c").read_text()
        header = (ROOT / "soh/include/z64bgcheck.h").read_text()
        constants = "\n".join(re.findall(r"^#define (?:BGCHECK_SCENE|BGACTOR_INVALID|BGACTOR_INITIAL_MAX)[^\n]+", header, re.M))
        body = '''#include <cassert>
#include <cstdint>
#include <vector>
#define osSyncPrintf(...) ((void)0)
#define MIN(a,b) ((a)<(b)?(a):(b))
#define DYNAPOLY_INVALIDATE_LOOKUP 1
using s32=int32_t;
struct Actor {};struct CollisionHeader {};struct PlayState {};
struct BgActor { Actor* actor=nullptr; };
struct DynaCollisionContext { s32 bgActorMax=0;std::vector<unsigned> bgActorFlags;std::vector<BgActor> bgActors;unsigned bitFlag=0; };
static unsigned initialized=0;
void DynaPoly_GrowBgActorTable(PlayState*,DynaCollisionContext* d,s32 n) { d->bgActorMax=n;d->bgActorFlags.resize(n);d->bgActors.resize(n); }
void BgActor_SetActor(BgActor* b,Actor* a,CollisionHeader*) { b->actor=a;++initialized; }
''' + constants + "\n" + function(source, "s32 DynaPoly_SetBgActor(")
        body += '''int main() {
    PlayState play;Actor actor;CollisionHeader header;DynaCollisionContext d;
    DynaPoly_GrowBgActorTable(&play,&d,64);std::fill(d.bgActorFlags.begin(),d.bgActorFlags.end(),1);
    assert(DynaPoly_SetBgActor(&play,&d,&actor,&header)==64 && d.bgActorMax==128);
    DynaPoly_GrowBgActorTable(&play,&d,16384);std::fill(d.bgActorFlags.begin(),d.bgActorFlags.end(),1);
    assert(DynaPoly_SetBgActor(&play,&d,&actor,&header)==16384 && d.bgActorMax==32767);
    std::fill(d.bgActorFlags.begin(),d.bgActorFlags.end(),1);unsigned before=initialized;
    assert(DynaPoly_SetBgActor(&play,&d,&actor,&header)==BGACTOR_INVALID);
    assert(d.bgActorMax==32767 && initialized==before);
    d.bgActorFlags[100]=0;assert(DynaPoly_SetBgActor(&play,&d,&actor,&header)==100);
}
'''
        compile_run(self, body, "dyna-sentinel")

    def test_custom_title_card_uses_registry_path_in_every_language(self):
        source = (ROOT / "soh/src/code/z_actor.c").read_text()
        native = function(source, "void TitleCard_InitPlaceName(")
        scenes = sorted(set(re.findall(r"case (SCENE_[A-Z0-9_]+):", native)))
        textures = sorted(set(re.findall(r"texture = (g[A-Za-z0-9_]+);", native)))
        body = '''#include <cassert>
#include <cstring>
using s32=int;
enum { LANGUAGE_ENG,LANGUAGE_FRA,LANGUAGE_GER,LANGUAGE_JPN };
struct { int language; } gSaveContext;
struct PlayState { int sceneNum; };
struct TitleCardContext { void* texture;bool isBossCard,hasTranslation;int x,y,width,height,durationTimer,delayTimer; };
size_t strlen(void* p) { return std::strlen(static_cast<const char*>(p)); }
char* strcpy(char* a,void* b) { return std::strcpy(a,static_cast<const char*>(b)); }
const char customPath[]="__OTR__custom/mod/title_card";
const char* SceneDB_GetTitleCardTexture(int id) { return id==128?customPath:nullptr; }
'''
        body += "enum {" + ",".join(scenes) + "};\n"
        body += "\n".join('char ' + name + '[]="__OTR__titleCardENGTex";' for name in textures) + "\n"
        body += native
        body += '''int main() {
    PlayState p{128};TitleCardContext title{};
    for(int language=0;language<4;++language) {
        gSaveContext.language=language;
        TitleCard_InitPlaceName(&p,&title,nullptr,160,180,144,24,7);
        assert(title.texture==customPath && !title.isBossCard && title.delayTimer==7);
        assert(std::strcmp(static_cast<const char*>(title.texture),"__OTR__custom/mod/title_card")==0);
    }
    p.sceneNum=SCENE_HYRULE_FIELD;gSaveContext.language=LANGUAGE_ENG;
    TitleCard_InitPlaceName(&p,&title,nullptr,160,180,144,24,7);
    assert(title.texture==gHyruleFieldTitleCardENGTex);
    p.sceneNum=999;TitleCard_InitPlaceName(&p,&title,nullptr,160,180,144,24,7);
    assert(title.texture==nullptr);
}
'''
        compile_run(self, body, "custom-title")

    def test_actor_admission_stops_at_live_capacity(self):
        source = (ROOT / "soh/src/code/z_actor.c").read_text()
        header = (ROOT / "soh/include/z64actor.h").read_text()
        cap = re.search(r"^#define ACTOR_NUMBER_MAX[^\n]+", header, re.M).group()
        gate = function(source, "if (actorCtx->total ")
        gate = gate.replace("return NULL;", "return false;")
        body = '''#include <cassert>
#define LUSLOG_WARN(...) ((void)0)
#define osSyncPrintf(...) ((void)0)
struct ActorContext { unsigned total; };
''' + cap + "\nbool CanAdmit(ActorContext* actorCtx) {\n" + gate + "\nreturn true; }\n"
        body += '''int main() {
    ActorContext c{8191};assert(CanAdmit(&c));
    c.total=8192;assert(!CanAdmit(&c));
    c.total=8193;assert(!CanAdmit(&c));
}
'''
        compile_run(self, body, "actor-cap")

    def test_entrance_bounds_reject_negative_and_overflowing_layer_indices(self):
        source = (ROOT / "soh/soh/unbound/SceneDB.cpp").read_text()
        signature = 'extern "C" int32_t EntranceDB_IsIndexValid('
        if signature not in source:
            self.fail("entrance validation API is absent before native table access")
        body = '''#include <cassert>
#include <cstdint>
#include <climits>
int32_t EntranceDB_GetEntryCount() { return 1560; }
''' + function(source, signature)
        body += '''int main() {
    assert(EntranceDB_IsIndexValid(1556,3));
    assert(!EntranceDB_IsIndexValid(1557,3));
    assert(!EntranceDB_IsIndexValid(-1,0));
    assert(!EntranceDB_IsIndexValid(0,-1));
    assert(!EntranceDB_IsIndexValid(INT_MAX,INT_MAX));
}
'''
        compile_run(self, body, "entrance-bounds")

    def test_room_actor_and_collision_counts_do_not_wrap(self):
        # A reverted narrow game declaration must lose the 300th actor/room or
        # the 70000th collision primitive. Compile those actual declarations,
        # with only the unrelated PlayState dependencies omitted.
        declarations = []
        for file, names in [("z64.h", ["total", "numSetupActors", "numRooms"]),
                            ("z64bgcheck.h", ["numVertices", "numPolygons"])]:
            source = (ROOT / "soh/include" / file).read_text()
            for name in names:
                match = re.search(r"\b(?:u|s)(?:8|16|32|64)\s+" + name + r"\s*;", source)
                self.assertIsNotNone(match, f"native {name} declaration missing")
                declarations.append(match.group())
        body = """#include <cstdint>
#include <cstdio>
using u8=uint8_t; using u16=uint16_t; using u32=uint32_t; using u64=uint64_t;
using s8=int8_t; using s16=int16_t; using s32=int32_t; using s64=int64_t;
struct Counts { %s };
int main() {
    Counts c{};
    unsigned actors=300, polygons=70000;
    c.total=actors; c.numSetupActors=actors; c.numRooms=actors;
    c.numVertices=polygons; c.numPolygons=polygons;
    if(c.total!=300 || c.numSetupActors!=300 || c.numRooms!=300 ||
       c.numVertices!=70000 || c.numPolygons!=70000) {
        std::fprintf(stderr,"native count wrapped: %%u/%%u/%%u/%%u/%%u\\n",
          unsigned(c.total),unsigned(c.numSetupActors),unsigned(c.numRooms),
          unsigned(c.numVertices),unsigned(c.numPolygons));
        return 1;
    }
}
""" % " ".join(declarations)
        with tempfile.TemporaryDirectory(prefix="unbound-counts-") as tmp:
            src = Path(tmp) / "counts.cpp"
            src.write_text(body)
            exe = Path(tmp) / "counts"
            subprocess.run(["g++", "-std=c++20", str(src), "-o", str(exe)], check=True)
            run = subprocess.run([str(exe)], capture_output=True, text=True)
            self.assertEqual(run.returncode, 0, run.stderr)

    def test_polygon_ignore_flags_keep_entity_and_projectile_semantics(self):
        # The widened polygon stores exclusions in bits 29..31. A leftover
        # vanilla 0x4000/0x8000 mask must fail these actual native queries.
        source = (ROOT / "soh/src/code/z_bgcheck.c").read_text()
        header = (ROOT / "soh/include/z64bgcheck.h").read_text()
        flag_macro = "\n".join(re.findall(r"^#define COLPOLY_(?:VIA_FLAG_TEST|VIA_FLAGS_SHIFT)[^\n]+", header, re.M))
        body = """#include <cstdint>
#include <cassert>
#include <cstddef>
using s32=int32_t; using u32=uint32_t;
struct CollisionContext {}; struct CollisionPoly { u32 flags_vIA; };
static bool headerAvailable=true;
void* BgCheck_GetCollisionHeader(CollisionContext*,s32) { return headerAvailable ? (void*)1 : nullptr; }
""" + flag_macro + "\n"
        for name in ["SurfaceType_IsIgnoredByEntities", "SurfaceType_IsIgnoredByProjectiles"]:
            body += function(source, "s32 " + name + "(") + "\n"
        body += """int main() {
    CollisionContext c; CollisionPoly p{};
    assert(!SurfaceType_IsIgnoredByEntities(&c,&p,0));
    assert(!SurfaceType_IsIgnoredByProjectiles(&c,&p,0));
    p.flags_vIA=1u<<30;
    assert(SurfaceType_IsIgnoredByEntities(&c,&p,0));
    assert(!SurfaceType_IsIgnoredByProjectiles(&c,&p,0));
    p.flags_vIA=1u<<31;
    assert(!SurfaceType_IsIgnoredByEntities(&c,&p,0));
    assert(SurfaceType_IsIgnoredByProjectiles(&c,&p,0));
    p.flags_vIA=0xC000;
    assert(!SurfaceType_IsIgnoredByEntities(&c,&p,0));
    assert(!SurfaceType_IsIgnoredByProjectiles(&c,&p,0));
    headerAvailable=false;
    assert(SurfaceType_IsIgnoredByEntities(&c,&p,0));
    assert(SurfaceType_IsIgnoredByProjectiles(&c,&p,0));
}
"""
        compile_run(self, body, "ignore-flags")


class LegacyRouting(unittest.TestCase):
    def test_actual_linked_scene_graph_keeps_room_collision_alternate_and_alt_overrides(self):
        source = (ROOT / "soh/soh/unbound/LegacySceneRouting.cpp").read_text()
        fixture = (ROOT / "tests/unbound/legacy_routing_fixture.cpp").read_text()
        fixture = fixture.replace("/* ACTUAL LEGACY GRAPH */", function(source, "class LegacyGraph") + ";")
        fixture = fixture.replace("/* ACTUAL LEGACY ROUTING */", function(source, "bool LegacySceneOverridesJson("))
        compile_run(self, fixture, "linked-legacy-routing")

    def test_structured_override_precedence_and_native_bulk_replacements(self):
        body = r'''#include <cstdio>
#if !__has_include("soh/soh/unbound/LegacyScenePolicy.h")
int main() { std::fputs("legacy structured scene compatibility is absent\n", stderr); return 1; }
#else
#include "soh/soh/unbound/LegacyScenePolicy.h"
#include <cassert>
int main() {
    using SOH::Unbound::UseLegacyScene;
    // Vanilla scene/room/collision sources below the generated base use JSON.
    assert(!UseLegacyScene(true, 1, 2));
    // A legacy room or collision override above the base keeps every native
    // actor/setup/header effective; bulk-only mesh edits supply no rank.
    assert(UseLegacyScene(true, 3, 2));
    assert(!UseLegacyScene(true, -1, 2));
    // The later format wins when both encode structured changes to this scene.
    assert(!UseLegacyScene(true, 3, 4));
    // One mod shipping both forms deliberately selects the JSON form.
    assert(!UseLegacyScene(true, 4, 4));
    assert(UseLegacyScene(false, -1, -1));
}
#endif
'''
        compile_run(self, body, "legacy-routing")


class CrossResourceOwnership(unittest.TestCase):
    def test_json_layers_bulk_registry_and_nested_resources_use_oot_while_mm_is_active(self):
        source = (ROOT / "soh/soh/unbound/UnboundJson.cpp").read_text()
        scene = (ROOT / "soh/soh/unbound/UnboundSceneFactory.cpp").read_text()
        fixture = (ROOT / "tests/unbound/cross_rm_fixture.cpp").read_text()
        bodies = "\n".join(function(source, signature) for signature in [
            "static bool IsReplaceDirective(", "void MergeJson(", "static void StripDirectives(",
            "Json LoadMergedJson(", "std::vector<char> LoadBulk(",
        ])
        start = source.index("static constexpr std::string_view kRegistryFileSuffix")
        end = source.index("// The merged document at `file.path`", start)
        bodies += "\n" + source[start:end]
        fixture = fixture.replace("/* ACTUAL JSON LOADERS */", bodies)
        fixture = fixture.replace("/* ACTUAL NESTED RESOURCE LOADER */", function(scene, "std::shared_ptr<Ship::IResource> LoadSub("))
        compile_run(self, fixture, "cross-resource-ownership")


class StructuredDocuments(unittest.TestCase):
    def test_actual_merge_key_order_numbers_and_wide_vectors(self):
        source = (ROOT / "soh/soh/unbound/UnboundJson.cpp").read_text()
        signatures = [
            "static bool IsReplaceDirective(", "void MergeJson(", "static void StripDirectives(",
            "static bool IsIntegerKey(", "static std::vector<std::string> OrderedKeys(",
            "static bool IsReservedKey(", "std::vector<std::string> ListKeys(",
            "std::vector<std::string> PositionalKeys(", "static bool IsDigitOf(",
            "bool ParseIntString(", "bool ParseNumberString(", "int64_t ToInt(",
            "double ToNumber(", "int64_t Field(", "double NumberField(", "s32 IntegralField(",
            "std::string PathField(", "const Json& Sub(", "const Json& SubArray(",
            "std::string SchemaOf(", "bool ParseSchema(", "Vec3s ReadVec3s(",
            "Vec3f ReadVec3f(", "Vec3i ReadVec3i(",
        ]
        # Only archive IO is omitted; these are the production parser/merge
        # bodies and the real nlohmann JSON implementation, without mocks.
        body = '''#include <cassert>
#include <algorithm>
#include <cmath>
#include <cerrno>
#include <cstdlib>
#include <string_view>
#include "soh/soh/unbound/UnboundJson.h"
#include "soh/soh/unbound/UnboundSchema.h"
namespace SOH::Unbound {
''' + "\n".join(function(source, signature) for signature in signatures)
        body += r'''
}
int main() {
    using namespace SOH::Unbound;
    Json base=Json::parse(R"({"actors":{"0":{"id":1,"pos":[1,2,3]},"1":{"id":2}},"water":[1,2]})");
    MergeJson(base,Json::parse(R"({"actors":{"0":{"pos":[4,5,6]},"1":null,"custom":{"id":33}},"water":[9]})"));
    StripDirectives(base);
    assert(base==Json::parse(R"({"actors":{"0":{"id":1,"pos":[4,5,6]},"custom":{"id":33}},"water":[9]})"));
    MergeJson(base,Json::parse(R"({"actors":{"$replace":true,"only":{"id":34}}})"));
    StripDirectives(base);
    assert(base["actors"]==Json::parse(R"({"only":{"id":34}})"));
    Json single=Json::parse(R"({"$replace":true,"a":null,"nested":{"gone":null,"kept":1}})");
    StripDirectives(single);
    assert(single==Json::parse(R"({"nested":{"kept":1}})"));
    assert((ListKeys(Json::parse(R"({"$order":["b","b","missing"],"10":{},"2":{},"-1":{},"a":{},"b":{}})")) ==
            std::vector<std::string>{"b","-1","2","10","a"}));
    assert((PositionalKeys(Json::parse(R"({"0":{},"1":{}})"),"room") == std::vector<std::string>{"0","1"}));
    for(const char* text : {R"({"0":{},"2":{}})",R"({"$order":[],"0":{}})"}) {
        bool rejected=false; try { PositionalKeys(Json::parse(text),"room"); } catch(const DocumentError&) { rejected=true; }
        assert(rejected);
    }
    int64_t integer=0; double number=0;
    assert(ParseIntString("-0x20",integer) && integer==-32);
    for(const char* text : {"1garbage"," 1","1 ","0x","1.5"}) assert(!ParseIntString(text,integer));
    assert(ParseNumberString("-1.25e2",number) && number==-125);
    for(const char* text : {"NaN","inf","1x"," 1","0x1p2"}) assert(!ParseNumberString(text,number));
    assert(ToInt(Json(true))==1 && ToNumber(Json(true),-7)==-7);
    assert(ToInt(Json(2.9))==2 && ToInt(Json(-2.9))==-2);
    auto vector=ReadVec3f(Json::parse("[100000.5,-60000.25,0.125]"));
    assert(vector.x==100000.5f && vector.y==-60000.25f && vector.z==0.125f);
    auto integral=ReadVec3i(Json::parse("[100000.6,-60000.6,0.1]"));
    assert(integral.x==100001 && integral.y==-60001 && integral.z==0);
    std::string type; int version;
    assert(ParseSchema("unbound/collision/3",type,version) && type=="unbound/collision" && version==3);
    assert(!ParseSchema("unbound/collision/3trailing",type,version));
}
'''
        compile_run(self, body, "json-contract")


class MessageOwnership(unittest.TestCase):
    def test_growing_tables_and_combo_resume_keep_owned_message_bytes(self):
        source = (ROOT / "soh/soh/z_message_OTR.cpp").read_text()
        body = r'''#include <array>
#include <cassert>
#include <cstdint>
#include <deque>
#include <string>
#include <unordered_map>
#include <vector>
using u16=uint16_t;using u32=uint32_t;
struct MessageTableEntry { uint16_t textId;uint8_t typePos;const char* segment;uint32_t msgSize; };
namespace SOH { struct MessageEntry { uint16_t id;uint8_t textboxType,textboxYPos;std::string msg; }; }
constexpr uint16_t kTerminatorId=0xFFFF;
constexpr char kMessageEnd='\x02';
enum MessageLanguage { MSG_NES,MSG_GER,MSG_FRA,MSG_JPN,MSG_STAFF,MSG_LANGUAGE_COUNT };
'''
        body += function(source, "struct MessageTable") + ";\n"
        body += r'''
std::array<MessageTable,MSG_LANGUAGE_COUNT> sTables;
bool sInitialized=false;
MessageTableEntry *sNesMessageEntryTablePtr=nullptr,*sGerMessageEntryTablePtr=nullptr,
 *sFraMessageEntryTablePtr=nullptr,*sJpnMessageEntryTablePtr=nullptr,*sStaffMessageEntryTablePtr=nullptr;
char* _message_0xFFFC_nes=nullptr;
'''
        body += function(source, "void PublishTables(") + "\n"
        body += r'''
static unsigned rebuilds=0;
extern "C" void OTRMessage_Init(void) {
    if(sInitialized) return;
    sInitialized=true;++rebuilds;
    auto& table=sTables[MSG_NES];
    table.Set(100+rebuilds,0,"new resume message");
    table.Set(0xFFFC,0,"owned font data");table.Finalize();PublishTables();
}
'''
        body += function(source, 'extern "C" void OTRMessage_ResetForResume(') + "\n"
        body += r'''
int main() {
    MessageTable table;table.Set(1,0,"first");
    const char* first=table.Find(1)->segment;
    for(unsigned i=2;i<10000;++i) table.Set(i,0,"additional");
    assert(std::string(first)==std::string("first\x02"));
    assert(table.Find(9999) && table.Find(9999)->textId==9999);
    table.Set(1,0,"replacement");
    assert(std::string(table.Find(1)->segment)==std::string("replacement\x02"));
    assert(std::string(first)==std::string("first\x02"));
    auto* published=table.Finalize();
    assert(published[table.entries.size()-1].textId==0xFFFF && !table.Find(0xFFFF));
    OTRMessage_Init();assert(rebuilds==1 && sTables[MSG_NES].Find(101));
    OTRMessage_ResetForResume();
    assert(rebuilds==2 && !sTables[MSG_NES].Find(101) && sTables[MSG_NES].Find(102));
    assert(sNesMessageEntryTablePtr==sTables[MSG_NES].entries.data());
    assert(std::string(_message_0xFFFC_nes)==std::string("owned font data\x02"));
    assert(!sGerMessageEntryTablePtr && !sFraMessageEntryTablePtr && !sJpnMessageEntryTablePtr);
}
'''
        compile_run(self, body, "message-ownership")


class ArchiveStartup(unittest.TestCase):
    def test_clean_base_precedes_nei_and_mod_archives_and_preserves_alt_assets(self):
        source = (ROOT / "soh/soh/OTRGlobals.cpp").read_text()
        fixture = (ROOT / "tests/unbound/archive_startup_fixture.cpp").read_text()
        body = "\n".join(function(source, signature) for signature in [
            "static void MountOotSupplementalArchives(", "static bool MountStandaloneUnboundBase(",
            "static void FinishUnboundArchiveSetup(",
        ])
        compile_run(self, fixture.replace("/* ACTUAL ARCHIVE STARTUP */", body), "archive-startup")


class FactoryRegistration(unittest.TestCase):
    def test_unbound_resource_requests_resolve_with_scene_and_room_aliases(self):
        source = (ROOT / "soh/soh/OTRGlobals.cpp").read_text()
        signature = "static void RegisterUnboundFactories("
        if signature not in source:
            self.fail("Unbound resource factories have not been wired into the owning OOT resource manager")
        body = '''#include <cassert>
#include <map>
#include <memory>
#include <string>
#include <tuple>
#include "soh/soh/unbound/UnboundSchema.h"
#define RESOURCE_FORMAT_BINARY 0
#define RESOURCE_FORMAT_JSON 2
struct Factory { virtual ~Factory()=default; };
namespace Fast {
enum class ResourceType { Vertex=1 };
struct ResourceFactoryBinaryVertexV1:Factory {};
}
namespace SOH {
enum class ResourceType { SOH_Array=2,SOH_Room=3,SOH_CollisionHeader=4,SOH_Path=5 };
struct ResourceFactoryBinaryArrayV1:Factory {};
struct ResourceFactoryJsonSceneV1:Factory {};
struct ResourceFactoryJsonCollisionHeaderV3:Factory {};
struct ResourceFactoryJsonPathV1:Factory {};
}
namespace Ship {
struct Loader {
    std::map<std::string,unsigned> types;
    std::map<std::tuple<unsigned,unsigned,unsigned>,std::shared_ptr<Factory>> factories;
    void RegisterResourceFactory(std::shared_ptr<Factory> f,unsigned format,std::string name,unsigned type,unsigned version) {
        auto key=std::make_tuple(format,type,version);
        if(factories.contains(key)) assert(factories[key]==f);
        factories[key]=f;types[name]=type;
    }
    std::shared_ptr<Factory> Resolve(const std::string& name,unsigned format,unsigned version) {
        return factories.at({format,types.at(name),version});
    }
};
struct ResourceManager { std::shared_ptr<Loader> loader=std::make_shared<Loader>();std::shared_ptr<Loader> GetResourceLoader() { return loader; } };
}
''' + function(source, signature)
        body += '''int main() {
    auto owner=std::make_shared<Ship::ResourceManager>();RegisterUnboundFactories(owner);
    auto loader=owner->loader;
    assert(std::dynamic_pointer_cast<Fast::ResourceFactoryBinaryVertexV1>(loader->Resolve("Vertex",0,1)));
    assert(std::dynamic_pointer_cast<SOH::ResourceFactoryBinaryArrayV1>(loader->Resolve("Array",0,1)));
    auto scene=loader->Resolve("unbound/scene",2,1);
    assert(std::dynamic_pointer_cast<SOH::ResourceFactoryJsonSceneV1>(scene));
    assert(scene==loader->Resolve("unbound/room",2,1));
    assert(std::dynamic_pointer_cast<SOH::ResourceFactoryJsonCollisionHeaderV3>(loader->Resolve("unbound/collision",2,3)));
    assert(std::dynamic_pointer_cast<SOH::ResourceFactoryJsonPathV1>(loader->Resolve("unbound/paths",2,1)));
}
'''
        compile_run(self, body, "factory-registration")


if __name__ == "__main__":
    unittest.main()
