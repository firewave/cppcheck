add_executable(test-filelist
        testfilelist.cpp
        ${CMAKE_SOURCE_DIR}/gui/filelist.cpp
        ${CMAKE_SOURCE_DIR}/lib/pathmatch.cpp
        ${CMAKE_SOURCE_DIR}/lib/path.cpp)
target_include_directories(test-filelist PRIVATE ${CMAKE_SOURCE_DIR}/gui)
target_compile_definitions(test-filelist PRIVATE SRCDIR="${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(test-filelist Qt5::Core Qt5::Test)