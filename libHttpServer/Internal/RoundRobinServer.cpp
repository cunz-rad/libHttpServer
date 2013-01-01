
#include "libHttpServer/Internal/Http.hpp"
#include "libHttpServer/Internal/RoundRobinServer.hpp"

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


}
