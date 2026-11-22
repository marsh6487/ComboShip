#!/usr/bin/env python3
from pathlib import Path
import os,subprocess,tempfile,re
ROOT=Path(__file__).resolve().parents[3]
def function(source,name):
    a=source.index(name);b=source.index('{',a);e=b+1;depth=1
    while depth:depth+=(source[e]=='{')-(source[e]=='}');e+=1
    return source[a:e]
adapter=(ROOT/'soh/soh/Enhancements/randomizer/SongEvents.cpp').read_text()
actors=(ROOT/'soh/src/code/z_actor.c').read_text()
with tempfile.TemporaryDirectory(prefix='song-event-oot-') as td:
    names=['void Rando::Context::GenerateSongEvents','void Rando::Context::ImportSongEvents','nlohmann::json Rando::Context::ExportSongEvents','bool Rando::Logic::CanPlaySongEvent','static int EventForScene','static int EventActionForSong','int SongEvents_OotAssignedSong','extern "C" int SongEvents_OotAction','extern "C" int SongEvents_OotClockLocked','extern "C" void SongEvents_OotApplyClockLock','bool Rando::Logic::ClockAllowsTimePass','extern "C" int SongEvents_OotIsEventAction','extern "C" int SongEvents_OotShouldCapture','extern "C" int SongEvents_OotSunSongTimeLocked']
    flag_functions=('s32 Flags_GetRandomizerInf','void Flags_SetRandomizerInf','s32 Flags_GetEventChkInf',
                    'void Flags_SetEventChkInf','s32 Flags_GetSwitch','void Flags_SetSwitch','void Flags_UnsetSwitch')
    code='\n'.join(function(actors,n) for n in flag_functions)+'\n'+'\n'.join(re.findall(r'^static int sArmedOot.*;',adapter,re.M))+'\n'+'\n'.join(function(adapter,n) for n in names)
    ocarina_tag=(ROOT/'soh/src/overlays/actors/ovl_En_Okarina_Tag/z_en_okarina_tag.c').read_text()
    tag_functions=('static s32 EnOkarinaTag_IsShuffledWaterLevelCrest','int EnOkarinaTag_GetPendingSongAction',
                   'void EnOkarinaTag_Init(Actor* thisx, PlayState* play) {',
                   *(f'void {name}(EnOkarinaTag* this, PlayState* play) {{' for name in
                     ('func_80ABEF2C', 'func_80ABF0CC', 'func_80ABF28C', 'func_80ABF4C8')))
    code+='\n'+'\n'.join(re.sub(r'\bthis\b','self',function(ocarina_tag,n)) for n in tag_functions)
    markers=(ROOT/'soh/soh/Enhancements/randomizer/SongEventMarkers.cpp').read_text()
    code+='\n'+function(markers,'int MarkerSong(')
    water=(ROOT/'soh/src/overlays/actors/ovl_Bg_Mizu_Water/z_bg_mizu_water.c').read_text()
    a=water.index('typedef struct {');b=water.index('\nconst ActorInit',a)
    # Preserve the native table's C float-to-integer initializer conversions in C++.
    native_levels=re.sub(r'(WATER_TEMPLE_WATER_F[12]_Y - WATER_TEMPLE_WATER_F3_Y)', r'static_cast<s32>(\1)', water[a:b])
    code+='\n'+native_levels+'\n'+function(water,'u32 BgMizuWater_GetWaterLevelActionIndex(')
    code+='\n'+re.sub(r'\bthis\b','self',function(water,'void BgMizuWater_WaitForAction(BgMizuWater* this, PlayState* play) {'))
    well=(ROOT/'soh/src/overlays/actors/ovl_Bg_Haka_Water/z_bg_haka_water.c').read_text()
    well_functions=('BgHakaWater_LowerWater','BgHakaWater_Wait','BgHakaWater_ChangeWaterLevel')
    code+='\n'+'\n'.join(re.sub(r'\bthis\b','self',function(well,f'void {name}(BgHakaWater* this, PlayState* play) {{'))
                       for name in well_functions)
    rd=(ROOT/'soh/src/overlays/actors/ovl_En_Rd/z_en_rd.c').read_text()
    code+='\n'+re.sub(r'\bthis\b','self',function(rd,'void EnRd_SetupAction(EnRd* this, EnRdActionFunc actionFunc) {'))
    code+='\n'+re.sub(r'\bthis\b','self',function(rd,'void EnRd_SetupStunned(EnRd* this) {'))
    message=(ROOT/'soh/src/code/z_message_PAL.c').read_text()
    a=message.index('    static u16 sOcarinaSongFlagsMap[] = {');b=message.index('    };',a)+len('    };')
    owned=message[a:b]+'\ns16 i;\n'+function(message,'for (i = sOcarinaSongBitFlags = 0;')
    a=message.index('            if (msgCtx->ocarinaAction == OCARINA_ACTION_UNK_0',message.index('// "Ocarina（'))
    end='osSyncPrintf("222222222222  -> ");'
    b=message.index(end,a)+len(end);b=message.index('}',b)+1
    choose=message[a:b]
    a=message.index('                if (msgCtx->msgMode == MSGMODE_OCARINA_STARTING)')
    b=message.index('                if (msgCtx->ocarinaAction != OCARINA_ACTION_FREE_PLAY',a)
    dispatch=message[a:b]
    code+='\nu32 gBitFlags[32]{};static u32 sOcarinaSongBitFlags=0,observedRecognitionFlags=0;\n'
    code+='void AudioOcarina_Start(u32 flags){observedRecognitionFlags=flags;}\n'
    code+='#undef osSyncPrintf\n#define osSyncPrintf(...) ((void)0)\n'
    code+='void DispatchNativeEventCheck(PlayState*play){auto*msgCtx=&play->msgCtx;'+owned+'\n'+choose+'\n'+dispatch+'}\n'
    access=(ROOT/'soh/soh/Enhancements/randomizer/location_access.cpp').read_text()
    code += '''
struct Region {
    bool timePass=true, childDay=false, childNight=false, adultDay=false, adultNight=false;
    bool Child() const { return childDay || childNight; }
    bool Adult() const { return adultDay || adultNight; }
    bool TimePass();
    void ApplyTimePass();
    void ResetVariables() { childDay=childNight=adultDay=adultNight=false; }
};
static std::array<Region, RR_MAX> testRegions;
static Region* RegionTable(RandomizerRegion key) { return &testRegions[key]; }
static Rando::Logic trackerLogic;
static auto* logic=&trackerLogic;
constexpr int PT_TOD_ACCESS=0;
void StartPerformanceTimer(int) {}
void StopPerformanceTimer(int) {}
struct GetAccessibleLocationsStruct {
    bool haveTimeAccess=true, foundTempleOfTime=true, validatedStartingRegion=true, logicUpdated=false;
    bool timePassChildDay=false, timePassChildNight=false, timePassAdultDay=false, timePassAdultNight=false;
};
void ProcessRegion(Region*, GetAccessibleLocationsStruct&, RandomizerGet, bool, bool) {}
'''
    code += function(access, 'bool Region::TimePass(') + '\n' + function(access, 'void Region::ApplyTimePass(')
    fill=(ROOT/'soh/soh/Enhancements/randomizer/3drando/fill.cpp').read_text()
    code += '\n' + function(fill, 'static void SeedRegionAgeTime(') + '\n'
    code += function(fill, 'static void PropagateTimeTravel(') + '\n'
    search = function(fill, 'std::vector<RandomizerCheck> ReachabilitySearch(')
    code += 'void NativeTrackerSeedRoot(RandoAgeTime startingAgeTime){' + function(
        search, 'if (startingAgeTime != RAT_NONE)') + '}\n'
    process = function(fill, 'void ProcessRegion(Region* region')
    preflight = process[process.index('    if (gals.haveTimeAccess)'):process.index('    if (region->UpdateEvents()')]
    code += 'void NativeEntranceTimePreflight(Region* region, GetAccessibleLocationsStruct& gals){' + preflight + '}\n'
    for file, region, symbol in (
        ('overworld/hyrule_field.cpp', 'RR_MARKET_ENTRANCE', 'FieldCanReachMarket'),
        ('overworld/market.cpp', 'RR_HYRULE_FIELD', 'MarketCanReachField'),
        ('root.cpp', 'RR_PRELUDE_OF_LIGHT_WARP', 'CanWarpWithPrelude'),
    ):
        graph = (ROOT / 'soh/soh/Enhancements/randomizer/location_access' / file).read_text()
        edge = next(line.strip() for line in graph.splitlines() if line.strip().startswith(f'ENTRANCE({region},'))
        condition = edge.split(',', 1)[1].rsplit('),', 1)[0].strip()
        code += f'bool {symbol}() {{ return {condition}; }}\n'
    (Path(td)/'oot_native_functions.inc').write_text(code)
    flags=['-std=c++20','-DF3DEX_GBI_2','-DGBI_FLOAT_MTX','-DGBI_S32_VTX','-DCONTROLLERBUTTONS_T=uint32_t','-I'+str(ROOT),'-I'+td]
    if os.environ.get('SANITIZE'): flags+=['-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer','-fno-pie','-no-pie']
    for p in ('soh','soh/include','soh/src','soh/assets','combo','libultraship/include'):flags+=['-isystem',str(ROOT/p)]
    for combo in (False,True):
        binary=Path(td)/('combo' if combo else 'original')
        subprocess.run([os.environ.get('CXX','c++'),*flags,*(['-DCOMBO_BUILD'] if combo else []),str(Path(__file__).with_name('oot_native.cpp')),'-o',str(binary)],check=True)
        subprocess.run([str(binary)],check=True);print('PASS native OoT200x18 actual request/solver/clock flags/save and song marker completion',combo,flush=True)
