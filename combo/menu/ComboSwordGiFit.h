#ifndef COMBO_SWORD_GI_FIT_H
#define COMBO_SWORD_GI_FIT_H

extern "C" int ResourceMgr_GetGiModelFitForGame(const char* owner, const char* path, float scale, float tilt, int shop,
                                                float fit[2]);
extern "C" int ResourceMgr_GetGiModelsFitForGame(const char* owner, const char* const* paths, int count, float scale,
                                                 float tilt, int presentation, float fit[2]);

static inline void ComboSwordGi_ApplyModelsFit(const char* owner, const char* const* paths, int count, float scale,
                                               float tilt, bool shop = false, int mmPickup = 0) {
    float fit[2] = { 1.f, 0.f };
    if (ResourceMgr_GetGiModelsFitForGame(owner, paths, count, scale, tilt,
                                          shop       ? 1
                                          : mmPickup ? 1 + mmPickup
                                                     : 0,
                                          fit)) {
        Matrix_Translate(0.f, fit[1], 0.f, MTXMODE_APPLY);
        Matrix_Scale(fit[0], fit[0], fit[0], MTXMODE_APPLY);
    }
}

static inline void ComboSwordGi_ApplyFit(const char* owner, const char* path, float scale, float tilt,
                                         bool shop = false, int mmPickup = 0) {
    float fit[2] = { 1.f, 0.f };
    // Preserve the C integer seam: world 0, shop 1, MM receipt 2, Goron 3.
    if (ResourceMgr_GetGiModelFitForGame(owner, path, scale, tilt, shop ? 1 : mmPickup ? 1 + mmPickup : 0, fit)) {
        Matrix_Translate(0.f, fit[1], 0.f, MTXMODE_APPLY);
        Matrix_Scale(fit[0], fit[0], fit[0], MTXMODE_APPLY);
    }
}

#endif
