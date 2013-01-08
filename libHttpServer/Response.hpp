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

#ifndef HTTP_RESPONSE_HPP
#define HTTP_RESPONSE_HPP

#include <QDateTime>

#include "libHttpServer/Http.hpp"

namespace HTTP
{

    class HTTP_SERVER_API Response : public QObject
    {
        Q_OBJECT
    public:
        ~Response();

    public:
        void addHeader( const HeaderName& header, const HeaderValue& value );
        void addHeader( const HeaderName& header, const QDateTime& value );
        bool hasHeader( const HeaderName& name ) const;
        bool headersSent() const;

        void addBody( const QByteArray& data );
        void fixBody();

        void sendHeaders( StatusCode code );
        void send( StatusCode code );
        void finish( StatusCode code );
        void send();

    private:
        friend class Request;
        class Data;
        Response( Data* d );
        Data* d;
    };

}

#endif
