set(LIB_INSTALL_DIR "${PROJECT_SOURCE_DIR}/../../install/lib")

# Sources
set(PASS_SRC
    PassUtils.cpp
)

# Compilation flags
set_source_files_properties(${PASS_SRC} PROPERTIES COMPILE_FLAGS " -std=c++17 -fPIC")

# configure LLVM
find_package(LLVM REQUIRED CONFIG)

set(LLVM_RUNTIME_OUTPUT_INTDIR ${CMAKE_BINARY_DIR}/${CMAKE_CFG_INTDIR}/)
set(LLVM_LIBRARY_OUTPUT_INTDIR ${CMAKE_BINARY_DIR}/${CMAKE_CFG_INTDIR}/)

list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")
include(HandleLLVMOptions)
include(AddLLVM)

message(STATUS "LLVM_DIR IS ${LLVM_CMAKE_DIR}.")

include_directories(
    ${LLVM_INCLUDE_DIRS}
    $ENV{NOELLE}/include
    $ENV{NOELLE}/include/svf
    ${CMAKE_SOURCE_DIR}/include
    ${COMMON_DIR}/include
)

# Declare the LLVM pass to compile
add_llvm_library(${PROJECT_NAME} MODULE ${PASS_SRC})

# Copy the .so file to the lib dir
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E
    copy $<TARGET_FILE:${PROJECT_NAME}> ${LIB_INSTALL_DIR}
)
