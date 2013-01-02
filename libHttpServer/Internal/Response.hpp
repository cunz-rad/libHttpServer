/*
 * Modern CI
 * Copyright (C) 2012-2013 Sascha Cunz <sascha@babbelbox.org>
 *
 * This program is free software; you can redistribute it and/or modify it under the terms of the
 * GNU General Public License (Version 2) as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
 * even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with this program; if
 * not, see <http://www.gnu.org/licenses/>.
 *
 */

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
