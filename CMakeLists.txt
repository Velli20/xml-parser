cmake_minimum_required (VERSION 3.8.0)
project(libxml_test)

set(ProjDirPath ${CMAKE_CURRENT_SOURCE_DIR})

file(GLOB LIBXML_TEST_SOURCES
    "${ProjDirPath}/xml_parser.c"
    "${ProjDirPath}/xml_parser_alloc.c"
    "${ProjDirPath}/test.h"
    "${ProjDirPath}/test.c"
)

include_directories(${ProjDirPath}/.)
add_executable(libxml_test ${LIBXML_TEST_SOURCES})

target_compile_features(libxml_test PUBLIC cxx_std_17)

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(libxml_test PUBLIC -Weverything)
endif()

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU" )
    target_compile_options(libxml_test PUBLIC -Wall)
    target_compile_options(libxml_test PUBLIC -Wextra)
    target_compile_options(libxml_test PUBLIC -Werror)
    target_compile_options(libxml_test PUBLIC -Wno-unused-macros)
    target_compile_options(libxml_test PUBLIC -Wno-format-nonliteral)
    target_compile_options(libxml_test PUBLIC -Wno-c++98-compat)
endif()

add_custom_target(run
    COMMAND libxml_test
    DEPENDS libxml_test
    WORKING_DIRECTORY ${CMAKE_PROJECT_DIR}
)
