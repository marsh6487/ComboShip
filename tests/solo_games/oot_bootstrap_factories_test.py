#!/usr/bin/env python3
"""Execute real ROM-free/full resource registrations at the factory-binding boundary.

Factory constructors and ResourceManager are boundary fixtures; the production
registration calls, types/formats/versions and constructor sequence are verbatim.
No game systems or ROM archives are constructed. A native decoder/load is outside
this test; full production OTRGlobals is checked separately against real headers.
"""
from pathlib import Path
import os,re,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[2]
s=(ROOT/'soh/soh/OTRGlobals.cpp').read_text()
def function(marker):
 a=s.index(marker);b=s.index('{',a);n=1;i=b+1
 while n:n+=(s[i]=='{')-(s[i]=='}');i+=1
 return s[a:i]+'\n'
ctor=s[s.index('    context->InitResourceManager({ portArchivePath }'):s.index('    context->InitConsole();',s.index('OTRGlobals::OTRGlobals()'))]
a=s.index('    auto loader = context->GetResourceManager()->GetResourceLoader();',s.index('void OTRGlobals::Initialize'))
full=s[a:s.index('    Lang::LoadLangs();',a)]
helper=''.join(function(marker) for marker in ('static void RegisterUnboundFactories(',
                                              'static void RegisterRenderingFactories(') if marker in s)
registration=full+helper
cpp=r'''
#include <cassert>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <initializer_list>
constexpr int RESOURCE_FORMAT_BINARY=0,RESOURCE_FORMAT_XML=1,RESOURCE_FORMAT_JSON=2;
struct ResourceFactory{virtual ~ResourceFactory()=default;};
struct Loader{
 std::map<std::tuple<int,std::string,int>,int> counts;
 std::map<std::tuple<int,std::string,int>,std::shared_ptr<ResourceFactory>> factories;
 template<class T>void RegisterResourceFactory(std::shared_ptr<T> factory,int format,std::string name,uint32_t,int version){auto key=std::tuple{format,name,version};++counts[key];factories[key]=factory;}
};
namespace Ship{struct ResourceManager{std::shared_ptr<Loader> loader=std::make_shared<Loader>();std::shared_ptr<Loader> GetResourceLoader(){return loader;}void* GetArchiveManager(){return nullptr;}};
struct CrossRMRegistry{static void Register(const char*,std::shared_ptr<ResourceManager>){};};}
struct Context{std::shared_ptr<Ship::ResourceManager> manager;void InitResourceManager(std::initializer_list<std::string>,std::initializer_list<std::string>,int,bool){manager=std::make_shared<Ship::ResourceManager>();}std::shared_ptr<Ship::ResourceManager> GetResourceManager(){return manager;}};
std::shared_ptr<Ship::ResourceManager> sOOTResourceManager;std::string portArchivePath="soh.o2r";
'''
schema=(ROOT/'soh/soh/unbound/UnboundSchema.h').read_text()
schema_names=sorted(set(re.findall(r'SOH::Unbound::Schema::(\w+)',registration)))
cpp+='namespace SOH::Unbound::Schema{\n'
for name in schema_names:
 declaration=re.search(r'^inline constexpr const char\* '+re.escape(name)+r' = "[^"]*";',schema,re.M)
 if declaration is None:raise RuntimeError('Missing production schema declaration: '+name)
 cpp+=declaration[0]+'\n'
cpp+='}\n'
for namespace in ('Fast','Ship','SOH'):
 values=sorted(set(re.findall(namespace+r'::ResourceType::(\w+)',registration)))
 if values:cpp+=f'namespace {namespace}{{enum class ResourceType{{'+','.join(values)+'};}\n'
for factory in sorted(set(re.findall(r'std::make_shared<([\w:]+)>',registration))):
 namespace,name=factory.rsplit('::',1)
 cpp+=f'namespace {namespace}{{struct {name}:ResourceFactory{{template<class...T>{name}(T&&...){{}}}};}}\n'
cpp+=helper+'void Bootstrap(Context* context){\n'+ctor+'}\nvoid FullFactories(Context* context){\n'+full+'}\n'
cpp+=r'''
int main(){
 Context context;Bootstrap(&context);auto loader=context.GetResourceManager()->GetResourceLoader();
 for(auto key : {std::tuple{0,std::string("Texture"),0},std::tuple{0,std::string("Texture"),1},
                 std::tuple{0,std::string("Vertex"),0},std::tuple{1,std::string("Vertex"),0},
                 std::tuple{0,std::string("DisplayList"),0},std::tuple{1,std::string("DisplayList"),0},
                 std::tuple{0,std::string("Matrix"),0},std::tuple{0,std::string("Blob"),0},
                 std::tuple{0,std::string("Array"),0},std::tuple{0,std::string("Vertex"),1},
                 std::tuple{0,std::string("Array"),1},std::tuple{2,std::string("unbound/scene"),1},
                 std::tuple{2,std::string("unbound/room"),1},std::tuple{2,std::string("unbound/collision"),3},
                 std::tuple{2,std::string("unbound/paths"),1}}){
   assert(loader->counts[key]==1 && "MM-only bootstrap has every authored model resource factory");
 }
 const auto scene=std::tuple{2,std::string("unbound/scene"),1},room=std::tuple{2,std::string("unbound/room"),1};
 assert(loader->factories.at(scene)==loader->factories.at(room) && "scene and room schemas share the native factory");
 FullFactories(&context);
 for(auto& [key,count]:loader->counts)assert(count==1 && "full OoT initialization never duplicates bootstrap bindings");
}
'''
with tempfile.TemporaryDirectory(prefix='oot-bootstrap-factories-') as tmp:
 p=Path(tmp);(p/'test.cpp').write_text(cpp)
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-DCOMBO_BUILD',str(p/'test.cpp'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
print('PASS ROM-free authored model factory bindings and no duplicate full-init registrations')
