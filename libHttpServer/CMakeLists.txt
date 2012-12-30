
PROJECT( LIB_HTTP_SERVER )

QT_PREPARE( Core Network -Gui )

SET( SRC_FILES
    http_parser.c
    HttpServer.cpp
)

SET( HDR_C_FILES
    http_parser.h
)

SET( HDR_CPP_FILES
    HttpServer.hpp
)

SET( HDR_CPP_PRV_FILES
    HttpServerPrivate.hpp
)

QT_MOC( MOC_FILES ${HDR_CPP_FILES} ${HDR_CPP_PRV_FILES} )

ADD_QT_LIBRARY(
    HttpServer SHARED

    ${HDR_C_FILES}
    ${HDR_CPP_FILES}
    ${HDR_CPP_PRV_FILES}
    ${SRC_FILES}
    ${MOC_FILES}
)

