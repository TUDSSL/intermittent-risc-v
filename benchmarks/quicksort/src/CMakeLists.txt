# Add executable target
add_executable(${PROJECT_NAME}
    main.c
    ${ALL_TOOLCHAIN_CODE}
    )

# Change target suffix
set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ${OUTPUT_SUFFIX})

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND echo "Disassembling..."
    COMMAND ${CMAKE_OBJDUMP} -d --mattr=+m,+c --print-imm-hex --symbolize-operands ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.elf > ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.dis
    COMMAND ${CMAKE_SOURCE_DIR}/../instruction_labler.sh ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.dis
)