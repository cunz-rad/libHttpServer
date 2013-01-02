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

#include <QUrl>

#include "libHttpServer/Internal/Request.hpp"
#include "libHttpServer/Internal/Response.hpp"
#include "libHttpServer/Internal/Connection.hpp"

namespace HTTP
{

    Request::Data::Data()
    {
        mState = ReceivingHeaders;
        mRequest = NULL;
        mResponse = NULL;
    }

    void Request::Data::appendHeaderData( const HeaderName& header, const QByteArray& data )
    {
        if( !mHeaders.contains( header ) )
        {
            mHeaders.insert( header, data );
        }
        else
        {
            mHeaders[ header ].append( data );
        }
    }

    void Request::Data::appendBodyData( const QByteArray& data )
    {
        mBodyData.append( data );
    }

    void Request::Data::enterRecvBody()
    {
        mState = ReceivingBody;
    }

    void Request::Data::enterFinished()
    {
        mState = Finished;
        if( mResponse )
        {
            mResponse->d->mFlags |= Response::Data::ReadyToSend;
        }

        mRequest->completed( mRequest );
    }

    Request::Request( Connection* parent, Data* data )
        : QObject( parent )
        , d( data )
    {
        d->mRequest = this;
    }

    Request::~Request()
    {
        delete d;
    }

    Response* Request::response()
    {
        if( !d->mResponse )
        {
            Response::Data* rd = new Response::Data;
            rd->mRequest = this;
            rd->mConnection = qobject_cast< Connection* >( parent() );

            if( d->mVersion < V_1_1 )
            {
                rd->mFlags = Response::Data::Close;
                rd->mFlags |= Response::Data::SendAtOnce;
            } else
                rd->mFlags = Response::Data::KeepAlive;

            if( d->mState == Finished )
            {
                rd->mFlags |= Response::Data::ReadyToSend;
            }

            d->mResponse = new Response( rd );
        }
        return d->mResponse;
    }

    QByteArray Request::urlText() const
    {
        return d->mUrl;
    }

    QUrl Request::url() const
    {
        return QUrl::fromEncoded( d->mUrl );
    }

    bool Request::hasHeader( const HeaderName& header ) const
    {
        return d->mHeaders.contains( header );
    }

    HeaderValue Request::header( const HeaderName& header ) const
    {
        return d->mHeaders.value( header, QByteArray() );
    }

    HeadersHash Request::allHeaders() const
    {
        return d->mHeaders;
    }

    bool Request::hasBodyData() const
    {
        return !d->mBodyData.isEmpty();
    }

    QByteArray Request::bodyData() const
    {
        return d->mBodyData;
    }

    Request::State Request::state() const
    {
        return d->mState;
    }

    Version Request::httpVersion() const
    {
        return d->mVersion;
    }

    Method Request::method() const
    {
        return d->mMethod;
    }

    QHostAddress Request::remoteAddr() const
    {
        return d->mRemoteAddr;
    }

    quint16 Request::remotePort() const
    {
        return d->mRemotePort;
    }

}
