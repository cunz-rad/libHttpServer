
PROJECT( LIB_HTTP_SERVER )

QT_PREPARE( Core Network -Gui )

SET( SRC_FILES
    http_parser.c
    HttpServer.cpp
    Request.cpp
    Response.cpp
    Connection.cpp
)

SET( HDR_C_FILES
    http_parser.h
)

SET( HDR_CPP_FILES
    Http.hpp
    HttpServer.hpp
    Request.hpp
    Response.hpp
)

SET( HDR_CPP_PRV_FILES
    HttpServerPrivate.hpp
    RequestPrivate.hpp
    ResponsePrivate.hpp

    Connection.hpp
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

