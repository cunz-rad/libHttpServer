
#ifndef HTTP_RESPONSE_PRIVATE_HPP
#define HTTP_RESPONSE_PRIVATE_HPP


#include "libHttpServer/Response.hpp"

namespace HTTP
{

    class Request;
    class Connection;

    class Response::Data
    {
    public:
        enum Flag
        {
            KeepAlive           = 1 << 0,
            Close               = 1 << 1,
            ChunkedEncoding     = 1 << 2,

            BodyIsFixed         = 1 << 27,
            DataIsCompressed    = 1 << 28,
            SendAtOnce          = 1 << 29,
            ReadyToSend         = 1 << 30,
            HeadersSent         = 1 << 31
        };
        typedef QFlags< Flag > Flags;

        Connection*     mConnection;
        Request*        mRequest;
        Flags           mFlags;
        HeadersHash     mHeaders;
        QByteArray      mBodyData;
    };

}

#endif
