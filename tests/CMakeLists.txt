cmake_minimum_required(VERSION 3.16)
project(playerbots_plus_tests CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(GTest REQUIRED)
enable_testing()

add_executable(planner_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/Errands/ErrandPlanner.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/Errands/HuntPlanner.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/Errands/SharePlanner.cpp
    ErrandPlannerTest.cpp
    HuntPlannerTest.cpp
    SharePlannerTest.cpp)
target_include_directories(planner_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src/Errands)
target_compile_options(planner_tests PRIVATE -Wall -Wextra -Werror)
target_link_libraries(planner_tests PRIVATE GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(planner_tests)
