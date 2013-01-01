
#ifndef HTTP_CONNECTION_HPP
#define HTTP_CONNECTION_HPP

#include <QObject>

#include "libHttpServer/Http.hpp"
#include "libHttpServer/Request.hpp"

#include "libHttpServer/Internal/http_parser.h"

class QTcpSocket;

namespace HTTP
{

    class Establisher;
    class Server;

    class Connection : public QObject
    {
        Q_OBJECT
    public:
        Connection( QTcpSocket* socket, Server* server, Establisher* parent );
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
        Server* server() const;

    private:
        int             mConnectionId;
        Server*     mServer;
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
