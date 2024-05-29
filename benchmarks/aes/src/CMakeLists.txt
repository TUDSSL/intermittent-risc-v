project(aes LANGUAGES C ASM)

# List source files to be compiled
add_executable(${PROJECT_NAME}
    # Benchmark files
    aes/aes.c
    aes/test.c

    # Other
    ${ALL_TOOLCHAIN_CODE}
    )

target_compile_definitions(${PROJECT_NAME}
    PUBLIC
    AES256
    #AES192=1
    #AES128
    )


# Change target suffix
set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ${OUTPUT_SUFFIX})

target_include_directories(${PROJECT_NAME}
    PUBLIC
    aes
    )

target_link_libraries(${PROJECT_NAME}
    #m
    )

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND echo "Disassembling..."
    COMMAND ${CMAKE_OBJDUMP} -d --start-address=2147490328 --mattr=+m,+c --print-imm-hex --symbolize-operands ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.elf > ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.dis
    COMMAND ${CMAKE_SOURCE_DIR}/../instruction_labler.sh ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.dis
)
