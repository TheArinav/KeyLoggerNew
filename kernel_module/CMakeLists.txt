cmake_minimum_required(VERSION 3.0.0 FATAL_ERROR)

project(keylogger VERSION 0.1.0 LANGUAGES C)
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

# Build Kernel Module (keylogger.ko)
set(KMOD_FILE ${CMAKE_CURRENT_BINARY_DIR}/keylogger.ko)
set(KBUILD_CMD make -C ${KERNELHEADERS_DIR} modules M=${CMAKE_CURRENT_BINARY_DIR} src=${CMAKE_CURRENT_SOURCE_DIR})

file(WRITE ${CMAKE_CURRENT_SOURCE_DIR}/Kbuild "obj-m := keylogger.o")

add_custom_command(OUTPUT ${KMOD_FILE}
        COMMAND ${KBUILD_CMD}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        DEPENDS keylogger.c
        COMMENT "Building kernel module keylogger.ko"
        VERBATIM)

add_custom_target(driver ALL DEPENDS ${KMOD_FILE})

# Optional dummy target - remove if unnecessary
# add_library(dummy_target MODULE keylogger.c)
