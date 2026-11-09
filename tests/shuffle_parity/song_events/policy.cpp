#include "combo/rando/SongEvents.h"
#include <cassert>
#include <stdexcept>
int main() {
    using namespace ComboRando::SongEvents;
    auto a = Generate(Game::Oot, 142, true);
    assert(a == Generate(Game::Oot, 142, true) && a != Generate(Game::Oot, 143, true));
    assert(Generate(Game::Oot, 1, false) == Defaults(Game::Oot));
    for (unsigned seed=0; seed<1000; ++seed)
        for (auto song: Generate(Game::Mm, seed, true)) assert(Playable(Game::Mm, song));
    auto plan=a;plan[0]=9;Validate(Game::Oot,plan);
    plan[0]=255;bool rejected=false;try{Validate(Game::Oot,plan);}catch(const std::invalid_argument&){rejected=true;}
    assert(rejected);
    assert(ClockLocked(true,false) && !ClockLocked(true,true) && !ClockLocked(false,false));
    assert(ClockStart(0,42)==0x8000 && ClockStart(1,42)==0xe000 && ClockStart(2,42)==ClockStart(2,42));
}
