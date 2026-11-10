#include "combo/rando/RitoMaskSettingsSnapshot.h"
#include "tests/test_require.h"
#include <map>
#include <string>
static std::map<std::string,int> values;
extern "C" int32_t CVarGetInteger(const char* n,int32_t d) { return values.count(n)?values[n]:d; }
extern "C" void CVarSetInteger(const char* n,int32_t v) { values[n]=v; }
extern "C" void CVarClear(const char* n) { values.erase(n); }
namespace M=ComboRando::RitoMaskSettings;
int main(){
 REQUIRE(M::Preview()==0);
 for(int a:{0,1}) for(int b:{0,1}) for(int s:{0,1}) for(int n:{0,1}){
  values={{M::OotCvar,a},{M::MmCvar,b},{M::SharedCvar,s}};
  REQUIRE(M::Shared()==s); M::PrepareGeneration(); REQUIRE(values[M::GenerationCvar]==s);
  {M::NativePreferenceScope scope(true); CVarSetInteger(M::MmCvar,n);}
  REQUIRE(M::Shared()==n && values[M::OotCvar]==a && values[M::MmCvar]==b);
  nlohmann::json oot={{M::SharedCvar,s}},mm={{M::MmCvar,!s}};
  M::NormalizeSnapshots(oot,mm); REQUIRE(oot[M::OotCvar]==s && mm[M::MmCvar]==s);
  M::RestoreSnapshot(oot); REQUIRE(M::Shared()==s && !oot.contains(M::OotCvar));
 }
 nlohmann::json a=nlohmann::json::object(),b=a; M::NormalizeSnapshots(a,b); REQUIRE(a[M::SharedCvar]==0);
 bool threw=false; a={{M::SharedCvar,1}};b={{M::SharedCvar,0}};try{M::NormalizeSnapshots(a,b);}catch(...){threw=true;} REQUIRE(threw);
 puts("PASS Rito shared/solo preferences, generation snapshots, legacy OFF and conflict rejection");
}
