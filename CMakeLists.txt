cmake_minimum_required(VERSION 3.22)
project(MicrophonicFuzz VERSION 0.1.0 LANGUAGES C CXX)

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

juce_generate_juce_header(MicrophonicFuzz)

target_sources(MicrophonicFuzz PRIVATE
  Source/PluginProcessor.cpp
  Source/PluginEditor.cpp
)
target_compile_definitions(MicrophonicFuzz PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(MicrophonicFuzz PRIVATE juce::juce_audio_utils juce::juce_dsp juce::juce_audio_processors)

# Supplied v0.4 prototype artwork; artistic approval is a separate release gate.
option(MF_GUI_ASSETS "Embed the reference-based raster rack artwork" ON)
include(CTest)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
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
if(MF_GUI_ASSETS)
  if(NOT MF_GUI_ALL_PRESENT)
    message(FATAL_ERROR "Raster assets requested but missing; import the pinned v0.4 pack or set MF_GUI_ASSETS=OFF")
  endif()
  execute_process(COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/import_gui_assets.py" --verify
    RESULT_VARIABLE MF_ASSET_CHECK)
  if(NOT MF_ASSET_CHECK EQUAL 0)
    message(FATAL_ERROR "Prototype asset integrity check failed")
  endif()
  file(GLOB MF_RENDERED_ASSETS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/assets/gui/rendered-v1/*.png")
  juce_add_binary_data(MicrophonicFuzzGUI SOURCES ${MF_RENDERED_ASSETS})
  add_custom_target(MicrophonicFuzzAssetCheck
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/import_gui_assets.py" --verify
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/audit_gui_assets.py"
    VERBATIM)
  add_dependencies(MicrophonicFuzzGUI MicrophonicFuzzAssetCheck)
  target_link_libraries(MicrophonicFuzz PUBLIC MicrophonicFuzzGUI)
  target_compile_definitions(MicrophonicFuzz PRIVATE MF_GUI_ASSETS=1)
else()
  message(STATUS "Explicit fallback build: native JUCE controls (not final GUI)")
  target_compile_definitions(MicrophonicFuzz PRIVATE MF_GUI_ASSETS=0)
endif()

if(BUILD_TESTING)
  add_executable(MicrophonicFuzzDSPTests tests/DspEngineTests.cpp)
  target_include_directories(MicrophonicFuzzDSPTests PRIVATE Source)
  add_test(NAME dsp_engine COMMAND MicrophonicFuzzDSPTests)
  add_executable(MicrophonicFuzzSmoke tests/ProcessorSmoke.cpp)
  target_include_directories(MicrophonicFuzzSmoke PRIVATE Source "${juce_SOURCE_DIR}/modules")
  target_link_libraries(MicrophonicFuzzSmoke PRIVATE MicrophonicFuzz)
  add_test(NAME processor_smoke COMMAND MicrophonicFuzzSmoke)
  add_executable(MicrophonicFuzzIRTests tests/ProcessorIRTests.cpp)
  target_include_directories(MicrophonicFuzzIRTests PRIVATE Source "${juce_SOURCE_DIR}/modules")
  target_link_libraries(MicrophonicFuzzIRTests PRIVATE MicrophonicFuzz)
  add_test(NAME processor_ir_recall COMMAND MicrophonicFuzzIRTests)
  add_executable(MicrophonicFuzzConcurrencyTests tests/ProcessorConcurrencyTests.cpp)
  target_include_directories(MicrophonicFuzzConcurrencyTests PRIVATE Source "${juce_SOURCE_DIR}/modules")
  target_link_libraries(MicrophonicFuzzConcurrencyTests PRIVATE MicrophonicFuzz)
  add_test(NAME concurrent_ir_state_and_sample_rate COMMAND MicrophonicFuzzConcurrencyTests)
  add_executable(MicrophonicFuzzRoomTests tests/RoomConvolutionTests.cpp)
  target_include_directories(MicrophonicFuzzRoomTests PRIVATE Source "${juce_SOURCE_DIR}/modules")
  target_link_libraries(MicrophonicFuzzRoomTests PRIVATE MicrophonicFuzz)
  add_test(NAME long_room_convolution COMMAND MicrophonicFuzzRoomTests)
  add_executable(MicrophonicFuzzBinaryTests tests/Vst3BinarySmoke.cpp)
  target_include_directories(MicrophonicFuzzBinaryTests PRIVATE "${juce_SOURCE_DIR}/modules" "${juce_SOURCE_DIR}/modules/juce_audio_processors/format_types/VST3_SDK")
  target_link_libraries(MicrophonicFuzzBinaryTests PRIVATE MicrophonicFuzz ${CMAKE_DL_LIBS})
  add_dependencies(MicrophonicFuzzBinaryTests MicrophonicFuzz_VST3)
  add_test(NAME vst3_binary_load_process_unload COMMAND MicrophonicFuzzBinaryTests "$<TARGET_FILE:MicrophonicFuzz_VST3>")
  add_executable(MicrophonicFuzzRoomBenchmark EXCLUDE_FROM_ALL tests/RoomBenchmark.cpp)
  target_include_directories(MicrophonicFuzzRoomBenchmark PRIVATE Source "${juce_SOURCE_DIR}/modules")
  target_link_libraries(MicrophonicFuzzRoomBenchmark PRIVATE MicrophonicFuzz)
  if(MSVC)
    target_link_options(MicrophonicFuzzDSPTests PRIVATE /STACK:8388608)
    target_link_options(MicrophonicFuzzIRTests PRIVATE /STACK:8388608)
  endif()
  add_test(NAME rendered_asset_integrity COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/audit_gui_assets.py")
  add_test(NAME importer_tests COMMAND "${Python3_EXECUTABLE}" -m unittest discover -s tests -v)
  set_tests_properties(importer_tests PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
  if(MF_GUI_ASSETS)
    add_test(NAME asset_integrity COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/import_gui_assets.py" --verify)
  endif()
endif()
