cmake_minimum_required(VERSION 2.8)

project( OttDate )

find_package( OpenSSL REQUIRED )

set( FOSSA_DIR "${CMAKE_SOURCE_DIR}" )
find_file( FOSSA_C "fossa.c" ${FOSSA_DIR} )
include_directories( ${FOSSA_DIR} )

add_executable( ottdate ottdate.c ${FOSSA_C} ${FROZEN_C} )
target_link_libraries( ottdate ${OPENSSL_LIBRARIES} )
