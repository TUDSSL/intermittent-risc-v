project(sha LANGUAGES C ASM)

# List source files to be compiled
add_executable(${PROJECT_NAME}
    # Benchmark files
    sha/sha.c
    sha/main.c

    # Other
    portme/fread.c
    ${ALL_TOOLCHAIN_CODE}
    )

# Change target suffix
set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ${OUTPUT_SUFFIX})

target_include_directories(${PROJECT_NAME}
    PUBLIC
    sha
    portme
     )

target_link_libraries(${PROJECT_NAME}
    )

target_compile_definitions(${PROJECT_NAME}
    PUBLIC
    LITTLE_ENDIAN
    )
    
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND echo "Disassembling..."
    COMMAND ${CMAKE_OBJDUMP} -d --start-address=2147489432 --mattr=+m,+c --print-imm-hex --symbolize-operands ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.elf > ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.dis
    COMMAND ${CMAKE_SOURCE_DIR}/../instruction_labler.sh ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.dis
)