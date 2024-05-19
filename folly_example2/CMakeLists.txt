cmake_minimum_required(VERSION 3.10)

project(folly_example2 VERSION 1.0.0 LANGUAGES CXX)

find_package(Boost REQUIRED)
find_package(folly REQUIRED)
find_package(Threads REQUIRED)
find_package(gflags REQUIRED)

# Set the C++ standard and standard library
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")

# Include directories
include_directories(SYSTEM ${Boost_INCLUDE_DIRS} ${FOLLY_INCLUDE_DIR})

# Add executable
add_executable(${PROJECT_NAME} src/main.cpp)

# Link libraries
target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES} ${FOLLY_LIBRARIES})
