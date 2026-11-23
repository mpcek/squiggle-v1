cmake_minimum_required(VERSION 3.22)

project(Squiggle VERSION 0.4.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.6
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(Squiggle
    COMPANY_NAME "SQUIGGLE"
    BUNDLE_ID "com.squiggle.synth"
    PLUGIN_MANUFACTURER_CODE Sqgl
    PLUGIN_CODE Sqg1
    FORMATS VST3 Standalone
    PRODUCT_NAME "SQUIGGLE!"
    DESCRIPTION "Hybrid sample and band-limited wavetable synthesizer"
    IS_SYNTH TRUE
    VST3_CATEGORIES Instrument Synth
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE
)

juce_generate_juce_header(Squiggle)

target_sources(Squiggle
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h
        Source/PluginEditor.cpp
        Source/PluginEditor.h
        Source/SynthVoice.cpp
        Source/SynthVoice.h
)

target_compile_definitions(Squiggle
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
        JUCE_REPORT_APP_USAGE=0
)

target_link_libraries(Squiggle
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_audio_formats
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

if(MSVC)
    target_compile_options(Squiggle PRIVATE /utf-8 /W4)
endif()
