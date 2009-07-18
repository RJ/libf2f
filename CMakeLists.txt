PROJECT(libf2f)
CMAKE_MINIMUM_REQUIRED(VERSION 2.6 FATAL_ERROR)

SET(F2F_PATH "${CMAKE_CURRENT_SOURCE_DIR}")
SET(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}" "${F2F_PATH}/CMakeModules")

SET(SRC "${F2F_PATH}/src")
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${F2F_PATH}/bin")
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${F2F_PATH}/lib")

set(CMAKE_CXX_FLAGS "-pthread")

# binaries get installed here
SET(CMAKE_INSTALL_PREFIX "/usr/local/")

# on linux x86-64 set this to OFF if you are having (-fPIC...) build problems.
# then: cd build/ && rm -rf CMakeCache.txt CMakeFiles/ cmake_install.cmake resolvers/
# then: cmake .. ; make
# alternatively, recompile boost with -fPIC like so:
# bjam --v2 cxxflags=-fPIC stage
SET(Boost_USE_STATIC_LIBS ON)
#SET(Boost_USE_STATIC_LIBS OFF)

SET(Boost_USE_MULTITHREADED ON)
FIND_PACKAGE(Boost 1.35 REQUIRED COMPONENTS filesystem system regex thread program_options date_time)

INCLUDE_DIRECTORIES(
                    ${F2F_PATH}/include
                    ${Boost_INCLUDE_DIR}
                   )

LINK_DIRECTORIES(
                    ${F2F_PATH}/lib
                    /usr/local/lib
                ) 

SET(CMAKE_VERBOSE_MAKEFILE ON)
INCLUDE(InstallRequiredSystemLibraries)

IF(WIN32)
	ADD_DEFINITIONS(/DNOMINMAX)
	ADD_DEFINITIONS(/DWIN32_LEAN_AND_MEAN)
ENDIF(WIN32)

IF(NOT WIN32)
    # removes operators like "and", "or" and "not"
    #ADD_DEFINITIONS(-Wall -ggdb)
    ADD_DEFINITIONS(-g )
    ADD_DEFINITIONS(-fno-operator-names)
    ADD_DEFINITIONS(-fPIC)
    ADD_DEFINITIONS(-O2)
ENDIF(NOT WIN32)
                     
ADD_LIBRARY( f2f STATIC
             ${SRC}/router.cpp
             ${SRC}/protocol.cpp
             ${SRC}/connection.cpp
           )

SET_TARGET_PROPERTIES(  f2f PROPERTIES
                        CMAKE_RUNTIME_OUTPUT_DIRECTORY "${F2F_PATH}/lib" 
                     )

ADD_EXECUTABLE( f2f-demo
                ${F2F_PATH}/app/main.cpp
              )

TARGET_LINK_LIBRARIES( f2f
					   ${Boost_LIBRARIES}	  # Boost license
                     )

TARGET_LINK_LIBRARIES( f2f-demo
                       f2f
					   ${Boost_LIBRARIES}	  # Boost license
                     )

INSTALL(TARGETS f2f ARCHIVE DESTINATION lib)
INSTALL(TARGETS f2f-demo RUNTIME DESTINATION bin)
INSTALL(DIRECTORY include/libf2f DESTINATION include PATTERN "*~" EXCLUDE)
