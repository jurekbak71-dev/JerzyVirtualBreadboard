cmake_minimum_required(VERSION 3.22)
project(MicrophonicFuzz VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
  JUCE
  GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
  GIT_TAG 8.0.10
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(MicrophonicFuzz
  COMPANY_NAME "JerzyAudio"
  BUNDLE_ID "com.jerzyaudio.microphonicfuzz"
  PLUGIN_MANUFACTURER_CODE Jrzy
  PLUGIN_CODE McFz
  FORMATS VST3
  PRODUCT_NAME "Microphonic Fuzz"
  IS_SYNTH FALSE
  NEEDS_MIDI_INPUT FALSE
  NEEDS_MIDI_OUTPUT FALSE
  EDITOR_WANTS_KEYBOARD_FOCUS FALSE
  COPY_PLUGIN_AFTER_BUILD FALSE
)

target_sources(MicrophonicFuzz PRIVATE
  Source/PluginProcessor.cpp
  Source/PluginEditor.cpp
)
target_compile_definitions(MicrophonicFuzz PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
target_link_libraries(MicrophonicFuzz PRIVATE juce::juce_audio_utils juce::juce_dsp juce::juce_audio_processors)

# Original-design raster filmstrips. Optional until PNG pack is committed.
set(MF_GUI_ASSET_DIR "${CMAKE_CURRENT_SOURCE_DIR}/assets/gui/production")
set(MF_GUI_FILMSTRIPS
  "${MF_GUI_ASSET_DIR}/knobs/steel_large_128.png"
  "${MF_GUI_ASSET_DIR}/knobs/steel_warm_128.png"
  "${MF_GUI_ASSET_DIR}/knobs/black_anodized_128.png"
  "${MF_GUI_ASSET_DIR}/knobs/steel_bright_128.png"
  "${MF_GUI_ASSET_DIR}/knobs/chrome_small_128.png"
  "${MF_GUI_ASSET_DIR}/knobs/black_small_128.png"
  "${MF_GUI_ASSET_DIR}/knobs/brass_small_128.png"
)
set(MF_GUI_ALL_PRESENT TRUE)
foreach(asset IN LISTS MF_GUI_FILMSTRIPS)
  if(NOT EXISTS "${asset}")
    set(MF_GUI_ALL_PRESENT FALSE)
  endif()
endforeach()
if(MF_GUI_ALL_PRESENT)
  juce_add_binary_data(MicrophonicFuzzGUI SOURCES ${MF_GUI_FILMSTRIPS})
  target_link_libraries(MicrophonicFuzz PRIVATE MicrophonicFuzzGUI)
  target_compile_definitions(MicrophonicFuzz PRIVATE MF_GUI_ASSETS=1)
else()
  message(WARNING "Original GUI PNG filmstrips not present: falling back to native JUCE controls")
  target_compile_definitions(MicrophonicFuzz PRIVATE MF_GUI_ASSETS=0)
endif()
