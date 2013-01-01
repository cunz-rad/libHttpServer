
PROJECT( LIB_HTTP_SERVER )

QT_PREPARE( Core Network -Gui )

SET( SRC_FILES
    HttpServer.cpp
    Request.cpp
    Response.cpp

    Internal/http_parser.c
    Internal/Connection.cpp
)

SET( HDR_C_FILES
    Internal/http_parser.h
)

SET( HDR_CPP_FILES
    Http.hpp
    HttpServer.hpp
    Request.hpp
    Response.hpp
)

SET( HDR_CPP_PRV_FILES
    HttpServerPrivate.hpp

    Internal/Request.hpp
    Internal/Response.hpp
    Internal/Connection.hpp
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

