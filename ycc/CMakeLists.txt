cmake_minimum_required(VERSION 3.8)
project(ycc)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES
        include/buffer.h
        include/colors.h
        include/error.h
        include/stddef.h
        include/Token.h
        include/Lex.h
        include/path.h
        include/utils.h
        include/Memory.h
        include/parser.h
        include/enumerate.h
        include/keyword.inc
        include/macro.h
        main.cpp
#        )
        src/buffer.cpp
        src/error.cpp
        src/Token.cpp
        src/path.cpp src/Memory.cpp src/parser.cpp src/Lex.cpp src/utils.cpp  test/funcTest.cpp test/funcTest.h src/macro.cpp )

add_executable(ycc ${SOURCE_FILES})