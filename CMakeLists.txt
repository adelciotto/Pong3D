cmake_minimum_required(VERSION 3.20)
project(pong3d)
set(CMAKE_CXX_STANDARD 17)
# Linux -pthread shenanigans
if (CMAKE_SYSTEM_NAME STREQUAL Linux)
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)
endif ()

#=== LIBRARY: HandmadeMath
add_library(HandmadeMath INTERFACE code/vendor/HandmadeMath/HandmadeMath.h)
target_include_directories(HandmadeMath INTERFACE code/vendor/HandmadeMath)

#=== LIBRARY: mattiasgustavsson libs
add_library(libs STATIC code/vendor/libs/libs.cpp code/vendor/libs/rnd.h)
target_include_directories(libs INTERFACE code/vendor/libs)

#=== LIBRARY: sokol
# add headers to the the file list because they are useful to have in IDEs
set(SOKOL_HEADERS
        code/vendor/sokol/sokol_app.h
        code/vendor/sokol/sokol_debugtext.h
        code/vendor/sokol/sokol_gfx.h
        code/vendor/sokol/sokol_glue.h
        code/vendor/sokol/sokol_log.h
        code/vendor/sokol/sokol_time.h)
if (CMAKE_SYSTEM_NAME STREQUAL Darwin)
    add_library(sokol STATIC code/vendor/sokol/sokol.cpp ${SOKOL_HEADERS})
    target_compile_options(sokol PRIVATE -x objective-c)
    target_link_libraries(sokol PUBLIC
            "-framework QuartzCore"
            "-framework Cocoa"
            "-framework MetalKit"
            "-framework Metal")
else ()
    add_library(sokol STATIC code/vendor/sokol/sokol.cpp ${SOKOL_HEADERS})
    if (CMAKE_SYSTEM_NAME STREQUAL Linux)
        target_link_libraries(sokol INTERFACE X11 Xi Xcursor GL dl m)
        target_link_libraries(sokol PUBLIC Threads::Threads)
    endif ()
endif ()
target_include_directories(sokol INTERFACE code/vendor/sokol)

#=== EXECUTABLE: demo
if (CMAKE_SYSTEM_NAME STREQUAL Windows)
    add_executable(pong3d WIN32)
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT demo)
else ()
    add_executable(pong3d)
endif ()
target_sources(pong3d PRIVATE
        code/game.cpp
        code/input.cpp
        code/main.cpp
        code/renderer.cpp)
target_link_libraries(pong3d HandmadeMath libs sokol)

# Emscripten-specific linker options
if (CMAKE_SYSTEM_NAME STREQUAL Emscripten)
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
    # use our own minimal shell.html
    target_link_options(pong3d PRIVATE --shell-file ../sokol/shell.html)
    # link with WebGL2
    target_link_options(pong3d PRIVATE -sUSE_WEBGL2=1)
    # WASM+JS size optimizations
    target_link_options(pong3d PRIVATE -sNO_FILESYSTEM=1 -sASSERTIONS=0 -sMALLOC=emmalloc --closure=1)
endif ()

# explicitly strip dead code
if (CMAKE_C_COMPILER_ID MATCHES "Clang" AND NOT CMAKE_SYSTEM_NAME STREQUAL Emscripten)
    target_link_options(pong3d PRIVATE LINKER:-dead_strip)
endif ()

#=== SHADERS
set(SHDC_EXE "${CMAKE_CURRENT_SOURCE_DIR}/tools/sokol-shdc")
set(SHDC_SLANG "glsl410")
set(SHADERS "game_phong.glsl" "game_basic.glsl" "bloom.glsl" "combine_display.glsl")
set(GENERATED_SHADER_HEADERS)
foreach (shader ${SHADERS})
    string(REPLACE ".glsl" ".glsl.h" shader_header ${shader})
    list(APPEND GENERATED_SHADER_HEADERS ${shader_header})
    add_custom_command(
            OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${shader_header}
            COMMAND ${SHDC_EXE} --input ${CMAKE_CURRENT_SOURCE_DIR}/code/shaders/${shader} --output ${CMAKE_CURRENT_BINARY_DIR}/${shader_header} --slang ${SHDC_SLANG}
            DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/code/shaders/${shader}
            COMMENT "Generating ${shader_header} from ${shader}"
            VERBATIM
    )
endforeach ()
add_custom_target(
        shaders_all
        DEPENDS ${GENERATED_SHADER_HEADERS}
)
add_dependencies(pong3d shaders_all)
target_include_directories(pong3d PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
