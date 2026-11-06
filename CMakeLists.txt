cmake_minimum_required(VERSION 3.21)
project(OneManRestaurant LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(glfw3 CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)

add_executable(OneManRestaurant src/main.cpp)

target_compile_definitions(OneManRestaurant PRIVATE
    GLFW_INCLUDE_NONE
    OFFICE_MODEL_PATH="${CMAKE_CURRENT_SOURCE_DIR}/assets/models/office.glb"
)

target_link_libraries(OneManRestaurant PRIVATE
    glfw
    glad::glad
    glm::glm
    assimp::assimp
)
