project(kronos)

cmake_minimum_required(VERSION 3.12)
cmake_policy(VERSION 3.12)
set(CMAKE_C_STANDARD 11) 
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/exe)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
set(CMAKE_C_FLAGS_DEBUG "-O0 -g -ffast-math")
set(CMAKE_C_FLAGS_RELEASE "-O3 -ffast-math -DNDEBUG -s -fomit-frame-pointer -march=native -funroll-loops -fno-PIE -no-pie -mfpmath=sse -m64")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g -ffast-math -fpermissive -Wno-write-strings -Wno-literal-suffix")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -ffast-math -DNDEBUG -s -fomit-frame-pointer -march=native -funroll-loops -fno-PIE -no-pie -mfpmath=sse -m64 -fpermissive -Wno-write-strings -Wno-literal-suffix")
set(CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES} "-lm")

add_definitions(-DIMPROVED_SAVESTATES)
add_definitions(-DDYNAREC_KRONOS=1)
add_definitions(-DUSE_SCSPMIDI=1)
add_definitions(-DHAVE_BUILTIN_BSWAP16=1)
add_definitions(-DHAVE_BUILTIN_BSWAP32=1)
add_definitions(-DHAVE_STDINT_H=1)
add_definitions(-DARCH_IS_LINUX=1)
add_definitions(-DLINUX_CDROM_H_IS_BROKEN)

option(YAB_WANT_VDP1RAM_CS_ASYNC "Enable Threaded update of VDP1 Ram for CS core" ON)
option(YAB_WANT_ASYNC_CELL "Enable Threaded rendering of nbgx cells" ON)
option(YAB_WANT_ASYNC_RGB "Enable Threaded rendering of rgb0" ON)
option(YAB_WANT_ASYNC_VDP1_TEXTURE "Enable Threaded rendering of vdp1 texture" ON)
option(YAB_FORCE_SECURE_STRINGS "Turns all string format warnings into errors." OFF)

include(CheckCSourceCompiles)
include(CheckFunctionExists)
include(CheckIncludeFile)

include_directories(src)
include_directories(src/core)
include_directories(src/core/sh2)
include_directories(src/core/scsp)
include_directories(src/core/memory)
include_directories(src/utils)
include_directories(src/core/video/core)
include_directories(src/core/video/compute_shader)
include_directories(src/core/video/common)
include_directories(src/core/sh2)
include_directories(src/core/peripheral)
include_directories(src/ctrl)
include_directories(src/tools/junzip)
include_directories(src/tools/libchdr)
include_directories(src/core/m68k)

set(kronos_HEADERS
    ${kronos_HEADERS}
    src/core/bios.h
    src/core/vdp1.h
    src/core/vdp2.h
    src/core/sh2/sh2core.h
    src/core/scu.h
    src/core/scsp/scsp.h
    src/core/scsp/scspdsp.h
    src/core/smpc.h
    src/core/memory/memory.h
    src/core/memory/cs0.h
    src/core/memory/cs1.h
    src/core/memory/cs2.h
    src/core/memory/eeprom.h
    src/core/memory/decrypt.h
    src/core/m68k/m68kcore.h
    src/core/video/core/vidogl.h
    src/core/video/common/vidshared.h
    src/core/video/common/ygl.h
    src/core/sh2/sh2int_kronos.h
    src/core/peripheral/peripheral.h
    src/ctrl/core.h
    src/ctrl/yabause.h
    src/ctrl/movie.h
    src/ctrl/yui.h
    src/ctrl/threads.h
    src/utils/error.h
    src/utils/cdbase.h
    src/tools/junzip/junzip.h
)
set(kronos_SOURCES
    ${kronos_SOURCES}
    src/core/m68k/m68kcore.c
    src/core/bios.cpp
    src/core/vdp1.cpp
    src/core/vdp2.cpp
    src/core/sh2/sh2core.cpp
    src/core/scu.cpp
    src/core/scsp/scsp.cpp
    src/core/scsp/scspdsp.cpp
    src/core/smpc.cpp
    src/core/memory/memory.cpp
    src/core/memory/cs0.cpp
    src/core/memory/cs1.cpp
    src/core/memory/cs2.cpp
    src/core/memory/eeprom.cpp
    src/core/memory/decrypt.cpp
    src/core/sh2/sh2int.cpp
    src/core/video/common/vidshared.cpp
    src/core/peripheral/peripheral.cpp
    src/core/snddummy.cpp
    src/ctrl/yabause.cpp
    src/ctrl/stv.cpp
    src/ctrl/movie.cpp
    src/utils/error.cpp
    src/utils/cdbase.cpp
    src/tools/junzip/junzip.cpp
)
set(kronos_SOURCES
    ${kronos_SOURCES}
    src/core/sh2/sh2int.cpp
    src/core/sh2/sh2_opcodes.cpp
)

if(YAB_WANT_VDP1RAM_CS_ASYNC)
    add_definitions(-DVDP1RAM_CS_ASYNC=1)
endif()
if(YAB_WANT_ASYNC_CELL)
    add_definitions(-DCELL_ASYNC=1)
endif()
if(YAB_WANT_ASYNC_RGB)
    add_definitions(-DRGB_ASYNC=1)
endif()

if(YAB_WANT_ASYNC_VDP1_TEXTURE)
    add_definitions(-DVDP1_TEXTURE_ASYNC=1)
endif()
if(CMAKE_COMPILER_IS_GNUCC AND YAB_FORCE_SECURE_STRINGS)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wformat -Werror=format-security")
endif()
if(UNIX)
    set(KRONOS_LIBRARIES ${KRONOS_LIBRARIES} "m")
endif()


add_subdirectory(src/tools/libchdr)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/tools/libchdr/src)
set(KRONOS_LIBRARIES ${KRONOS_LIBRARIES} chdr-static)


check_c_source_compiles("#define MACRO(...) puts(__VA_ARGS__)
	int main(int argc, char ** argv) { MACRO(\"foo\"); }"
    VARIADIC_MACROS_OK)
if(VARIADIC_MACROS_OK)
    add_definitions(-DHAVE_C99_VARIADIC_MACROS=1)
endif(VARIADIC_MACROS_OK)

check_function_exists(gettimeofday GETTIMEOFDAY_OK)
if(GETTIMEOFDAY_OK)
    add_definitions(-DHAVE_GETTIMEOFDAY=1)
endif()

check_function_exists(floorf FLOORF_OK)
if(FLOORF_OK)
    add_definitions(-DHAVE_FLOORF=1)
endif()

check_function_exists(_wfopen WFOPEN_OK)
if(WFOPEN_OK)
    add_definitions(-DHAVE_WFOPEN=1)
endif()

check_function_exists(strcasecmp STRCASECMP_OK)
if(STRCASECMP_OK)
    add_definitions(-DHAVE_STRCASECMP=1)
endif()

check_function_exists(stricmp STRICMP_OK)
if(STRICMP_OK)
    add_definitions(-DHAVE_STRICMP=1)
endif()

check_include_file("src/core/time.h" SYSTIME_OK)
if(SYSTIME_OK)
    add_definitions(-DHAVE_SYS_TIME_H=1)
endif()

find_package(Threads)
set(KRONOS_LIBRARIES ${KRONOS_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

set(YAB_RGB "" CACHE STRING "Bit configuration of pixels in the display buffer.")
if(YAB_RGB STREQUAL "555")
    add_definitions(-DUSE_16BPP=1 -DUSE_RGB_555=1)
elseif(YAB_RGB STREQUAL "565")
    add_definitions(-DUSE_16BPP=1 -DUSE_RGB_565=1)
endif()

include(FindOpenGL)

if(OPENGL_FOUND)
    message(STATUS "OPENGL Core supported")
    add_definitions(-D_OGL3_)
    SET(NEED_GLEW "YES")
    SET(OPENGL_CURRENT_LIBRARIES ${OPENGL_LIBRARIES})
    SET(OPENGL_SUPPORTED "YES")
endif(OPENGL_FOUND)

if(OPENGL_SUPPORTED)
    add_definitions(-DHAVE_LIBGL=1)
    set(KRONOS_LIBRARIES ${KRONOS_LIBRARIES} ${OPENGL_CURRENT_LIBRARIES})

    set(CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES} ${OPENGL_CURRENT_LIBRARIES})
    check_function_exists(glXGetProcAddress GLXGETPROCADDRESS_OK)

    if(GLXGETPROCADDRESS_OK)
        add_definitions(-DHAVE_GLXGETPROCADDRESS=1)
    endif()

    set(kronos_SOURCES
        ${kronos_SOURCES}
        src/core/video/common/commongl.cpp
        src/core/video/core/yglcache.cpp
        src/core/video/core/vidogl.cpp
        src/core/video/common/ogl_shader.cpp
        src/core/video/common/rbg_compute.cpp
        src/core/video/common/common_glshader.cpp
        src/core/video/common/upscale_shader.cpp
        src/core/video/compute_shader/yglcs.cpp
        src/core/video/compute_shader/vidcs.cpp
        src/core/video/compute_shader/vdp1_compute.cpp
    )
    set(kronos_HEADERS
        ${kronos_HEADERS}
        src/core/video/common/common_glshader.h
        src/core/video/compute_shader/vidcs.h
        src/core/video/compute_shader/vdp1_compute.h
        src/core/video/compute_shader/vdp1_prog_compute.h
    )
endif(OPENGL_SUPPORTED)

set(MUSASHI_GENERATED_SOURCES
    ${CMAKE_CURRENT_BINARY_DIR}/src/core/m68k/musashi/m68kopac.c
    ${CMAKE_CURRENT_BINARY_DIR}/src/core/m68k/musashi/m68kopdm.c
    ${CMAKE_CURRENT_BINARY_DIR}/src/core/m68k/musashi/m68kopnz.c
    ${CMAKE_CURRENT_BINARY_DIR}/src/core/m68k/musashi/m68kops.c)

set(MUSASHI_GENERATED_HEADERS
    ${CMAKE_CURRENT_BINARY_DIR}/src/core/m68k/musashi/m68kops.h)

include(ExternalProject)
ExternalProject_Add(m68kmake
    DOWNLOAD_COMMAND ""
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/core/m68k/musashi
    CMAKE_GENERATOR "${CMAKE_GENERATOR}"
    INSTALL_COMMAND ""
    BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/src/core/m68k/musashi
)

include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}/src/core/m68k/musashi
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/m68k/musashi
)

set(kronos_SOURCES ${kronos_SOURCES}
    ${MUSASHI_GENERATED_SOURCES}
    src/core/m68k/musashi/m68kcpu.c
    src/core/m68k/m68kmusashi.c
)

set(kronos_HEADERS
    ${kronos_HEADERS}
    src/core/m68k/musashi/m68k.h
    src/core/m68k/musashi/m68kcpu.h
    ${MUSASHI_GENERATED_HEADERS}
    src/core/m68k/m68kmusashi.h
)

set_property(SOURCE ${MUSASHI_GENERATED_SOURCES} ${MUSASHI_GENERATED_HEADERS} PROPERTY GENERATED ON)
option(SH2_UBC "Enable SH2 User Break Controller" OFF)
if(SH2_UBC)
    add_definitions(-DSH2_UBC=1)
endif()

check_include_file("src/linux/joystick.h" LINUX_HAS_JOYSTICK)

set(kronos_SOURCES ${kronos_SOURCES} src/ctrl/thr-linux.cpp src/core/cd-linux.cpp)
set(kronos_SOURCES ${kronos_SOURCES} src/core/peripheral/perlinuxjoy.cpp)
set(kronos_HEADERS ${kronos_HEADERS} src/core/peripheral/perlinuxjoy.h)


add_executable(kronos ${kronos_SOURCES} ${kronos_HEADERS} src/main.cpp)
target_link_libraries(kronos ${KRONOS_LIBRARIES} ${PORT_LIBRARIES})
add_dependencies(kronos m68kmake)

# glfw
find_package(glfw3 REQUIRED)
include_directories(${GLFW_INCLUDE_DIRS})
link_libraries(${GLFW_LIBRARY_DIRS})

# opengl
find_package(OpenGL REQUIRED)
include_directories(${OPENGL_INCLUDE_DIRS})

# glew
find_package(GLEW REQUIRED)
include_directories(${GLEW_INCLUDE_DIRS})
add_definitions(-D_USEGLEW_)
add_definitions(-DGLEW_STATIC)

target_link_libraries(${PROJECT_NAME} 
${OPENGL_LIBRARIES}
${GLEW_LIBRARIES}
${EXTRA_LIBS}
glfw SDL2_image SDL2_ttf SDL2 SDL2main
)
