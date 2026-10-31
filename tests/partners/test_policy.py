from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]


class Policy(unittest.TestCase):
    def test_control_ownership_and_release(self):
        # Wrong ownership/game guards or counting invulnerable hits would fail these assertions.
        code = r'''
#include "combo/partners/PartnerPolicy.h"
#include <cassert>
using namespace PartnerPolicy;
int main() {
 ControlConditions c{};
 assert(!CanBegin(c));
 c.enabled=c.songOwned=c.activeWorld=c.partnerLive=c.grounded=true;
 assert(CanBegin(c));
 c.busy=true;assert(!CanBegin(c));c.busy=false;
 c.held=true;assert(!CanBegin(c));c.held=false;
 assert(!MustRelease(c));
 c.activeWorld=false;assert(MustRelease(c));c.activeWorld=true;
 c.damaged=true;assert(MustRelease(c));c.damaged=false;
 c.distance=651;assert(MustRelease(c));c.distance=650;assert(!MustRelease(c));
 c.enabled=false;assert(MustRelease(c));c.enabled=true;
 c.partnerLive=false;assert(MustRelease(c));
 assert(!CojiroEligible(false,false));assert(CojiroEligible(true,false));assert(CojiroEligible(false,true));
}
'''
        with tempfile.TemporaryDirectory() as tmp:
            src=Path(tmp)/'test.cpp';src.write_text(code);exe=Path(tmp)/'test'
            subprocess.run(['c++','-std=c++17','-Wall','-Wextra','-Werror','-I'+str(ROOT),str(src),'-o',str(exe)],check=True)
            subprocess.run([str(exe)],check=True)


if __name__=='__main__': unittest.main()
