cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

project (crack)

execute_process(COMMAND root-config --incdir OUTPUT_VARIABLE ROOT_INCLUDE_DIRS OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND root-config --libs OUTPUT_VARIABLE ROOT_LIBRARIES_FLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND root-config --version OUTPUT_VARIABLE ROOT_VERSION OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND root-config --cflags OUTPUT_VARIABLE ROOT_CFLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)
separate_arguments(ROOT_LIBRARIES_LIST UNIX_COMMAND "${ROOT_LIBRARIES_FLAGS}")

# Dynamically set C++ standard to match ROOT
if (ROOT_CFLAGS MATCHES "-std=c\\+\\+20")
    set(CMAKE_CXX_STANDARD 20)
elseif (ROOT_CFLAGS MATCHES "-std=c\\+\\+17")
    set(CMAKE_CXX_STANDARD 17)
else ()
    set(CMAKE_CXX_STANDARD 17)
endif ()
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Geant4 REQUIRED ui_all vis_all)

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