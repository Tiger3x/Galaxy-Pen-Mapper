cmake_minimum_required(VERSION 3.21)

project(GalaxyPenMapper VERSION 0.1.0 LANGUAGES CXX)

add_executable(GalaxyPenHidScanner
    src/main.cpp
)

target_compile_features(GalaxyPenHidScanner PRIVATE cxx_std_20)
target_compile_definitions(GalaxyPenHidScanner PRIVATE UNICODE _UNICODE NOMINMAX)
target_link_libraries(GalaxyPenHidScanner PRIVATE setupapi hid)

if(MSVC)
    target_compile_options(GalaxyPenHidScanner PRIVATE /W4 /permissive-)
endif()
