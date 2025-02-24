cmake_minimum_required(VERSION 3.10)
project(ThreadSimulator VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add source files
set(SOURCES
    src/main.cpp
    src/thread_models.cpp
)

# Create executable
add_executable(thread_simulator ${SOURCES})

# Add include directories
target_include_directories(thread_simulator PRIVATE src)

# Link against pthread on Unix-like systems
if(UNIX)
    target_link_libraries(thread_simulator PRIVATE pthread)
endif()
