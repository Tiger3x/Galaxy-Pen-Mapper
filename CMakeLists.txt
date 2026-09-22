cmake_minimum_required(VERSION 3.21)

project(GalaxyPenMapper VERSION 0.2.5 LANGUAGES CXX)

add_executable(GalaxyPenHidScanner
    src/main.cpp
)

add_executable(GalaxyPenEventMonitor WIN32
    src/monitor.cpp
    src/capture/SessionWriter.cpp
    src/input/PenCapture.cpp
    src/input/RawHidCapture.cpp
    src/ui/DiagnosticPanel.cpp
    src/ui/MainWindow.cpp
)

foreach(target GalaxyPenHidScanner GalaxyPenEventMonitor)
    target_compile_features(${target} PRIVATE cxx_std_20)
    target_compile_definitions(${target} PRIVATE UNICODE _UNICODE NOMINMAX _WIN32_WINNT=0x0A00)
endforeach()

target_link_libraries(GalaxyPenHidScanner PRIVATE setupapi hid)
target_link_libraries(GalaxyPenEventMonitor PRIVATE user32 gdi32)

if(MSVC)
    target_compile_options(GalaxyPenHidScanner PRIVATE /W4 /permissive-)
    target_compile_options(GalaxyPenEventMonitor PRIVATE /W4 /permissive-)
endif()
