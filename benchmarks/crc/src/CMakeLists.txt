project(crc LANGUAGES C ASM)

# List source files to be compiled
add_executable(${PROJECT_NAME}
    # Benchmark files
    crc/crc.c
    crc/main.c

    # Other
    ${ALL_TOOLCHAIN_CODE}
    )

# Change target suffix
set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ${OUTPUT_SUFFIX})

target_include_directories(${PROJECT_NAME}
    PUBLIC
    crc
    portme
    )

target_link_libraries(${PROJECT_NAME}
    )

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND echo "Disassembling..."
    COMMAND ${CMAKE_OBJDUMP} -d --start-address=2147489544 --mattr=+m,+c --print-imm-hex --symbolize-operands ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.elf > ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.dis
    COMMAND ${CMAKE_SOURCE_DIR}/../instruction_labler.sh ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.dis
)