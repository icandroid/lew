cmake_minimum_required(VERSION 3.0)
set(PROJ_NAME   lew)
project(${PROJ_NAME})

set(PROJ_ROOT  ${CMAKE_SOURCE_DIR})
add_definitions( "-std=c++11" )
add_definitions( "-pthread" )
add_definitions( "-D_REENTRANT" )
add_definitions( "-D_GNU_SOURCE" )
add_definitions( "-Wall" )
if(DEBUG)
    add_definitions( "-g" )
    add_definitions( "-DDEBUG" )
endif(DEBUG)

include_directories( "${PROJ_ROOT}/include" )
include_directories( "${PROJ_ROOT}/test/gtest/include" )
include_directories( "${PROJ_ROOT}/test/gtest" )
include_directories( "${PROJ_ROOT}/test" )

file(GLOB MODULE_SRC "${PROJ_ROOT}/src/*.cc")

set(TEST_UNITTEST_SRC "${PROJ_ROOT}/test/test_lew.cc" )
add_library(${PROJ_NAME}  STATIC  ${MODULE_SRC})
add_executable(test_lew   ${TEST_UNITTEST_SRC} ${MODULE_SRC})
target_link_libraries( test_lew pthread )
target_link_libraries( test_lew event)
target_link_libraries( test_lew event_pthreads)


