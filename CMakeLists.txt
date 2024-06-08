cmake_minimum_required(VERSION 3.22)
project(expe)

set(CMAKE_CXX_STANDARD 20)

set(SOURCES
	cmdparser.hpp
	testing/memory.hpp
	fast_float.h
	functions.h
	controller.h
	tabulate.hpp
)

if(WIN32)
	list(APPEND SOURCES 
		main.h
		main.cpp
		c-win.cpp
	)
else()
	list(APPEND SOURCES 
		main.h
		main.cpp
		c-unx.c
	)
endif()
add_executable(expe ${SOURCES})
	
if(MSVC)
    target_compile_options(expe PRIVATE /FA)
elseif(MINGW OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(expe PRIVATE -O3 -msse2 -msse -pipe -march=alderlake -fno-math-errno)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(expe PRIVATE -O3)
endif()

if(UNIX)
    message(STATUS "Configuring assembly file generation...")
    add_custom_target(GenerateAssembly_Main ALL
        COMMAND ${CMAKE_CXX_COMPILER} -S ${CMAKE_CXX_FLAGS} -O3 -msse2 -fno-math-errno -fverbose-asm -o main.s ${CMAKE_SOURCE_DIR}/main.cpp
        COMMENT "Generating assembly for main.cpp"
        DEPENDS ${CMAKE_SOURCE_DIR}/main.cpp
    )
endif()