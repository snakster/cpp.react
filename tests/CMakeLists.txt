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
