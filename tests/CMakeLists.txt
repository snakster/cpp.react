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
	src/EventStreamTest.cpp
	src/EventStreamTestQ.cpp
	src/MoveTest.cpp
	src/ObserverTest.cpp
	src/ObserverTestQ.cpp
	src/OperationsTest.cpp
	src/OperationsTestQ.cpp
	src/SignalTest.cpp
	src/SignalTestQ.cpp
	src/TransactionTest.cpp)

target_link_libraries(CppReactTest CppReact gtest gtest_main)
