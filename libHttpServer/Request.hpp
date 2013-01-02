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

#ifndef HTTP_REQUEST_HPP
#define HTTP_REQUEST_HPP

#include <QUrl>
#include <QHostAddress>

#include "libHttpServer/Http.hpp"

namespace HTTP
{

    class Connection;
    class Response;

    class HTTP_SERVER_API Request : public QObject
    {
        Q_OBJECT
    public:
        enum State
        {
            ReceivingHeaders,
            ReceivingBody,
            Finished
        };

    public:
        ~Request();

    public:
        Response* response();

    public:
        QByteArray urlText() const;
        QUrl url() const;

        bool hasHeader( const HeaderName& header ) const;
        HeaderValue header( const HeaderName& header ) const;
        HeadersHash allHeaders() const;

        bool hasBodyData() const;
        QByteArray bodyData() const;
        State state() const;

        Version httpVersion() const;
        Method method() const;

        QHostAddress remoteAddr() const;
        quint16 remotePort() const;

    signals:
        void completed( HTTP::Request* request );

    private:
        friend class Connection;
        class Data;
        Request( Connection* parent, Data* data );
        Data* d;
    };

}

#endif
