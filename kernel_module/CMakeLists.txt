cmake_minimum_required(VERSION 3.0.0 FATAL_ERROR)

project(keylogger_kernel_module VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Kernel Module Info
add_definitions(-D__KERNEL__ -DMODULE)

# Find the kernel release
execute_process(
        COMMAND uname -r
        OUTPUT_VARIABLE KERNEL_RELEASE
        OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Find the kernel headers
find_path(
        KERNELHEADERS_DIR
        include/linux/user.h
        PATHS /usr/src/linux-headers-${KERNEL_RELEASE}
)

message(STATUS "Kernel release: ${KERNEL_RELEASE}")
message(STATUS "Kernel headers: ${KERNELHEADERS_DIR}")

# Set output file for kernel module
set(KMOD_FILE ${CMAKE_BINARY_DIR}/keylogger.ko)

# Command to build the kernel module using the kernel's build system
add_custom_command(
        OUTPUT ${KMOD_FILE}
        COMMAND make -C ${KERNELHEADERS_DIR} M=${CMAKE_CURRENT_SOURCE_DIR} modules
        COMMENT "Building kernel module keylogger.ko"
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

# Target to build the kernel module
add_custom_target(driver ALL DEPENDS ${KMOD_FILE})
