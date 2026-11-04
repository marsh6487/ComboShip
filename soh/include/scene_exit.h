#ifndef SOH_SCENE_EXIT_H
#define SOH_SCENE_EXIT_H
#include <stdint.h>

// OTR/Unbound lists carry their exact length. Raw N64 scene commands have no
// count; their packed surface exit field is restricted to indices 1..31.
static inline int SceneExit_IsValid(const int16_t* exits, int32_t count, int32_t index) {
    return exits != 0 && index > 0 && (count >= 0 ? index <= count : index <= 31);
}
#endif
