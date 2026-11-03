#include "audio/NightMusic.h"
#include <cassert>
#include <map>
#include <set>
#include <iostream>

using namespace ComboNightMusic;
int main() {
    Lifecycle life;
    Input in{.scene=85,.source=0x22,.enabled=false,.night=true,.eligible=true,
             .canStart=true,.playing=false,.suspendWhenBlocked=true,.stopOnDay=true};
    assert(life.Update(in)==Action::None); // Off by default.
    in.enabled=true;
    assert(life.Update(in)==Action::Start);
    life.DidStart(0x15d);
    assert(life.Owns() && life.Playback()==0x15d && life.Source()==0x22);
    in.playing=true;
    assert(life.Update(in)==Action::None); // No command every frame.
    life.Refresh();
    assert(life.Update(in)==Action::Start);
    life.DidStart(0x1c6);
    assert(life.Playback()==0x1c6); // Full-width selection retained, resolved once by adapter.
    in.blocked=true;
    assert(life.Update(in)==Action::Stop); // OoT releases SUB for previews/fanfares.
    assert(!life.Owns());
    in.playing=false; in.canStart=false;
    assert(life.Update(in)==Action::None); // Battle/positional owner cannot be stolen.
    in.blocked=false;
    assert(life.Update(in)==Action::None);
    in.canStart=true;
    assert(life.Update(in)==Action::Start);
    life.DidStart(0x15d); in.playing=true;
    in.enabled=false;
    assert(life.Update(in)==Action::Stop);
    assert(life.Update(in)==Action::None); // Toggle off stops once.
    in.enabled=true;
    assert(life.Update(in)==Action::Start); life.DidStart(0x15d);
    in.night=false;
    assert(life.Update(in)==Action::Stop); // OoT dawn stops optional SUB only.
    in.night=true;
    assert(life.Update(in)==Action::Start); life.DidStart(0x15d);
    in.scene=90; in.playing=false; in.canStart=false;
    assert(life.Update(in)==Action::None && !life.Owns()); // Destination owns its audio.
    in.canStart=true; in.eligible=false;
    assert(life.Update(in)==Action::None); // Includes dedicated HF override and untimed scenes.
    in.eligible=true;
    assert(life.Update(in)==Action::Start); life.DidStart(0x15d);
    in.playing=true; in.suspendWhenBlocked=false; in.stopOnDay=false; in.blocked=true;
    assert(life.Update(in)==Action::None); // MM native fanfare/preview manages MAIN.
    in.blocked=false; in.night=false;
    assert(life.Update(in)==Action::None && !life.Owns()); // Never stop MM daytime music.
    life.Reset(); assert(life.Playback()==0xffff && !life.Owns());

    const std::map<uint16_t,int> rows{{2,0},{3,0},{5,0},{0xff00,0},{0xff01,0},{0xff02,0}};
    const auto ordered=OrderedRows(rows,2,0xff02);
    assert((ordered==std::vector<uint16_t>{2,0xff02,3,5,0xff00,0xff01}));
    assert(std::set<uint16_t>(ordered.begin(),ordered.end()).size()==rows.size());
    assert((OrderedRows(rows,999,0xff02)==std::vector<uint16_t>{2,3,5,0xff00,0xff01,0xff02}));
    std::cout << "PASS night lifecycle: toggles, day/night, native ownership, scene changes, refresh; stable row keys\n";
}
