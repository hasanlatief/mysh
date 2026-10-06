
add_executable(find_test find_test.c ../src/builtin_commands.c ../src/string_helpers.c)
add_test(NAME find_test COMMAND ./find_test)

add_executable(dispatch_test dispatch_test.c ../src/process_dispatch.c ../src/builtin_commands.c ../src/string_helpers.c)
add_test(NAME dispatch_test COMMAND ./dispatch_test)

add_executable(wildcard_test wildcard_test.c ../src/string_helpers.c ../src/builtin_commands)
add_test(NAME wildcard_test COMMAND ./wildcard_test)

add_test(
  NAME batch_test
  COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/test_batch.sh $<TARGET_FILE:mysh>
)
set_tests_properties(batch_test PROPERTIES WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

