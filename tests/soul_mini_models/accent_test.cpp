// Actual menu registrations/scopes and both native CVarCheckbox renderers are
// extracted by the driver; only config storage and save/initialization are mocked.
#include "gui/WhatsNew.h"
#include "menu/ComboSoulModelSettings.h"
#include <imgui_internal.h>
#include <map>
#include <vector>
#include <string>
#include <iostream>
#include <stdexcept>

static void Check(bool condition,const char* why){if(!condition)throw std::runtime_error(why);}
static bool Ship_IsCStringEmpty(const char* value){return !value||!value[0];}
static std::map<std::string,int> config;
static int saves=0,initializations=0;
int CVarGetInteger(const char* key,int fallback){auto it=config.find(key);return it==config.end()?fallback:it->second;}
void CVarSetInteger(const char* key,int value){config[key]=value;}
namespace Ship {struct Context {
static Context* GetRawInstance(){static Context context;return &context;}
Context* GetWindow(){return this;}Context* GetGui(){return this;}
void SaveConsoleVariablesNextFrame(){++saves;}
};}
namespace ShipInit {void Init(const char*){++initializations;}}
struct Row {std::string label,key;ImGuiID id{},original{};ImVec2 min,max;ImVec4 text;bool changed=false;int goldVertices=0;};
static std::vector<Row> rows;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-compare"
#include "accent_controls.inc"
#pragma GCC diagnostic pop

using Draw=void(*)();
static std::vector<Row> Frame(Draw draw,bool accent,ImVec2 mouse=ImVec2(-100,-100),bool down=false,float width=440){
config[WhatsNew::kEnabledCvar]=accent;
auto& io=ImGui::GetIO();io.AddMousePosEvent(mouse.x,mouse.y);io.AddMouseButtonEvent(0,down);
ImGui::NewFrame();ImGui::SetNextWindowPos(ImVec2(20,20));ImGui::SetNextWindowSize(ImVec2(width,260));
ImGui::Begin("Soul preferences",nullptr,ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoTitleBar);
rows.clear();const int colors=GImGui->ColorStack.Size;
draw();Check(GImGui->ColorStack.Size==colors,"accent must balance native color stack");
ImGui::End();ImGui::Render();return rows;
}
static bool Same(const ImVec4& a,const ImVec4& b){return a.x==b.x&&a.y==b.y&&a.z==b.z&&a.w==b.w;}
static void Probe(const char* host,Draw draw){
ImGui::CreateContext();auto& io=ImGui::GetIO();io.IniFilename=nullptr;io.DisplaySize=ImVec2(900,600);io.DeltaTime=1.f/60;
unsigned char* pixels;int width,height;io.Fonts->GetTexDataAsRGBA32(&pixels,&width,&height);
config.clear();saves=initializations=0;config["gRandoEnhancements.SimplerBossSoulModels"]=1;
Frame(draw,false);const auto plain=Frame(draw,false);const auto gold=Frame(draw,true);
Check(plain.size()==2&&gold.size()==2,"both exact soul preference controls must render");
const char* labels[]={"Simpler Enemy Soul Models","Simpler NPC Soul Models"};
const char* keys[]={ComboSoulModels::kSimplerEnemyCvar,ComboSoulModels::kSimplerNpcCvar};
for(int i=0;i<2;i++){
Check(gold[i].label==labels[i]&&gold[i].key==keys[i],"native labels and shared keys must remain exact");
Check(gold[i].id==gold[i].original&&plain[i].id==gold[i].id,"accent must preserve checkbox ID");
Check(gold[i].min.x==plain[i].min.x&&gold[i].min.y==plain[i].min.y&&gold[i].max.x==plain[i].max.x&&gold[i].max.y==plain[i].max.y,"accent must preserve full layout");
Check(Same(gold[i].text,WhatsNew::Accent(WhatsNew::Kind::New)),"What's New enabled must select gold caption color");
Check(gold[i].goldVertices>0,"actual native caption vertices must use the gold accent");
Check(plain[i].goldVertices==0&&!Same(plain[i].text,gold[i].text),"What's New disabled must remove caption accent");
Check(CVarGetInteger(keys[i],0)==0,"styling must preserve unchecked default");
}
for(int i=0;i<2;i++){
const ImVec2 hit(gold[i].min.x+8,(gold[i].min.y+gold[i].max.y)/2);
Frame(draw,true,hit);Frame(draw,true,hit,true);const auto clicked=Frame(draw,true,hit,false);
Check(clicked[i].changed&&CVarGetInteger(keys[i],0)==1,"highlighted real native checkbox must update its CVar");
Check(CVarGetInteger(keys[1-i],0)==(i?1:0),"enemy and NPC controls must remain independent");
Check(CVarGetInteger("gRandoEnhancements.SimplerBossSoulModels",0)==1,"new preference must preserve boss preference");
const auto checkedOff=Frame(draw,false);Check(CVarGetInteger(keys[i],0)==1&&!checkedOff[i].goldVertices,"turning accent off must preserve checked state");
}
Check(saves==2&&initializations==2,"each native click must retain its save and initialization callback");
for(int i=0;i<2;i++){
const auto native=Frame(draw,false);const ImVec2 hit(native[i].min.x+8,(native[i].min.y+native[i].max.y)/2);
Frame(draw,false,hit);Frame(draw,false,hit,true);const auto clicked=Frame(draw,false,hit,false);
Check(clicked[i].changed&&CVarGetInteger(keys[i],1)==0,"unaccented native checkbox must still uncheck normally");
Check(CVarGetInteger("gRandoEnhancements.SimplerBossSoulModels",0)==1,"unaccented click must preserve boss preference");
}
Check(saves==4&&initializations==4,"unaccented clicks must preserve native save callbacks");
for(float narrow:{310.f,340.f}){
const auto off=Frame(draw,false,ImVec2(-100,-100),false,narrow),on=Frame(draw,true,ImVec2(-100,-100),false,narrow);
for(int i=0;i<2;i++)Check(on[i].min.x==off[i].min.x&&on[i].max.x==off[i].max.x&&on[i].max.y==off[i].max.y,"accent must preserve narrow-column geometry");
}
ImGui::DestroyContext();std::cout<<"PASS "<<host<<": both exact soul checkboxes gold on/off, actual caption vertices, clicks/CVars, checked state, IDs/layout, save callbacks and boss independence\n";
}
int main(){OotWidgets::Register();MmWidgets::Register();Probe("OoT",OotWidgets::DrawControls);Probe("MM",MmWidgets::DrawControls);}
