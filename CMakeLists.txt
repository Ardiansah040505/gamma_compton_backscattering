cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

project (crack)

set (CMAKE_CXX_STANDARD 20)
set (CMAKE_CXX_STANDARD_REQUIRED)

find_package(Geant4 REQUIRED ui_all vis_all)

execute_process(COMMAND root-config --incdir OUTPUT_VARIABLE ROOT_INCLUDE_DIRS OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND root-config --libs OUTPUT_VARIABLE ROOT_LIBRARIES_FLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND root-config --version OUTPUT_VARIABLE ROOT_VERSION OUTPUT_STRIP_TRAILING_WHITESPACE)
separate_arguments(ROOT_LIBRARIES_LIST UNIX_COMMAND "${ROOT_LIBRARIES_FLAGS}")

include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${ROOT_INCLUDE_DIRS}
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
    ${ROOT_LIBRARIES_LIST}
)

file (GLOB MACRO_FILES ${PROJECT_SOURCE_DIR}/macros/*.mac)
file (GLOB ${MACRO_FILES} DESTINATION ${PROJECT_BINARY_DIR})

message (STATUS "Geant4 Version : $(Geant4_VISIOn)")
message (STATUS "ROOT Version   : ${ROOT_VERSION}")
message (STATUS "Project Cracks Configured Succesfully")