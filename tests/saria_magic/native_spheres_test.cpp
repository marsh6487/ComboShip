// Compile the actual GeneratePlaythrough body. Region evaluation is the boundary:
// its winning itemSphere deliberately omits magic still in pending item effects.
#include <cassert>
#include <vector>
#include <list>
#include <memory>
using RandomizerCheck=int;
constexpr int RG_NONE=0,RG_MAGIC_STAT_UPGRADE=7;
struct Location{int check,item;int GetRandomizerCheck(){return check;}int GetPlacedRandomizerGet(){return item;}};
Location locations[]={{0,1},{1,7},{2,7}};
struct Logic{void Reset(){}} logicStorage;
auto logic=&logicStorage;
struct Shuffler{std::vector<std::list<int>> playthroughEntrances;bool HasNoRandomEntrances(){return true;}};
struct Context{bool playthroughBeatable=false;std::vector<std::vector<int>> playthroughLocations;Shuffler shuffler;Shuffler* GetEntranceShuffler(){return &shuffler;}static auto GetInstance(){static auto c=std::make_shared<Context>();return c;}};
namespace Rando{using Context=::Context;}
struct GetAccessibleLocationsStruct{std::vector<int> regionPool{0},itemSphere,lateLocations;std::vector<Location*> newItemLocations;std::list<int> entranceSphere;bool resetSphere=false,logicUpdated=false;GetAccessibleLocationsStruct(int){}void InitLoop(){newItemLocations.clear();itemSphere.clear();logicUpdated=false;}};
int GetMaxGSCount(){return 0;}int RegionTable(int x){return x;}void ResetLogic(std::shared_ptr<Context>&,GetAccessibleLocationsStruct&,bool){}
int phase=0;
void ProcessRegion(int,GetAccessibleLocationsStruct& g,int,bool,bool){
 if(phase==0){g.newItemLocations={&locations[0],&locations[1]};g.itemSphere={0};Context::GetInstance()->playthroughBeatable=true;g.logicUpdated=true;}
 if(phase==1){g.newItemLocations={&locations[2]};g.logicUpdated=true;}
 ++phase;
}
std::vector<std::vector<int>> captured;
void InvalidateSariaMagicHintLocation(){captured.clear();}
void CaptureSariaMagicHintLocation(const std::vector<std::vector<int>>& spheres){captured=spheres;}
/* GENERATE_PLAYTHROUGH */
int main(){GeneratePlaythrough();assert(captured.size()==3);assert(captured[0]==std::vector<int>{1});assert(captured[1]==std::vector<int>{2});assert(captured[2].empty());assert(Context::GetInstance()->playthroughLocations==std::vector<std::vector<int>>{{0}});}
