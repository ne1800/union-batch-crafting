execute_process(
    COMMAND
        "${CMAKE_COMMAND}" --build "${BUILD}" --target
        ubc_unrepresentable_game_text_tests
    RESULT_VARIABLE status
    OUTPUT_VARIABLE output
    ERROR_VARIABLE error
)
if(
    status EQUAL 0
    OR NOT "${output}${error}" MATCHES "Unrepresentable character in game codepage"
)
    message(
        FATAL_ERROR
        "Expected a game codepage compilation error: ${output}${error}"
    )
endif()
