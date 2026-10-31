from pathlib import Path
import subprocess
import tempfile
import unittest
import sys,re
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'scripts/diagnostics'))
from run_time_pedestal_tests import block_from

class MmLynel(unittest.TestCase):
    def test_native_aim_state_and_damage_indices(self):
        s=(ROOT/'mm/src/overlays/actors/ovl_NeiLynel/z_nei_lynel.c').read_text()
        aim=re.sub(r'\bthis\b','self',block_from(s,s.index('static bool EnLynel_IsPlayerAimingAt(')))
        table=block_from(s,s.index('static DamageTable sDamageTable ='))+';'
        damage=(ROOT/'mm/include/z64collision_check.h').read_text()
        flags='\n'.join(re.findall(r'^#define DMG_\w+\s+\(1 << 0x[0-9A-Fa-f]+\)',damage,re.M))
        code=r'''
#include <cassert>
struct Actor{};struct EnLynel{Actor actor;};struct Player{int stateFlags1=0,heldItemAction=0,unk_AA5=0;Actor*focusActor=nullptr;};
constexpr int PLAYER_IA_BOW=9,PLAYER_IA_HOOKSHOT=13,PLAYER_UNKAA5_3=3;
bool facing=true;bool Actor_IsFacingPlayer(Actor*,int){return facing;}
struct Damage{int damage,effect;};using DamageTable=Damage[32];
enum{LYNEL_DMGEFF_NONE,LYNEL_DMGEFF_STUN,LYNEL_DMGEFF_FIRE,LYNEL_DMGEFF_ICE,LYNEL_DMGEFF_LIGHT};
#define DMG_ENTRY(d,e) {d,e}
'''+flags+'\n'+aim+'\n'+table+r'''
unsigned index(unsigned flag){unsigned i=0;while((flag>>i)!=1)++i;return i;}
int main(){EnLynel l;Player p;
 p.stateFlags1=0x200;assert(!EnLynel_IsPlayerAimingAt(&l,&p));
 p.heldItemAction=PLAYER_IA_BOW;p.unk_AA5=PLAYER_UNKAA5_3;assert(EnLynel_IsPlayerAimingAt(&l,&p));
 p.unk_AA5=0;p.focusActor=&l.actor;assert(EnLynel_IsPlayerAimingAt(&l,&p));
 facing=false;assert(!EnLynel_IsPlayerAimingAt(&l,&p));
 assert(sDamageTable[index(DMG_FIRE_ARROW)].effect==LYNEL_DMGEFF_FIRE);
 assert(sDamageTable[index(DMG_ICE_ARROW)].effect==LYNEL_DMGEFF_ICE);
 assert(sDamageTable[index(DMG_LIGHT_ARROW)].effect==LYNEL_DMGEFF_LIGHT);
 assert(sDamageTable[index(DMG_SWORD)].damage==2);
 assert(sDamageTable[index(DMG_GORON_PUNCH)].damage==4);
 assert(sDamageTable[index(DMG_POWDER_KEG)].damage==8);
}
'''
        with tempfile.TemporaryDirectory() as t:
            p=Path(t)/'t.cpp';p.write_text(code);exe=Path(t)/'t'
            subprocess.run(['c++','-std=c++17',str(p),'-o',str(exe)],check=True)
            subprocess.run([str(exe)],check=True)
if __name__=='__main__':unittest.main()
