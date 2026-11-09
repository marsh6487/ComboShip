void TestLotteryRules() { using namespace Rando;using namespace Rando::Logic;
#include "lottery_region.inc"
 auto& shop=Regions[RR_LOTTERY_SHOP];
 const TimeSlice mornings[]={TIME_DAY1_AM_06_00,TIME_DAY2_AM_06_00,TIME_DAY3_AM_06_00};
 const TimeSlice evenings[]={TIME_NIGHT1_PM_06_00,TIME_NIGHT2_PM_06_00,TIME_NIGHT3_PM_06_00};
 const TimeSlice closings[]={TIME_NIGHT1_PM_11_00,TIME_NIGHT2_PM_11_00,TIME_NIGHT3_PM_11_00};
 const RandoCheckId ids[]={RC_CLOCK_TOWN_WEST_LOTTERY,RC_CLOCK_TOWN_WEST_LOTTERY_NIGHT_2,RC_CLOCK_TOWN_WEST_LOTTERY_NIGHT_3};
 const RandoEvent tickets[]={RE_LOTTERY_TICKET_DAY_1,RE_LOTTERY_TICKET_DAY_2,RE_LOTTERY_TICKET_DAY_3};
 REQUIRE(shop.timeStayRestrictions.size()==3);
 for(auto time:closings) REQUIRE(!shop.timeStayRestrictions.at(time)());
 // All six clock ownership permutations, all clock modes, and precise native slices.
 for(int on=0;on<2;++on) for(int mode:{RO_CLOCK_SHUFFLE_RANDOM,RO_CLOCK_SHUFFLE_ASCENDING,RO_CLOCK_SHUFFLE_DESCENDING})
 for(int day=0;day<3;++day) for(int mask=0;mask<64;++mask) {
  gSaveContext={};gSaveContext.save.shipSaveInfo.saveType=SAVETYPE_RANDO;RANDO_SAVE_OPTIONS[RO_SHUFFLE_LOTTERY_NIGHTS]=on;RANDO_SAVE_OPTIONS[RO_CLOCK_SHUFFLE]=1;
  RANDO_SAVE_OPTIONS[RO_CLOCK_SHUFFLE_PROGRESSIVE]=mode;
  for(int half=0;half<6;++half) if(mask&(1<<half)) Flags_SetRandoInf((RandoInf)(RANDO_INF_OBTAINED_CLOCK_DAY_1+half));
  const bool dayOwned=OwnsHalfDayForMode(2*day), nightOwned=OwnsHalfDayForMode(2*day+1);
  gCurrentRegionTime=TIME_BIT_ONE << mornings[day];
  for(int i=0;i<3;++i) {
   REQUIRE(shop.events[i].first==tickets[i]);
   REQUIRE(shop.events[i].second()==bool(on&&i==day&&dayOwned));
   RANDO_EVENTS[tickets[i]]=shop.events[i].second();
   REQUIRE(shop.checks.at(ids[i]).first()==bool(!on&&i==0&&dayOwned&&nightOwned));
  }
  // A missing base wallet blocks purchase and collection, even with every clock.
  RANDO_SAVE_OPTIONS[RO_SHUFFLE_CHILD_WALLET]=1;
  for(const auto& event:shop.events) REQUIRE(!event.second());
  RANDO_SAVE_OPTIONS[RO_SHUFFLE_CHILD_WALLET]=0;
  for(int slice=evenings[day]-1;slice<=closings[day]+1;++slice) {
   gCurrentRegionTime=TIME_BIT_ONE << slice;
   for(int i=0;i<3;++i) {
    const bool evening=slice>=evenings[day]&&slice<closings[day];
    // Legacy grants retain their old daytime predicate; separate rewards never unlock at noon.
    bool expected=on&&i==day&&dayOwned&&nightOwned&&evening;
    if(!on) expected=i==0&&slice<evenings[day]&&dayOwned&&nightOwned;
    REQUIRE(shop.checks.at(ids[i]).first()==expected);
   }
   if(on) {
    RANDO_SAVE_OPTIONS[RO_SHUFFLE_CHILD_WALLET]=1;
    for(auto id:ids) REQUIRE(!shop.checks.at(id).first());
    RANDO_SAVE_OPTIONS[RO_SHUFFLE_CHILD_WALLET]=0;
   }
  }
  // Night ownership alone, and access on a different ticket day, cannot invent a ticket.
  for(auto ticket:tickets) RANDO_EVENTS[ticket]=false;
  gCurrentRegionTime=TIME_BIT_ONE << evenings[day];
  for(auto id:ids) REQUIRE(!shop.checks.at(id).first());
 }
 // No clock shuffle: tickets still need daytime purchase, each evening its own receipt.
 gSaveContext={};gSaveContext.save.shipSaveInfo.saveType=SAVETYPE_RANDO;RANDO_SAVE_OPTIONS[RO_SHUFFLE_LOTTERY_NIGHTS]=1;
 for(int day=0;day<3;++day) {
  gCurrentRegionTime=TIME_BIT_ONE << mornings[day];
  REQUIRE(shop.events[day].second());RANDO_EVENTS[tickets[day]]=true;
  REQUIRE(!shop.checks.at(ids[day]).first());
  gCurrentRegionTime=TIME_BIT_ONE << evenings[day];REQUIRE(shop.checks.at(ids[day]).first());
  gCurrentRegionTime=TIME_BIT_ONE << closings[day];REQUIRE(!shop.checks.at(ids[day]).first());
 }
}
