include(CTest)

# ------------------------------------------------------- #
# CTEST 1
# test a simple project

add_test(NAME hello_world
        WORKING_DIRECTORY ${GEMSTONE_TEST_DIR}/hello_world
        COMMAND python ${GEMSTONE_TEST_DIR}/hello_world/test_hello_world.py)
