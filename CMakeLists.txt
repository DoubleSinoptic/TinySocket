add_executable (tinysockettest tinysocket.cpp tinysockettest.cpp)
if(WIN32)
    target_link_libraries(tinysockettest ws2_32)
endif()