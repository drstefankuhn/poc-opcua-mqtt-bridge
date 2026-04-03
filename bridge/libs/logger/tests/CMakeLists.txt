find_package(Catch2 REQUIRED)

add_executable(logger_tests
    stdout_logger_test.cpp
)

target_link_libraries(logger_tests
    PRIVATE
        logger::logger
        Catch2::Catch2WithMain
)

add_test(NAME logger_tests COMMAND logger_tests)
