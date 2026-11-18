# Spin Attack and PAK Loader popouts

Parent: `deb0e47392b6b68a5dbedb94ebdf98fc555b906c`.

Spin Attack settings now have a popout button beside Spin Style in each game's
Graphics menu. The window contains the existing variant dropdown and Elemental
Spin Effects checkbox. SoH's NEI > Pak Loader sidebar has a popout button for
its complete model/outfit/equipment/slot/voice panel.

The detached panels render the original WidgetInfo objects through each host's
native MenuDrawItem, preserving live callbacks, selectors, disabled/race rules,
What's New styling and CVar persistence. Window buttons are excluded from the
detached groups to prevent recursive embedding. MM now honors its existing
WindowButtonOptions::embedWindow flag, retaining the default behavior for other
window buttons.

Popouts have independent host visibility keys and unique ImGui window IDs.
Foreground guards suppress the inactive game's copy while preserving the
user's open/closed preference across game changes. Weak menu references avoid
retaining an obsolete menu; registration can rebind an existing window.

Full changed menu translation units are checked with actual native headers in
standalone and Combo configurations. This is compile verification; opening,
closing, dragging, changing settings with the main menu closed, and switching
games still need a configured UI runtime check. No models or assets change.
