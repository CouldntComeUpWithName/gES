add_executable(bench_iteration)

target_sources(bench_iteration PRIVATE "iteration.cpp")

target_link_libraries(bench_iteration PRIVATE ges)
target_compile_options(bench_iteration PRIVATE /FAs /FAcs)