# Add executable target
add_executable(${PROJECT_NAME}
    main.c
    # main.S
    ${ALL_TOOLCHAIN_CODE}
    )

# Change target suffix
set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ${OUTPUT_SUFFIX})

# Compiler options for this project
target_compile_options(${PROJECT_NAME} PUBLIC
    -Wall
    -g
    -O1
    )

# Linker options for this project
target_link_options(${PROJECT_NAME} PUBLIC
    LINKER:-Map=${PROJECT_NAME}.map
    )

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND echo "Disassembling..."
    COMMAND ${CMAKE_OBJDUMP} -D --mattr=+m,+c --print-imm-hex --symbolize-operands ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.elf > ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.dis
    COMMAND ${CMAKE_SOURCE_DIR}/../instruction_labler.sh ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.dis
)
