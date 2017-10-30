### Configuration
if(DEFINED ENV{GTEST_DIR})
	message("Using gtest found in $ENV{GTEST_DIR}.")
else()
	message("GTEST_DIR is not defined. You must tell CMake where to find the gtest source.")
	return()
endif()
add_subdirectory($ENV{GTEST_DIR} ${CMAKE_CURRENT_BINARY_DIR}/gtest)

### CppReactTest
add_executable(CppReactTest
	src/algorithm_tests.cpp
	src/common_tests.cpp
	src/event_tests.cpp
	src/observer_tests.cpp
	src/state_tests.cpp
	src/transaction_tests.cpp)

target_link_libraries(CppReactTest CppReact gtest gtest_main)
