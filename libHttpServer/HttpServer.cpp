
#include <QUrl>
#include <QThread>
#include <QTcpSocket>
#include <QStringBuilder>

#include "HttpServer.hpp"
#include "HttpServerPrivate.hpp"

#define CRLF "\r\n"

#define HTTP_DBG if( 1 ) qDebug
#define HTTP_NO_DBG if( 0 ) qDebug

#define HTTP_PARSER_DBG HTTP_NO_DBG

namespace HTTP
{
    static inline const char* const code2Text( int code )
    {
        switch( code )
        {
            #define SC(x,t) case x: return t;
            SC(100, "Continue")
            SC(101, "Switching Protocols")
            SC(102, "Processing")                 // RFC 2518) obsoleted by RFC 4918
            SC(200, "OK")
            SC(201, "Created")
            SC(202, "Accepted")
            SC(203, "Non-Authoritative Information")
            SC(204, "No Content")
            SC(205, "Reset Content")
            SC(206, "Partial Content")
            SC(207, "Multi-Status")               // RFC 4918
            SC(300, "Multiple Choices")
            SC(301, "Moved Permanently")
            SC(302, "Moved Temporarily")
            SC(303, "See Other")
            SC(304, "Not Modified")
            SC(305, "Use Proxy")
            SC(307, "Temporary Redirect")
            SC(400, "Bad Request")
            SC(401, "Unauthorized")
            SC(402, "Payment Required")
            SC(403, "Forbidden")
            SC(404, "Not Found")
            SC(405, "Method Not Allowed")
            SC(406, "Not Acceptable")
            SC(407, "Proxy Authentication Required")
            SC(408, "Request Time-out")
            SC(409, "Conflict")
            SC(410, "Gone")
            SC(411, "Length Required")
            SC(412, "Precondition Failed")
            SC(413, "Request Entity Too Large")
            SC(414, "Request-URI Too Large")
            SC(415, "Unsupported Media Type")
            SC(416, "Requested Range Not Satisfiable")
            SC(417, "Expectation Failed")
            SC(418, "I\"m a teapot")              // RFC 2324
            SC(422, "Unprocessable Entity")       // RFC 4918
            SC(423, "Locked")                     // RFC 4918
            SC(424, "Failed Dependency")          // RFC 4918
            SC(425, "Unordered Collection")       // RFC 4918
            SC(426, "Upgrade Required")           // RFC 2817
            SC(500, "Internal Server Error")
            SC(501, "Not Implemented")
            SC(502, "Bad Gateway")
            SC(503, "Service Unavailable")
            SC(504, "Gateway Time-out")
            SC(505, "HTTP Version not supported")
            SC(506, "Variant Also Negotiates")    // RFC 2295
            SC(507, "Insufficient Storage")       // RFC 4918
            SC(509, "Bandwidth Limit Exceeded")
            SC(510, "Not Extended")                // RFC 2774
        #undef SC
        }
        return NULL;
    }

    Establisher::Establisher( QObject* parent )
        : QObject( parent )
    {
    }

    void Establisher::incommingConnection( int socketDescriptor )
    {
        QMutexLocker l( &mPendingMutex );
        QTcpSocket* sock = new QTcpSocket( this );
        sock->setSocketDescriptor( socketDescriptor );
        mPending.append( sock );
        l.unlock();

        HTTP_DBG( "incomming connection; Sock=%i; Thread=%p; TcpSocket=%p",
                  socketDescriptor, QThread::currentThread(), sock );

        emit newConnection();
    }

    bool Establisher::hasPendingConnections() const
    {
        QMutexLocker l( &mPendingMutex );

        return !mPending.isEmpty();
    }

    QTcpSocket* Establisher::nextPendingConnection()
    {
        QMutexLocker l( &mPendingMutex );

        if( !mPending.isEmpty() )
        {
            return mPending.takeFirst();
        }

        return NULL;
    }

    RoundRobinServer::RoundRobinServer( QObject* parent )
        : QTcpServer( parent )
    {
    }

    Establisher* RoundRobinServer::addThread( QThread* thread )
    {
        QMutexLocker l( &mThreadsMutex );

        Establisher* e = new Establisher;
        e->moveToThread( thread );

        ThreadInfo ti;
        ti.mEstablisher = e;
        ti.mThread = thread;
        mThreads.append( ti );

        return e;
    }

    void RoundRobinServer::removeThread( QThread* thread )
    {
        QMutexLocker l( &mThreadsMutex );

        for( int i = 0; i < mThreads.count(); i++ )
        {
            if( mThreads[ i ].mThread == thread )
            {
                ThreadInfo ti = mThreads.takeAt( i );
                ti.mEstablisher->disconnect();
                ti.mEstablisher->deleteLater();

                if( mNextHandler > i )
                {
                    #if 0   // seems unneccessary
                    if( mNextHandler == mThreads.count() )
                        mNextHandler = 0;
                    else
                    #endif
                    mNextHandler--;
                }
                return;
            }
        }
    }

    void RoundRobinServer::threadEnded()
    {
        QThread* t = qobject_cast< QThread* >( sender() );
        if( t )
        {
            removeThread( t );
        }
    }

    void RoundRobinServer::incomingConnection( int socketDescriptor )
    {
        QMutexLocker l( &mThreadsMutex );

        if( mThreads.isEmpty() )
        {
            Q_ASSERT_X( !mThreads.isEmpty(), "RoundRobinServer", "No threads available" );
            return;
        }

        Establisher* e = mThreads[ mNextHandler++ ].mEstablisher;
        QMetaObject::invokeMethod( e, "incommingConnection",
                                   Qt::QueuedConnection, Q_ARG( int, socketDescriptor ) );

        if( mNextHandler == mThreads.count() )
        {
            mNextHandler = 0;
        }
    }

    Connection::Connection( QTcpSocket* socket, QObject* parent )
        : QObject( parent )
        , mSocket( socket )
        , mCurrentRequest( NULL )
    {
        mParser = (http_parser*) malloc( sizeof( http_parser ) );
        http_parser_init( mParser, HTTP_REQUEST );
        mParser->data = this;

        connect( socket, SIGNAL(readyRead()), this, SLOT(dataArrived()) );
        connect( socket, SIGNAL(disconnected()), this, SLOT(socketLost()) );

        HTTP_DBG( "New connection %p; Thread=%p", this, QThread::currentThread() );
    }

    Connection::~Connection()
    {
        HTTP_DBG( "Deleted connection %p; Thread=%p", this, QThread::currentThread() );

        delete mSocket;
        mSocket = NULL;

        free( mParser );
        mParser = NULL;
    }

    void Connection::dataArrived()
    {
        Q_ASSERT( mSocket );
        Q_ASSERT( mParser );

        while( mSocket->bytesAvailable() )
        {
            QByteArray arr = mSocket->readAll();
            HTTP_DBG( "RECV: %s", arr.constData() );
            http_parser_execute(mParser, &sParserCallbacks, arr.constData(), arr.size());
        }
    }

    void Connection::socketLost()
    {
        HTTP_DBG( "Connection %p lost its socket Thread=%p", this, QThread::currentThread() );
        deleteLater();
    }

    void Connection::write( const QByteArray& data )
    {
        mSocket->write( data.constData(), data.length() );
    }

    int Connection::onMessageBegin( http_parser* parser )
    {
        Connection* that = static_cast< Connection* >( parser->data );
        Q_ASSERT( that );

        HTTP_PARSER_DBG( "PARSER: Message Begin" );

        Q_ASSERT( !that->mCurrentRequest );
        that->mCurrentRequest = new Request::Data;

        return 0;
    }

    int Connection::onUrl( http_parser *parser, const char* at, size_t length )
    {
        Connection* that = static_cast< Connection* >( parser->data );
        Q_ASSERT( that );

        HTTP_PARSER_DBG( "PARSER: URL" );

        Q_ASSERT( that->mCurrentRequest );
        that->mCurrentRequest->mUrl = QByteArray( at, length );

        return 0;
    }

    int Connection::onHeaderField( http_parser *parser, const char* at, size_t length )
    {
        Connection* that = static_cast< Connection* >( parser->data );
        Q_ASSERT( that );

        HTTP_PARSER_DBG( "PARSER: Header Field" );

        that->mCurrentHeader = HeaderName( at, length );

        return 0;
    }

    int Connection::onHeaderValue( http_parser* parser, const char* at, size_t length )
    {
        Connection* that = static_cast< Connection* >( parser->data );
        Q_ASSERT( that );

        HTTP_PARSER_DBG( "PARSER: Header Value" );

        Q_ASSERT( that->mCurrentRequest );
        Q_ASSERT( that->mCurrentRequest->mState == Request::ReceivingHeaders );

        QByteArray data( at, length );

        HTTP_PARSER_DBG( "Appending '%s' to header %s",
                         data.constData(),
                         that->mCurrentHeader.constData() );

        that->mCurrentRequest->appendHeaderData( that->mCurrentHeader, data );

        return 0;
    }

    int Connection::onHeadersComplete( http_parser* parser )
    {
        Connection* that = static_cast< Connection* >( parser->data );
        Q_ASSERT( that );

        HTTP_PARSER_DBG( "PARSER: Headers Complete" );

        Q_ASSERT( that->mCurrentRequest );

        Request::Data* req = that->mCurrentRequest;

        Q_ASSERT( req->mState == Request::ReceivingHeaders );

        req->mRemoteAddr = that->mSocket->peerAddress();
        req->mRemotePort = that->mSocket->peerPort();

        if( parser->http_major == 1 && parser->http_minor == 0 )
            req->mVersion = V_1_0;
        else if( parser->http_major == 1 && parser->http_minor == 1 )
            req->mVersion = V_1_1;
        else
            req->mVersion = V_Unknown;

        that->mCurrentHeader = HeaderName();
        req->enterRecvBody();

        that->newRequest( new Request( that, req ) );

        return 0;
    }

    int Connection::onBody( http_parser* parser, const char* at, size_t length )
    {
        Connection* that = static_cast< Connection* >( parser->data );
        Q_ASSERT( that );

        HTTP_PARSER_DBG( "PARSER: Body" );

        Q_ASSERT( that->mCurrentRequest );
        Q_ASSERT( that->mCurrentRequest->mState == Request::ReceivingBody );

        return 0;
    }

    int Connection::onMessageComplete( http_parser* parser )
    {
        Connection* that = static_cast< Connection* >( parser->data );
        Q_ASSERT( that );

        HTTP_PARSER_DBG( "PARSER: Message Complete" );

        Q_ASSERT( that->mCurrentRequest );
        Q_ASSERT( that->mCurrentRequest->mState == Request::ReceivingBody );

        that->mCurrentRequest->enterFinished();
        that->mCurrentRequest = NULL;

        return 0;
    }

    const http_parser_settings Connection::sParserCallbacks = {
        &Connection::onMessageBegin,
        &Connection::onUrl,
        &Connection::onHeaderField,
        &Connection::onHeaderValue,
        &Connection::onHeadersComplete,
        &Connection::onBody,
        &Connection::onMessageComplete
    };

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

    Request::State Request::state() const
    {
        return d->mState;
    }

    Version Request::httpVersion() const
    {
        return d->mVersion;
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

    Server::Server( QObject* parent )
        : QObject( parent )
        , d( new Data )
    {
    }

    Server::~Server()
    {
        delete d;
    }

    bool Server::listen( quint16 port )
    {
        return listen( QHostAddress::Any, port );
    }

    bool Server::listen( const QHostAddress& addr, quint16 port )
    {
        if( d->mTcpServer )
        {
            return false;
        }

        RoundRobinServer* tcpServer = new RoundRobinServer( this );

        Establisher* e = tcpServer->addThread( QThread::currentThread() );

        connect( e, SIGNAL(newConnection()), this, SLOT(newConnection()), Qt::DirectConnection );

        if( tcpServer->listen( addr, port ) )
        {
            d->mTcpServer = tcpServer;
            return true;
        }

        return false;
    }

    void Server::newConnection()
    {
        Establisher* e = qobject_cast< Establisher* >( sender() );
        if( !e )
        {
            return;
        }

        while( e->hasPendingConnections() )
        {
            Connection* c = new Connection( e->nextPendingConnection(), e );
            connect( c, SIGNAL(newRequest(HTTP::Request*)),
                     this, SIGNAL(newRequest(HTTP::Request*)),
                     Qt::DirectConnection );
        }
    }

}
