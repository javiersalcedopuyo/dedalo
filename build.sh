#!/bin/bash

compile_tests()
{
    echo "Compiling tests..."
    clang++ -std=c++20 -DNDEBUG tests.cpp -o testing -Werror -Wall -pedantic -fsanitize=undefined,address -O0 -g -MJ compile_commands.json
}

compile_tests_if_needed()
{
    if [ -f "testing" ]; then
        local executable_timestamp=$(stat -f "%m" "testing")
        local dedalo_cpp_timestamp=$(stat -f "%m" "dedalo.cpp")
        local tests_cpp_timestamp=$(stat -f "%m" "tests.cpp")

        if [[ $executable_timestamp -lt $dedalo_cpp_timestamp || $executable_timestamp -lt $tests_cpp_timestamp ]]; then
            compile_tests
        fi
    else
        compile_tests
    fi
}

compile_debug()
{
    echo "Compiling..."
    time clang++ -std=c++20 dedalo.cpp -o dedalo -Werror -Wall -pedantic -fsanitize=undefined,address -O0 -g -MJ compile_commands.json
    echo "...Done"
}

if [ $# -eq 0 ] || [ "$1" == "debug" ]; then
    compile_debug
elif [ "$1" == "release" ]; then
    time clang++ -std=c++20 dedalo.cpp -o dedalo -Werror -Wall -pedantic -fsanitize=undefined -O3
elif [ "$1" == "test" ] || [ "$1" == "-t" ]; then
    compile_tests_if_needed
    if [ "$2" != "--no-run" ]; then
        echo "Running tests..."
        echo "---"
        ./testing $2
    fi
    echo "...Done"
elif [ "$1" == "clean" ] || [ "$1" == "-c" ]; then
    rm -rf dedalo compile_commands.json build *.dSYM 
elif [ "$1" == "run" ]; then
    compile_debug
    echo "Running..."
    ./dedalo
    echo "...Done"
else
    echo "Invalid arguments"
fi
