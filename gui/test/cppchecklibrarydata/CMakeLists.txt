add_executable(test-cppchecklibrarydata
        testcppchecklibrarydata.cpp
        ${CMAKE_SOURCE_DIR}/gui/cppchecklibrarydata.cpp)
target_include_directories(test-cppchecklibrarydata PRIVATE ${CMAKE_SOURCE_DIR}/gui)
target_link_libraries(test-cppchecklibrarydata Qt5::Core Qt5::Test)