from pathlib import Path
import subprocess
import tempfile
import unittest
ROOT=Path(__file__).resolve().parents[2]

class Policy(unittest.TestCase):
    def test_no_repeat_defeats_and_independent_enemy_selection(self):
        source=r'''
#include "combo/encounters/EncounterPolicy.h"
#include <cassert>
using namespace CustomEncounters;
int main(){
 Visit v;v.Enter(81,0,0);
 assert(!v.Request(0,false,true,true));
 assert(!v.Request(0,true,false,true));
 v.Enter(82,0,0);assert(v.Request(0,true,true,true));
 v.Spawned(0);assert(!v.Request(0,true,true,true));
 v.Defeated(0);v.Enter(82,0,0);assert(!v.Request(0,true,true,true));
 v.Disable();assert(!v.Request(0,true,true,true));
 v.Enter(81,0,0);assert(v.Request(0,true,true,true));
 v.Spawned(0);v.Disable();assert(v.Request(0,true,true,true));
 v.Enter(81,1,0);assert(v.Request(0,true,true,true));
 v.Enter(81,1,1);assert(v.Request(0,true,true,true));
 assert(Choose(0,0,false)==Enemy::None);
 assert(Choose(8,1,false)==Enemy::Miniblin);
 assert(Choose(8,8,true)==Enemy::None);
 assert(Choose(8,2,true)==Enemy::None);
 for(unsigned i=0;i<100;++i){auto e=Choose(i,15,true);assert(e==Enemy::Miniblin||e==Enemy::Beetle);}
}
'''
        with tempfile.TemporaryDirectory() as tmp:
            p=Path(tmp)/'t.cpp';p.write_text(source);exe=Path(tmp)/'t'
            subprocess.run(['c++','-std=c++17','-Wall','-Wextra','-Werror','-I'+str(ROOT),str(p),'-o',str(exe)],check=True)
            subprocess.run([str(exe)],check=True)

if __name__=='__main__':unittest.main()
