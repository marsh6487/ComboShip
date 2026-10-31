"""Exercise actual native controller filtering and Cojiro damage functions."""
from pathlib import Path
import subprocess
import tempfile
import unittest
import sys
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'scripts/diagnostics'))
from run_time_pedestal_tests import block_from

class NativeControl(unittest.TestCase):
    def compile_run(self,code):
        with tempfile.TemporaryDirectory() as t:
            p=Path(t)/'test.cpp';p.write_text(code);exe=Path(t)/'test'
            subprocess.run(['c++','-std=c++17',str(p),'-o',str(exe)],check=True)
            subprocess.run([str(exe)],check=True)

    def test_filter_and_release_leave_pause_available(self):
        for game in ('soh','mm'):
            path=ROOT/game/'src/overlays/actors/ovl_NeiPartners/z_nei_command_melody.c'
            s=path.read_text();body=block_from(s,s.index('void Partners_Filter('))
            with self.subTest(game=game):
                self.compile_run(r'''
#include <cassert>
#include <cstdint>
#include <cstring>
struct Input{struct{uint32_t button;}cur,press,rel;int stickX,stickY;};struct Player{};
struct{struct{Input input[1];}state;}play,*gPlayState=&play;
int sActive=-1,sRelease=0;Input sInput;constexpr int BTN_START=0x1000;
'''+body+r'''
int main(){Player player;Input in{};in.cur.button=1;in.press.button=BTN_START|2;in.stickX=65;
 auto original=in;Partners_Filter(&player,&in);assert(!memcmp(&in,&original,sizeof(in)));
 sActive=0;play.state.input[0]=original;Partners_Filter(&player,&in);
 assert(in.cur.button==0&&in.press.button==BTN_START&&in.stickX==0);
 assert(!memcmp(&sInput,&original,sizeof(sInput)));
 sActive=-1;sRelease=2;in=original;Partners_Filter(&player,&in);assert(sRelease==1&&in.cur.button==0);
 in=original;Partners_Filter(&player,&in);assert(sRelease==0&&in.cur.button==0);
 in=original;Partners_Filter(&player,&in);assert(!memcmp(&in,&original,sizeof(in)));
}
''')

    def test_actual_three_hits_ignore_invulnerable_hits(self):
        for game in ('soh','mm'):
            s=(ROOT/game/'src/overlays/actors/ovl_NeiPartners/z_nei_cojiro_companion.c').read_text()
            body=block_from(s,s.index('static bool TakeHit('))
            body=body.replace('Actor_PlaySfx','Audio_PlayActorSound2')
            with self.subTest(game=game):
                self.compile_run(r'''
#include <cassert>
struct Actor{bool killed=false;};struct Cojiro{struct{Actor actor;struct{struct{int acFlags;}base;}collider;}niw;int invulnerable=0,hits=0,fleeTimer=0;};
int sRecovery=0;constexpr int AC_HIT=2,NA_SE_EV_CHICKEN_CRY_M=1,COJIRO_RECOVERY=400;
void Audio_PlayActorSound2(Actor*,int){}void Actor_Kill(Actor*a){a->killed=true;}
'''+body+r'''
int main(){Cojiro c{};
 c.niw.collider.base.acFlags=AC_HIT;assert(!TakeHit(&c)&&c.hits==1&&c.invulnerable==16);
 c.niw.collider.base.acFlags=AC_HIT;assert(!TakeHit(&c)&&c.hits==1);
 c.invulnerable=0;c.niw.collider.base.acFlags=AC_HIT;assert(!TakeHit(&c)&&c.hits==2);
 c.invulnerable=0;c.niw.collider.base.acFlags=AC_HIT;assert(TakeHit(&c)&&c.niw.actor.killed&&sRecovery==400);
}
''')
if __name__=='__main__':unittest.main()
