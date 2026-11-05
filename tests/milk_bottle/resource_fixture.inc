// Controlled implementation of the pinned LUS API boundary. Method names and
// signatures are taken from 42f280db's native headers, not invented engine APIs.
namespace Ship {
struct ResourceInitData {};
struct IResource { virtual ~IResource()=default; std::vector<std::string> dependencies; };
struct Archive { std::string path; std::unordered_map<std::string,std::shared_ptr<IResource>> files; };
struct ArchiveManager { std::shared_ptr<std::vector<std::shared_ptr<Archive>>> archives=std::make_shared<std::vector<std::shared_ptr<Archive>>>();
 std::shared_ptr<std::vector<std::shared_ptr<Archive>>> GetArchives() { return archives; }
};
struct ResourceManager {
 std::shared_ptr<ArchiveManager> manager=std::make_shared<ArchiveManager>();
 std::shared_ptr<Archive> pinned;
 bool alt=false,ready=true;
 std::shared_ptr<ArchiveManager> GetArchiveManager() { return manager; }
 std::shared_ptr<ResourceManager> CreateResourceView(const std::shared_ptr<Archive>& archive) {
  if (!ready) return nullptr;
  auto view=std::make_shared<ResourceManager>(); view->manager=manager; view->pinned=archive; return view;
 }
 std::shared_ptr<IResource> LoadResource(const std::string& path, bool=false, std::shared_ptr<ResourceInitData> = nullptr) {
  std::string key=path; if (key.starts_with("__OTR__")) key.erase(0,7);
  if (pinned) return pinned->files.contains(key)?pinned->files[key]:nullptr;
  auto load=[&](const std::string& k) -> std::shared_ptr<IResource> { for(auto it=manager->archives->rbegin();it!=manager->archives->rend();++it) if((*it)->files.contains(k)) return (*it)->files[k]; return nullptr; };
  if(alt) if(auto r=load("alt/"+key)) return r;
  return load(key);
 }
 bool IsAltAssetsEnabled() { return alt; }
};
struct Context { static inline Context* instance=nullptr; std::shared_ptr<ResourceManager> active;
 static Context* GetRawInstance() { return instance; }
 std::shared_ptr<ResourceManager> GetResourceManager() { return active; }
 void SetResourceManager(std::shared_ptr<ResourceManager> rm) { active=rm; }
};
struct CrossRMRegistry { static inline std::unordered_map<std::string,std::shared_ptr<ResourceManager>> map;
 static void Register(const std::string& key,std::shared_ptr<ResourceManager> rm) { map[key]=rm; }
 static void Unregister(const std::string& key) { map.erase(key); }
 static std::shared_ptr<ResourceManager> Get(const std::string& key) { return map.contains(key)?map[key]:nullptr; }
};
struct ResourceManagerScope { std::shared_ptr<ResourceManager> previous;
 ResourceManagerScope(std::shared_ptr<ResourceManager> target) { previous=Context::instance->active; Context::instance->active=target; }
 ~ResourceManagerScope() { Context::instance->active=previous; }
};
}
namespace Fast { struct DisplayList : Ship::IResource { std::vector<Gfx> Instructions; }; }
namespace NeiAssetPriority {
bool IsModAsset(const char*,const char*,const char* path) {
 auto mm=Ship::Context::instance->active;
 assert(mm==Ship::CrossRMRegistry::Get("mm"));
 return mm->alt && (strstr(path,"Contents")?contentsMod:glassMod);
}
}
int diagnosticCount=0;
#define SPDLOG_WARN(...) (++diagnosticCount)
#define SPDLOG_INFO(...) (++diagnosticCount)
