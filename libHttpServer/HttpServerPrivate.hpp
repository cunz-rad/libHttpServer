
#ifndef HTTP_SERVER_PRIVATE_HPP
#define HTTP_SERVER_PRIVATE_HPP

#include <QMutex>
#include <QTcpServer>

#include "http_parser.h"

#include "HttpServer.hpp"

namespace HTTP
{

    class Establisher : public QObject
    {
        Q_OBJECT
    public:
        Establisher( QObject* parent = 0 );

    public slots:
        void incommingConnection( int socketDescriptor );

    signals:
        void newConnection();

    public:
        bool hasPendingConnections() const;
        QTcpSocket* nextPendingConnection();

    private:
        mutable QMutex          mPendingMutex;
        QList< QTcpSocket* >    mPending;
    };

    class RoundRobinServer : public QTcpServer
    {
        Q_OBJECT
    public:
        RoundRobinServer( QObject* parent );

    public:
        Establisher* addThread( QThread* thread );
        void removeThread( QThread* thread );

    private slots:
        void threadEnded();

    protected:
        void incomingConnection( int socketDescriptor );

    private:
        struct ThreadInfo
        {
            QThread*        mThread;
            Establisher*    mEstablisher;
        };

        QMutex              mThreadsMutex;
        QList< ThreadInfo > mThreads;
        int                 mNextHandler;
    };

    class Connection : public QObject
    {
        Q_OBJECT
    public:
        Connection( QTcpSocket* socket, ServerBase* server, Establisher* parent );
        ~Connection();

    signals:
        void newRequest( HTTP::Request* request );

    private slots:
        void dataArrived();
        void socketLost();

    public:
        void write( const QByteArray& data );

    public:
        int id() const;
        ServerBase* server() const;

    private:
        int             mConnectionId;
        ServerBase*   mServer;
        QTcpSocket*     mSocket;
        http_parser*    mParser;
        HeaderName      mNextHeader;
        Request::Data*  mNextRequest;

    private:
        static int onMessageBegin( http_parser* parser );
        static int onUrl( http_parser *parser, const char* at, size_t length );
        static int onHeaderField( http_parser *parser, const char* at, size_t length );
        static int onHeaderValue( http_parser* parser, const char* at, size_t length );
        static int onHeadersComplete( http_parser* parser );
        static int onBody( http_parser* parser, const char* at, size_t length );
        static int onMessageComplete( http_parser* parser );

        static int sNextId;
        static const http_parser_settings sParserCallbacks;
    };

    class ServerBase::Data : private IAccessLog
    {
    public:
        RoundRobinServer*   mTcpServer;
        IAccessLog*         mAccessLog;

    private:
        void access( int connectionId, Version version, Method method, const QUrl& url,
                     const QHostAddress& remoteAddr, quint16 remotePort, StatusCode response );
    };

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

    class Response::Data
    {
    public:
        enum Flag
        {
            KeepAlive           = 1 << 0,
            Close               = 1 << 1,
            ChunkedEncoding     = 1 << 2,

            BodyIsFixed         = 1 << 27,
            DataIsCompressed    = 1 << 28,
            SendAtOnce          = 1 << 29,
            ReadyToSend         = 1 << 30,
            HeadersSent         = 1 << 31
        };
        typedef QFlags< Flag > Flags;

        Connection*     mConnection;
        Request*        mRequest;
        Flags           mFlags;
        HeadersHash     mHeaders;
        QByteArray      mBodyData;
    };

}

#endif
