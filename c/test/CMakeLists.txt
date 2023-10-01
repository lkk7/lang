project(langTests LANGUAGES C)

add_executable(lang_test source/lang_test.c)
target_link_libraries(lang_test PRIVATE lang_lib)
target_compile_features(lang_test PRIVATE c_std_99)

add_test(NAME lang_test COMMAND lang_test)
add_folders(Test)
