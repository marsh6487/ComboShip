// Appended to the native item fixture; all queries, camera functions, constants,
// math and engine structures come from production. Input/resource services only
// are controlled. Alternate assets (including Okayu) share the ordinary adult
// skeleton path and have no separate camera age/type.
static void NativeOcarina() {
  for (float scale : {.005f, .01f, .015f}) for (float offset : {-5.f, 0.f, 12.f}) {
    Reset(); modelScale = scale; modelYOffset = offset;
    player.stateFlags1 = 0;
    player.unk_AB8 = 200.f; // Native sinking displacement must remain subtracted.
    auto c = CameraFixture(CAM_SET_ITEM1);
    Camera_KeepOn4(&c);
    const float height = 68.f * scale/.01f - 2.f;
    const float normalized = .5f*(height+68.f);
    const auto &ro = c.paramData.keep4.roData;
    Expect(ro.unk_00, -.45f*normalized, "adult ocarina normalized Y");
    Expect(ro.unk_04, .75f*normalized, "adult ocarina normalized radius");
    Expect(D_801EDDD0.y, 100.f+height+offset+ro.unk_00, "adult ocarina target");
    Expect(ro.unk_08, -12.f, "native ocarina pitch");
    Expect(ro.unk_0C, 165.f, "native ocarina yaw");
    Expect(ro.unk_10, 5.f, "native ocarina forward offset");
    Expect(ro.unk_18, 70.f, "native ocarina FOV");
    assert(ro.timer==6 && c.paramData.keep4.rwData.timer==5);
    const auto nativeFlags=D_801B53EC[6].val;
    assert(ro.interfaceFlags==nativeFlags);
  }
  Reset(); player.stateFlags1=PLAYER_STATE1_800000;
  auto raised=CameraFixture(CAM_SET_ITEM1); Camera_KeepOn4(&raised);
  Expect(raised.paramData.keep4.roData.unk_04,63.f,"native state height retained");
  Expect(D_801EDDD0.y,162.2f,"native state target retained");
}
static void NativeCameraControls() {
  // Each ineligible presentation exactly matches the native, mode-off result.
  for(int variant=0; variant<10+CUSTOM_FORM_MAX; ++variant) {
    Reset(); player.stateFlags1=0;
    switch(variant) {
      case 0: mode=0; break;
      case 1: sReady=0; break;
      case 2: sSkel=nullptr; break;
      case 3: sIsChildRig=1; break;
      case 4: mario=true; break;
      case 5: wolf=true; break;
      case 6: player.transformation=PLAYER_FORM_DEKU; break;
      case 7: player.transformation=PLAYER_FORM_GORON; break;
      case 8: player.transformation=PLAYER_FORM_ZORA; break;
      case 9: player.transformation=PLAYER_FORM_FIERCE_DEITY; break;
      default: form=variant-10; break;
    }
    assert(!AdultLink_UsesAdultItemPresentation(&player));
    modelScale=.015f; modelYOffset=30.f;
    auto actual=CameraFixture(CAM_SET_ITEM1); Camera_KeepOn4(&actual);
    mode=0;
    auto expected=CameraFixture(CAM_SET_ITEM1); Camera_KeepOn4(&expected);
    assert(!memcmp(&actual.paramData,&expected.paramData,sizeof(actual.paramData)));
    Expect(actual.at.y,expected.at.y,"ineligible camera target unchanged");
  }
  Reset(); player.stateFlags1=0; peer=player;
  auto actual=CameraFixture(CAM_SET_ITEM1); actual.focalActor=&peer.actor;
  Camera_KeepOn4(&actual);
  mode=0;
  auto expected=CameraFixture(CAM_SET_ITEM1); expected.focalActor=&peer.actor;
  Camera_KeepOn4(&expected);
  assert(!memcmp(&actual.paramData,&expected.paramData,sizeof(actual.paramData)));
  Reset(); mode=0; player.stateFlags1=0;
  auto child=CameraFixture(CAM_SET_ITEM1); Camera_KeepOn4(&child);
  Expect(child.paramData.keep4.roData.unk_00,-25.2f,"native child Y");
  Expect(child.paramData.keep4.roData.unk_04,42.f,"native child radius");
  Expect(D_801EDDD0.y,118.8f,"native child target");
}
static void NativeCameraTransitions() {
  Reset(); player.stateFlags1=0;
  auto c=CameraFixture(CAM_SET_ITEM1);
  for(int frame=0;frame<10;++frame) Camera_KeepOn4(&c);
  assert(c.paramData.keep4.rwData.timer==0);
  modelScale=.015f; modelYOffset=9.f;
  Camera_KeepOn4(&c);
  Expect(c.paramData.keep4.roData.unk_04,63.75f,"hot scaled ocarina radius");
  Expect(D_801EDDD0.y,172.75f,"hot scaled ocarina target");
  mode=0; Camera_KeepOn4(&c);
  Expect(c.paramData.keep4.roData.unk_04,42.f,"hot child radius");
  Expect(D_801EDDD0.y,118.8f,"hot child target");
  assert(c.paramData.keep4.rwData.timer==0); // Never restart session/lifecycle.
  mode=1; modelScale=.01f; modelYOffset=0;
  Camera_KeepOn4(&c);
  Expect(c.paramData.keep4.roData.unk_04,51.f,"hot adult radius restored");
  c.data2++; Camera_KeepOn4(&c); assert(c.animState==20);
  Camera_KeepOn4(&c); assert(c.paramData.keep4.rwData.timer==5);
  auto next=CameraFixture(CAM_SET_ITEM1); Camera_KeepOn4(&next);
  Expect(next.paramData.keep4.roData.unk_04,51.f,"reentry radius");
  // Authored/scripted setting that happens to use KeepOn4 is untouched.
  sCameraSettings[CAM_SET_ELEGY_SHELL]=sCameraSettings[CAM_SET_ITEM1];
  auto scripted=CameraFixture(CAM_SET_ELEGY_SHELL); Camera_KeepOn4(&scripted);
  Expect(scripted.paramData.keep4.roData.unk_04,42.f,"scripted radius unchanged");
}
static void NativeFirstPerson() {
  static CameraMode modes[CAM_MODE_MAX]{};
  modes[CAM_MODE_FIRSTPERSON]={CAM_FUNC_SUBJECT1,ARRAY_COUNT(sSetNormal0ModeFirstPersonData),sSetNormal0ModeFirstPersonData};
  for(int custom : {CUSTOM_FORM_NONE,CUSTOM_FORM_RITO}) for(float focusY : {144.f,168.f,190.f}) {
    Reset(); form=custom;
    sCameraSettings[CAM_SET_NORMAL0].cameraModes=modes;
    player.actor.focus.pos={10,focusY,20};
    player.actor.focus.rot={0,0,0};
    // Focus supplied by native post-head limb already contains adult/form
    // scale, Rito root offset and model translation. Adding these again breaks FP.
    player.actor.shape.yOffset=1800.f; modelYOffset=16.f; modelScale=.015f;
    auto c=CameraFixture(CAM_SET_NORMAL0); c.mode=CAM_MODE_FIRSTPERSON;
    for(int frame=0;frame<8;++frame) Camera_Subject1(&c);
    Expect(c.at.y,focusY,"FP focuses transformed head exactly once");
    Expect(c.eye.y,focusY,"FP native eye height unchanged");
    Expect(c.eye.z,15.f,"FP native eye distance");
    player.actor.focus.pos.y+=11.f;
    Camera_Subject1(&c);
    Expect(c.eye.y,focusY+11.f,"FP follows current head without cached height");
    mode=0; Camera_Subject1(&c);
    Expect(c.eye.y,focusY+11.f,"FP mode change preserves focus ownership");
  }
}
int main(int argc,char**argv) {
  assert(argc==2); Reset();
  const std::string name=argv[1];
  if(name=="ocarina") NativeOcarina();
  else if(name=="controls") NativeCameraControls();
  else if(name=="transitions") NativeCameraTransitions();
  else if(name=="firstperson") NativeFirstPerson();
  else return 2;
  puts("PASS native camera case");
}
