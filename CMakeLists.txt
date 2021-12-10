cmake_minimum_required(VERSION 3.5)

project(fm-radio-service)

option(ENABLE_TESTS "Build tests" ON)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
set(CMAKE_AUTOMOC ON)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

include(cmake/EnableCoverageReport.cmake)
include(CTest)

include(GNUInstallDirs)

find_package(PkgConfig)
find_package(Qt5Core REQUIRED)
find_package(Qt5DBus REQUIRED)
find_package(Qt5Test REQUIRED)

string(TOLOWER "${CMAKE_BUILD_TYPE}" cmake_build_type_lower) # Build types should always be lowercase but sometimes they are not.

if(cmake_build_type_lower MATCHES coverage)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage" )
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --coverage" )
    SET(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} --coverage" )
    SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} --coverage" )
endif()

add_definitions(
  -DQT_NO_KEYWORDS
)

add_subdirectory(src)
if(ENABLE_TESTS)
    enable_testing()
    add_subdirectory(tests)
    enable_coverage_report(
        TARGETS fm-radio-service
        FILTER tests/* obj-*/*
    )
endif()
