# Ring Out for Nintendo Switch: the .nro.
#
# Configured from ModernGekko/CMakeLists.txt when MODERNGEKKO_PLATFORM_SWITCH is
# on (automatic under devkitPro's Switch.cmake). See switch/README.md at the
# repository root for the whole flow.

# CMake only knows how to spell WHOLE_ARCHIVE for the platforms it ships
# definitions for, and "NintendoSwitch" is not one of them. devkitA64 links
# with GNU ld, so give it the GNU spelling.
if(NOT CMAKE_LINK_LIBRARY_USING_WHOLE_ARCHIVE_SUPPORTED)
    set(CMAKE_LINK_LIBRARY_USING_WHOLE_ARCHIVE
        "LINKER:--whole-archive" "<LINK_ITEM>" "LINKER:--no-whole-archive")
    set(CMAKE_LINK_LIBRARY_USING_WHOLE_ARCHIVE_SUPPORTED TRUE)
endif()

set(RINGOUT_NX_MODULE "" CACHE FILEPATH
    "The game module as a static archive (g<ID>_recomp.a, module-src built with MODULE_STATIC=ON). Empty builds an interpreter-only .nro.")

add_executable(ringout_nx
    main.cpp
    profiler.cpp
    static_module.cpp
    thread_stack.cpp
    posix_compat.c)

# 8 MiB stacks for every thread instead of libnx's 128 KiB; see thread_stack.cpp.
target_link_options(ringout_nx PRIVATE "-Wl,--wrap=pthread_create")

target_include_directories(ringout_nx PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${PROJECT_SOURCE_DIR}/include"
    "${PROJECT_SOURCE_DIR}/vendor/dolphin/Source/Core")
target_compile_features(ringout_nx PRIVATE cxx_std_23)
# Must agree with the Dolphin libraries; see the moderngekko target.
target_compile_definitions(ringout_nx PRIVATE _ARCH_64=1 _M_GENERIC=1)

# The runtime object defines the Host_* callbacks Dolphin's static libraries
# call, and nothing in main.cpp references them directly -- without
# --whole-archive the linker would drop them and fail on Dolphin's side.
target_link_libraries(ringout_nx PRIVATE
    "$<LINK_LIBRARY:WHOLE_ARCHIVE,moderngekko>"
    core uicommon)

if(RINGOUT_NX_MODULE)
    if(NOT EXISTS "${RINGOUT_NX_MODULE}")
        message(FATAL_ERROR "RINGOUT_NX_MODULE does not exist: ${RINGOUT_NX_MODULE}")
    endif()
    target_compile_definitions(ringout_nx PRIVATE RINGOUT_NX_HAVE_MODULE=1)
    # Whole archive as well: the chunk functions are only reached through the
    # module's dispatch tables, never by name from here.
    add_library(ringout_nx_module STATIC IMPORTED)
    set_target_properties(ringout_nx_module PROPERTIES
        IMPORTED_LOCATION "${RINGOUT_NX_MODULE}")
    target_link_libraries(ringout_nx PRIVATE
        "$<LINK_LIBRARY:WHOLE_ARCHIVE,ringout_nx_module>" m)
    message(STATUS "ringout_nx: linking game module ${RINGOUT_NX_MODULE}")
else()
    message(STATUS "ringout_nx: no RINGOUT_NX_MODULE -- interpreter-only build")
endif()

# The .nro itself (NACP + RomFS with Dolphin's Sys/ folder) is assembled from
# this ELF by build-switch.sh with elf2nro, so the packaging steps are visible
# in one place rather than split between CMake and the script.
set_target_properties(ringout_nx PROPERTIES OUTPUT_NAME "RingOut")
