cmake_minimum_required(VERSION 3.21)

project(GalaxyPenMapper VERSION 0.3.2 LANGUAGES CXX)

if(MSVC)
    add_compile_options(/utf-8)
endif()

# Some localized MSVC installations emit /showIncludes text that Ninja does not
# recognize, leaving stale objects after a header layout changes. This small
# project explicitly depends on its own headers so those objects are rebuilt.
file(GLOB_RECURSE GALAXY_PROJECT_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/driver/shared/*.h")
file(GLOB_RECURSE GALAXY_PROJECT_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp")
set_source_files_properties(${GALAXY_PROJECT_SOURCES} PROPERTIES OBJECT_DEPENDS "${GALAXY_PROJECT_HEADERS}")

add_executable(GalaxyPenHidScanner
    src/main.cpp
)

add_executable(GalaxyPenEventMonitor WIN32
    src/monitor.cpp
    src/capture/SessionWriter.cpp
    src/input/PenCapture.cpp
    src/input/PenSignature.cpp
    src/input/RawHidCapture.cpp
    src/ui/DiagnosticPanel.cpp
    src/ui/MainWindow.cpp
)

add_executable(GalaxyPenMapperTray WIN32
    src/tray.cpp
    src/ui/TipTestPanel.cpp
)
target_include_directories(GalaxyPenMapperTray PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/driver/shared")

foreach(target GalaxyPenHidScanner GalaxyPenEventMonitor GalaxyPenMapperTray)
    target_compile_features(${target} PRIVATE cxx_std_20)
    target_compile_definitions(${target} PRIVATE UNICODE _UNICODE NOMINMAX _WIN32_WINNT=0x0A00)
endforeach()

target_link_libraries(GalaxyPenHidScanner PRIVATE setupapi hid)
target_include_directories(GalaxyPenHidScanner PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/driver/shared")
target_link_libraries(GalaxyPenEventMonitor PRIVATE user32 gdi32)
target_link_libraries(GalaxyPenMapperTray PRIVATE user32 gdi32 comctl32 shell32 comdlg32)
target_link_options(GalaxyPenMapperTray PRIVATE "/MANIFESTUAC:level='requireAdministrator' uiAccess='false'")

if(MSVC)
    target_compile_options(GalaxyPenHidScanner PRIVATE /W4 /permissive-)
    target_compile_options(GalaxyPenEventMonitor PRIVATE /W4 /permissive-)
    target_compile_options(GalaxyPenMapperTray PRIVATE /W4 /permissive-)
endif()

include(CTest)
if(BUILD_TESTING)
    add_executable(GalaxyPenMapperUiCheck WIN32 src/tray.cpp src/ui/TipTestPanel.cpp)
    target_compile_features(GalaxyPenMapperUiCheck PRIVATE cxx_std_20)
    target_compile_definitions(GalaxyPenMapperUiCheck PRIVATE GALAXY_PEN_UI_CHECK UNICODE _UNICODE NOMINMAX _WIN32_WINNT=0x0A00)
    target_link_libraries(GalaxyPenMapperUiCheck PRIVATE user32 gdi32 comctl32 shell32 comdlg32)
    target_link_options(GalaxyPenMapperUiCheck PRIVATE "/MANIFESTUAC:level='asInvoker' uiAccess='false'")
    add_executable(SessionWriterSmoke
        tests/SessionWriterSmoke.cpp
        src/capture/SessionWriter.cpp
    )
    target_compile_features(SessionWriterSmoke PRIVATE cxx_std_20)
    target_compile_definitions(SessionWriterSmoke PRIVATE UNICODE _UNICODE NOMINMAX _WIN32_WINNT=0x0A00)
    if(MSVC)
        target_compile_options(SessionWriterSmoke PRIVATE /W4 /permissive-)
    endif()
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/test-smoke")
    add_test(NAME SessionWriterSmoke COMMAND SessionWriterSmoke)
    set_tests_properties(SessionWriterSmoke PROPERTIES
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/test-smoke")

    add_executable(PenSignatureTest
        tests/PenSignatureTest.cpp
        src/input/PenSignature.cpp
    )
    target_compile_features(PenSignatureTest PRIVATE cxx_std_20)
    target_compile_definitions(PenSignatureTest PRIVATE UNICODE _UNICODE NOMINMAX _WIN32_WINNT=0x0A00)
    if(MSVC)
        target_compile_options(PenSignatureTest PRIVATE /W4 /permissive-)
    endif()
    add_test(NAME PenSignatureTest COMMAND PenSignatureTest)

    add_executable(PenSignatureReplay
        tests/PenSignatureReplay.cpp
        src/input/PenSignature.cpp
    )
    target_compile_features(PenSignatureReplay PRIVATE cxx_std_20)
    target_compile_definitions(PenSignatureReplay PRIVATE UNICODE _UNICODE NOMINMAX _WIN32_WINNT=0x0A00)
    if(MSVC)
        target_compile_options(PenSignatureReplay PRIVATE /W4 /permissive-)
    endif()

    add_executable(PressureTransformTest tests/PressureTransformTest.cpp)
    target_compile_features(PressureTransformTest PRIVATE cxx_std_20)
    if(MSVC)
        target_compile_options(PressureTransformTest PRIVATE /W4 /permissive-)
    endif()
    add_test(NAME PressureTransformTest COMMAND PressureTransformTest)

    add_executable(ToolFlagsTest tests/ToolFlagsTest.cpp)
    target_compile_features(ToolFlagsTest PRIVATE cxx_std_20)
    target_include_directories(ToolFlagsTest PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/driver/shared")
    if(MSVC)
        target_compile_options(ToolFlagsTest PRIVATE /W4 /permissive-)
    endif()
    add_test(NAME ToolFlagsTest COMMAND ToolFlagsTest)

    foreach(test DriverEngineTest DriverClientStateTest TipTestCaptureTest)
        add_executable(${test} tests/${test}.cpp)
        target_compile_features(${test} PRIVATE cxx_std_20)
        target_include_directories(${test} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/driver/shared")
        if(MSVC)
            target_compile_options(${test} PRIVATE /W4 /permissive-)
        endif()
        add_test(NAME ${test} COMMAND ${test})
    endforeach()
    add_executable(DriverCaptureReplay tests/DriverCaptureReplay.cpp)
    target_compile_features(DriverCaptureReplay PRIVATE cxx_std_20)
    target_include_directories(DriverCaptureReplay PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/driver/shared")
    if(MSVC)
        target_compile_options(DriverCaptureReplay PRIVATE /W4 /permissive-)
    endif()
endif()
