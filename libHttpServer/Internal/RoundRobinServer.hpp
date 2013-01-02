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

#ifndef HTTP_ROUND_ROBIN_SERVER_HPP
#define HTTP_ROUND_ROBIN_SERVER_HPP

#include <QObject>
#include <QList>
#include <QTcpServer>
#include <QTcpSocket>
#include <QMutex>
#include <QThread>

namespace HTTP
{

    class ServerPrivate;

    class Establisher : public QObject
    {
        Q_OBJECT
    public:
        Establisher( ServerPrivate* server );

    public slots:
        void incommingConnection( int socketDescriptor );

    private:
        ServerPrivate*          mServer;
    };

    class RoundRobinServer : public QTcpServer
    {
        Q_OBJECT
    public:
        RoundRobinServer( QObject* parent, ServerPrivate* server );

    public:
        void addThread( QThread* thread );
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
        ServerPrivate*      mServer;
    };

}

#endif
