cmake_minimum_required(VERSION 3.15)
project(ImageToICNSConverter VERSION 1.0 LANGUAGES CXX)

# Use C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Only support Windows
if(NOT WIN32)
    message(FATAL_ERROR "This project is currently supported only on Windows.")
endif()

# Set output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# Add the include folder for headers
include_directories(${CMAKE_SOURCE_DIR}/include)

# Source and header files
file(GLOB_RECURSE SOURCES src/*.cpp)
file(GLOB_RECURSE HEADERS include/*.h)

# Executable
add_executable(imagetoicns ${SOURCES} ${HEADERS})
# Link GDI+
target_link_libraries(imagetoicns PRIVATE "${CMAKE_SOURCE_DIR}/zlib.lib")

# Install target (optional)
install(TARGETS imagetoicns DESTINATION bin)
