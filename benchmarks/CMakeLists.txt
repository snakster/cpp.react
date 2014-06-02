### CppReactBenchmark

add_executable(CppReactBenchmark src/Main.cpp)
target_link_libraries(CppReactBenchmark CppReact tbbmalloc_proxy)
