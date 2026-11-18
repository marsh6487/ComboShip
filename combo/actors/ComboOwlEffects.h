#pragma once
#include <string.h>

// The caller supplies the opened statue's raw-model pose. Optional assets gate
// the effect; ordinary owl packs and closed statues keep their existing draw.
#ifdef __cplusplus
extern "C" {
#endif
struct PlayState;
void ComboOwl_DrawEffects(struct PlayState* play, const char* owner);
#ifdef __cplusplus
}
#endif

static inline int ComboOwl_IsOpenedModel(const char* path) {
    if (!path)
        return 0;
    const char* root = strstr(path, "objects/object_sek/");
    return root && !strcmp(root, "objects/object_sek/gOwlStatueOpenedDL");
}
