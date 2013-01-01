
#ifndef HTTP_SERVER_HPP
#define HTTP_SERVER_HPP

#include "libHttpServer/Http.hpp"
#include "libHttpServer/Request.hpp"
#include "libHttpServer/Response.hpp"

namespace HTTP
{

    class Request;
    class Connection;

    class IAccessLog
    {
    public:
        virtual void access( int connectionId, Version version, Method method, const QUrl& url,
                             const QHostAddress& remoteAddr, quint16 remotePort,
                             StatusCode response ) = 0;
    };

    class HTTP_SERVER_API Server : public QObject
    {
        Q_OBJECT
    public:
        Server( QObject* parent = 0 );
        ~Server();

    public:
        bool listen( const QHostAddress& addr, quint16 port = 0 );
        bool listen( quint16 port = 0 );
        void setAccessLog( IAccessLog* log );
        IAccessLog* accessLog() const;
        static QByteArray methodName( Method method );

    signals:
        void newRequest( HTTP::Request* request );

    private slots:
        void newConnection();

    private:
        class Data;
        Data* d;
    };

}

#endif
