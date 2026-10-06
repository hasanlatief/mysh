add_executable(mysh
        main.c
        builtin_commands.c
        process_dispatch.c
        string_helpers.c
)

add_compile_options(
        -Wall -Wextra -Wpedantic -Werror
        -Wconversion
        -Wshadow
        -fanalyzer
        -fsanitize=address,undefined
        -flto
)

add_link_options(
        -fsanitize=address,undefined
        -flto
)
