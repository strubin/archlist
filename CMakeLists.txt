project(archlist)


cmake_minimum_required(VERSION 2.8)

if(NOT ${CMAKE_CXX_COMPILER_VERSION} MATCHES "4\\.[6-9]\\.[0-9]")
    message(FATAL_ERROR "Compiler supports version 4.6 and above")
endif()

#message(${CMAKE_CXX_COMPILER_VERSION})
#if(gcc_compiler_version VERSION_GREATER "4.5.99")

set(CMAKE_C_FLAGS "-std=c++0x")
set(CMAKE_CXX_FLAGS "-std=c++0x")

set(CMAKE_INSTALL_DO_STRIP "ON")
set(CMAKE_INSTALL_PREFIX "/usr")
set( REQUIRED_LIBRARIES magic)


FIND_PATH(MAGIC_INCLUDE_DIR magic.h
/usr/local/include
/usr/include
/usr/include/linux
)

FIND_PATH(MAGIC_INCLUDE_DIR magic.h /usr/include/foo /usr/local/include/foo)
	
	FIND_LIBRARY(MAGIC_LIBRARY NAMES magic PATH /usr/lib /usr/local/lib /usr/lib/i386-linux-gnu /usr/lib/x86_64-linux-gnu) 
	
	IF (MAGIC_INCLUDE_DIR AND MAGIC_LIBRARY)
	   SET(MAGIC_FOUND TRUE)
	ENDIF (MAGIC_INCLUDE_DIR AND MAGIC_LIBRARY)
	
	
	IF (MAGIC_FOUND)
	   IF (NOT Magic_FIND_QUIETLY)
	      MESSAGE(STATUS "Found libmagic: ${MAGIC_LIBRARY}")
	   ENDIF (NOT Magic_FIND_QUIETLY)
	ELSE (MAGIC_FOUND)
	   IF (Magic_FIND_REQUIRED)
	      MESSAGE(FATAL_ERROR "Could not find libmagic")
	   ENDIF (Magic_FIND_REQUIRED)
	ENDIF (MAGIC_FOUND)

include_directories( "/usr/include" ${MAGIC_INCLUDE_DIR})

add_executable(archlist main.cpp)
target_link_libraries(archlist magic)

#add_executable(test_regex regex.cpp)
#target_link_libraries(archlist magic)

install(TARGETS archlist RUNTIME DESTINATION bin)
add_custom_target(uninstall COMMAND xargs rm -f < install_manifest.txt)


##set(CPACK_SOURCE_IGNORE_FILES _CPack_Packages  *.bz2  *.tar.* *.Z *.gz .git .kde* *.kdev4 CMakeCache.txt Testing)
set(CPACK_SOURCE_IGNORE_FILES archlist$ _CPack_Packages/* CMakeCache.txt .kde* .kdev4)
set(CPACK_GENERATOR TBZ2 RPM ZIP)
set(CPACK_PACKAGE_NAME ${CMAKE_PROJECT_NAME})
set(CPACK_PACKAGE_VENDOR "CMake.org")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY ${CMAKE_PROJECT_NAME} " - CPack Component Installation Example")
set(CPACK_PACKAGE_VERSION "1.0.0")
set(CPACK_PACKAGE_VERSION_MAJOR "1")
set(CPACK_PACKAGE_VERSION_MINOR "0")
set(CPACK_PACKAGE_VERSION_PATCH "0")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "CPack Component Example")
set(CPACK_NSIS_MODIFY_PATH ON)
set(CPACK_INSTALL_PREFIX "/usr")

set(CPACK_PACKAGE_CONTACT "alchlist@trubin.spb.ru")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Sergey Trubin")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libmagic")
INCLUDE(CPack)
#endif()
