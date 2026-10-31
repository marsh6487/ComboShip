"""Execute the production room request function with isolated engine dependencies."""
import ast, pathlib, subprocess, tempfile
r=pathlib.Path(__file__).parent
tree=ast.parse((r/'test_spawn.py').read_text())
prefix=next(ast.literal_eval(n.value) for n in tree.body if isinstance(n,ast.Assign) and any(isinstance(t,ast.Name) and t.id=='prefix' for t in n.targets))
prefix=prefix.replace('struct PlayState {int sceneNum=0,sceneConfig=0,roomCtx=0,numRooms=1; void* sceneSegment=nullptr; ActorEntry* linkActorEntry=nullptr;};', '''
using u32=unsigned;
struct Room {int num=0; void* segment=nullptr;};
struct RoomContext {int status=0,activeBufPage=0,loadQueue=0,loadMsg=0; Room curRoom,prevRoom; void* unk_34=nullptr; void* roomToLoad=nullptr; void* bufPtrs[2]{};};
struct RomFile {uintptr_t vromStart=0,vromEnd=0;const char* fileName="room";};
struct PlayState {int sceneNum=1,sceneConfig=0,colCtx=0,numRooms=1;RoomContext roomCtx;RomFile roomList[1];void* sceneSegment=nullptr;ActorEntry* linkActorEntry=nullptr;};
''')
prefix=prefix.replace('int func_80096FE8(PlayState*,int*)','int func_80096FE8(PlayState*,RoomContext*)')
prefix+='''
#define ALIGN16(x) (((x)+15)&~uintptr_t(15))
void BgCheck_InvalidateRoomSnapshots(int*){}
void osCreateMesgQueue(int*,int*,int){}
auto ResourceMgr_GetResourceByNameHandlingMQ(const char* p){return Ship::Context::GetRawInstance()->rm->LoadResource(p);}
'''
source_root = r/'candidate' if (r/'candidate').is_dir() else r.parents[1]
source=(source_root/'soh/soh/z_scene_otr.cpp').read_text()
body=source[source.index('extern "C" s32 OTRRoom_RequestNewRoom('):]
main='''
int main(){
 PlayState p;auto oot=std::make_shared<Ship::RM>(),mm=std::make_shared<Ship::RM>();
 oot->resource=std::make_shared<SOH::Scene>();mm->resource=std::make_shared<SOH::Scene>();
 Ship::CrossRMRegistry::oot=oot;Ship::Context::GetRawInstance()->rm=mm;
 assert(OTRRoom_RequestNewRoom(&p,&p.roomCtx,-1)==0 && oot->calls==0 && p.roomCtx.status==0);
 puts("invalid-room PASS");
 assert(OTRRoom_RequestNewRoom(&p,&p.roomCtx,0)==1);
 assert(p.roomCtx.roomToLoad==oot->resource.get() && mm->calls==0 && Ship::Context::GetRawInstance()->rm==mm);
 puts("room-owner PASS");
 p.roomCtx.status=0;oot->resource=std::make_shared<Ship::IResource>();
 assert(OTRRoom_RequestNewRoom(&p,&p.roomCtx,0)==1 && p.roomCtx.roomToLoad==nullptr);
 puts("room-type PASS");
}
'''
with tempfile.TemporaryDirectory() as d:
 p=pathlib.Path(d);(p/'test.cpp').write_text(prefix+body+main)
 subprocess.run(['c++','-std=c++20','-Werror=format','-fsanitize=undefined','-fno-sanitize-recover=all',str(p/'test.cpp'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
