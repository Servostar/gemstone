include(CTest)

set(PROJECT_BINARY_DIR bin)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/tests)
set(CTEST_BINARY_DIRECTORY ${PROJECT_BINARY_DIR}/tests)

# Provide test to run here or include another CMakeLists.txt

add_subdirectory(logging)
add_subdirectory(input_file)
add_subdirectory(ast)
add_subdirectory(glib)
add_subdirectory(llvm)
add_subdirectory(project)
add_subdirectory(cache)
