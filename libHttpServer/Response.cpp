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

#include <QStringBuilder>

#include "libHttpServer/Internal/Http.hpp"
#include "libHttpServer/Internal/Response.hpp"
#include "libHttpServer/Internal/Connection.hpp"
#include "libHttpServer/Internal/Server.hpp"

namespace HTTP
{

    static const char* const wkdays[] = { "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun" };
    static const char* const mths[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

    QByteArray toRfc1123date( const QDateTime& dt )
    {
        char date[ 30 ] = "xxx, 00 123 0000 11:11:11 GMT";

        QDate d = dt.toUTC().date();
        QTime t = dt.toUTC().time();

        sprintf( date, "%s, %.2d %s %.4d %.2d:%.2d:%.2d GMT",
                 wkdays[ d.dayOfWeek() - 1 ],
                d.day(), mths[ d.month() - 1 ], d.year(), t.hour(), t.minute(), t.second() );

        return QByteArray( date );
    }

    QDateTime fromRfc1123date( const QByteArray& data )
    {
        if( data.count() != 29 )
        {
            return QDateTime();
        }

        const char* d = data.constData();

        #define num(x) (((int)x)-0x30)

        int day = num(d[5]) * 10 + num(d[6]);
        int year = num(d[12]) * 1000 +
                num(d[13]) * 100 +
                num(d[14]) * 10 +
                num(d[15]);
        int month = 0;
        for( int i = 0; i < 12; i++ )
        {
            if( !strncasecmp( d + 8, mths[i], 3 ) )
                month = i + 1;
        }

        int hour = num(d[17]) * 10 + num(d[18]);
        int min  = num(d[20]) * 10 + num(d[21]);
        int sec  = num(d[23]) * 10 + num(d[24]);
        #undef num

        QDateTime dt( QDate( year, month, day ), QTime( hour, min, sec ), Qt::UTC );
        return dt.toLocalTime();
    }

    Response::Response( Data* data )
        : d( data )
    {
    }

    Response::~Response()
    {
        delete d;
    }

    void Response::addHeader( const HeaderName& header, const HeaderValue& value )
    {
        d->mHeaders[ header ] = value;
    }

    void Response::addHeader( const HeaderName& header, const QDateTime& value )
    {
        addHeader( header, toRfc1123date( value ) );
    }

    bool Response::headersSent() const
    {
        return d->mFlags.testFlag( Data::HeadersWritten );
    }

    bool Response::hasHeader( const HeaderName& name ) const
    {
        return d->mHeaders.contains( name.toLower() );
    }

    void Response::addBody( const QByteArray& data )
    {
        Q_ASSERT( !d->mFlags.testFlag( Data::BodyIsFixed ) );
        Q_ASSERT( !d->mFlags.testFlag( Data::DataIsCompressed ) );
        Q_ASSERT( !headersSent() || d->mFlags.testFlag( Data::ChunkedEncoding ) ||
                  !d->mFlags.testFlag( Data::KeepAlive ) );

        d->mBodyData.append( data );
    }

    void Response::sendHeaders( StatusCode code )
    {
        Q_ASSERT( !headersSent() );
        Q_ASSERT( !d->mFlags.testFlag( Data::SendAtOnce ) );
        //Q_ASSERT( d->mFlags.testFlag( Data::ReadyToSend ) );
        Q_ASSERT( d->mFlags.testFlag( Data::ChunkedEncoding ) || hasHeader( "Content-Length" ) ||
                  !d->mFlags.testFlag( Data::KeepAlive ) || d->mFlags.testFlag( Data::BodyIsFixed ) );

        if( !d->mFlags.testFlag( Data::ChunkedEncoding ) && !hasHeader( "Content-Length" ) &&
                d->mFlags.testFlag( Data::BodyIsFixed ) )
        {
            addHeader( "Content-Length", QByteArray::number( d->mBodyData.count() ) );
        }

        bool sentKeepAlive = false;
        bool sentContentLength = false;

        QByteArray out = "HTTP/1.1 " % QByteArray::number( code ) % " " % code2Text( code ) % CRLF;
        foreach( HeaderName name, d->mHeaders.keys() )
        {
            const QByteArray& value = d->mHeaders.value( name );

            if( name.toLower() == "connection" )
            {
                if( value.toLower() == "keep-alive" )
                {
                    Q_ASSERT( !d->mFlags.testFlag( Data::Close ) );
                    d->mFlags = d->mFlags | Data::KeepAlive;
                    sentKeepAlive = true;
                }
                else if( value.toLower() == "close" )
                {
                    Q_ASSERT( !d->mFlags.testFlag( Data::KeepAlive ) );
                    d->mFlags = d->mFlags | Data::Close;
                    sentKeepAlive = true;
                }
                out += name % ": " % value % CRLF;
            }
            else if( name.toLower() == "content-length" )
            {
                sentContentLength = true;
                out += name % ": " % value % CRLF;
            }
            else
            {
                out += name % ": " % value % CRLF;
            }
        }

        if( !d->mFlags.testFlag( Data::ChunkedEncoding ) )
        {
            if( !sentKeepAlive && d->mFlags.testFlag( Data::Close ) )
            {
                out += "Connection: Close" CRLF;
            }

            if( !sentContentLength )
            {
                out += "Content-Length: " % QByteArray::number( d->mBodyData.count() ) % CRLF;
            }
        }

        out += CRLF;

        HTTP_DBG( "Out: %s", out.constData() );

        d->mConnection->write(out);
        d->mFlags |= Data::HeadersWritten;

        Server* svr = d->mConnection->server();
        IAccessLog* log = svr->accessLog();
        if( log )
        {
            log->access( d->mConnection->id(),
                         d->mRequest->httpVersion(),
                         d->mRequest->method(),
                         d->mRequest->url(),
                         d->mRequest->remoteAddr(),
                         d->mRequest->remotePort(),
                         code );
        }
    }

    void Response::fixBody()
    {
        d->mFlags |= Data::BodyIsFixed;
    }

    void Response::send( StatusCode code )
    {
        Q_ASSERT( !headersSent() );
        //Q_ASSERT( d->mFlags.testFlag( Data::ReadyToSend ) );

        sendHeaders( code );
        d->mFlags &= ~Data::SendAtOnce;
        send();
    }

    void Response::finish( StatusCode code )
    {
        fixBody();
        send( code );
    }

    void Response::send()
    {
        Q_ASSERT( headersSent() );
        Q_ASSERT( !d->mFlags.testFlag( Data::SendAtOnce ) );
        //Q_ASSERT( d->mFlags.testFlag( Data::ReadyToSend ) );

        if( d->mFlags.testFlag( Data::ChunkedEncoding ) )
        {
        }
        else
        {
            d->mConnection->write( d->mBodyData );
        }
        deleteLater();
    }

}
