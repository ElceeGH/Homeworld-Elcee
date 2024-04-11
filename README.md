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



Changelog (experimental, 2021-Nov-08)
- Fix camera moving instantly on right mouse button press. It should have remained still until the mouse actually moved.
- Fix objects incorrectly appearing partially transparent depending on what else is onscreen.
- Fix animated hit effects using the wrong texture in their last couple of frames.
- Fix animated hit effects sometimes being opaque instead of additively blended.
- Fix anti-aliasing causing ugly lines on the UI when using multisampling.
- Fix anti-aliasing causing blurry graphics on UI when using supersampling.
- Fix research UI lab button scanning effect speed being dependent on refresh rate.
- Fix tutorial pulsing effect being dependent on refresh rate.
- Fix sensor view ping sound crackling and clipping, matching its volume with the original game.
- Fix buffer overflows in UI, sound, scripting, and logging systems. These may have caused unexplained crashes.
- Fix trails using an undefined alpha value and appearing far too dim as a result.
- Fix navigation lights moving to incorrect positions at high detail levels.
- Fix wildly inaccurate in-engine cutscene audio timing. Now plays as in the original game.
- Fix audio streamer using 100% of an entire CPU core at all times. It is now nearly zero.
- Fix animatic cutscenes not playing. They now play normally between missions, with audio.
- Fix intro videos not playing and not having sound. They now play with the correct music tracks.
- Fix the space key being typed as "s" typing in textboxes. It was trying to type the word "space" :)
- Fix sounds stopping after changing resolution in video options, then suddenly playing all at once again when in-game.
- Fix aspect ratio not updating after changing resolution in video options, distorting the game view.
- Fix crash on game exit when a multiplayer game was played during the session.
- Fix crash during multiplayer when a player unexpectedly leaves mid-game.
- Fix audio glitches caused by loading faster than the fadeout time. Loading time has been extended to take a few seconds.
- Fix audio glitches in speech when running at very high framerates.
- Fix healthbar width changing when cloud lightning effects were onscreen.

- Backgrounds now render with temporal dithering to remove banding artifacts.
- Hyperspace gates now render the intersected hull cross section, rather than just clipping the graphics in an ugly way.
- Ship trails are now always rendered in detail in 3D, instead of turning into 2D lines a short distance from the camera.
- Animatics now scale with resolution on widescreen monitors, instead of rendering in a little box.
- Level of detail for objects is now much higher in the main view.
- Level of detail for objects is now maximised in the sensor view for large ships.
- Level of detail for healthbar/fuel/team overlay is now independent from ship detail level.
- Effect density has been increased. Hit effects now trigger more often for example.
- Background stars now scale further with resolution, extrapolating the original design.
- Sensor view dots now scale with resolution.
- Tiny asteroid dots now scale with resolution.
- Ship navigation lights now scale with resolution at a distance.
- Explosion and impact line effect thickness now scales with resolution.
- Sensor view and movement line thickness now scales with resolution.
- Missile trail thickness now scales with resolution.
- Missile trails now interpolate colour per-pixel rather than per-vertex, appearing smoother.
- Mines now scale with resolution at a distance, instead of being drawn as a single pixel.
- Circles are now rendered in much greater detail, so they actually look like circles.
- Savegames, configuration, logs and such are now all stored in appdata instead of the game's directory.
- The appdata directory is now called "Homeworld" on Windows as opposed to the Linux-style ".homeworld".
- Take screenshots with PrintScreen. Used to be Scroll Lock.
- Loading time between levels has been artificially extended, allowing the path through the galaxy to be seen when playing.
- Loading screens now redraw much more often, making the loading bar much smoother.
- Loading screens now update the loading bars for each player much more frequently in multiplayer.
- Antialiasing can now be controlled in the options file (screenMSAA).
