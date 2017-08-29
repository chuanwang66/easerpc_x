cmake_minimum_required(VERSION 2.8.12)

project(easerpc_x)

if(WIN32)
	set(DepsPath "${CMAKE_SOURCE_DIR}/3rd/")
endif()

if(APPLE)
	set(CMAKE_MACOSX_RPATH ON)
	set(DepsPath "${CMAKE_SOURCE_DIR}/3rd_osx/")
endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

add_subdirectory(deps)
add_subdirectory(easerpc)
add_subdirectory(callee)
#add_subdirectory(caller)
