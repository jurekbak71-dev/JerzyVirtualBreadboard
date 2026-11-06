cmake_minimum_required(VERSION 3.20)
project(VirtualBreadboardAudioLab LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_library(vbal_audio src/audio.cpp src/spice.cpp src/spice_render.cpp src/circuit.cpp src/realtime_circuit.cpp)
target_include_directories(vbal_audio PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
add_executable(vbal_rc src/main.cpp)
target_link_libraries(vbal_rc PRIVATE vbal_audio)
enable_testing()
add_executable(vbal_native_tests tests/tests.cpp)
target_link_libraries(vbal_native_tests PRIVATE vbal_audio)
add_test(NAME NativeAudioTests COMMAND vbal_native_tests)

add_executable(vbal_spice_tests tests/spice_tests.cpp)
target_link_libraries(vbal_spice_tests PRIVATE vbal_audio)
add_test(NAME SpiceNetlistTests COMMAND vbal_spice_tests)

add_executable(vbal_spice src/spice_main.cpp)
target_link_libraries(vbal_spice PRIVATE vbal_audio)

add_executable(vbal_spice_render_tests tests/spice_render_tests.cpp)
target_link_libraries(vbal_spice_render_tests PRIVATE vbal_audio)
add_test(NAME SpiceRenderTests COMMAND vbal_spice_render_tests)
add_executable(vbal_render_spice src/render_main.cpp)
target_link_libraries(vbal_render_spice PRIVATE vbal_audio)

if(WIN32)
  include(FetchContent)
  FetchContent_Declare(miniaudio URL https://github.com/mackron/miniaudio/archive/refs/tags/0.11.26.tar.gz)
  FetchContent_MakeAvailable(miniaudio)
  add_library(vbal_live_audio STATIC src/live_audio_win.cpp src/miniaudio_impl.cpp)
  target_include_directories(vbal_live_audio PRIVATE ${miniaudio_SOURCE_DIR})
  target_link_libraries(vbal_live_audio PUBLIC vbal_audio)
  add_executable(vbal_gui WIN32 src/gui_win.cpp)
  target_link_libraries(vbal_gui PRIVATE vbal_audio comdlg32 user32 gdi32 shell32)

  set(VBAL_WEBVIEW2_NATIVE "${CMAKE_CURRENT_SOURCE_DIR}/deps/Microsoft.Web.WebView2.1.0.4314.35/build/native")
  if(EXISTS "${VBAL_WEBVIEW2_NATIVE}/include/WebView2.h")
    add_executable(vbal_breadboard WIN32 src/breadboard_win.cpp)
    set_target_properties(vbal_breadboard PROPERTIES OUTPUT_NAME "Vbreadboard uruchom")
    target_include_directories(vbal_breadboard PRIVATE "${VBAL_WEBVIEW2_NATIVE}/include")
    target_link_libraries(vbal_breadboard PRIVATE
      vbal_audio vbal_live_audio "${VBAL_WEBVIEW2_NATIVE}/x64/WebView2Loader.dll.lib"
      user32 comdlg32 shlwapi ole32)
    add_custom_command(TARGET vbal_breadboard POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
       "${VBAL_WEBVIEW2_NATIVE}/x64/WebView2Loader.dll"
       "$<TARGET_FILE_DIR:vbal_breadboard>/WebView2Loader.dll")
  else()
    message(STATUS "WebView2 SDK not installed: vbal_breadboard target skipped")
  endif()
endif()

add_executable(vbal_circuit_tests tests/circuit_tests.cpp)
target_link_libraries(vbal_circuit_tests PRIVATE vbal_audio)
add_test(NAME CircuitGraphTests COMMAND vbal_circuit_tests)

add_executable(vbal_render_graph src/graph_render_main.cpp)
target_link_libraries(vbal_render_graph PRIVATE vbal_audio)

add_executable(vbal_realtime_tests tests/realtime_circuit_tests.cpp)
target_link_libraries(vbal_realtime_tests PRIVATE vbal_audio)
add_test(NAME RealtimeCircuitTests COMMAND vbal_realtime_tests)
