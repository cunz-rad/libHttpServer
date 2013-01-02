/*
 * Modern CI
 * Copyright (C) 2012-2013 Sascha Cunz <sascha@babbelbox.org>
 *
 * This program is free software; you can redistribute it and/or modify it under the terms of the
 * GNU General Public License (Version 2) as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
 * even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with this program; if
 * not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "libHttpServer/Internal/Http.hpp"
#include "libHttpServer/Internal/RoundRobinServer.hpp"
#include "libHttpServer/Internal/Connection.hpp"

namespace HTTP
{

    Establisher::Establisher( ServerPrivate* server )
        : QObject( NULL )
        , mServer( server )
    {
    }

    void Establisher::incommingConnection( int socketDescriptor )
    {
        QTcpSocket* sock = new QTcpSocket( this );
        sock->setSocketDescriptor( socketDescriptor );

        HTTP_DBG( "incomming connection; Sock=%i; Thread=%p; TcpSocket=%p",
                  socketDescriptor, QThread::currentThread(), sock );

        new Connection( sock, mServer, this );
    }

    RoundRobinServer::RoundRobinServer( QObject* parent, ServerPrivate* server )
        : QTcpServer( parent )
        , mNextHandler( 0 )
        , mServer( server )
    {
    }

    void RoundRobinServer::addThread( QThread* thread )
    {
        QMutexLocker l( &mThreadsMutex );

        Establisher* e = new Establisher( mServer );
        e->moveToThread( thread );

        ThreadInfo ti;
        ti.mEstablisher = e;
        ti.mThread = thread;
        mThreads.append( ti );
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
