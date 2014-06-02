### Configuration
cmake_minimum_required (VERSION 2.6)

project (CppReact)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wpedantic")

include_directories ("${PROJECT_SOURCE_DIR}/include")

### CppReact
add_library(CppReact 
	src/engine/PulsecountEngine.cpp
	src/engine/SubtreeEngine.cpp
	src/engine/ToposortEngine.cpp
	src/logging/EventLog.cpp
	src/logging/EventRecords.cpp)

target_link_libraries(CppReact tbb)

### examples/ 
option(build_examples "Build examples?" ON)
if(build_examples)
	add_subdirectory(examples)
endif()

### benchmarks/ 
option(build_benchmarks "Build benchmarks?" OFF)
if(build_benchmarks)
	add_subdirectory(benchmarks)
endif()

### tests/ 
option(build_tests "Build unit tests?" OFF)
if(build_tests)
	add_subdirectory(tests)
endif()

# cmake -Dbuild_tests=BOOL:ON 
if(build_tests)   
	if(DEFINED ENV{GTEST_DIR})
		message("Using gtest found in $ENV{GTEST_DIR}.")
	else()
		message("GTEST_DIR is not defined. You must tell CMake where to find the gtest source.")
		return()
	endif()
	add_subdirectory($ENV{GTEST_DIR} ${CMAKE_CURRENT_BINARY_DIR}/gtest)
endif()
