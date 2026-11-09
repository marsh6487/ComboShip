"""Native song adapter closure for bounded inventory/actor-body fixtures.

Only surrounding context and capability storage are test interfaces. The song
selection, learned-song mapping and button checks below come from production.
"""
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3]
def body(source,signature):
 a=source.index(signature);e=source.index('{',a)+1;d=1
 while d:d+=(source[e]=='{')-(source[e]=='}');e+=1
 return source[a:e]
def oot_support():
 s=(ROOT/'soh/soh/Enhancements/randomizer/SongEvents.cpp').read_text()
 return '''
namespace Rando { struct Context {
 struct Option {unsigned value=0;unsigned Get(){return value;}};Option option;
 std::vector<uint8_t> songEvents=ComboRando::SongEvents::Defaults(ComboRando::SongEvents::Game::Oot);
 static Context* GetInstance(){static Context c;return &c;}
 Option& GetOption(RandomizerSettingKey){return option;}
}; }
using Context=Rando::Context;
using namespace ComboRando::SongEvents;
'''+body(s,'bool Rando::Logic::CanPlaySongEvent(').replace('bool Rando::Logic::','bool Logic::',1)+'\n'
def mm_song_selection():
 s=(ROOT/'mm/2s2h/Rando/SongEvents.cpp').read_text()
 return 'namespace Rando::SongEvents {\n'+body(s,'static int NativeSong(')+'\n}\n'+body(s,'extern "C" int SongEvents_MmSong(')+'\n'
def mm_solver():
 s=(ROOT/'mm/2s2h/Rando/SongEvents.cpp').read_text()
 logic=(ROOT/'mm/2s2h/Rando/Logic/Logic.h').read_text()
 return ('namespace Rando::Logic {\n'+body(logic,'inline uint8_t FoundOcarinaButtons(')+'\n'+body(logic,'inline bool canPlaySong(')+'\n}\n'
         +'namespace Rando::SongEvents {\n'+body(s,'static int NativeSong(')+'\n'+body(s,'bool CanPlay(')+'\n}\n')
