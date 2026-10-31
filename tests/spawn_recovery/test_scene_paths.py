"""Exercise the production SceneDB path methods without game assets."""
import pathlib
import subprocess
import tempfile

root = pathlib.Path(__file__).parent
source_root = root / 'candidate' if (root / 'candidate').is_dir() else root.parents[1]
source = (source_root / 'soh/soh/unbound/SceneDB.cpp').read_text()

def function(signature):
    start = source.index(signature)
    end = source.index('{', start) + 1
    depth = 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]

prefix = r'''
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <string>
enum {SCENE_DEKU_TREE=0,SCENE_ICE_CAVERN=9,SCENE_GERUDO_TRAINING_GROUND=11,SCENE_INSIDE_GANONS_CASTLE=13};
namespace StringHelper {
std::string Sprintf(const char* fmt,const char* a,const char* b,const char* c) {
 char out[512];snprintf(out,sizeof(out),fmt,a,b,c);return out;
}}
static bool preferLegacy=false;
namespace SOH::Unbound {bool LegacySceneOverridesJson(const std::string&,const std::string&){return preferLegacy;}}
struct SceneDB {
 struct Entry {bool valid=true,isCustom=false;std::string sceneFileName="spot16_scene",scenePath="custom/scene.json";} entry;
 bool unboundBase=true;
 const Entry& RetrieveEntry(int32_t) const {return entry;}
 std::string GetLegacyScenePath(int32_t,bool) const;
 std::string GetScenePath(int32_t,bool) const;
};
'''
main = r'''
int main() {
 SceneDB db;
 assert(db.GetLegacyScenePath(96,false)=="scenes/shared/spot16_scene/spot16_scene");
 assert(db.GetLegacyScenePath(96,true)==db.GetLegacyScenePath(96,false));
 puts("shared-layout PASS");
 db.entry.sceneFileName="ddan_scene";
 assert(db.GetLegacyScenePath(1,false)=="scenes/nonmq/ddan_scene/ddan_scene");
 assert(db.GetLegacyScenePath(1,true)=="scenes/mq/ddan_scene/ddan_scene");
 puts("native-mq-layout PASS");
 assert(db.GetScenePath(1,false)=="scenes/ddan/scene.json");
 assert(db.GetScenePath(1,true)=="scenes/ddan_mq/scene.json");
 puts("json-mq-layout PASS");
 preferLegacy=true;assert(db.GetScenePath(1,true)==db.GetLegacyScenePath(1,true));
 puts("legacy-override-precedence PASS");
 db.entry.isCustom=true;
 assert(db.GetLegacyScenePath(1,false).empty() && db.GetScenePath(1,false)=="custom/scene.json");
 puts("custom-no-native-path PASS");
 db.entry.valid=false;assert(db.GetLegacyScenePath(1,false).empty() && db.GetScenePath(1,false).empty());
 puts("invalid-no-path PASS");
}
'''
with tempfile.TemporaryDirectory() as folder:
    path = pathlib.Path(folder)
    test = path / 'test.cpp'
    test.write_text(prefix + function('std::string SceneDB::GetLegacyScenePath(') +
                    function('std::string SceneDB::GetScenePath(int32_t id, bool masterQuest)') + main)
    subprocess.run(['c++', '-std=c++20', '-Werror=format', '-fsanitize=undefined', '-fno-sanitize-recover=all',
                    str(test), '-o', str(path / 'test')], check=True)
    subprocess.run([str(path / 'test')], check=True)
