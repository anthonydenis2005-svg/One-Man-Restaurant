cmake_minimum_required(VERSION 3.21)

project(OneManRestaurant VERSION 0.2.0 LANGUAGES C CXX)

find_package(glfw3 CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)

add_executable(OneManRestaurant
    src/main.cpp
    src/Game.cpp
    src/Camera.cpp
    src/Renderer.cpp
    src/ModelLoader.cpp
    src/Input.cpp
    src/Audio.cpp
)
target_include_directories(OneManRestaurant PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(OneManRestaurant PRIVATE cxx_std_17)
target_link_libraries(OneManRestaurant PRIVATE
        glfw
        glad::glad
        glm::glm
        assimp::assimp
)

add_custom_command(TARGET OneManRestaurant POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:OneManRestaurant>/assets
)

if(WIN32)
    target_link_libraries(OneManRestaurant PRIVATE windowscodecs ole32)
elseif(APPLE)
    target_link_libraries(OneManRestaurant PRIVATE
        "-framework CoreFoundation"
        "-framework CoreAudio"
        "-framework AudioToolbox"
    )
endif()

if(MSVC)
    target_compile_options(OneManRestaurant PRIVATE /W4)
else()
    target_compile_options(OneManRestaurant PRIVATE -Wall -Wextra -Wpedantic)
endif()