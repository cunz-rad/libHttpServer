
#ifndef HTTP_SERVER_HPP
#define HTTP_SERVER_HPP

#include "libHttpServer/Http.hpp"

class QHostAddress;

namespace HTTP
{

    class Request;
    class Connection;

    class IAccessLog
    {
    public:
        virtual ~IAccessLog() {}

    public:
        virtual void access( int connectionId, Version version, Method method, const QUrl& url,
                             const QHostAddress& remoteAddr, quint16 remotePort,
                             StatusCode response ) = 0;
    };

    class HTTP_SERVER_API ServerBase : public QObject
    {
        Q_OBJECT
    public:
        ServerBase( QObject* parent = 0 );
        ~ServerBase();

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
