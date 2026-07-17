cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

project (crack)

set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED)

find_package(Geant4 REQUIRED ui_all vis_all)

include_directories(
    ${PROJECT_SOURCE_DIR}/include
)

file (GLOB sources
    ${PROJECT_SOURCE_DIR}/include
)

file (GLOB sources
    ${PROJECT_SOURCE_DIR}/src/*.cc
)

add_executable(crack
    main.cc
    ${sources}
)

target_link_libraries(crack
    ${Geant4_LIBRARIES}
)

file (GLOB MACRO_FILES ${PROJECT_SOURCE_DIR}/macros/*.mac)
file (GLOB ${MACRO_FILES} DESTINATION ${PROJECT_BINARY_DIR})

message (STATUS "Geant4 Version : $(Geant4_VISIOn)")
message (STATUS "Project Cracks Configured Succesfully")