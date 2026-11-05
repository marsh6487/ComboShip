// Engine services are recording boundaries; all selection/routing bodies are native.
using json = nlohmann::json;
using u32 = uint32_t;
constexpr int RCTYPE_GOSSIP_STONE=1, ACTORCAT_MAX=12;
constexpr int RSK_GOSSIP_STONE_HINTS=0, RO_GOSSIP_STONES_NEED_TRUTH=1,
              RO_GOSSIP_STONES_NEED_STONE=2, PLAYER_MASK_TRUTH=1, QUEST_STONE_OF_AGONY=0;
struct Location {
    int type=0; RandomizerCheck rc=RC_UNKNOWN_CHECK;
    int GetRCType() const { return type; }
    RandomizerCheck GetRandomizerCheck() const { return rc; }
};
namespace Rando {
namespace StaticData {
    std::array<Location,RC_MAX> locationTable{};
    extern std::unordered_map<RandomizerCheck,RandomizerHint> gossipStoneCheckToHint;
    extern std::unordered_map<u32,RandomizerHint> stoneParamsToHint,grottoChestParamsToHint;
    std::unordered_map<std::string,RandomizerCheck> locationNameToEnum;
    std::vector<RandomizerCheck> GetGossipStoneLocations();
}
}
struct Actor { int id=0; int16_t params=0; };
struct Player { Actor* talkActor=nullptr; } player;
struct PlayState { struct { struct { int length=0; Actor* head=nullptr; } actorLists[ACTORCAT_MAX]; } actorCtx; } play;
PlayState* gPlayState=&play;
#define GET_PLAYER(play) (&player)
int gossipMode=0, mask=0; bool hasStone=false;
struct Option { bool Is(int value)const{return gossipMode==value;} };
#define RAND_GET_OPTION(key) Option{}
#define CHECK_QUEST_ITEM(key) (hasStone)
int Player_GetMask(PlayState*) { return mask; }
json fontMessages;
void CustomMessage::LoadIntoFont() {
    fontMessages={{"en",GetEnglish(MF_RAW)},{"de",GetGerman(MF_RAW)},{"fr",GetFrench(MF_RAW)}};
}
struct StoredHint {
    std::vector<CustomMessage> messages;
    CustomMessage GetHintMessage(MessageFormat,size_t id) const {
        auto result=messages.at(id); result.SetSingularPlural(); return result;
    }
};
struct Context {
    std::map<RandomizerHint,StoredHint> hints;
    const StoredHint* GetHint(RandomizerHint id) const {return &hints.at(id);}
} context;
struct OTRGlobals {
    static OTRGlobals* Instance;
    Context* gRandoContext=&context;
} globals;
OTRGlobals* OTRGlobals::Instance=&globals;
std::vector<RandomizerHint> revealed;
struct GameInteractor {
    struct OnRandoHintRevealed {};
    static GameInteractor* Instance;
    template<class T> void ExecuteHooks(RandomizerHint hint) { revealed.push_back(hint); }
} interactor;
GameInteractor* GameInteractor::Instance=&interactor;
