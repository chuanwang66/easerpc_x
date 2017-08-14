project(callee)

set(callee_HEADERS
	cJSON.h
	)

set(callee_SOURCES
	cJSON.c
	main.cpp)

add_executable(callee
	${callee_HEADERS}
	${callee_SOURCES})

target_link_libraries(callee
	easerpc)

#copy files generated
function(install_callee)
	
	#copy callee

	add_custom_command(TARGET callee POST_BUILD
		COMMAND "${CMAKE_COMMAND}" -E copy
			"${CMAKE_BINARY_DIR}${EASERPC_OUTPUT_DIR}/callee/$<CONFIGURATION>/callee.exe" "${CMAKE_BINARY_DIR}${EASERPC_OUTPUT_DIR}/$<CONFIGURATION>/"
		VERBATIM)
	
	add_custom_command(TARGET callee POST_BUILD
		COMMAND "${CMAKE_COMMAND}"
			"-DCONFIG=$<CONFIGURATION>"
			"-DFNAME=callee.exe"
			"-DINPUT=${CMAKE_BINARY_DIR}${EASERPC_OUTPUT_DIR}/callee/$<CONFIGURATION>/"
			"-DOUTPUT=${CMAKE_BINARY_DIR}${EASERPC_OUTPUT_DIR}/$<CONFIGURATION>/"
			-P "${CMAKE_SOURCE_DIR}/cmake/copy_on_debug_helper.cmake"
		VERBATIM)
endfunction()

install_callee()