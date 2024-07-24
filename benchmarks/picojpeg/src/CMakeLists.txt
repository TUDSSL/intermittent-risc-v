project(picojpeg LANGUAGES C ASM)

# List source files to be compiled
add_executable(${PROJECT_NAME}
    # Benchmark files
    picojpeg/picojpeg.c
    picojpeg/picojpegtest.c

    # Other
    ${ALL_TOOLCHAIN_CODE}
    )

# Change target suffix
set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ${OUTPUT_SUFFIX})

target_include_directories(${PROJECT_NAME}
    PUBLIC
    picojpeg
    )

target_link_libraries(${PROJECT_NAME}
    )

if(OPT_LEVEL STREQUAL "-O3")
set(START_ADDR "2147538224")
elseif(OPT_LEVEL STREQUAL "-O2")
set(START_ADDR "2147538224")
elseif(OPT_LEVEL STREQUAL "-O1")
set(START_ADDR "2147538240")
else()
set(START_ADDR "2147537160")
endif()

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND echo "Disassembling..."
    COMMAND ${CMAKE_OBJDUMP} -d --start-address=${START_ADDR} --mattr=+m,+c --print-imm-hex --symbolize-operands ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.elf > ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.dis
    COMMAND ${CMAKE_SOURCE_DIR}/../instruction_labler.sh ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.dis
)