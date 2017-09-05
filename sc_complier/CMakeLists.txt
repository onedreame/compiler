cmake_minimum_required(VERSION 3.7)
project(sc_complier)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES main.cpp token_analysier/Exception_Handler.h token_analysier/TokenCode.h token_analysier/DynString.h token_analysier/TKTable.h token_analysier/global_var.h token_analysier/TokenParser.h token_analysier/DynString.cpp token_analysier/DynArray.cpp token_analysier/DynArray.h token_analysier/TkTable.cpp token_analysier/DynStringDef.h token_analysier/DynArrayDef.h token_analysier/TkWordDef.h token_analysier/Exception_Handler.cpp token_analysier/TokenParser.cpp syntax_analysier/syntax_analysier.h syntax_analysier/external_declaration.h syntax_analysier/syntax_analysier.cpp syntax_analysier/Stack.h syntax_analysier/Stack.cpp syntax_analysier/Symbol.h syntax_analysier/Symbol.cpp syntax_analysier/syn_const.h)
add_executable(sc_complier ${SOURCE_FILES} token_analysier/DynString.cpp token_analysier/DynArray.cpp token_analysier/TkTable.cpp token_analysier/DynStringDef.h token_analysier/DynArrayDef.h token_analysier/TkWordDef.h token_analysier/Exception_Handler.cpp token_analysier/TokenParser.cpp syntax_analysier/syntax_analysier.h syntax_analysier/external_declaration.h syntax_analysier/syntax_analysier.cpp syntax_analysier/Stack.h syntax_analysier/Symbol.h syntax_analysier/Symbol.cpp syntax_analysier/syn_const.h )
