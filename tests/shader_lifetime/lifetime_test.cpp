// Real XML factory/destructor + production shader handlers. Only the interpreter
// instance/address-resolution boundary is supplied; source strings and commands
// use their actual engine containers and resource classes.
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <vector>
#include <fast/interpreter.h>
#include <fast/resource/factory/DisplayListFactory.h>
#include <fast/resource/type/DisplayList.h>
#include <tinyxml2.h>

static bool failNextMalloc = false;
extern "C" void* __real_malloc(size_t bytes);
extern "C" void* __wrap_malloc(size_t bytes) {
    if (failNextMalloc) { failNextMalloc = false; return nullptr; }
    return __real_malloc(bytes);
}
namespace Fast {
Interpreter::Interpreter() { mRsp=nullptr; mRdp=nullptr; mBufVbo=nullptr; }
Interpreter::~Interpreter() {}
void* Interpreter::SegAddr(uintptr_t address) { return reinterpret_cast<void*>(address); }
int32_t gfx_check_image_signature(const char* path) {
    if (reinterpret_cast<uintptr_t>(path)<0x10000) return 0;
    return std::strncmp(path,"__OTR__",7)==0;
}
static std::weak_ptr<Interpreter> mInstance;
#define SPDLOG_ERROR(...) ((void)0)
/* PRODUCTION_SHADER_HANDLERS */
}
static void Require(bool value,const char* why) {
    if(!value){std::fprintf(stderr,"FAIL: %s\n",why);std::exit(1);}
}
static std::shared_ptr<Fast::DisplayList> Load(const char* xml,bool allocationFailure=false) {
    auto document=std::make_shared<tinyxml2::XMLDocument>();
    Require(document->Parse(xml)==tinyxml2::XML_SUCCESS,"fixture XML must parse");
    auto file=std::make_shared<Ship::File>();file->Reader=document;
    auto init=std::make_shared<Ship::ResourceInitData>();init->Format=RESOURCE_FORMAT_XML;init->Path="shader-lifetime";
    Fast::ResourceFactoryXMLDisplayListV0 factory;
    failNextMalloc=allocationFailure;
    auto dl=std::dynamic_pointer_cast<Fast::DisplayList>(factory.ReadResource(file,init));
    if(allocationFailure)Require(!failNextMalloc,"allocation failure reached the shader copy");
    failNextMalloc=false;
    Require(dl!=nullptr,"factory returns DL on malformed optional shader command");
    return dl; // XML document and its attribute storage retire here.
}
static Gfx* Command(Fast::DisplayList& dl,unsigned opcode) {
    for(auto& command:dl.Instructions)if((command.words.w0>>24)==opcode)return &command;
    return nullptr;
}
static void Push(Fast::Interpreter* runtime,Gfx* command) {
    auto* native=reinterpret_cast<Fast::F3DGfx*>(command);Fast::gfx_push_shader(runtime,&native);
}
static void Pop(Fast::Interpreter* runtime) {
    Gfx command{};auto* native=reinterpret_cast<Fast::F3DGfx*>(&command);Fast::gfx_pop_shader(runtime,&native);
}
int main(int argc,char** argv) {
    const std::string mode=argc>1?argv[1]:"xml";
    if(mode=="xml") {
        auto dl=Load("<DisplayList><PushShader Shader=\"__OTR__shaders/lifetime\"/><PopShader/></DisplayList>");
        auto* command=Command(*dl,G_PUSH_SHADER);Require(command!=nullptr,"valid shader command emitted");
        auto* path=reinterpret_cast<const char*>(command->words.w1);
        Require(std::strcmp(path,"__OTR__shaders/lifetime")==0,"shader path survives XML teardown");
        Require(dl->Strings.size()==1&&dl->Strings[0]==path,"display list owns emitted path");
        dl.reset();
        for(const auto* xml:{"<DisplayList><PushShader/><PopShader/></DisplayList>","<DisplayList><PushShader Shader=\"\"/><PopShader/></DisplayList>"}) {
            auto bad=Load(xml);Require(!Command(*bad,G_PUSH_SHADER)&&bad->Strings.empty(),"missing/empty attribute emits no borrowed shader command");
        }
        auto oom=Load("<DisplayList><PushShader Shader=\"__OTR__shaders/lifetime\"/><PopShader/></DisplayList>",true);
        Require(!Command(*oom,G_PUSH_SHADER)&&oom->Strings.empty(),"OOM skips unsafe command");
        std::puts("shader XML ownership/missing/OOM: PASS");return 0;
    }
    auto runtime=std::make_shared<Fast::Interpreter>();Fast::mInstance=runtime;
    if(mode=="cache") {
        auto dl=Load("<DisplayList><PushShader Shader=\"__OTR__shaders/lifetime\"/></DisplayList>");
        Push(runtime.get(),Command(*dl,G_PUSH_SHADER));
        Require(runtime->mShaderStack.size()==1,"valid push registers a shader scope");
        auto id=runtime->mShaderStack.top();Require(id==0,"first shader ID is initialized deterministically");
        dl.reset();
        Require(std::strcmp(Fast::gfx_get_shader(id),"shaders/lifetime")==0,"shader cache owns path after DL destruction");
        auto second=Load("<DisplayList><PushShader Shader=\"__OTR__shaders/lifetime\"/></DisplayList>");Push(runtime.get(),Command(*second,G_PUSH_SHADER));
        Require(runtime->mShaders.size()==1&&runtime->mShaderStack.top()==id,"equal paths dedupe after source teardown");
        second.reset();Pop(runtime.get());Pop(runtime.get());Require(runtime->mShaderStack.empty(),"valid nested push/pop balances");
        runtime.reset();Require(Fast::gfx_get_shader(id)==nullptr,"expired interpreter lookup is safe");
        std::puts("shader cache DL teardown/deduplication: PASS");return 0;
    }
    if(mode=="invalid") {
        runtime->mShadersIndex=0;
        for(const auto* name:{"not-a-resource","__OTR__",""}) {
            std::string path=name;Gfx command{};command.words.w1=reinterpret_cast<uintptr_t>(path.c_str());
            Push(runtime.get(),&command);Require(runtime->mShaderStack.empty(),"invalid shader push must be skipped");Pop(runtime.get());
            Require(runtime->mShaderStack.empty(),"pop after skipped push stays empty");
        }
        Gfx null{};Push(runtime.get(),&null);Pop(runtime.get());Pop(runtime.get());
        Require(runtime->mShaderStack.empty(),"null push and excess pops remain safe");
        auto missing=Load("<DisplayList><PushShader/><PopShader/></DisplayList>");Pop(runtime.get());
        Require(runtime->mShaderStack.empty(),"XML missing push followed by pop is safe");
        std::puts("shader invalid push/pop: PASS");return 0;
    }
    return 2;
}
