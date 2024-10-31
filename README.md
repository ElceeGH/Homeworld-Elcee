# Homeworld-Elcee

This is a source port of the original Homeworld based on the great work done in Homeworld SDL.

The objectives here are:
- Be faithful to the original game
- Make it buildable in modern Visual Studio
- Fix bugs
- Improve stability
- Support modern systems
- Improved graphics scalability for modern systems
- Restore animatic playback functionality



Changelog (experimental, 2024-Oct-31)

Bugfixes:
- Fix camera moving instantly on right mouse button press. It should have remained still until the mouse actually moved.
- Fix mouse moving at double speed after using the ship viewer and when making a selection in sensor view, until right mouse was clicked again.
- Fix mouse very rarely moving a huge distance on the first frame after exiting any mode where the mouse was held stationary.
- Fix objects incorrectly appearing partially transparent depending on what else is onscreen.
- Fix animated hit effects using the wrong texture in their last couple of frames.
- Fix animated hit effects sometimes being opaque instead of additively blended.
- Fix anti-aliasing causing ugly lines on the UI when using multisampling.
- Fix anti-aliasing causing blurry graphics on UI when using supersampling.
- Fix research UI lab button scanning effect speed being dependent on refresh rate.
- Fix tutorial pulsing effect being dependent on refresh rate.
- Fix sensor ping sounds triggering way too quickly when the framerate was high. Now limited to the same speed as Homeworld 1999.
- Fix buffer overflows in UI, sound, scripting, and logging systems. These may have caused unexplained crashes.
- Fix trails using an undefined alpha value and appearing far too dim as a result.
- Fix navigation lights moving to incorrect positions at high detail levels.
- Fix wildly inaccurate in-engine cutscene audio timing. Now plays as in the original game.
- Fix audio streamer using 100% of an entire CPU core at all times. It's nearly zero now.
- Fix animatic cutscenes not playing. They now play normally between missions, with audio.
- Fix intro videos not playing and not having sound. They now play with the correct music tracks.
- Fix the space key being typed as "s" typing in textboxes. It was trying to type the word "space" :)
- Fix sounds stopping after changing resolution in video options, then suddenly playing all at once again when in-game.
- Fix aspect ratio not updating after changing resolution in video options, distorting the game view.
- Fix crash on game exit when a multiplayer game was played during the session.
- Fix crash during multiplayer when a player unexpectedly leaves mid-game.
- Fix audio glitches caused by loading too fast. Loading time now has a minimum time accounting for this.
- Fix audio glitches in speech when running at very high framerates.
- Fix healthbar width changing when cloud lightning effects were onscreen.
- Fix sensor ping circles animating in a strange sequence when framerate was high, instead of simply growing and disappearing.
- Fix Gardens/Cathedral sensor interference update rate being too fast on >60Hz monitors.
- Fix Gardens/Cathedral sensor interference drawing dots even if they're behind the camera and shouldn't be seen. It's now done in 3D space.
- Fix missing stippling in movement line when issuing move commands. I also added the stippling animation like in the original game.
- Fix horizon angle display not taking aspect ratio into account properly and prematurely ending on widescreen displays.
- Fix incorrect blending in BTG backgrounds. This caused odd lines, holes, bad glows, and areas of missing colour.
- Fix stars being way too small and lacking texture filtering. Even in 1999 they were filtered in software. Kharak's star is big and bright again!
- Fix stars being scaled down when resolution increased. They're already resolution-independent. Likely was a mistaken fix due to missing filtering.
- Fix letterboxing in cutscenes. The aspect ratio for cutscenes is now always 2.35:1 unless the display is even wider than that.
- Fix loading bars sometimes being rendered as black until textures were loaded along the way.
- Fix build queue being limited by ship class. This was NOT in Homeworld 1999! I checked! You can now build every class of ship at once again.
- Fix potential crash when drawing active gravwell sphere in sensor view with tactical overlay enabled.
- Fix potential crash when drawing active proximity sensor in normal view with tactical overlay enabled.
- Fix objective and tutorial circles/lines mismatching more the further the screen aspect was from 1:1. Quite a pain to fix.
- Fix sensor ping legend icons overlapping in sensor view.
- Fix sensor view tactical overlay legend text being misaligned with the icons.
- Fix game being unresponsive when hitting the load game button due to waiting on audio fadeout. The fade and loading now happen simultaneously.
- Fix user screenshots being stretched when shown as backgrounds in multiplayer/skirmish loading screens.
- Fix max number of audio sources for fighters/corvettes being limited to the max for capital ships due to a typo.
- Fix cardiod audio filter never being used due to a typo.
- Fix mouse hover text giving the full name of a ship when the new max detail option was enabled, even if you didn't yet look closely at it.
- Fix some mission/AI/hovertext flags not being set correctly when full detail models are forced on. Some depend on how close you've looked!
- Fix raycasting not properly accounting for perspective when using the move command.  Deprojection is now implemented correctly for any aspect ratio.

Improvements:
- Objects now have interpolated movement and rotation, making the game animate much more smoothly.  Used to be fixed at 16 Hz.
- Particle systems and hyperspace effects now update and animate at the full framerate. Used to be fixed at 16 Hz.
- Various animated UI elements such as sensor pings and the tactical overlay now animate at full framerate.
- In-engine cutscenes now animate smoothly at full framerate.
- Backgrounds now render with temporal ordered dithering to remove banding artifacts. (Configurable in options file).
- Ship trails are now always rendered in detail in 3D, instead of turning into 2D lines a short distance from the camera.
- Ship trail engine glow now always reaches white at its centre no matter how dark the trail colour is.
- Rendering performance is greatly improved with the addition of a graphics state caching system.
- Rendering performance is improved by hardware accelerating capital ship trail specular lighting.
- Animatics now scale with resolution on widescreen monitors, instead of rendering in a little box.
- Level of detail for objects is now maximised in the main view.
- Level of detail for objects is now maximised in the sensor view for large ships like motherships, carriers and cruisers.
- Level of detail for healthbar/fuel/team overlay is now independent from ship detail level. Very ugly/obscuring if kept at full detail all the time.
- Level of detail for muzzle flashes is kept independent from ship detail level, in order to preserve their clarity at a distance.
- Effect frequency has been increased by default. Impact effects now trigger pretty much always upon bullet impact.
- Effect frequency scaling can be adjusted in the options file (EffectFrequencyPercent).
- Background stars now scale further with resolution, extrapolating the original design to modern resolutions.
- Sensor view dots now scale with resolution.
- Sensor view now fades in dots over big ships if they're so small on screen that a dot would be bigger.
- Tiny asteroid dots now scale with resolution.
- Tiny asteroid dots now fade into view, and are drawn at a slightly longer distance.
- Tiny asteroid dots blur when they're close to the camera, so you can tell they're really close.
- Ship navigation lights now scale with resolution at a distance.
- Fuzzy blobs on the sensor view now scale with resolution. (e.g. for resource dust clouds)
- Explosion and impact line effect thickness now scales with resolution, line length and distance from the camera.
- Sensor view and movement line thickness now scales with resolution.
- Sensor view plane marker line endings are now neatly capped off.
- Missile trail thickness now scales with resolution.
- Missile trails now have proportional thickness when they get near the camera.
- Missile trails now interpolate colour per-pixel rather than per-vertex, appearing smoother.
- Missile trails now fade out gently at the end of their life.
- Mines, when drawn as dots, now scale with resolution instead of being drawn as a single pixel. Keeps the same visibility/feel as the original.
- Mines now smoothly transition from 3D models to dots with no LOD popping.
- Circles are now rendered in much greater detail, so they actually look like circles.
- Savegames, configuration, logs and such are now all stored in appdata instead of the game's directory.
- Savegame names can now be quite a bit longer. Changed from 32 to 52 characters. They were always annoyingly short, right?
- The appdata directory is now called "Homeworld" on Windows as opposed to the Linux-style ".homeworld".
- Take screenshots with PrintScreen. Used to be Scroll Lock.
- Loading time between levels has been [optionally] extended, allowing the path through the galaxy to be seen when playing.
- Loading time can be manually adjusted in the options file (LoadingTimeMinimumMillisecs).
- Loading screens now redraw much more often, making the loading bar much smoother.
- Loading screens in multiplayer ("horse race") now update the loading bars for each player much more frequently, so it's more fun.
- Antialiasing can now be controlled in the options file (screenMSAA). Defaults to 16x.

Changes (from Homeworld 1999):
- Hyperspace effects now show the glowing cross-section where the hyperspace field intersects the ship, rather than empty shells being seen.
- Space now contains dust motes to improve perception of movement. The density varies by level.
- Nebula tendrils render as soft fuzzy lines in the sensor view, rather than solid lines, to match dust clouds in their fuzziness.
- Launch manager now shows the maximum slots for fighters/corvettes numerically too ("count/max" instead of just "count").
- You can change your ships' trail colours in the options file, separately for begin and end of trail. This only affects singleplayer campaign games.
- NLIPS scaling of ships is very slightly reduced proportionally with increase in resolution (at most objects are allowed to be 15% smaller).
- When issuing move commands to ships off-plane, the line between the two planes is now stippled. It's much more readable.

