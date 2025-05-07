add_executable("event-queue-test")

target_sources("event-queue-test" PRIVATE test.cpp)

target_link_libraries("event-queue-test" PRIVATE ges)