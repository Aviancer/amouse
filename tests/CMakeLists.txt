cmake_minimum_required(VERSION 3.14)
project(amouse-tests)

# GoogleTest requires at least C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(settings-tests
  src/settings-tests.cc ../shared/crc8/libcrc8.c ../shared/mouse_defs.h ../shared/settings.c ../shared/utils.c 
)
target_link_libraries(settings-tests
  GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(settings-tests)
