
PROJECT( LIB_HTTP_SERVER )

QT_PREPARE( Core Network -Gui )

SET( SRC_FILES
    Internal/http_parser.c
    Internal/Connection.cpp
    Internal/RoundRobinServer.cpp

    Request.cpp
    Response.cpp
    Server.cpp
    Session.cpp
    Cookie.cpp
    ContentProvider.cpp
)

SET( HDR_C_FILES
    Internal/http_parser.h
)

SET( HDR_CPP_FILES
    Http.hpp
    Request.hpp
    Response.hpp
    Server.hpp
    Session.hpp
    Cookie.hpp
    ContentProvider.hpp
)

SET( HDR_CPP_PRV_FILES

    Internal/Http.hpp
    Internal/Request.hpp
    Internal/Response.hpp
    Internal/Connection.hpp
    Internal/Server.hpp
    Internal/RoundRobinServer.hpp
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

