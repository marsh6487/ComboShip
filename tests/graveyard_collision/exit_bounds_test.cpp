#include <cassert>
#include <cstdint>
#include "scene_exit.h"
int main() {
 int16_t exits[3]={1,2,3};
 assert(SceneExit_IsValid(exits,3,1));assert(SceneExit_IsValid(exits,3,3));
 assert(!SceneExit_IsValid(exits,3,4));assert(!SceneExit_IsValid(exits,3,0));
 assert(!SceneExit_IsValid(exits,3,-1));assert(!SceneExit_IsValid(exits,3,(int32_t)0x80000046));
 assert(!SceneExit_IsValid(nullptr,3,1));assert(!SceneExit_IsValid(exits,0,1));
 assert(SceneExit_IsValid(exits,-1,31));assert(!SceneExit_IsValid(exits,-1,32));
}
