add_executable(FTPCommandTest FTPCommandTest.cpp)
target_include_directories(FTPCommandTest PRIVATE ${FTPSERVER_INCLUDE_DIRS})
target_link_libraries(FTPCommandTest PRIVATE ${FTPSERVER_LINK_LIBRARIES} gtest pthread)
target_link_directories(FTPCommandTest PRIVATE ${FTPSERVER_LIBRARY_DIRS})
add_test(NAME CommandTest COMMAND FTPCommandTest)

