# Reuse the same selected JSON implementation as the demo and allocation tests.
add_library(simpleson_profile_workload STATIC profile_workload.cpp)
target_include_directories(simpleson_profile_workload PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(simpleson_profile_workload PUBLIC simpleson)
target_compile_definitions(simpleson_profile_workload PUBLIC PROFILE_STAGE="${SIMPLESON_VARIANT}")
set_property(TARGET simpleson_profile_workload PROPERTY INTERPROCEDURAL_OPTIMIZATION FALSE)

add_executable(simpleson_profile_cpu main.cpp)
target_link_libraries(simpleson_profile_cpu PRIVATE simpleson_profile_workload)
set_property(TARGET simpleson_profile_cpu PROPERTY INTERPROCEDURAL_OPTIMIZATION FALSE)
add_test(NAME assignment_profile COMMAND simpleson_profile_cpu 1)

# SDK and profiler binaries are installed separately, not copied into the homework.
option(PROFILE_WITH_MTUNER "Build the MTuner SDK capture target" OFF)
set(MTUNER_SDK_DIR "" CACHE PATH "Path to the installed MTuner SDK directory")
if(PROFILE_WITH_MTUNER)
	if(NOT WIN32 OR NOT MSVC OR NOT EXISTS "${MTUNER_SDK_DIR}/src/rmem_lib.cpp")
		message(FATAL_ERROR "MTuner capture requires Windows, MSVC and -DMTUNER_SDK_DIR=<installed SDK>")
	endif()
	set(MTUNER_SOURCES
		"${MTUNER_SDK_DIR}/src/rmem_lib.cpp"
		"${MTUNER_SDK_DIR}/src/rmem_hook.cpp"
		"${MTUNER_SDK_DIR}/src/rmem_get_module_info.cpp"
		"${MTUNER_SDK_DIR}/src/rmem_wrap_win.cpp")
	add_executable(simpleson_profile_mtuner main.cpp ${MTUNER_SOURCES})
	target_include_directories(simpleson_profile_mtuner PRIVATE "${MTUNER_SDK_DIR}/inc")
	target_compile_definitions(simpleson_profile_mtuner PRIVATE PROFILE_WITH_MTUNER)
	target_link_libraries(simpleson_profile_mtuner PRIVATE simpleson_profile_workload psapi)
	set_property(TARGET simpleson_profile_mtuner PROPERTY INTERPROCEDURAL_OPTIMIZATION FALSE)
	# Keep third-party SDK warning settings separate from the assignment sources.
	set_source_files_properties(${MTUNER_SOURCES} PROPERTIES COMPILE_OPTIONS "/W0;/WX-")
	target_compile_options(simpleson_profile_mtuner PRIVATE /utf-8 /Oy- /GL-)
	target_link_options(simpleson_profile_mtuner PRIVATE /LTCG:OFF)
endif()

if(MSVC)
	target_compile_options(simpleson_profile_workload PRIVATE /utf-8 /Oy- /GL-)
	target_compile_options(simpleson_profile_cpu PRIVATE /utf-8 /GL-)
	target_link_options(simpleson_profile_cpu PRIVATE /LTCG:OFF)
endif()
