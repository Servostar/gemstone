include(CTest)

# ------------------------------------------------------- #
# CTEST 1
# test compilation and execution with GCC driver

add_test(NAME driver_gcc
        WORKING_DIRECTORY ${GEMSTONE_TEST_DIR}/driver/gcc
        COMMAND python ${GEMSTONE_TEST_DIR}/driver/gcc/test_driver_gcc.py)

add_test(NAME driver_clang
        WORKING_DIRECTORY ${GEMSTONE_TEST_DIR}/driver/clang
        COMMAND python ${GEMSTONE_TEST_DIR}/driver/clang/test_driver_clang.py)
