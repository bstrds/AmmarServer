project( geoposshare ) 
cmake_minimum_required( VERSION 2.8.7 )

set(CMAKE_C_FLAGS "-fPIE -pie -s -Wl,-z,relro,-z,now -fstack-protector -O2")


# Extend cmake's module path
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../cmake/modules ${CMAKE_MODULE_PATH})
   
add_executable(geoposshare main.c ) 
target_link_libraries(geoposshare pthread rt AmmarServer )
set_target_properties(geoposshare PROPERTIES DEBUG_POSTFIX "D")
add_dependencies(geoposshare AmmarServer)  
       
    set_target_properties(geoposshare PROPERTIES 
                          DEBUG_POSTFIX ""
                          ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                          LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                          RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                         )

#    install(TARGETS AmmarServer
#                    LIBRARY DESTINATION lib
#                    ARCHIVE DESTINATION lib
#                    RUNTIME DESTINATION bin)

