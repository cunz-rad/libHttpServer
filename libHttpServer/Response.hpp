
#ifndef HTTP_RESPONSE_HPP
#define HTTP_RESPONSE_HPP

#include <QDateTime>

#include "libHttpServer/Http.hpp"

namespace HTTP
{

    class HTTP_SERVER_API Response : public QObject
    {
        Q_OBJECT
    public:
        ~Response();

    public:
        void addHeader( const HeaderName& header, const HeaderValue& value );
        void addHeader( const HeaderName& header, const QDateTime& value );
        bool hasHeader( const HeaderName& name ) const;
        bool headersSent() const;

        void addBody( const QByteArray& data );
        void fixBody();

        void sendHeaders( StatusCode code );
        void send( StatusCode code );
        void send();

    private:
        friend class Request;
        class Data;
        Response( Data* d );
        Data* d;
    };

}

#endif
