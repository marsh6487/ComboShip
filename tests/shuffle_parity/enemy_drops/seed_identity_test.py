"""Execute actual native playthrough hash body/FNV with an isolated UI boundary."""
from pathlib import Path
import os,re,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[3]

def run():
 native=(ROOT/'soh/soh/Enhancements/randomizer/3drando/playthrough.cpp').read_text()
 settings=(ROOT/'soh/soh/Enhancements/randomizer/settings.cpp').read_text()
 # Supply the exposed option through the actual generation group schema. The
 # previous hand-authored group omitted it and could not detect double hashing
 # when a hidden option became a menu entry.
 shuffle=re.search(r'mOptionGroups\[RSG_SHUFFLE\]\s*=\s*OptionGroup\(.*?\}\);',settings,re.S)[0]
 keys=re.findall(r'&mOptions\[(RSK_\w+)\]',shuffle)
 assert keys.count('RSK_SHUFFLE_ENEMY_DROPS') == 1, 'first drops missing from native generation group'
 menu=re.search(r'mOptionGroups\[RSG_MENU_SECTION_BASIC_SHUFFLES\].*?WidgetContainerType::SECTION\);',settings,re.S)[0]
 assert menu.count('&mOptions[RSK_SHUFFLE_ENEMY_DROPS]') == 1, 'first drops missing from real menu'
 declaration=re.search(r'OPT_BOOL\(RSK_SHUFFLE_ENEMY_DROPS,.*?\);',settings,re.S)[0]
 assert not re.search(r'RO_GENERIC_OFF,\s*true',declaration), 'first drops still hidden'
 randomize=settings[settings.index('void Settings::RandomizeAllSettings()'):]
 assert not re.search(r'case RSK_SHUFFLE_ENEMY_DROPS\s*:',randomize), 'first drops excluded from random settings'
 start=native.index('    std::string settingsStr;');end=native.index('    int ret = Fill();',start)
 body=native[start:end]
 util=(ROOT/'soh/soh/util.cpp').read_text();a=util.index('uint32_t SohUtils::Hash(');b=util.index('\n}',a)+2
 option=(ROOT/'soh/soh/Enhancements/randomizer/option.h').read_text()
 enums='\n'.join(re.search(r'enum class '+name+r'\s*\{.*?\};',option,re.S)[0] for name in ('OptionCategory','OptionGroupType'))
 source=r'''
#include "soh/soh/Enhancements/randomizer/randomizerEnums.h"
#include <array>
#include <cstdint>
#include <vector>
#include <string>
#include <cstdio>
#include <cstdlib>
#define REQUIRE(x) do{if(!(x)){fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#x);abort();}}while(0)
#define CVAR_RANDOMIZER_SETTING(x) x
bool noSpoiler=false;int CVarGetInteger(const char*,int){return noSpoiler;}
const char gBuildVersion[]="fixture-build";
uint32_t rngSeed;void Random_Init(uint32_t seed){rngSeed=seed;}
namespace SohUtils{uint32_t Hash(std::string);}
'''+util[a:b]+r'''
namespace Rando {
'''+enums+r'''
// These facades supply already serialized UI choices. The algorithm and hash
// under test are the untouched production bodies, not a recreated hash policy.
struct Value{uint8_t value=0;uint8_t Get(){return value;}};
struct Option{
 RandomizerSettingKey key=RSK_FOREST;
 OptionCategory category=OptionCategory::Setting;
 OptionCategory GetCategory(){return category;}
 size_t GetOptionCount(){return 2;}
 RandomizerSettingKey GetKey(){return key;}
 std::string GetOptionText(uint8_t value){return value?"On":"Off";}
};
struct LocationOption:Option{RandomizerCheck GetKey(){return RC_UNKNOWN_CHECK;}};
struct TrickSetting:Option{RandomizerTrick GetKey(){return (RandomizerTrick)0;}};
struct Group{
 OptionGroupType type=OptionGroupType::SUBGROUP;
 std::vector<Option*>options;
 OptionGroupType GetContainsType(){return type;}
 std::vector<Option*>&GetOptions(){return options;}
};
struct Settings{
 Option legacy,firstDrops;std::array<Group,RSG_MAX>groups;
 Settings(){
  firstDrops.key=RSK_SHUFFLE_ENEMY_DROPS;
  groups[0].type=OptionGroupType::DEFAULT;groups[0].options={&legacy};
  groups[RSG_SHUFFLE].type=OptionGroupType::DEFAULT;groups[RSG_SHUFFLE].options={&firstDrops};
 }
 static Settings*GetInstance(){static Settings value;return &value;}
 auto&GetOptionGroups(){return groups;}
};
}
struct HashContext{
 std::array<Rando::Value,RSK_MAX>options;Rando::Value unused;
 uint32_t seed=0;std::string hash;
 auto&GetOption(RandomizerSettingKey key){return options[key];}
 auto&GetLocationOption(RandomizerCheck){return unused;}
 auto&GetTrickOption(RandomizerTrick){return unused;}
 uint32_t GetSeed(){return seed;}
 void SetHash(std::string value){hash=value;}
} state;auto*ctx=&state;
uint32_t NativeSettingsHash(){
'''+body+r'''
 return finalHash;
}
int main(){
 for(auto seed:{0u,1u,20261109u,0xffffffffu})for(bool version:{false,true}){
  state.seed=seed;noSpoiler=version;state.GetOption(RSK_FOREST).value=0;
  state.GetOption(RSK_SHUFFLE_ENEMY_DROPS).value=0;
  auto expected=SohUtils::Hash(std::to_string(seed)+"Off"+(version?gBuildVersion:""));
  auto off=NativeSettingsHash();REQUIRE(off==expected);REQUIRE(rngSeed==off);REQUIRE(state.hash==std::to_string(off));
  state.GetOption(RSK_SHUFFLE_ENEMY_DROPS).value=1;auto on=NativeSettingsHash();
  REQUIRE(on==SohUtils::Hash(std::to_string(seed)+"Off|native-first-enemy-drops=1|"+(version?gBuildVersion:"")));
  REQUIRE(on!=off);REQUIRE(on==NativeSettingsHash());REQUIRE(rngSeed==on);
  state.GetOption(RSK_SHUFFLE_ENEMY_DROPS).value=0;REQUIRE(NativeSettingsHash()==off);
  state.GetOption(RSK_FOREST).value=1;REQUIRE(NativeSettingsHash()!=off);
 }
 // Exclusion keys inhabit their own enum and may have the same numeric value
 // as an appended setting. They must still serialize their ordinary choice.
 noSpoiler=false;state.seed=1;state.GetOption(RSK_FOREST).value=0;
 state.GetOption(RSK_SHUFFLE_ENEMY_DROPS).value=0;
 Rando::LocationOption collision;collision.key=RSK_SHUFFLE_ENEMY_DROPS;
 collision.category=Rando::OptionCategory::LocationExclusion;
 auto&group=Rando::Settings::GetInstance()->groups[RSG_EXCLUDES_KOKIRI_FOREST];
 group.type=Rando::OptionGroupType::DEFAULT;group.options={&collision};
 REQUIRE(NativeSettingsHash()==SohUtils::Hash("1OffOff"));
 puts("PASS native settings hash/FNV: deterministic On identity, exact legacy Off, existing setting/version controls");
}
'''
 with tempfile.TemporaryDirectory(prefix='enemy-drop-native-hash-') as directory:
  directory=Path(directory);cpp=directory/'hash.cpp';cpp.write_text(source)
  for mode in ('original','combo'):
   exe=directory/mode
   subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-I'+str(ROOT),*(['-DCOMBO_BUILD'] if mode=='combo' else []),str(cpp),'-o',str(exe)],check=True)
   subprocess.run([str(exe)],check=True)

if __name__=='__main__':run()
