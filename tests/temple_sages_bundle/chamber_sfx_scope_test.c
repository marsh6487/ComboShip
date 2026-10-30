#include <assert.h>
#include <math.h>
#include "chamber_sfx_scope.h"

typedef struct { float x, y, z; } Vec3f;
typedef struct { int params; struct { Vec3f pos; } home, world; } Actor;
typedef struct { Actor actor; } Player;
typedef struct { int sceneNum; struct { struct { int num; } curRoom; } roomCtx; Player* player; } PlayState;
#define COMBO_BUILD 1
#define SCENE_TEMPLE_OF_TIME 0x43
#define RS_CHAMBER_OF_SAGES_1 0x10
#define GET_PLAYER(play) ((play)->player)
static unsigned submissions;
static void Submit(Actor* thisx, PlayState* play) {
    /* PRODUCTION GUARD */
    ++submissions;
}
int main(void) {
    assert(ChamberSfx_Contains(0, -120, -2400));
    const float pads[5][2] = {{-779,-1950},{-779,-2850},{0,-3300},{779,-2850},{779,-1950}};
    for (int i=0;i<5;i++) assert(ChamberSfx_Contains(pads[i][0],-120,pads[i][1]));
    assert(ChamberSfx_Contains(0,-120,-1200));
    assert(!ChamberSfx_Contains(0,-120,-1199.99f));
    assert(!ChamberSfx_Contains(0,0,-720));
    assert(!ChamberSfx_Contains(0,0,0));
    assert(!ChamberSfx_Contains(1072,-120,-2400));
    assert(!ChamberSfx_Contains(0,-120,-3652));
    assert(!ChamberSfx_Contains(0,2575,-2400));
    assert(!ChamberSfx_Contains(NAN,-120,-2400));
    assert(!ChamberSfx_Contains(0,-120,INFINITY));
    Actor emitter = {.params=0x10, .home={{0,-120,-2400}}};
    Player player = {.actor={.world={{0,-120,-2400}}}};
    PlayState play = {.sceneNum=SCENE_TEMPLE_OF_TIME,.roomCtx={{0}},.player=&player};
    Submit(&emitter,&play); assert(submissions==1);
    player.actor.world.pos.z=0;Submit(&emitter,&play);assert(submissions==1);
    player.actor.world.pos.z=-2400;play.roomCtx.curRoom.num=1;Submit(&emitter,&play);assert(submissions==1);
    play.roomCtx.curRoom.num=0;play.player=0;Submit(&emitter,&play);assert(submissions==1);
    play.player=&player;Submit(&emitter,&play);assert(submissions==2);
    play.sceneNum=0;Submit(&emitter,&play);assert(submissions==3);
    play.sceneNum=SCENE_TEMPLE_OF_TIME;emitter.home.pos.z=0;
    player.actor.world.pos.z=0;Submit(&emitter,&play);assert(submissions==4);
    emitter.home.pos.z=-2400;emitter.params=1;Submit(&emitter,&play);assert(submissions==5);
    for (int i=0;i<100;i++) {
        assert(ChamberSfx_Contains(0,-120,-1240));
        assert(!ChamberSfx_Contains(0,-120,-1160));
    }
}
