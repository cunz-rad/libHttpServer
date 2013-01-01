
#ifndef HTTP_CONNECTION_HPP
#define HTTP_CONNECTION_HPP

#include <QObject>
#include <QMutex>
#include <QTcpServer>

#include "libHttpServer/Http.hpp"
#include "libHttpServer/Request.hpp"

#define CRLF "\r\n"

#define HTTP_DO_DBG if( 1 ) qDebug
#define HTTP_NO_DBG if( 0 ) qDebug

#define HTTP_PARSER_DBG HTTP_NO_DBG

#define HTTP_DBG HTTP_NO_DBG

extern "C" {
    struct http_parser;
    struct http_parser_settings;
}

class QTcpSocket;

namespace HTTP
{

    class ServerBase;

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
        ServerBase*     mServer;
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

    const char* const code2Text( int code );
    Method method( unsigned char method );
    const char* const method2text( Method m );

}

#endif
