file(GLOB hdrs "*.h")
file(GLOB srcs "*.cpp")

add_library(simplecpp_objs OBJECT ${srcs} ${hdrs})
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(simplecpp_objs PRIVATE -Wno-zero-as-null-pointer-constant)
endif()

# TODO: conflicts with ANALYZE_* options
if (ENABLE_OSS_FUZZ AND CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_library(simplecpp_objs_sanitized OBJECT EXCLUDE_FROM_ALL ${srcs} ${hdrs})
    target_compile_options(simplecpp_objs_sanitized PRIVATE -fsanitize=address)
    target_compile_options(simplecpp_objs_sanitized PRIVATE -Wno-zero-as-null-pointer-constant)
endif()


