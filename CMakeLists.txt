cmake_minimum_required(VERSION 2.8)

project( OttDate )

find_package( OpenSSL REQUIRED )

#TODO: decide on how to integrate the fossa library
#set( FOSSA_DIR "${CMAKE_SOURCE_DIR}/../fossa" )
#find_file( FOSSA_C "fossa.c" ${FOSSA_DIR} )
#include_directories( ${FOSSA_DIR} )
#FOR NOW:
set(FOSSA_C fossa.c)

add_executable( ottdate main.cpp ottdate.cpp ${FOSSA_C} )
target_link_libraries( ottdate ${OPENSSL_LIBRARIES} )
install( TARGETS ottdate DESTINATION bin )

add_library( ottdate_static STATIC ottdate.cpp ${FOSSA_C} )
set_target_properties(ottdate_static PROPERTIES OUTPUT_NAME ottdate)
install( TARGETS ottdate_static DESTINATION lib )

add_library( ottdate_shared SHARED ottdate.cpp ${FOSSA_C} )
set_target_properties( ottdate_shared PROPERTIES OUTPUT_NAME ottdate)
target_link_libraries( ottdate_shared ${OPENSSL_LIBRARIES} )
install( TARGETS ottdate_shared DESTINATION lib )

