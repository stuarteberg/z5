# define test libraries
SET(TEST_LIBS
    ${Boost_FILESYSTEM_LIBRARY}    
    ${Boost_SYSTEM_LIBRARY}    
    gtest
    gtest_main
    pthread
)

# add metadata test
add_executable(test_metadata test_metadata.cxx)
target_link_libraries(test_metadata ${TEST_LIBS})

# add zarray test
add_executable(test_dataset test_dataset.cxx)
target_link_libraries(test_dataset ${TEST_LIBS} ${COMPRESSION_LIBRARIES})

# add factory test
add_executable(test_factories test_factories.cxx)
target_link_libraries(test_factories ${TEST_LIBS} ${COMPRESSION_LIBRARIES})

# add attributes test
add_executable(test_attributes test_attributes.cxx)
target_link_libraries(test_attributes ${TEST_LIBS} ${COMPRESSION_LIBRARIES})

add_subdirectory(compression)
add_subdirectory(io)
add_subdirectory(multiarray)
add_subdirectory(test_zarr)
add_subdirectory(test_n5)
