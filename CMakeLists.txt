cmake_minimum_required(VERSION 2.8)

project( OttDate )

find_package( OpenSSL REQUIRED )

#TODO: decide on how to integrate the fossa library
#set( FOSSA_DIR "${CMAKE_SOURCE_DIR}/../fossa" )
#find_file( FOSSA_C "fossa.c" ${FOSSA_DIR} )
#include_directories( ${FOSSA_DIR} )
#FOR NOW:
set(FOSSA_C fossa.c)

add_executable( ottdate ottdate.c ${FOSSA_C} )
target_link_libraries( ottdate ${OPENSSL_LIBRARIES} )
