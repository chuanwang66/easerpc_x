project(easerpc)

find_package(Threads REQUIRED)

find_package(ZMQ REQUIRED)
include_directories(SYSTEM ${ZMQ_INCLUDE_DIR})

add_definitions(-DEASERPC_EXPORTS)

if(WIN32)
	set(easerpc_PLATFORM_SOURCES
		util/threading-windows.c
		util/platform-windows.c)
	set(easerpc_PLATFORM_HEADERS
		util/threading-windows.h)
	if(MSVC)
		set(easerpc_PLATFORM_DEPS
		${easerpc_PLATFORM_DEPS}
		w32-pthreads)
	endif()
elseif(APPLE)
	set(easerpc_PLATFORM_SOURCES
		util/threading-posix.c
		util/platform-nix.c)
	set(easerpc_PLATFORM_HEADERS
		util/threading-posix.h)
elseif(UNIX)
endif()

if(MSVC)
	set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /SAFESEH:NO")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /EHc-")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHc-")
endif()

set(easerpc_util_SOURCES
	util/cJSON.c
	util/bmem.c
	util/base.c)
set(easerpc_util_HEADERS
	util/vc/vc_inttypes.h
	util/vc/vc_stdbool.h
	util/vc/vc_stdint.h
	util/c99defs.h
	util/threading.h
	util/platform.h
	util/cJSON.h
	util/bmem.h
	util/base.h)

set(easerpc_test_SOURCES
	test/broker.cpp
	test/worker.cpp
	test/client.cpp
	test/simple_pthread.cpp)
set(easerpc_test_HEADERS
	${easerpc_test_HEADERS})

set(easerpc_easerpc_SOURCES
	${easerpc_PLATFORM_SOURCES}
	rpc_client.cpp
	rpc_node.cpp
	easerpc.cpp)
set(easerpc_easerpc_HEADERS
	${easerpc_PLATFORM_HEADERS}
	rpc_common.h
	rpc_client.h
	rpc_node.h
	easerpc.h)
	
set(easerpc_SOURCES
	${easerpc_util_SOURCES}
	${easerpc_easerpc_SOURCES}
	${easerpc_test_SOURCES})

set(easerpc_HEADERS
	${easerpc_util_HEADERS}
	${easerpc_easerpc_HEADERS}
	${easerpc_test_HEADERS})

source_group("easerpc\\Source Files" FILES ${easerpc_easerpc_SOURCES})
source_group("easerpc\\Header Files" FILES ${easerpc_easerpc_HEADERS})
source_group("util\\Source Files" FILES ${easerpc_util_SOURCES})
source_group("util\\Header Files" FILES ${easerpc_util_HEADERS})
source_group("test\\Source Files" FILES ${easerpc_test_SOURCES})
source_group("test\\Header Files" FILES ${easerpc_test_HEADERS})

add_library(easerpc SHARED ${easerpc_SOURCES} ${easerpc_HEADERS})

set_target_properties(easerpc PROPERTIES
	OUTPUT_NAME easerpc
	VERSION "0"
	SOVERSION "0")

if(NOT MSVC)
	target_compile_options(easerpc
		PUBLIC
			-mmmx
			-msse
			-msse2)
endif()

target_compile_options(easerpc
	PUBLIC
		"${THREADS_DEFINITIONS}")
target_include_directories(easerpc
	PUBLIC
		"$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>")
target_link_libraries(easerpc
	PRIVATE
		${ZMQ_LIBRARIES}
		${easerpc_PLATFORM_DEPS}
	PUBLIC
		${THREADS_LIBRARIES})


#copy files generated
function(install_easerpc)

	#copy zmq & w32-phreads

	add_custom_command(TARGET easerpc POST_BUILD
		COMMAND "${CMAKE_COMMAND}" -E copy
			"${CMAKE_SOURCE_DIR}/3rd/bin32/libzmq-v120-mt-4_0_4.dll" "${CMAKE_BINARY_DIR}${EASERPC_OUTPUT_DIR}/$<CONFIGURATION>/"
		VERBATIM)

	add_custom_command(TARGET easerpc POST_BUILD
		COMMAND "${CMAKE_COMMAND}" -E copy
			"${CMAKE_BINARY_DIR}${EASERPC_OUTPUT_DIR}/deps/w32-pthreads/$<CONFIGURATION>/w32-pthreads.dll" "${CMAKE_BINARY_DIR}${EASERPC_OUTPUT_DIR}/$<CONFIGURATION>/"
		VERBATIM)
	
	#copy easerpc

	add_custom_command(TARGET easerpc POST_BUILD
		COMMAND "${CMAKE_COMMAND}" -E copy
			"${CMAKE_BINARY_DIR}${EASERPC_OUTPUT_DIR}/easerpc/$<CONFIGURATION>/easerpc.dll" "${CMAKE_BINARY_DIR}${EASERPC_OUTPUT_DIR}/$<CONFIGURATION>/"
		VERBATIM)
	
	add_custom_command(TARGET easerpc POST_BUILD
		COMMAND "${CMAKE_COMMAND}"
			"-DCONFIG=$<CONFIGURATION>"
			"-DFNAME=easerpc.dll"
			"-DINPUT=${CMAKE_BINARY_DIR}${EASERPC_OUTPUT_DIR}/easerpc/$<CONFIGURATION>/"
			"-DOUTPUT=${CMAKE_BINARY_DIR}${EASERPC_OUTPUT_DIR}/$<CONFIGURATION>/"
			-P "${CMAKE_SOURCE_DIR}/cmake/copy_on_debug_helper.cmake"
		VERBATIM)
endfunction()

if(WIN32)
	install_easerpc()
endif()
