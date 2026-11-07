# Keep the allocator inside this assignment so it builds without the training repo.
get_filename_component(ALLOC_COUNTER_SOURCE_DIR
	"${CMAKE_CURRENT_SOURCE_DIR}/../alloccounter" ABSOLUTE)

add_executable(simpleson_task02
	simpleson_memory_test.cpp
	"${ALLOC_COUNTER_SOURCE_DIR}/alloc_counter.cpp"
)
target_include_directories(simpleson_task02 PRIVATE "${ALLOC_COUNTER_SOURCE_DIR}")
target_link_libraries(simpleson_task02 PRIVATE simpleson)
