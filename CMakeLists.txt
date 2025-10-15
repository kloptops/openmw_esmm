cmake_minimum_required(VERSION 3.10)
project(openmw_esmm CXX)

# Use C++14 standard
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake/")

# Find SDL2 and its libraries
find_package(SDL2 REQUIRED)
find_package(SDL2_ttf REQUIRED)

# Find Boost, requiring the filesystem component
find_package(Boost REQUIRED COMPONENTS filesystem program_options)

# --- GATHER SOURCE FILES ---
# THIS IS THE FIRST FIX: Add the backends directory to the GLOB
file(GLOB IMGUI_SOURCES
    "libs/imgui/*.cpp"
    "libs/imgui/backends/*.cpp"
)
# Application sources
file(GLOB_RECURSE APP_SOURCES "src/*.cpp")

# --- DEFINE THE EXECUTABLE ---
add_executable(openmw_esmm ${APP_SOURCES} ${IMGUI_SOURCES})

# --- Define Font Path ---
set(FONT_PATH "assets/DejaVuSansMono.ttf")
target_compile_definitions(openmw_esmm PUBLIC FONT_PATH="${FONT_PATH}")

# --- Link Libraries ---
target_include_directories(openmw_esmm PUBLIC
    src
    libs/imgui
    libs/imgui/backends
    ${SDL2_INCLUDE_DIRS}
    ${SDL2_TTF_INCLUDE_DIRS}
    ${Boost_INCLUDE_DIRS}
)

target_link_libraries(openmw_esmm PUBLIC
    ${SDL2_TTF_LIBRARIES}
    ${SDL2_LIBRARIES}
    ${Boost_LIBRARIES}
    dl
)
