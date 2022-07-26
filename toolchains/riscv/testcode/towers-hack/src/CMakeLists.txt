project(towers LANGUAGES C ASM)

# List source files to be compiled
add_executable(${PROJECT_NAME}
    # Benchmark files
    towers/towers_main.c

    # Other
    portme/syscalls.c
    ${STARTUP_CODE}
    )

# Change target suffix
set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ${OUTPUT_SUFFIX})

target_include_directories(${PROJECT_NAME}
       PUBLIC
       towers
       portme
     )

 #target_link_libraries(${PROJECT_NAME}
 #    gcc
 #    c
 #    m
 #    )
