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

#ifndef HTTP_REQUEST_PRIVATE_HPP
#define HTTP_REQUEST_PRIVATE_HPP

#include <QHostAddress>
#include <QByteArray>
#include <QHash>

#include "libHttpServer/Request.hpp"

namespace HTTP
{

    class Request::Data
    {
    public:
        Data();

    public:
        void appendHeaderData( const HeaderName& header, const QByteArray& data );
        void appendBodyData( const QByteArray& data );
        void enterRecvBody();
        void enterFinished();

    public:
        Request*        mRequest;
        Request::State  mState;
        QByteArray      mUrl;
        HeadersHash     mHeaders;
        QByteArray      mBodyData;
        Response*       mResponse;
        Version         mVersion;
        quint16         mRemotePort;
        QHostAddress    mRemoteAddr;
        Method          mMethod;
    };
}

#endif
