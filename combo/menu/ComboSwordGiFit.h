#ifndef COMBO_SWORD_GI_FIT_H
#define COMBO_SWORD_GI_FIT_H

extern "C" int ResourceMgr_GetGiModelFitForGame(const char* owner, const char* path, float scale, float tilt,
                                                int shop, float fit[2]);

static inline void ComboSwordGi_ApplyFit(const char* owner, const char* path, float scale, float tilt,
                                         bool shop = false) {
    float fit[2] = { 1.f, 0.f };
    if (ResourceMgr_GetGiModelFitForGame(owner, path, scale, tilt, shop, fit)) {
        Matrix_Translate(0.f, fit[1], 0.f, MTXMODE_APPLY);
        Matrix_Scale(fit[0], fit[0], fit[0], MTXMODE_APPLY);
    }
}

#endif
