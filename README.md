# One Man Restaurant

CAP4052 - Game Design & Development  
Homework #3 - Game Project, Phase 1  
Genre: 3D Management / Arcade / Resource Management

## Project Members

- Anthony Denis - 6471798
- Jaylena Burgos - 6405502

## Game Concept

One Man Restaurant is a 3D restaurant management game where the player is an understaffed manager trying to keep a restaurant running from an office. The planned game revolves around multitasking, completing paperwork, monitoring the restaurant, and responding to problems before they become too costly.

This phase is a playable prototype focused on the office environment, object interaction, and basic restaurant management systems. Later phases are intended to introduce employees, customers, security cameras, and more complex resource management.

## Objective

Survive the restaurant shift while keeping customer happiness above 0%. The current prototype starts with a 180-second shift and 100% customer happiness. Paperwork and fire alarm events can occur during the shift. Ignoring these tasks for too long reduces customer happiness.

- Reaching 0% customer happiness causes a game-over state.
- Reaching the end of the shift prints a victory message in the console.

The proposed profit and money-management systems are not implemented yet.

## Controls

| Input | Action |
| Hold middle mouse button and move mouse | Look around the office |
| Left mouse button | Click an interactive object, including the clipboard |
| Right mouse button | Close the raised clipboard |
| Left click outside the raised clipboard | Close the clipboard |
| `1` | Complete the payroll paperwork task when active |
| `2` | Complete the safety-check paperwork task when active |
| `3` | Complete the inventory paperwork task when active |
| `E` | Turn off an active fire alarm |
| `Esc` | Exit the game |

The camera is temporarily locked while the clipboard is raised. The fire alarm currently uses the `E` key rather than a completed 3D control interaction.

## Implemented Features

- A real-time game loop with separate input, game-state updates, and OpenGL rendering.
- A 3D manager's office loaded from a Blender-exported GLB model.
- Mouse-controlled camera rotation and 3D object interaction through ray-versus-bounding-box tests.
- A clickable clipboard that moves toward the camera, displays a paperwork image, and returns to the desk when closed.
- Separate interaction groups for the clipboard and fire alarm objects. The fire alarm's clickable gameplay action is not connected yet.
- A 180-second shift timer and customer happiness value displayed in the window title.
- Periodic paperwork events with three tasks completed using keys `1`, `2`, and `3`.
- Fire alarm events that can be cleared with `E`, with a red flashing background while active.
- Customer happiness penalties for paperwork or fire alarm events left unresolved for too long.
- Basic success and failure conditions, including console feedback and a game-over window title.
- Reusable `spawnText("ModelName")` and `hideText()` functions for displaying a named Blender text mesh in front of the camera. These functions are available but are not yet called by the gameplay code.

## Technology and Dependencies

- C++17 for game logic and project structure.
- OpenGL 3.3 for rendering.
- GLFW for window creation and mouse/keyboard input.
- GLAD for OpenGL function loading.
- GLM for camera math and 3D transformations.
- Assimp for importing the GLB scene and materials.
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
- `src/Camera.cpp`: camera transforms and mouse-ray collision calculations.
- `src/Input.cpp`: mouse input callbacks.
- `src/ModelLoader.cpp`: GLB model loading, materials, mesh data, and interaction groups.
- `src/Renderer.cpp`: OpenGL rendering, textures, clipboard animation, and reusable text overlay functions.
- `assets/models/`: the exported GLB environment.
- `assets/textures/clipboard/`: PNG images used for paperwork.
- `blender/`: Blender project files, where included.

## Known Limitations and Next Steps

- Paperwork tasks are completed with number keys rather than a complete mouse-based paperwork interface.
- Profit, employee management, customers, and security camera gameplay from the original proposal are still planned for later phases.
- The shift-complete message is printed to the console; a complete win screen, restart flow, and multiple days are not implemented.
- The reusable text overlay functions are not yet connected to a game event.
- This is a prototype and may require additional testing, balancing, and visual polish.

## Development Plan

- HW #3: 3D office, camera and object interaction, clipboard, basic paperwork, alarm events, timer, and customer happiness.
- HW #4: Add employee/customer behaviors, security cameras, a more complete paperwork interface, and playtesting improvements.
- HW #5: Add difficulty scaling, more interconnected management systems, sound/animation, balance, and final polish.

## Gameplay Video

https://youtu.be/d3K2K8w1mOc

## Contributions

Anthony Denis - Worked on the Camera, Blender environment and portions of the Renderer
Jaylena Burgos - Worked on the Game State, portions of the Renderer and foundations of future features


## External Assets and Credits

- Blender was used to create and edit the game environment.
- GLFW, GLAD, GLM, and Assimp are third-party libraries used by the project.


## Generative AI Use

ChatGPT was used during development for C++/OpenGL implementation guidance, troubleshooting and debugging.

