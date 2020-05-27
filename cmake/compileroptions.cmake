include(cmake/add_compile_options_optional.cmake)

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    if(CMAKE_BUILD_TYPE MATCHES "Release")
        # "Release" uses -O3 by default
        add_compile_options(-O2)
    endif()
    if (WARNINGS_ARE_ERRORS)
        add_compile_options(-Werror)
    endif()

    add_compile_options(-Wall -Wextra)
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.6)
        message(FATAL_ERROR "${PROJECT_NAME} c++11 support requires g++ 4.6 or greater, but it is ${CMAKE_CXX_COMPILER_VERSION}")
    endif ()

    add_compile_options(-Wcast-qual)                # Cast for removing type qualifiers
    add_compile_options(-Wno-deprecated-declarations)
    add_compile_options(-Wfloat-equal)              # Floating values used in equality comparisons
    add_compile_options(-Wmissing-declarations)     # If a global function is defined without a previous declaration
    add_compile_options(-Wmissing-format-attribute) #
    add_compile_options(-Wno-long-long)
    add_compile_options(-Woverloaded-virtual)       # when a function declaration hides virtual functions from a base class
    add_compile_options(-Wpacked)                   #
    add_compile_options(-Wredundant-decls)          # if anything is declared more than once in the same scope
    add_compile_options(-Wundef)
    add_compile_options(-Wno-shadow)                # whenever a local variable or type declaration shadows another one
    add_compile_options(-Wno-missing-field-initializers)
    add_compile_options(-Wno-missing-braces)
    add_compile_options(-Wno-sign-compare)
    add_compile_options(-Wno-multichar)
elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
   add_compile_options(-Werror)
   add_compile_options(-Weverything)

   # we do not care about these warnings
   add_compile_options(-Wno-c++98-compat)
   add_compile_options(-Wno-c++98-compat-pedantic)
   add_compile_options(-Wno-deprecated-declarations)
   add_compile_options(-Wno-four-char-constants)
   add_compile_options(-Wno-missing-braces)
   add_compile_options(-Wno-missing-field-initializers)
   add_compile_options(-Wno-multichar)
   add_compile_options(-Wno-sign-compare)
   add_compile_options(-Wno-unused-function)

   add_compile_options_optional(-Wno-redundant-parens) # cannot enable since it occurs in moc generated code
   add_compile_options_optional(-Wno-enum-enum-conversion) # cannot enable since it happens with valid Qt code

   # TODO: fix or move to warnings we do not care about
   add_compile_options_optional(-Wno-sign-conversion)
   add_compile_options_optional(-Wno-exit-time-destructors)
   add_compile_options_optional(-Wno-global-constructors)
   add_compile_options_optional(-Wno-padded)
   add_compile_options_optional(-Wno-format-nonliteral)
   add_compile_options_optional(-Wno-old-style-cast)
   add_compile_options_optional(-Wno-implicit-fallthrough)
   add_compile_options_optional(-Wno-double-promotion)
   add_compile_options_optional(-Wno-weak-vtables)
   add_compile_options_optional(-Wno-covered-switch-default)
   add_compile_options_optional(-Wno-shadow-field-in-constructor)
   add_compile_options_optional(-Wno-implicit-int-conversion)
   add_compile_options_optional(-Wno-shorten-64-to-32)
   add_compile_options_optional(-Wno-disabled-macro-expansion)
   add_compile_options_optional(-Wno-deprecated-copy-dtor)
   add_compile_options_optional(-Wno-newline-eof)
   add_compile_options_optional(-Wno-shadow-field)
   add_compile_options_optional(-Wno-shadow-uncaptured-local)
   add_compile_options_optional(-Wno-unreachable-code)
   add_compile_options_optional(-Wno-implicit-int-float-conversion)
   add_compile_options_optional(-Wno-switch-enum)
   add_compile_options_optional(-Wno-shadow)
   add_compile_options_optional(-Wno-unused-macros)
   add_compile_options_optional(-Wno-implicit-float-conversion)
   add_compile_options_optional(-Wno-unused-exception-parameter)
   add_compile_options_optional(-Wno-return-std-move-in-c++11)
   add_compile_options_optional(-Wno-unused-member-function)
   add_compile_options_optional(-Wno-missing-noreturn)
   add_compile_options_optional(-Wno-float-conversion)

   if(ENABLE_COVERAGE OR ENABLE_COVERAGE_XML)
      message(FATAL_ERROR "Not use clang for generate code coverage. Use gcc.")
   endif()
endif()

if (MSVC)
    add_compile_options(/W4)
    add_compile_options(/wd4018) # warning C4018: '>': signed/unsigned mismatch
    add_compile_options(/wd4127) # warning C4127: conditional expression is constant
    add_compile_options(/wd4244) # warning C4244: 'initializing': conversion from 'int' to 'char', possible loss of data
    add_compile_options(/wd4251)
    # Clang: -Wshorten-64-to-32 -Wimplicit-int-conversion
    add_compile_options(/wd4267) # warning C4267: 'return': conversion from 'size_t' to 'int', possible loss of data
    add_compile_options(/wd4389) # warning C4389: '==': signed/unsigned mismatch
    add_compile_options(/wd4482)
    add_compile_options(/wd4512)
    add_compile_options(/wd4701) # warning C4701: potentially uninitialized local variable 'err' used
    add_compile_options(/wd4706) # warning C4706: assignment within conditional expression
    add_compile_options(/wd4800) # warning C4800: 'const SymbolDatabase *' : forcing value to bool 'true' or 'false' (performance warning)

    if (WARNINGS_ARE_ERRORS)
        add_compile_options(/WX)
    endif()
endif()

# TODO: check if this can be enabled again - also done in Makefile
if (CMAKE_SYSTEM_NAME MATCHES "Linux" AND
    CMAKE_CXX_COMPILER_ID MATCHES "Clang")

    add_compile_options(-U_GLIBCXX_DEBUG)
endif()

if (MSVC)
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /STACK:8000000")
endif()

if (CYGWIN)
    # TODO: this is a linker flag - not a compiler flag
    add_compile_options(-Wl,--stack,8388608)
endif()

include(cmake/dynamic_analyzer_options.cmake)
