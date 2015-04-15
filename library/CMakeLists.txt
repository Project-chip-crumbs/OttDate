# create the static library
add_library( ottdate_static STATIC ottdate.cpp fossa.c )
set_target_properties( ottdate_static PROPERTIES OUTPUT_NAME   ottdate )
set_target_properties( ottdate_static PROPERTIES PUBLIC_HEADER ottdate.hpp )
target_link_libraries( ottdate_static ${OPENSSL_LIBRARIES} )
install(
  TARGETS ottdate_static
  EXPORT ${PROJECT_NAME}Targets
  ARCHIVE       DESTINATION lib     COMPONENT lib
  PUBLIC_HEADER DESTINATION include COMPONENT dev
)
