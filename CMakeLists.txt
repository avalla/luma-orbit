cmake_minimum_required(VERSION 3.22)
project(Luma VERSION 0.2.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include(FetchContent)
if(JUCE_SOURCE_DIR)
  add_subdirectory("${JUCE_SOURCE_DIR}" juce)
else()
  FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG 7c9d3783b127263d72bb65fe0a7e2dc8a02a7ac2 GIT_SHALLOW FALSE)
  FetchContent_MakeAvailable(JUCE)
endif()
juce_add_plugin(Luma COMPANY_NAME "Luma Audio" BUNDLE_ID "audio.luma.synth"
  PLUGIN_MANUFACTURER_CODE Luma PLUGIN_CODE Lum1 FORMATS VST3 AU Standalone
  PRODUCT_NAME "Luma" IS_SYNTH TRUE NEEDS_MIDI_INPUT TRUE NEEDS_MIDI_OUTPUT FALSE
  EDITOR_WANTS_KEYBOARD_FOCUS TRUE
  COPY_PLUGIN_AFTER_BUILD FALSE)
target_sources(Luma PRIVATE Source/Plugin.cpp Source/Editor.cpp)
target_compile_definitions(Luma PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(Luma PRIVATE juce::juce_audio_utils juce::juce_dsp PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
juce_add_console_app(LumaSmoke PRODUCT_NAME "Luma Smoke")
target_sources(LumaSmoke PRIVATE Tests/Smoke.cpp Source/Plugin.cpp Source/Editor.cpp)
target_compile_definitions(LumaSmoke PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
target_link_libraries(LumaSmoke PRIVATE juce::juce_audio_utils juce::juce_dsp)
enable_testing()
add_test(NAME synth_smoke COMMAND LumaSmoke)

juce_add_plugin(LumaRms COMPANY_NAME "Luma Audio" BUNDLE_ID "audio.luma.rms"
  PLUGIN_MANUFACTURER_CODE Luma PLUGIN_CODE Lrm1 FORMATS VST3 AU
  PRODUCT_NAME "Luma RMS" IS_SYNTH FALSE NEEDS_MIDI_INPUT FALSE NEEDS_MIDI_OUTPUT FALSE
  COPY_PLUGIN_AFTER_BUILD FALSE VST3_CATEGORIES Fx Analyzer AU_MAIN_TYPE kAudioUnitType_Effect)
target_sources(LumaRms PRIVATE Source/Rms/Processor.cpp Source/Rms/Editor.cpp)
target_compile_definitions(LumaRms PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(LumaRms PRIVATE juce::juce_audio_processors PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
juce_add_console_app(LumaRmsTests PRODUCT_NAME "Luma RMS Tests")
target_sources(LumaRmsTests PRIVATE Tests/Rms.cpp Source/Rms/Processor.cpp Source/Rms/Editor.cpp)
target_compile_definitions(LumaRmsTests PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
target_link_libraries(LumaRmsTests PRIVATE juce::juce_audio_processors)
add_test(NAME rms_measurement COMMAND LumaRmsTests)

juce_add_plugin(LumaOrbit COMPANY_NAME "Luma Audio" BUNDLE_ID "audio.luma.orbit"
  VERSION 0.4.0
  PLUGIN_MANUFACTURER_CODE Luma PLUGIN_CODE Orb1 FORMATS VST3 AU Standalone
  PRODUCT_NAME "Luma Orbit" IS_SYNTH TRUE NEEDS_MIDI_INPUT FALSE NEEDS_MIDI_OUTPUT FALSE
  COPY_PLUGIN_AFTER_BUILD FALSE)
target_sources(LumaOrbit PRIVATE Source/Orbit/Processor.cpp Source/Orbit/Editor.cpp)
target_compile_definitions(LumaOrbit PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(LumaOrbit PRIVATE juce::juce_audio_utils PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
juce_add_console_app(LumaOrbitTests PRODUCT_NAME "Luma Orbit Tests")
target_sources(LumaOrbitTests PRIVATE Tests/Orbit.cpp Source/Orbit/Processor.cpp Source/Orbit/Editor.cpp)
target_compile_definitions(LumaOrbitTests PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
target_link_libraries(LumaOrbitTests PRIVATE juce::juce_audio_utils)
add_test(NAME orbit_sequencer COMMAND LumaOrbitTests)
