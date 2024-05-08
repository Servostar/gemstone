include(CTest)

# ------------------------------------------------------- #
# CTEST 1
# test if the program accepts a file as input

add_test(NAME input_file_check
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/bin/check
        COMMAND python ${GEMSTONE_TEST_DIR}/input_file/test_input_file.py ${GEMSTONE_TEST_DIR}/input_file/test.gem)
