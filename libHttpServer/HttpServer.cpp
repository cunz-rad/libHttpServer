
#include <QUrl>
#include <QThread>
#include <QTcpSocket>
#include <QStringBuilder>

#include "HttpServer.hpp"
#include "HttpServerPrivate.hpp"

#include "libHttpServer/Connection.hpp"

namespace HTTP
{

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



    void AccessLogDebug::access( int connectionId, Version version, Method method,
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
        static AccessLogDebug dbg;
        return d->mAccessLog ? d->mAccessLog : &dbg;
    }

    QByteArray ServerBase::methodName( Method method )
    {
        return method2text( method );
    }

}
