cmake_minimum_required(VERSION 3.8)
project(ycc)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES
        include/buffer.h
        include/colors.h
        include/enumerate.h
        include/error.h
        include/keyword.inc
        include/Lex.h
        include/macro.h
        include/Memory.h
        include/parser.h
        include/path.h
        include/stddef.h
        include/Token.h
        include/utils.h
        src/buffer.cpp
        src/error.cpp
        src/Lex.cpp
        src/macro.cpp
        src/Memory.cpp
        src/parser.cpp
        src/path.cpp
        src/Token.cpp
        src/utils.cpp
        test/funcTest.h
        test/funcTest.cpp
        main.cpp src/encode.cpp include/encode.h src/code_gen.cpp include/code_gen.h include/configuration.h)

add_executable(ycc ${SOURCE_FILES})