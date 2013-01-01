
#include <QUrl>
#include <QThread>
#include <QTcpSocket>
#include <QStringBuilder>

#include "HttpServer.hpp"
#include "HttpServerPrivate.hpp"

#define CRLF "\r\n"

#define HTTP_DO_DBG if( 1 ) qDebug
#define HTTP_NO_DBG if( 0 ) qDebug

#define HTTP_PARSER_DBG HTTP_NO_DBG

#define HTTP_DBG HTTP_NO_DBG

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

    static inline Method method( http_method m )
    {
        switch( m )
        {
        case HTTP_DELETE:       return Delete;
        case HTTP_GET:          return Get;
        case HTTP_HEAD:         return Head;
        case HTTP_POST:         return Post;
        case HTTP_PUT:          return Put;
        case HTTP_CONNECT:      return Connect;
        case HTTP_OPTIONS:      return Options;
        case HTTP_TRACE:        return Trace;
        case HTTP_COPY:         return Copy;
        case HTTP_LOCK:         return Lock;
        case HTTP_MKCOL:        return MkCol;
        case HTTP_MOVE:         return Move;
        case HTTP_PROPFIND:     return PropFind;
        case HTTP_PROPPATCH:    return PropPatch;
        case HTTP_SEARCH:       return Search;
        case HTTP_UNLOCK:       return Unlock;
        case HTTP_REPORT:       return Report;
        case HTTP_MKACTIVITY:   return MkActivity;
        case HTTP_CHECKOUT:     return Checkout;
        case HTTP_MERGE:        return Merge;
        case HTTP_MSEARCH:      return MSearch;
        case HTTP_NOTIFY:       return Notify;
        case HTTP_SUBSCRIBE:    return Subscribe;
        case HTTP_UNSUBSCRIBE:  return Unsubscribe;
        case HTTP_PATCH:        return Patch;
        case HTTP_PURGE:        return Purge;
        default:                return Method(-1);
        }
    }

    static inline const char* const method2text( Method m )
    {
        switch( m )
        {
        case Delete:        return "DELETE";
        case Get:           return "GET";
        case Head:          return "HEAD";
        case Post:          return "POST";
        case Put:           return "PUT";
        case Connect:       return "CONNECT";
        case Options:       return "OPTIONS";
        case Trace:         return "TRACE";
        case Copy:          return "COPY";
        case Lock:          return "LOCK";
        case MkCol:         return "MKCOL";
        case Move:          return "MOVE";
        case PropFind:      return "PROPFIND";
        case PropPatch:     return "PROPPATCH";
        case Search:        return "SEARCH";
        case Unlock:        return "UNLOCK";
        case Report:        return "REPORT";
        case MkActivity:    return "MKACTIVITY";
        case Checkout:      return "CHECKOUT";
        case Merge:         return "MERGE";
        case MSearch:       return "MSEARCH";
        case Notify:        return "NOTIFY";
        case Subscribe:     return "SUBSCRIBE";
        case Unsubscribe:   return "UNSUBSCRIBE";
        case Patch:         return "PATCH";
        case Purge:         return "PURGE";
        default: return NULL;
        }
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

    Connection::Connection( QTcpSocket* socket, ServerBase* server, Establisher* parent )
        : QObject( parent )
        , mConnectionId( sNextId++ )
        , mServer( server )
        , mSocket( socket )
        , mNextRequest( NULL )
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

    int Connection::id() const
    {
        return mConnectionId;
    }

    ServerBase* Connection::server() const
    {
        return mServer;
    }

    int Connection::onMessageBegin( http_parser* parser )
    {
        Connection* that = static_cast< Connection* >( parser->data );
        Q_ASSERT( that );

        HTTP_PARSER_DBG( "PARSER: Message Begin" );

        Q_ASSERT( !that->mNextRequest );
        that->mNextRequest = new Request::Data;

        return 0;
    }

    int Connection::onUrl( http_parser *parser, const char* at, size_t length )
    {
        Connection* that = static_cast< Connection* >( parser->data );
        Q_ASSERT( that );

        HTTP_PARSER_DBG( "PARSER: URL" );

        Q_ASSERT( that->mNextRequest );
        that->mNextRequest->mUrl += QByteArray( at, length );

        return 0;
    }

    int Connection::onHeaderField( http_parser *parser, const char* at, size_t length )
    {
        Connection* that = static_cast< Connection* >( parser->data );
        Q_ASSERT( that );

        HTTP_PARSER_DBG( "PARSER: Header Field" );

        that->mNextHeader = HeaderName( at, length );

        return 0;
    }

    int Connection::onHeaderValue( http_parser* parser, const char* at, size_t length )
    {
        Connection* that = static_cast< Connection* >( parser->data );
        Q_ASSERT( that );

        HTTP_PARSER_DBG( "PARSER: Header Value" );

        Q_ASSERT( that->mNextRequest );
        Q_ASSERT( that->mNextRequest->mState == Request::ReceivingHeaders );

        QByteArray data( at, length );

        HTTP_PARSER_DBG( "Appending '%s' to header %s",
                         data.constData(),
                         that->mNextHeader.constData() );

        that->mNextRequest->appendHeaderData( that->mNextHeader, data );

        return 0;
    }

    int Connection::onHeadersComplete( http_parser* parser )
    {
        Connection* that = static_cast< Connection* >( parser->data );
        Q_ASSERT( that );

        HTTP_PARSER_DBG( "PARSER: Headers Complete" );

        Q_ASSERT( that->mNextRequest );

        Request::Data* req = that->mNextRequest;

        Q_ASSERT( req->mState == Request::ReceivingHeaders );

        req->mRemoteAddr = that->mSocket->peerAddress();
        req->mRemotePort = that->mSocket->peerPort();

        if( parser->http_major == 1 && parser->http_minor == 0 )
            req->mVersion = V_1_0;
        else if( parser->http_major == 1 && parser->http_minor == 1 )
            req->mVersion = V_1_1;
        else
            req->mVersion = V_Unknown;

        req->mMethod = method( (enum http_method) parser->method );

        that->mNextHeader = HeaderName();

        req->enterRecvBody();
        that->newRequest( new Request( that, req ) );

        return 0;
    }

    int Connection::onBody( http_parser* parser, const char* at, size_t length )
    {
        Connection* that = static_cast< Connection* >( parser->data );
        Q_ASSERT( that );

        HTTP_PARSER_DBG( "PARSER: Body" );

        Q_ASSERT( that->mNextRequest );
        Q_ASSERT( that->mNextRequest );

        Request::Data* req = that->mNextRequest;
        Q_ASSERT( req->mState == Request::ReceivingBody );

        QByteArray data( at, length );

        req->appendBodyData( data );

        return 0;
    }

    int Connection::onMessageComplete( http_parser* parser )
    {
        Connection* that = static_cast< Connection* >( parser->data );
        Q_ASSERT( that );

        HTTP_PARSER_DBG( "PARSER: Message Complete" );

        Q_ASSERT( that->mNextRequest );
        Q_ASSERT( that->mNextRequest->mState == Request::ReceivingBody );

        that->mNextRequest->enterFinished();
        that->mNextRequest = NULL;

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

    int Connection::sNextId = 1;

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

        ServerBase* svr = d->mConnection->server();
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

    void ServerBase::Data::access( int connectionId, Version version, Method method,
                                     const QUrl& url, const QHostAddress& remoteAddr,
                                     quint16 remotePort, StatusCode response )
    {
        qDebug( "%i: HTTP/%s %s %s %s %i %i",
                connectionId,
                ((version == V_1_0) ? "1.0" : "1.1"),
                method2text(method),
                qPrintable( url.toString() ),
                qPrintable( remoteAddr.toString() ),
                remotePort,
                response );
    }

    ServerBase::ServerBase( QObject* parent )
        : QObject( parent )
        , d( new Data )
    {
        d->mTcpServer = NULL;
        d->mAccessLog = NULL;
    }

    ServerBase::~ServerBase()
    {
        delete d;
    }

    bool ServerBase::listen( quint16 port )
    {
        return listen( QHostAddress::Any, port );
    }

    bool ServerBase::listen( const QHostAddress& addr, quint16 port )
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

    void ServerBase::newConnection()
    {
        Establisher* e = qobject_cast< Establisher* >( sender() );
        if( !e )
        {
            return;
        }

        while( e->hasPendingConnections() )
        {
            Connection* c = new Connection( e->nextPendingConnection(), this, e );
            connect( c, SIGNAL(newRequest(HTTP::Request*)),
                     this, SIGNAL(newRequest(HTTP::Request*)),
                     Qt::DirectConnection );
        }
    }

    void ServerBase::setAccessLog( IAccessLog* log )
    {
        d->mAccessLog = log;
    }

    IAccessLog* ServerBase::accessLog() const
    {
        return d->mAccessLog ? d->mAccessLog : (IAccessLog*)d;
    }

    QByteArray ServerBase::methodName( Method method )
    {
        return method2text( method );
    }

}
