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
)
target_include_directories(OneManRestaurant PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(OneManRestaurant PRIVATE cxx_std_17)
target_link_libraries(OneManRestaurant PRIVATE
        glfw
        glad::glad
        glm::glm
        assimp::assimp
)

if(WIN32)
    target_link_libraries(OneManRestaurant PRIVATE windowscodecs ole32)
endif()

if(MSVC)
    target_compile_options(OneManRestaurant PRIVATE /W4)
else()
    target_compile_options(OneManRestaurant PRIVATE -Wall -Wextra -Wpedantic)
endif()
