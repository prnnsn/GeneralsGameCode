# Standalone Windows SDK probe: no game libraries or legacy DX8 dependency.
cmake_minimum_required(VERSION 3.20)
project(GeneralsDX11Smoke LANGUAGES CXX)
if(NOT WIN32)
    message(FATAL_ERROR "DX11 smoke requires Windows and the D3D11 SDK")
endif()
add_executable(gfx_dx11_smoke smoke.cpp)
target_compile_features(gfx_dx11_smoke PRIVATE cxx_std_17)
target_link_libraries(gfx_dx11_smoke PRIVATE d3d11 dxgi user32)
