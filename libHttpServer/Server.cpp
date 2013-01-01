
#include <QStringBuilder>

#include "libHttpServer/Internal/Server.hpp"
#include "libHttpServer/Internal/Connection.hpp"

namespace HTTP
{

    class AccessLogDebug : public IAccessLog
    {
    public:
        void access( int connectionId, Version version, Method method, const QUrl& url,
                     const QHostAddress& remoteAddr, quint16 remotePort, StatusCode response )
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
    };

    Server::Server( QObject* parent )
        : QObject( parent )
        , d( new Data )
    {
        d->mTcpServer = NULL;
        d->mAccessLog = NULL;
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
            Connection* c = new Connection( e->nextPendingConnection(), this, e );
            connect( c, SIGNAL(newRequest(HTTP::Request*)),
                     this, SIGNAL(newRequest(HTTP::Request*)),
                     Qt::DirectConnection );
        }
    }

    void Server::setAccessLog( IAccessLog* log )
    {
        d->mAccessLog = log;
    }

    IAccessLog* Server::accessLog() const
    {
        static AccessLogDebug dbg;
        return d->mAccessLog ? d->mAccessLog : &dbg;
    }

    QByteArray Server::methodName( Method method )
    {
        return method2text( method );
    }

}
