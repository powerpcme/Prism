cmake_minimum_required(VERSION 3.22)
project(Prism VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Keep a compatible default deployment target for older macOS API usage
# unless the user explicitly overrides it.
if(APPLE AND NOT DEFINED CMAKE_OSX_DEPLOYMENT_TARGET)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "14.0" CACHE STRING "Minimum macOS deployment version" FORCE)
endif()

# Find JUCE - either via FetchContent or local installation
include(FetchContent)

FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.12
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(JUCE)

# For cross-compilation: use pre-built host tools
if(CMAKE_CROSSCOMPILING AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/build-host-tools/juce_vst3_helper")
    # Override the juce_vst3_helper with the host-built version
    add_executable(juce_vst3_helper IMPORTED GLOBAL)
    set_target_properties(juce_vst3_helper PROPERTIES
        IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/build-host-tools/juce_vst3_helper"
    )
endif()

# Cross-compilation settings for plugin options
if(CMAKE_CROSSCOMPILING)
    set(PRISM_COPY_AFTER_BUILD FALSE)
    set(PRISM_VST3_AUTO_MANIFEST FALSE)
else()
    set(PRISM_COPY_AFTER_BUILD TRUE)
    set(PRISM_VST3_AUTO_MANIFEST TRUE)
endif()

# Plugin configuration
juce_add_plugin(Prism
    COMPANY_NAME "PowerPCME"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD ${PRISM_COPY_AFTER_BUILD}
    PLUGIN_MANUFACTURER_CODE PwPc
    PLUGIN_CODE Prsm
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "Prism"
    VST3_CATEGORIES "Instrument" "Sampler"
    AU_MAIN_TYPE kAudioUnitType_MusicDevice
    BUNDLE_ID "com.powerpcme.prism"
    DESCRIPTION "Granular sampler with DAW sync"
    VST3_AUTO_MANIFEST ${PRISM_VST3_AUTO_MANIFEST}
)

# Source files
target_sources(Prism
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/Voice/Voice.cpp
        Source/Voice/VoiceManager.cpp
        Source/DSP/SamplePlayer.cpp
        Source/DSP/RateShifter.cpp
        Source/DSP/Overdrive.cpp
        Source/DSP/BitCrusher.cpp
        Source/DSP/AMModulator.cpp
        Source/DSP/EnvelopeShaper.cpp
        Source/Sync/TempoSync.cpp
        Source/UI/LCDDisplay.cpp
        Source/UI/VoiceElement.cpp
        Source/UI/LookAndFeel.cpp
        Source/UI/WaveformDisplay.cpp
)

# Include directories
target_include_directories(Prism
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Source
)

# JUCE modules
target_compile_definitions(Prism
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
)

# Additional cross-compilation overrides
if(CMAKE_CROSSCOMPILING)
    set_target_properties(Prism PROPERTIES JUCE_VST3_COPY_DIR "")
endif()

target_link_libraries(Prism
    PRIVATE
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_plugin_client
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_core
        juce::juce_data_structures
        juce::juce_events
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# Resources embedded into the plugin binary
set(PRISM_BINARY_RESOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Presets/Default.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/Images/PrismLogo.png"
)

set(PRISM_BINARY_RESOURCE_FILES)
foreach(resource ${PRISM_BINARY_RESOURCES})
    if(EXISTS "${resource}")
        list(APPEND PRISM_BINARY_RESOURCE_FILES "${resource}")
    endif()
endforeach()

if(PRISM_BINARY_RESOURCE_FILES)
    juce_add_binary_data(Prism_Data SOURCES ${PRISM_BINARY_RESOURCE_FILES})
    target_link_libraries(Prism PRIVATE Prism_Data)
endif()
