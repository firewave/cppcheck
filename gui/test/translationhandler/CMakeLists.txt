add_executable(test-translationhandler
        testtranslationhandler.cpp
        ${CMAKE_SOURCE_DIR}/gui/translationhandler.cpp)
target_include_directories(test-translationhandler PRIVATE ${CMAKE_SOURCE_DIR}/gui)
target_link_libraries(test-translationhandler Qt5::Core Qt5::Widgets Qt5::Test)