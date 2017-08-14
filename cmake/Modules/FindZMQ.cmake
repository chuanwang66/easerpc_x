# Once done these will be defined:
#
#  ZMQ_FOUND
#  ZMQ_INCLUDE_DIRS
#  ZMQ_LIBRARIES
#
# For use in OBS: 
#
#  ZMQ_INCLUDE_DIR

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
	set(_lib_suffix 64)
else()
	set(_lib_suffix 32)
endif()

find_path(ZMQ_INCLUDE_DIR
	NAMES zmq.h
	HINTS
		ENV zmqPath${_lib_suffix}
		ENV zmqPath
		ENV DepsPath${_lib_suffix}
		ENV DepsPath
		${zmqPath${_lib_suffix}}
		${zmqPath}
		${DepsPath${_lib_suffix}}
		${DepsPath}
		${_ZMQ_INCLUDE_DIRS}
	PATHS
		/usr/include /usr/local/include /opt/local/include /sw/include
	PATH_SUFFIXES
		include)

find_library(ZMQ_LIB
	NAMES ${_ZMQ_LIBRARIES} libzmq libzmq-v120-mt-4_0_4
	HINTS
		ENV zmqPath${_lib_suffix}
		ENV zmqPath
		ENV DepsPath${_lib_suffix}
		ENV DepsPath
		${zmqPath${_lib_suffix}}
		${zmqPath}
		${DepsPath${_lib_suffix}}
		${DepsPath}
		${_ZMQ_LIBRARY_DIRS}
	PATHS
		/usr/lib /usr/local/lib /opt/local/lib /sw/lib
	PATH_SUFFIXES
		lib${_lib_suffix} lib
		libs${_lib_suffix} libs
		bin${_lib_suffix} bin
		../lib${_lib_suffix} ../lib
		../libs${_lib_suffix} ../libs
		../bin${_lib_suffix} ../bin)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(zmq DEFAULT_MSG ZMQ_LIB ZMQ_INCLUDE_DIR)
mark_as_advanced(ZMQ_INCLUDE_DIR ZMQ_LIB)

if(ZMQ_FOUND)
	set(ZMQ_INCLUDE_DIRS ${ZMQ_INCLUDE_DIR})
	set(ZMQ_LIBRARIES ${ZMQ_LIB})
endif()
