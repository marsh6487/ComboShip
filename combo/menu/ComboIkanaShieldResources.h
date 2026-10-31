#ifndef COMBO_IKANA_SHIELD_RESOURCES_H
#define COMBO_IKANA_SHIELD_RESOURCES_H

#define COMBO_IKANA_GI_OPA "__OTR__objects/ikana_shield_gi/gIkanaShieldGiOpaDL"
#define COMBO_IKANA_GI_XLU "__OTR__objects/ikana_shield_gi/gIkanaShieldGiXluDL"
#define COMBO_IKANA_GI_OPA_MM "__OTR__@mm:objects/ikana_shield_gi/gIkanaShieldGiOpaDL"
#define COMBO_IKANA_GI_XLU_MM "__OTR__@mm:objects/ikana_shield_gi/gIkanaShieldGiXluDL"
// Solo OoT's strict optional ROM archive contains the original native graph.
#define COMBO_IKANA_NATIVE_OPA "__OTR__objects/object_gi_shield_3/gGiMirrorShieldEmptyDL"
#define COMBO_IKANA_NATIVE_XLU "__OTR__objects/object_gi_shield_3/gGiMirrorShieldDL"

#ifdef __cplusplus
extern "C" {
#endif
// Both native passes must be usable. Authored GI resources remain fallback only.
int ResourceMgr_IsIkanaShieldGiAvailable(void);
#ifdef __cplusplus
}
#endif

#endif
