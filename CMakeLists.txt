cmake_minimum_required(VERSION 3.21)

set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
project(xe86 VERSION 0.1)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE SRC_FILES src/*.cpp)
add_executable(xe86 ${SRC_FILES})

if(MSVC)
	target_compile_options(xe86 PRIVATE /W4)
else()
	target_compile_options(xe86 PRIVATE -Wall -Wextra)
endif()