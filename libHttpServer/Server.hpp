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

#ifndef HTTP_SERVER_HPP
#define HTTP_SERVER_HPP

#include "libHttpServer/Http.hpp"
#include "libHttpServer/Request.hpp"
#include "libHttpServer/Response.hpp"

namespace HTTP
{

    class Request;
    class Connection;

    class IAccessLog
    {
    public:
        virtual void access( int connectionId, Version version, Method method, const QUrl& url,
                             const QHostAddress& remoteAddr, quint16 remotePort,
                             StatusCode response ) = 0;
    };

    class HTTP_SERVER_API Server : public QObject
    {
        Q_OBJECT
    public:
        Server( QObject* parent = 0 );
        ~Server();

    public:
        bool listen( const QHostAddress& addr, quint16 port = 0 );
        bool listen( quint16 port = 0 );
        void setAccessLog( IAccessLog* log );
        IAccessLog* accessLog() const;
        static QByteArray methodName( Method method );

    signals:
        void newRequest( HTTP::Request* request );

    private slots:
        void newConnection();

    private:
        class Data;
        Data* d;
    };

}

#endif
