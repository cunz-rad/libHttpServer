
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

}

#endif
