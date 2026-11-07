# One Man Restaurant

A C++/OpenGL game project. This is the **clean build test**, not the game yet.
It intentionally has no external C++ dependencies so the compiler and CLion
can be verified before adding OpenGL, Assimp, or Blender model loading.

## Requirements (Windows)

- CLion, or another CMake-compatible IDE
- Microsoft Visual C++ (MSVC) toolchain with the Windows SDK; for example,
  Visual Studio Build Tools with the **Desktop development with C++** workload
- CMake 3.21 or newer
- Ninja (CLion bundles CMake and Ninja)

## Run in CLion

1. Open the folder containing `CMakeLists.txt` in CLion.
2. Under `Settings > Build, Execution, Deployment > Toolchains`, add a
   **Visual Studio** toolchain. Ensure the compiler is detected. Name the
   CLion toolchain **Visual Studio**, exactly, because the shared preset uses
   that name to select it. This is a name, not an installation path.
3. Under `Settings > Build, Execution, Deployment > CMake`, enable the
   `windows-msvc-debug` preset. Disable any old generic `Debug` profile
   that is tied to MinGW.
4. Reload CMake, select `OneManRestaurant`, and click Run.

Expected output:

    One Man Restaurant
    C++ compiler and CMake are working.
    Next: add the OpenGL window and 3D camera.

## Build from the command line

From an **x64 Native Tools Command Prompt for VS** (which sets up MSVC):

    cmake --preset windows-msvc-debug
    cmake --build --preset windows-msvc-debug
    .\build\windows-msvc-debug\OneManRestaurant.exe

The CLion-specific `vendor` entry in CMakePresets.json is ignored by
command-line CMake. All paths in the shared build configuration are relative
or tool-resolved, so no developer-specific folder is required.

## Project structure

- `src/` — C++ source code
- `assets/models/` — exported .glb assets (added later)
- `blender/` — editable Blender .blend sources (added later)
- `CMakeLists.txt` — defines which files compile
- `CMakePresets.json` — portable build configuration

## Next milestones

1. Add GLFW, GLAD, and GLM via a vcpkg manifest using the `x64-windows` triplet.
2. Open a 3D window and render a simple cube from a camera.
3. Add Assimp and load `assets/models/office.glb` from Blender.
4. Package executable, DLLs, and assets for the professor.

Note: the current starter does not open a 3D window or load a Blender model.
