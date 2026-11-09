#pragma once
// Real native layouts and accepting action callbacks. Shared by production
// eligibility and the native fixture; no synthetic actor layouts.
extern "C" {
#include "overlays/actors/ovl_Dm_Char01/z_dm_char01.h"
#include "overlays/actors/ovl_En_Dai/z_en_dai.h"
#include "overlays/actors/ovl_Dm_Char08/z_dm_char08.h"
#include "overlays/actors/ovl_En_Gb2/z_en_gb2.h"
#include "overlays/actors/ovl_En_Gg/z_en_gg.h"
#include "overlays/actors/ovl_En_Hg/z_en_hg.h"
#include "overlays/actors/ovl_En_Yb/z_en_yb.h"
#include "overlays/actors/ovl_En_Zog/z_en_zog.h"
#include "overlays/actors/ovl_En_Bsb/z_en_bsb.h"
#include "overlays/actors/ovl_En_Hidden_Nuts/z_en_hidden_nuts.h"
#include "overlays/actors/ovl_En_Gk/z_en_gk.h"
#include "overlays/actors/ovl_En_Po_Composer/z_en_po_composer.h"
#include "overlays/actors/ovl_En_Time_Tag/z_en_time_tag.h"
void func_80AA8698(DmChar01*, PlayState*);
void func_80B3EEDC(EnDai*, PlayState*);
void DmChar08_WaitForSong(DmChar08*, PlayState*);
void func_80B0FEBC(EnGb2*, PlayState*);
void func_80B35450(EnGg*, PlayState*);
void EnHg_PlayCutscene(EnHg*, PlayState*);
void EnHg_HandleCutscene(EnHg*, PlayState*);
void EnYb_Idle(EnYb*, PlayState*);
void func_80B9451C(EnZog*, PlayState*);
void func_80B948A8(EnZog*, PlayState*);
void func_80C0BFE8(EnBsb*, PlayState*);
void func_80BDB2B8(EnHiddenNuts*, PlayState*);
void func_80B5202C(EnGk*, PlayState*);
void EnPoComposer_PlayCurse(EnPoComposer*, PlayState*);
void EnTimeTag_RooftopOath_Wait(EnTimeTag*, PlayState*);
}
