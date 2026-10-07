# Changelog

## 0.1.0 ? first public release

Based on playtested internal revision 12.

- Add native Ctrl turbo toggle, Shift sprint toggle, and F9 turbo-off control.
- Keep turbo state across focus changes and continue background simulation with ordinary unfocused input suppressed.
- Add the DX9 turbo badge, with configurable position/style/visibility.
- Link supported SoraVoice dialogue volume to the existing SE slider and mute option, with adjustable relative gain.
- Separate physical pointer rendering from virtual menu-navigation coordinates to avoid cursor snapping and ghost trails.
- Keep the field's top bar open on mouse leave; Escape or an outside left click dismisses it.
- Handle stationary outside clicks without activating the old Status selection; consume the dismissal click through release.
- Clear the initial Status selection when opening the bar for mouse navigation.
- Restrict custom outside-click dismissal to the two field input call sites. Equipment and other modal menus reuse the parent widget and must keep their native transitions; closing that parent from a modal input loop could otherwise lock out cancellation.
- Add isolated regression coverage for field/modal scope, virtual coordinates, physical rendering, native clicks, and menu selection.

Separate third-party voices, instant-text changes, and detailed skill descriptions are credited and linked; their files are not part of this release.
