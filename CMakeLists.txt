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
add_executable(test_lew   ${TEST_UNITTEST_SRC} )
add_executable(c10kserver  "${PROJ_ROOT}/test/c10kserver.cc" )
add_executable(c10kclient  "${PROJ_ROOT}/test/c10kclient.cc" )
target_link_libraries( test_lew     pthread event event_pthreads ${PROJ_NAME})
target_link_libraries( c10kserver   pthread event event_pthreads ${PROJ_NAME})
target_link_libraries( c10kclient   pthread event event_pthreads ${PROJ_NAME})

install(TARGETS ${PROJ_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
install(DIRECTORY ${PROJ_ROOT}/include/lew    DESTINATION ${CMAKE_INSTALL_PREFIX}/include)

