/* Shared whip state and reach constants. Safe before the item unity initializer:
 * this header intentionally declares no state aliases, tables or colliders. */
#ifndef ITEM_WHIP_CONSTANTS_H
#define ITEM_WHIP_CONSTANTS_H

// =============================================================================
// States
// =============================================================================
#define WHIP_STATE_INACTIVE 0   // Not active
#define WHIP_STATE_EQUIP 1      // Held, coiled rope visible in hand
#define WHIP_STATE_EXTENDING 2  // Rope traveling forward
#define WHIP_STATE_HIT_ENEMY 3  // Hit enemy, processing interaction
#define WHIP_STATE_ATTACHED 4   // Attached to surface/actor, preparing swing
#define WHIP_STATE_SWINGING 5   // Pendulum swing active
#define WHIP_STATE_RETRACTING 6 // Rope returning to Link
#define WHIP_STATE_LAUNCHED 7   // Post-release momentum coast (keeps whip active briefly)

// =============================================================================
// Lash reach: 52 frames at 20 units/frame. Swing length remains fixed below.
// =============================================================================
#define WHIP_RANGE 1040.0f
#define WHIP_EXTEND_SPEED 20.0f
#define WHIP_RETRACT_SPEED 25.0f
#define WHIP_TIMER_MAX 52

#endif // ITEM_WHIP_CONSTANTS_H
