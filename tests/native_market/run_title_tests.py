#!/usr/bin/env python3
"""Missing optional title artwork must never be submitted to the renderer."""
from pathlib import Path
import os
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/diagnostics"))
from run_time_pedestal_tests import block_from

with tempfile.TemporaryDirectory(prefix="native-market-title-") as temporary:
    for host, path in (("mm", "mm/2s2h/Enhancements/Gameplay/BlackMarket.cpp"),
                       ("soh", "soh/soh/Enhancements/Gameplay/ComboBlackMarketNative.cpp")):
        source = (ROOT / path).read_text()
        if "void PresentTitle(" in source:
            body = block_from(source, source.index("void PresentTitle("))
        else:
            start = source.index("        if (sTitlePending && Active() && ComboPlaceTitleCard_Show(")
            end = source.index("sTitlePending = false;", start) + len("sTitlePending = false;")
            body = "void PresentTitle() {\n" + source[start:end] + "\n}\n"
        fixture = r'''
#include "global.h"
#include <cassert>
#include <cstdio>
#include <cstring>
PlayState* gPlayState;
bool sTitlePending=true, titleAvailable=false;
int submissions=0;
alignas(2) constexpr char kCard[]="__OTR__custom/skiejer_market/title";
bool Active(){return true;}
u8 ResourceMgr_FileExists(const char*){return titleAvailable;}
bool ResourceMgr_IsAltAssetsEnabled(){return false;}
u8 ResourceMgr_FileAltExists(const char*){return false;}
#ifndef OOT_TITLE_TEST
int ComboPlaceTitleCard_Show(PlayState*,TexturePtr,u8,u8){++submissions;return 1;}
#endif
/* PRODUCTION */
int main(){
 PlayState play{};gPlayState=&play;
 const auto before=play.actorCtx.titleCtx;
 PresentTitle();
#ifdef OOT_TITLE_TEST
 assert(std::memcmp(&before,&play.actorCtx.titleCtx,sizeof(before))==0);
 titleAvailable=true;PresentTitle();
 assert(play.actorCtx.titleCtx.texture==kCard && play.actorCtx.titleCtx.durationTimer==80);
#else
 assert(submissions==0);
 titleAvailable=true;sTitlePending=true;PresentTitle();
 assert(submissions==1 && !sTitlePending);
#endif
 puts("PASS optional market title: absent image never submitted, supplied image retains native presentation");
}
'''.replace("/* PRODUCTION */", body)
        target = Path(temporary) / (host + ".cpp")
        binary = Path(temporary) / host
        target.write_text(fixture)
        paths = [host, host + "/include", host + "/include/PR", host + "/src", host + "/assets",
                 host + ("/2s2h" if host == "mm" else "/soh"), "libultraship/include", "combo"]
        subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", "-w", "-DCOMBO_BUILD", "-DF3DEX_GBI_2",
                        *(["-DOOT_TITLE_TEST"] if host == "soh" else []),
                        *["-I" + str(ROOT / p) for p in paths], str(target), "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True)
