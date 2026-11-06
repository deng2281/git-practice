cmake_minimum_required(VERSION 3.10)

project(kxxx LANGUAGES CXX)

add_executable(kxxx
	kvector.h
	kvector.cpp
	klist.h
	klist.cpp
	main.cpp
)

target_compile_features(kxxx PRIVATE cxx_std_17)
set_target_properties(kxxx PROPERTIES CXX_EXTENSIONS OFF)

if(MSVC)
	# Replace CMake's default warning level instead of passing both /W3 and /W4.
	string(REGEX REPLACE "/W[0-4]" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
	target_compile_options(kxxx PRIVATE /W4 /WX /EHsc /permissive- /utf-8)
else()
	target_compile_options(kxxx PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

# Keep the compiler's runtime DLLs next to the executable. This avoids loading
# incompatible copies from other tools, such as Anaconda, through PATH.
if(MINGW)
	get_filename_component(mingwRuntimeDirectory "${CMAKE_CXX_COMPILER}" DIRECTORY)
	foreach(runtimeLibrary libstdc++-6.dll libgcc_s_seh-1.dll libwinpthread-1.dll)
		if(EXISTS "${mingwRuntimeDirectory}/${runtimeLibrary}")
			add_custom_command(TARGET kxxx POST_BUILD
				COMMAND "${CMAKE_COMMAND}" -E copy_if_different
					"${mingwRuntimeDirectory}/${runtimeLibrary}"
					"$<TARGET_FILE_DIR:kxxx>"
				VERBATIM
			)
		endif()
	endforeach()
endif()

enable_testing()
add_test(NAME kcontainers_core COMMAND kxxx)
set_tests_properties(kcontainers_core PROPERTIES TIMEOUT 30)
