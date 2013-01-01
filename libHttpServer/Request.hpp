
#ifndef HTTP_REQUEST_HPP
#define HTTP_REQUEST_HPP

#include <QUrl>
#include <QHostAddress>

#include "libHttpServer/Http.hpp"

namespace HTTP
{

    class Connection;
    class Response;

    class HTTP_SERVER_API Request : public QObject
    {
        Q_OBJECT
    public:
        enum State
        {
            ReceivingHeaders,
            ReceivingBody,
            Finished
        };

    public:
        ~Request();

    public:
        Response* response();

    public:
        QByteArray urlText() const;
        QUrl url() const;

        bool hasHeader( const HeaderName& header ) const;
        HeaderValue header( const HeaderName& header ) const;
        HeadersHash allHeaders() const;

        bool hasBodyData() const;
        QByteArray bodyData() const;
        State state() const;

        Version httpVersion() const;
        Method method() const;

        QHostAddress remoteAddr() const;
        quint16 remotePort() const;

    signals:
        void completed( HTTP::Request* request );

    private:
        friend class Connection;
        class Data;
        Request( Connection* parent, Data* data );
        Data* d;
    };

}

#endif
