# One Man Restaurant

CAP4052 - Game Design & Development  
Homework #3 - Game Project, Phase 2
Genre: 3D Management / Arcade / Resource Management

## Project Members

- Anthony Denis - 6471798
- Jaylena Burgos - 6405502

## Game Concept

One Man Restaurant is a 3D restaurant management game where the player is an understaffed manager trying to keep a restaurant running from an office. The planned game revolves around multitasking, completing paperwork, monitoring the restaurant, and responding to problems before they become too costly.

This phase is a playable alpha version continuing from Hw3 prototype with added a connecting restaurant scene with live CCTV monitoring, interactive paperwork stamping, on screen satisfaction bar, pause menu, office camera panning, analog shift clock, mini_audio based sounds, and customer/restaurant condition state tracking which will be more utilized when the player models are implemented in Phase 3 of the project. Phase 3 will intend to finalize player models and a more complex resource management built into the day. In addition to level difficulty as the player progresses through the levels. 

## Objective

Survive the restaurant shift while keeping customer happiness above 0%. The current prototype starts with a 180-second shift and 100% customer happiness mapped onto a 9-5 shift clock. Paperwork and fire alarm events can occur during the shift ignoring these tasks for too long reduces customer happiness. In addition to customer/resturant incidents that will drain happiness over time or trigger penalties. 

- Reaching 0% customer happiness causes a game-over state.
- Reaching the end of the shift prints a victory message in the console.
- Pressing P will toggle a pause overlay to reset the day, quit game, or continue when player is   ready

The proposed profit and money-management systems are not implemented yet.

## Controls

| Input | Action |
| On screen left/right arrow| smoothly pan the office camera (70 degrees)
| Left mouse button | Click an interactive object, including the clipboard |
| Right mouse button | Close the raised clipboard |
| Left click outside the raised clipboard | Close the clipboard |
| `E` | Turn off an active fire alarm |
|`P` or `Esc` | Pause/resume active gameplay (when not in Game Over or Shift Completed)
|`Enter`| Resume gameplay while paused
|`R` | Restart the shift when in Game over or Shift completed 

The camera is temporarily locked while the clipboard is raised, during active screen-edge panning, or while a model menu overlay is open.

## Implemented Features

- A real-time game loop with separate input, game-state updates, and OpenGL rendering.
- A 3D manager's office loaded from a Blender-exported GLB model.
- Mouse-controlled camera rotation and 3D object interaction through ray-versus-bounding-box tests.
- A clickable clipboard that moves toward the camera, displays a paperwork image, and returns to   the desk when closed.
- Separate interaction groups for the clipboard and fire alarm objects.
- A 180-second shift timer and customer happiness value displayed in the window title.
- Fire alarm events that can be cleared with `E` or `Left-click` , with a red flashing             background while active.
- Analog Shift Clock which will map shift timer progress across a 9:00-5:00 workday, draws dial    marking procedurally on `ClockBase`, and rotates `ClockHour` and `ClockMinute` hands.   
- Customer happiness penalties for paperwork or fire alarm events left unresolved for too long.
- Basic success and failure conditions, including console feedback and a game-over window title.
- Reusable `spawnText("ModelName")` and `hideText()` functions for displaying a named Blender      text mesh in front of the camera. These functions are available but are not yet called by the    gameplay code.
- Customer & restaurant Hazard States: tracks fire alarm triggers/silencing and `CustomerState`    conditions including sprinklers, room vs. comfort temperature.
- Modular audio System: Utilizing miniaudio.h to manage looping tracks and one shot sound          effects, pausing/resuming and supporting master volume adjustment. More sound effects will be    added into game with the incoming Phase 3
- Screen UI, satisfaction HUD & Camera Panning: Renders an on screen "Satisfaction" percentage indicator along with a "-PENALTY" popup text, paper-styled Pause (including Game over/Shift Completed menus, and 70 degree directional camera panning with screen arrows)

## Technology and Dependencies

- C++17 for game logic and project structure.
- OpenGL 3.3 for rendering.
- GLFW for window creation and mouse/keyboard input.
- GLAD for OpenGL function loading.
- GLM for camera math and 3D transformations.
- Assimp for importing the GLB scene and materials.
- miniaudio (including/miniaudio.h) for audio playback and looping/one-shot sound management.
- Blender for creating and exporting the 3D environment.
- CMake and Ninja for building the project.
- vcpkg for managing C++ libraries.

The current CMake configuration provides a Windows MSVC Debug preset. Other toolchains and operating systems have not been verified for this submission.

## Build and Run Instructions (Windows)

1. Install Visual Studio with the Desktop development with C++ workload, CMake, Ninja, Git, and vcpkg. CLion can be used as the IDE.
2. Install or set up vcpkg, then set the `VCPKG_ROOT` environment variable to the folder containing vcpkg. The project uses `vcpkg.json` to declare its dependencies.
3. Open the project folder in CLion with a Visual Studio toolchain, or open a Visual Studio developer terminal in the project folder.
4. Configure and build the game:

   ```powershell
   cmake --preset windows-msvc-debug
   cmake --build build/windows-msvc-debug
   ```

5. Run the executable from the project folder:

   ```powershell
   .\build\windows-msvc-debug\OneManRestaurant.exe
   ```

Alternatively, choose the `windows-msvc-debug` CMake preset in CLion, build, and run `OneManRestaurant`.

Keep the `assets` folder with the project. At startup, the game searches for `assets/models/office.glb` from the working directory or a nearby parent directory. The paperwork PNG files are loaded from `assets/textures/clipboard/` (or `Clipboard/`). If the model or images cannot be found, confirm that the assets were included and that the executable is running from the correct project location.

## Code Organization

- `src/main.cpp`: initializes the game and handles the main loop and object clicks.
- `src/Game.cpp` and `include/GameState.h`: shift timer, customer happiness, paperwork, alarms, and game state.
- `src/Paperwork.cpp` and `include/Paperwork.h.cpp`: paperwork sheet assignment, ray-triangle hit testing on the 3x4 grid, checkbox/stamp logic, and sheet slide transitions.
- `src/Camera.cpp`:camera transforms and mouse-ray collision calculations.
- `src/ScreenUI.cpp`: screen-space UI rendering,satisfaction HUD, Pause/end menus, menu/pan actions, and 70 degree office panning
- `src/ShiftClock.cpp`: shift hour calculation and hour/minute hand transforms
- `src/Audio.cpp`: miniaudio initalization, shutdown,one-shot playback, looping state sync, and volume adjustment.
- `src/Input.cpp`: mouse input callbacks.
- `src/ModelLoader.cpp`: GLB model loading, materials, mesh data, and interaction groups.
- `src/Renderer.cpp`: OpenGL rendering, textures, clipboard animation, and reusable text overlay functions.
- `assets/models/`: the exported GLB environment.
- `assets/textures/clipboard/`: PNG images used for paperwork.
- `blender/`: Blender project files, where included.

## Known Limitations and Next Steps

- Some customer/resturand incident flags in CustomerState are modeled in but are not connected with the full interactive resturant. Next step will be to get the character model for final Phase
- Audio files will be needing more adjustments and needing to add more interactive noise while interacting with the office.
- Player models will need to be implemented along with movement and set up with the customer states that are already made.
- Profit, employee management, customers, and security camera gameplay from the original proposal are still planned for later phases.
- The reusable text overlay functions are not yet connected to a game event.
- This is a prototype and may require additional testing, balancing, and visual polish.

## Development Plan

- HW #3: 3D office, camera and object interaction, clipboard, basic paperwork, alarm events, timer, and customer happiness.
- HW #4: Add employee/customer behaviors, security cameras, a more complete paperwork interface, and playtesting improvements.
- HW #5: Add difficulty scaling, more interconnected management systems, sound/animation, balance, and final polish.

## Gameplay Video

Phase 1:https://youtu.be/d3K2K8w1mOc
Phase 2:https://www.youtube.com/watch?v=_5OshY-bocg&feature=youtu.be

## Contributions

Anthony Denis:
- restaurant model was completed and connected with the office computer with the cameras.
- player movement updated to look around and added a customer satisfaction on screen
- Clipboard was finalized, pause menu added along with the clock

Jaylena Burgos: 
- Integrating the Audio system and added office background audio along with alarm sound to go off when ringing
- Core game state was updated to incudle all player models that will be utilized in Phase 3, and Alarm handeling with how it works with the audio
- Customer satisfaction and incident rules


## External Assets and Credits

- Blender was used to create and edit the game environment.
- GLFW, GLAD, GLM, and Assimp are third-party libraries used by the project.
- miniaudio (include/miniaudio.h) by David Reid (mackron/miniaudio) — Public Domain / MIT-0 single-header audio library.


## Generative AI Use

ChatGPT was used during development for C++/OpenGL implementation guidance, troubleshooting and debugging.

