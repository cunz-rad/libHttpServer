
#include <QThread>
#include <QTcpSocket>

#include "libHttpServer/Internal/Connection.hpp"
#include "libHttpServer/Internal/Request.hpp"
#include "libHttpServer/Internal/http_parser.h"

namespace HTTP
{

    const char* const code2Text( int code )
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

    Method method( unsigned char m )
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

    const char* const method2text( Method m )
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

        req->mMethod = method( parser->method );

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
}
