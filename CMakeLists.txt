cmake_minimum_required(VERSION 3.22)
project(Slapbox VERSION 0.2.0)

# Point this at your JUCE checkout:  cmake -B build -DJUCE_PATH=~/JUCE
# (or leave it empty if you installed JUCE with `cmake --install`)
set(JUCE_PATH "" CACHE PATH "Path to your JUCE source folder")

if(JUCE_PATH)
    add_subdirectory(${JUCE_PATH} ${CMAKE_BINARY_DIR}/JUCE)
else()
    find_package(JUCE CONFIG REQUIRED)
endif()

juce_add_plugin(Slapbox
    COMPANY_NAME            "Geode Audio"
    BUNDLE_ID               com.geodeaudio.slapbox
    PLUGIN_MANUFACTURER_CODE Geod        # 4 chars, first one uppercase
    PLUGIN_CODE             Slap         # 4 chars, unique per plugin
    FORMATS                 VST3 Standalone
    PRODUCT_NAME            "Slapbox"
    IS_SYNTH                FALSE
    NEEDS_MIDI_INPUT        FALSE
    NEEDS_MIDI_OUTPUT       FALSE
    IS_MIDI_EFFECT          FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE)

target_sources(Slapbox PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

# Poppins (SIL Open Font License, see Resources/OFL.txt), embedded so the UI looks the same everywhere.
juce_add_binary_data(SlapboxFonts SOURCES
    Resources/Poppins-Regular.ttf
    Resources/Poppins-SemiBold.ttf)

target_compile_features(Slapbox PRIVATE cxx_std_17)

target_compile_definitions(Slapbox PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(Slapbox
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
        SlapboxFonts
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
