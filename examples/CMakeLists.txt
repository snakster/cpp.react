### Example_BasicAlgorithms
add_executable(Example_BasicAlgorithms src/BasicAlgorithms.cpp)
target_link_libraries(Example_BasicAlgorithms CppReact)

### Example_BasicComposition
add_executable(Example_BasicComposition src/BasicComposition.cpp)
target_link_libraries(Example_BasicComposition CppReact)

### Example_BasicEvents
add_executable(Example_BasicEvents src/BasicEvents.cpp)
target_link_libraries(Example_BasicEvents CppReact)

### Example_BasicObservers
add_executable(Example_BasicObservers src/BasicObservers.cpp)
target_link_libraries(Example_BasicObservers CppReact)

### Example_BasicSignals
add_executable(Example_BasicSignals src/BasicSignals.cpp)
target_link_libraries(Example_BasicSignals CppReact)

### Example_BasicSynchronization
add_executable(Example_BasicSynchronization src/BasicSynchronization.cpp)
target_link_libraries(Example_BasicSynchronization CppReact)

### Example_Sandbox
add_executable(Example_Sandbox src/Main.cpp)
target_link_libraries(Example_Sandbox CppReact)
