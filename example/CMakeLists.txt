add_executable( ottdate main.cpp )

target_link_libraries( ottdate ${OPENSSL_LIBRARIES} ottdate_static )

install(
  TARGETS ottdate 
  EXPORT OttDateTargets
  RUNTIME DESTINATION bin
)
