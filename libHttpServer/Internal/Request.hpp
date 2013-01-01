
#ifndef HTTP_REQUEST_PRIVATE_HPP
#define HTTP_REQUEST_PRIVATE_HPP

#include <QHostAddress>
#include <QByteArray>
#include <QHash>

#include "libHttpServer/Request.hpp"

namespace HTTP
{

    class Request::Data
    {
    public:
        Data();

    public:
        void appendHeaderData( const HeaderName& header, const QByteArray& data );
        void appendBodyData( const QByteArray& data );
        void enterRecvBody();
        void enterFinished();

    public:
        Request*        mRequest;
        Request::State  mState;
        QByteArray      mUrl;
        HeadersHash     mHeaders;
        QByteArray      mBodyData;
        Response*       mResponse;
        Version         mVersion;
        quint16         mRemotePort;
        QHostAddress    mRemoteAddr;
        Method          mMethod;
    };
}

#endif
