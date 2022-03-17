cmake_minimum_required(VERSION 3.15)

## Create project
project(happy-hacking-gnu C)

## Include source code
file(GLOB_RECURSE src src/*.h src/*.c)

## Add dependencies
if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
	file(GLOB_RECURSE deps deps/argparse/argparse.c deps/hidapi/linux/hid.c)
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
	file(GLOB_RECURSE deps deps/argparse/argparse.c deps/hidapi/mac/hid.c)
else()
	file(GLOB_RECURSE deps deps/argparse/argparse.c deps/hidapi/windows/hid.c)
endif()

## Create executable
add_executable(happy-hacking-gnu ${src} ${deps})
set_target_properties(happy-hacking-gnu PROPERTIES OUTPUT_NAME "hhg")

## Include libraries 
if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
	target_link_libraries(happy-hacking-gnu PRIVATE udev)
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
	find_library(ObjC objc)
	find_library(CoreFoundation CoreFoundation)
	find_library(IOKit IOKit)
	find_library(AppKit AppKit)
	target_link_libraries(happy-hacking-gnu PRIVATE ${IOKit} ${CoreFoundation} ${ObjC} ${AppKit})
else()
	target_link_libraries(happy-hacking-gnu PRIVATE)
endif()

target_include_directories(happy-hacking-gnu PRIVATE deps/argparse deps/hidapi/hidapi)
