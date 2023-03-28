# CMake stuff
cmake_minimum_required(VERSION 3.15)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")

# General settings
set(PLUGIN_NAME "OneRiser")
set(COMPANY_NAME "OneRiser (open-source")
set(COMPANY_CODE ORos) # (requires at least 1 uppercase)
set(PLUGIN_DATA ${PLUGIN_NAME}Data)

# The CMake project
project(${PLUGIN_NAME} VERSION 1.0.4)

# The JUCE directory
add_subdirectory(<juce_path>)

# JUCE plugin information
juce_add_plugin(${PLUGIN_NAME}
    COMPANY_NAME ${COMPANY_NAME}
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE ${COMPANY_CODE}
# Exactly 1 uppercase (GarageBand requires 1 upper and 3 lower respectively)
    PLUGIN_CODE KsOR
# AAX requires its SDK to be linked above
    FORMATS AU VST3 Standalone
# Change this here if you wish the plugin name to differ from the project's
    PRODUCT_NAME "OneRiser")

# Sources to use when building
target_sources(${PLUGIN_NAME} PRIVATE
    Source/PluginEditor.cpp
    Source/PluginProcessor.cpp)

target_compile_definitions(${PLUGIN_NAME} PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

# Binary data sources (i.e. not source code), such as fonts or images
juce_add_binary_data(${PLUGIN_DATA} SOURCES
                     Source/Assets/Background2.5.png
                     Source/Assets/LargeKnobTop.png
                     Source/Assets/LargeKnobBase.png
                     Source/Assets/SmallKnobTop.png
                     Source/Assets/SmallKnobBase.png
                     Source/Assets/Muli-Bold.ttf
                     Source/Assets/Muli-ExtraBold.ttf
                     Source/Assets/MarkersIdle.png)

#set_target_properties(${PLUGIN_NAME} ${PLUGIN_DATA} PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Compiler warnings; "-W" prefix means enabled, "-Wno-" prefix means disabled
target_compile_options(${PLUGIN_NAME} PUBLIC
                       -Wno-implicit-float-conversion
                       -Wno-float-conversion
                       -Wno-switch-enum)

# External libraries to be linked during build — JUCE is here by default
target_link_libraries(${PLUGIN_NAME}
    PRIVATE
    ${PLUGIN_DATA}
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_audio_devices
    juce::juce_gui_basics
    juce::juce_dsp
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags)