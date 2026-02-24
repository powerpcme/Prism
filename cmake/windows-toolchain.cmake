set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# MINGW_C_COMPILER and MINGW_CXX_COMPILER are set by build.sh to the
# correct -posix variant. Fall back to plain names if not provided.
if(NOT DEFINED MINGW_C_COMPILER)
    set(MINGW_C_COMPILER "x86_64-w64-mingw32-gcc")
endif()
if(NOT DEFINED MINGW_CXX_COMPILER)
    set(MINGW_CXX_COMPILER "x86_64-w64-mingw32-g++")
endif()

set(CMAKE_C_COMPILER   "${MINGW_C_COMPILER}")
set(CMAKE_CXX_COMPILER "${MINGW_CXX_COMPILER}")
set(CMAKE_RC_COMPILER  x86_64-w64-mingw32-windres)

set(CMAKE_FIND_ROOT_PATH /usr/x86_64-w64-mingw32)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libgcc -static-libstdc++ -static -lpthread")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static -lpthread")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -static-libgcc -static-libstdc++ -Wl,-Bstatic -lpthread -Wl,-Bdynamic")

# Use host-built VST3 helper for cross-compilation
set(JUCE_TOOL_INSTALL_DIR "${CMAKE_CURRENT_LIST_DIR}/../build-host-tools" CACHE PATH "")
set(juce_vst3_helper_EXECUTABLE "${CMAKE_CURRENT_LIST_DIR}/../build-host-tools/juce_vst3_helper" CACHE FILEPATH "")
