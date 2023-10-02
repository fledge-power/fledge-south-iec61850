cmake_minimum_required(VERSION 3.16)

project(RunTests)

# Supported options:
# -DFLEDGE_INCLUDE
# -DFLEDGE_LIB
# -DFLEDGE_SRC
# -DFLEDGE_INSTALL
#
# If no -D options are given and FLEDGE_ROOT environment variable is set
# then Fledge libraries and header files are pulled from FLEDGE_ROOT path.

set(CMAKE_CXX_FLAGS "-std=c++11 -O3")

# Generation version header file
set_source_files_properties(version.h PROPERTIES GENERATED TRUE)

if (${CMAKE_BUILD_TYPE} STREQUAL Coverage)
	add_custom_command(
	  OUTPUT version.h
	  DEPENDS ${CMAKE_SOURCE_DIR}/VERSION
	  COMMAND ${CMAKE_SOURCE_DIR}/mkversion ${CMAKE_SOURCE_DIR}
	  COMMENT "Generating version header"
	  VERBATIM
	)
else()
	add_custom_command(
	  OUTPUT version.h
	  DEPENDS ${CMAKE_SOURCE_DIR}/../VERSION
	  COMMAND ${CMAKE_SOURCE_DIR}/../mkversion ${CMAKE_SOURCE_DIR}/..
	  COMMENT "Generating version header"
	  VERBATIM
	)
endif()

include_directories(${CMAKE_BINARY_DIR})

# Set plugin type (south, north, filter)
set(PLUGIN_TYPE "north")

# Add here all needed Fledge libraries as list
set(NEEDED_FLEDGE_LIBS common-lib services-common-lib)

set(BOOST_COMPONENTS system thread)

find_package(Boost 1.53.0 COMPONENTS ${BOOST_COMPONENTS} REQUIRED)
include_directories(SYSTEM ${Boost_INCLUDE_DIR})

# Find source files
file(GLOB SOURCES ../src/*.cpp)
file(GLOB unittests "*.cpp")

# Find Fledge includes and libs, by including FindFledge.cmak file
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/..)
find_package(Fledge)
# If errors: make clean and remove Makefile
if (NOT FLEDGE_FOUND)
	if (EXISTS "${CMAKE_BINARY_DIR}/Makefile")
		execute_process(COMMAND make clean WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
		file(REMOVE "${CMAKE_BINARY_DIR}/Makefile")
	endif()
	# Stop the build process
	message(FATAL_ERROR "Fledge plugin '${PROJECT_NAME}' build error.")
endif()
# On success, FLEDGE_INCLUDE_DIRS and FLEDGE_LIB_DIRS variables are set 

# Locate GTest
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

# Add ../include
include_directories(../include)
include_directories(/usr/local/include/libiec61850)
# Add Fledge include dir(s)
include_directories(${FLEDGE_INCLUDE_DIRS})

# Add Fledge lib path
link_directories(${FLEDGE_LIB_DIRS})

# copy certificates to output
file(COPY data DESTINATION ${CMAKE_BINARY_DIR}/tests)

# Link runTests with what we want to test and the GTest and pthread library
add_executable(RunTests ${unittests} ${SOURCES} version.h)

set(FLEDGE_INSTALL "" CACHE INTERNAL "")
# Install library
if (FLEDGE_INSTALL)
	message(STATUS "Installing ${PROJECT_NAME} in ${FLEDGE_INSTALL}/plugins/${PLUGIN_TYPE}/${PROJECT_NAME}")
	install(TARGETS ${PROJECT_NAME} DESTINATION ${FLEDGE_INSTALL}/plugins/${PLUGIN_TYPE}/${PROJECT_NAME})
endif()

target_link_libraries(${PROJECT_NAME} ${GTEST_LIBRARIES} pthread)
target_link_libraries(${PROJECT_NAME} ${NEEDED_FLEDGE_LIBS})
target_link_libraries(${PROJECT_NAME}  ${Boost_LIBRARIES})


# Add the libiec61850
find_library(LIBIEC61850 libiec61850.a)
if (NOT LIBIEC61850)
    message(FATAL_ERROR "The 61850 library 'libiec61850' was not found (in the standard lib dir)\n"
			"Please build and install the libiec61850 library")
	return()
endif()

target_link_libraries(${PROJECT_NAME} -L/usr/local/lib -liec61850)

target_link_libraries(${PROJECT_NAME} -lpthread -ldl)