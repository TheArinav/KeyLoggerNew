cmake_minimum_required(VERSION 3.0.0 FATAL_ERROR)

project(keylogger VERSION 0.1.0 LANGUAGES C)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Find the kernel release
execute_process(
        COMMAND uname -r
        OUTPUT_VARIABLE KERNEL_RELEASE
        OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Set the directory where the kernel module will be built
set(KERNEL_MODULE_DIR ${CMAKE_CURRENT_BINARY_DIR})

# Build Kernel Module (keylogger.ko) using the kernel build system
add_custom_command(
        OUTPUT ${KERNEL_MODULE_DIR}/keylogger.ko
        COMMAND make -C /lib/modules/${KERNEL_RELEASE}/build M=${KERNEL_MODULE_DIR} modules
        COMMENT "Building kernel module keylogger.ko"
        WORKING_DIRECTORY ${KERNEL_MODULE_DIR}
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/keylogger.c
)

# Custom target to invoke the build of the kernel module
add_custom_target(driver ALL DEPENDS ${KERNEL_MODULE_DIR}/keylogger.ko)

# Prevent CMake from trying to compile the kernel module directly
add_custom_target(keylogger ALL)
