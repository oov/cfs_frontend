add_custom_target(generate_version_h COMMAND
  ${CMAKE_COMMAND}
  -Dlocal_dir="${CMAKE_CURRENT_SOURCE_DIR}"
  -Dinput_file="${CMAKE_CURRENT_SOURCE_DIR}/version.h.in"
  -Doutput_file="${CMAKE_CURRENT_BINARY_DIR}/version.h"
  -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/version.cmake"
)

set(WEBVIEW2_NAME "Microsoft.Web.WebView2")
set(WEBVIEW2_VERSION "1.0.774.44")
set(WEBVIEW2_DLL "${CMAKE_CURRENT_BINARY_DIR}/${WEBVIEW2_NAME}-${WEBVIEW2_VERSION}/build/native/x86/WebView2Loader.dll")
set(WEBVIEW2_LIB "${CMAKE_CURRENT_BINARY_DIR}/${WEBVIEW2_NAME}-${WEBVIEW2_VERSION}/build/native/x86/WebView2Loader.dll.lib")
add_custom_target(extract_webview2 COMMAND
  ${CMAKE_COMMAND}
  -Dlocal_dir="${CMAKE_CURRENT_BINARY_DIR}"
  -Dname="${WEBVIEW2_NAME}"
  -Dversion="${WEBVIEW2_VERSION}"
  -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/extract-nupkg.cmake"
)

add_executable(main)
set_target_properties(main PROPERTIES OUTPUT_NAME cfs_frontend RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
target_sources(main PRIVATE
  main.cpp
  cfs_frontend.rc
)
target_include_directories(main BEFORE PRIVATE
  "${CMAKE_CURRENT_BINARY_DIR}" # for version.h
  "${CMAKE_CURRENT_BINARY_DIR}/${WEBVIEW2_NAME}-${WEBVIEW2_VERSION}/build/native/include" # for WebView2.h
)
target_link_directories(main BEFORE PRIVATE
  "${CMAKE_CURRENT_BINARY_DIR}" # WebView2Loader.lib
)
target_link_libraries(main PRIVATE
  comctl32
  wininet
  dwmapi
  WebView2Loader
)
file(GENERATE OUTPUT "${CMAKE_BINARY_DIR}/bin/cfs_frontend.txt" INPUT "${CMAKE_CURRENT_SOURCE_DIR}/../README.md")
add_custom_command(TARGET main PRE_LINK COMMAND ${CMAKE_COMMAND} -E copy "${WEBVIEW2_DLL}" "${CMAKE_BINARY_DIR}/bin/")
add_custom_command(TARGET main PRE_LINK COMMAND ${CMAKE_COMMAND} -E copy "${WEBVIEW2_LIB}" "${CMAKE_CURRENT_BINARY_DIR}/WebView2Loader.lib")
add_dependencies(main generate_version_h extract_webview2)
list(APPEND targets main)

foreach(target ${targets})
  target_compile_definitions(${target} PRIVATE
    WINVER=0x0601
    _WIN32_WINNT=0x0601
    _WINDOWS
    _UNICODE
    UNICODE
    $<$<CONFIG:Debug>:_DEBUG>
    $<$<CONFIG:Release>:NDEBUG>
  )
  target_compile_options(${target} PRIVATE
  	-flto
    -mstackrealign
    -Wall
    -Wextra
    -Werror=return-type
    -pedantic-errors
    $<$<CONFIG:Debug>:-O0>
    $<$<CONFIG:Release>:-O2>
  )
  target_link_options(${target} PRIVATE
  	-flto
    -mwindows
    -static
    $<$<CONFIG:Debug>:-O0>
    $<$<CONFIG:Release>:-O2>
    $<$<CONFIG:Release>:-s>
  )
endforeach(target)
