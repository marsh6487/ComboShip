#include "src/overlays/actors/ovl_En_Viewer/static_story_actor.h"
#include "z64object.h"
#include <assert.h>
#include <stdio.h>

/* Missing regional decode, accidental interaction, or wrong resource ownership
 * must fail at the catalogue's first placement consumer. */
int main(void) {
  const int16_t params[] = {0x7D04, 0x7D05, 0x7D06, 0x7D07, 0x7D08, 0x7D09};
  for (unsigned index = 0; index < 6; ++index) {
    StaticStoryActorType type = StaticStoryActor_GetType(params[index]);
    assert(type == 29 + index);
    assert(StaticStoryActor_IsAvailable(type));
    const StaticStoryActorDefinition *definition =
        StaticStoryActor_GetDefinition(type);
    StaticStoryObjectRequirements objects =
        StaticStoryActor_GetObjectRequirements(type);
    assert(objects.modelObjectId == (index < 5 ? OBJECT_FR : OBJECT_TK));
    assert(objects.animationObjectId == objects.modelObjectId);
    assert(definition->scale == (index < 5 ? 0.015f : 0.01f));
    assert(!StaticStoryActor_CanTalk(type));
    assert(!StaticStoryActor_CanTrack(type, 0));
    assert(StaticStoryActor_SelectTextId(type, NULL) == 0);
    assert(StaticStoryActor_GetResourceSource(type) ==
           STATIC_STORY_RESOURCE_OOT_OBJECT);
    for (unsigned pose = 0; pose < 16; ++pose) {
      assert(StaticStoryActor_GetType(params[index] | (pose << 4)) == type);
      assert(StaticStoryActor_SanitizePose(type, pose) == 0);
      assert(StaticStoryActor_ResolvePose(type, pose)->animation !=
             STATIC_ANIM_NONE);
    }
  }
  assert(StaticStoryActor_GetType(0x7D03) == STATIC_STORY_ACTOR_CHILD_ZELDA);
  assert(StaticStoryActor_GetType(0x7D0A) == STATIC_STORY_ACTOR_NONE);
  puts("PASS six decorative catalogue placements, resource ownership and safe "
       "pose decode");
}
