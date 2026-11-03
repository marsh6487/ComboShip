#pragma once
#include "global.h"
typedef struct PlayState {
    int sceneNum;
    struct {
        int state;
    } csCtx;
    struct {
        unsigned char natureAmbienceId = 2;
    } sequenceCtx;
} PlayState;
typedef struct {
    int nightFlag;
} NightTestSaveContext;
enum { SCENE_HYRULE_FIELD = 0x51, CS_STATE_IDLE = 0 };
