cmake_minimum_required(VERSION 3.0.0 FATAL_ERROR)

project(keylogger VERSION 0.1.0 LANGUAGES C CXX)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Step 1: Ensure the kernel_module directory exists and build the kernel module
add_subdirectory(kernel_module)

# Step 2: Proceed with the User Space Application
file(GLOB_RECURSE USR_SRCS "usr_src/*.c" "usr_src/*.cpp" "usr_src/*.h" "usr_src/*.hpp")

# User space executable
add_executable(keylogger_user ${USR_SRCS} usr_main.cpp)

# Set the output directory for the user-space application
set_target_properties(keylogger_user PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Find and link the libnl library
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBNL3 REQUIRED libnl-3.0)
pkg_check_modules(LIBNL_GENL3 REQUIRED libnl-genl-3.0)

# Add include directories for the user-space application
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/usr_src ${LIBNL3_INCLUDE_DIRS} ${LIBNL_GENL3_INCLUDE_DIRS})

# Link the user-space application with required libraries
target_link_libraries(keylogger_user PRIVATE ${LIBNL3_LIBRARIES} ${LIBNL_GENL3_LIBRARIES} pthread)
