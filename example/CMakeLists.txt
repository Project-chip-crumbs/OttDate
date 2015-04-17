add_executable( ottdate main.cpp )

message( "=${CMAKE_THREAD_LIBS_INIT}" )

target_link_libraries( ottdate ${OPENSSL_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ottdate_static )

install(
  TARGETS ottdate 
  EXPORT ${PROJECT_NAME}Targets
  RUNTIME DESTINATION bin
)
