add_executable(test-projectfile
        testprojectfile.cpp
        ${CMAKE_SOURCE_DIR}/gui/projectfile.cpp)
target_include_directories(test-projectfile PRIVATE ${CMAKE_SOURCE_DIR}/gui)
target_compile_definitions(test-projectfile PRIVATE SRCDIR="${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(test-projectfile Qt5::Core Qt5::Test)