#!/usr/bin/env python3
"""Execute the actual CLI replay dump guard; native exporters are service boundaries.

This checks setup-error propagation before traversal, not a native graph/fill.
"""
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
source = (ROOT / 'combo/ComboRandoHeadless.cpp').read_text()
start = source.index('            std::string sohDump = SOH_Dump(), mmDump = MM_Dump();')
end = source.index('            const std::string sharedPairs', start)
guard = source[start:end]
start = source.index('        auto r2 = runPass(true, &pt2);')
end = source.index('        if (r2.beatable)', start)
second_pass_guard = source[start:end]
program = r'''
#include "combo/rando/ComboPlaythrough.h"
#include <cassert>
std::string oot, mm;
bool entranceRestoreFailed = false;
int traversals = 0;
const char* SOH_Dump() { return oot.c_str(); }
const char* MM_Dump() { return mm.c_str(); }
ComboRando::PlaythroughResult ReplayBoundary() {
''' + guard + r'''
    ++traversals;
    ComboRando::PlaythroughResult result;
    result.beatable = true;
    return result;
}
int SecondPassExitBoundary(bool failed) {
    bool entranceRestoreFailed = failed;
    nlohmann::json pt2;
    auto runPass = [](bool, nlohmann::json*) { return ComboRando::PlaythroughResult{}; };
''' + second_pass_guard + r'''
    (void)r2;
    return 3;
}
int main() {
    for (const auto& bad : {std::string(R"({"error":"native budget failed","fixed":[]})"),
                            std::string("{bad"), std::string("[]")}) {
        for (bool badOot : {false, true}) {
            oot = badOot ? bad : "{}";
            mm = badOot ? "{}" : bad;
            entranceRestoreFailed = false;
            assert(!ReplayBoundary().beatable && entranceRestoreFailed && traversals == 0);
        }
    }
    oot = mm = "{}";
    entranceRestoreFailed = false;
    assert(ReplayBoundary().beatable && !entranceRestoreFailed && traversals == 1);
    assert(SecondPassExitBoundary(true) == 2); // A setup failure is not a circular-seed verdict.
    assert(SecondPassExitBoundary(false) == 3);
}
'''
with tempfile.TemporaryDirectory(prefix='replay-dump-error-') as temporary:
    folder = Path(temporary)
    cpp = folder / 'test.cpp'
    cpp.write_text(program)
    binary = folder / 'test'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++20', '-Wall', '-Wextra', '-Werror',
                    '-I' + str(ROOT), '-I' + str(ROOT / 'combo'), str(cpp), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
print('PASS actual CLI replay guard rejects either native diagnostic before traversal; valid legacy metadata preserved')
