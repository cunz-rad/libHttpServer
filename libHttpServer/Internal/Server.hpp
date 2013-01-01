
#include "libHttpServer/Server.hpp"
#include "libHttpServer/Internal/RoundRobinServer.hpp"

namespace HTTP
{

    class Server::Data
    {
    public:
        RoundRobinServer*   mTcpServer;
        IAccessLog*         mAccessLog;
    };

}
