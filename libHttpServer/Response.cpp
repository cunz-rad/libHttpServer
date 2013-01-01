
#include <QStringBuilder>

#include "libHttpServer/Internal/Http.hpp"
#include "libHttpServer/Internal/Response.hpp"
#include "libHttpServer/Internal/Connection.hpp"
#include "libHttpServer/Internal/Server.hpp"

namespace HTTP
{

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

    bool Response::headersSent() const
    {
        return d->mFlags.testFlag( Data::HeadersSent );
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

        d->mConnection->write( out );
        d->mFlags |= Data::HeadersSent;

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
    }

}
