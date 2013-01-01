
#ifndef HTTP_SERVER_PRIVATE_HPP
#define HTTP_SERVER_PRIVATE_HPP

#include <QMutex>
#include <QTcpServer>

#include "HttpServer.hpp"

namespace HTTP
{
    class RoundRobinServer;

    class ServerBase::Data : private IAccessLog
    {
    public:
        RoundRobinServer*   mTcpServer;
        IAccessLog*         mAccessLog;

    private:
        void access( int connectionId, Version version, Method method, const QUrl& url,
                     const QHostAddress& remoteAddr, quint16 remotePort, StatusCode response );
    };


}

#endif
