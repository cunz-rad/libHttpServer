
#ifndef HTTP_HTTP_PRIVATE_HPP
#define HTTP_HTTP_PRIVATE_HPP

#include "libHttpServer/Http.hpp"

namespace HTTP
{

    #define CRLF "\r\n"

    #define HTTP_DO_DBG if( 1 ) qDebug
    #define HTTP_NO_DBG if( 0 ) qDebug

    #define HTTP_PARSER_DBG HTTP_NO_DBG

    #define HTTP_DBG HTTP_NO_DBG

}

#endif
