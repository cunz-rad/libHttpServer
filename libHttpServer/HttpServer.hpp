
#ifndef HTTP_SERVER_HPP
#define HTTP_SERVER_HPP

#include <QObject>
#include <QMetaType>

#ifdef HttpServer_EXPORTS
#define HTTP_SERVER_API Q_DECL_EXPORT
#else
#define HTTP_SERVER_API Q_DECL_IMPORT
#endif

class QHostAddress;

namespace HTTP
{

    enum Version
    {
        V_Unknown   = -1,
        V_1_0       = 100,
        V_1_1       = 101
    };

    enum StatusCode
    {
        Continue                        = 100,
        SwitchProtocols                 = 101,

        Ok                              = 200,
        Created                         = 201,
        Accepted                        = 202,
        NonAuthoritaticeInformation     = 203,
        NoContent                       = 204,
        ResetContent                    = 205,
        PartialContent                  = 206,

        MultipleChoices                 = 300,
        MovedPermanently                = 301,
        Found                           = 302,
        SeeOther                        = 303,
        NotModified                     = 304,
        UseProxy                        = 305,
        TemporaryRedirect               = 307,

        BadRequest                      = 400,
        Unauthorized                    = 401,
        PaymentRequired                 = 402,
        Forbidden                       = 403,
        NotFound                        = 404,
        MethodNotAllowed                = 405,
        NotAcceptable                   = 406,
        ProxyAuthenticationRequired     = 407,
        RequestTimeout                  = 408,
        Conflict                        = 409,
        Gone                            = 410,
        LengthRequired                  = 411,
        PreconditionFailed              = 412,
        RequestEntityTooLarge           = 413,
        RequestUriTooLong               = 414,
        RrequestUnsupportedMediaType    = 415,
        RequestedRangeNotSatisfiable    = 416,
        ExpectationFailed               = 417,
        IAmATeaport                     = 418,

        InternalServerError             = 500,
        NotImplemented                  = 501,
        BadGateway                      = 502,
        ServerUnavailable               = 503,
        GatewayTimeout                  = 504,
        HttpVersionNotSupported         = 505
    };

    enum Method {
        Delete = 0, Get, Head, Post, Put, Connect, Options, Trace, Copy, Lock, MkCol, Move, PropFind,
        PropPatch, Search, Unlock, Report, MkActivity, Checkout, Merge, MSearch, Notify, Subscribe,
        Unsubscribe, Patch, Purge
    };

    typedef QByteArray HeaderName;
    typedef QByteArray HeaderValue;
    typedef QHash< HeaderName, HeaderValue > HeadersHash;

    class Response;
    class Connection;

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

    class HTTP_SERVER_API Response : public QObject
    {
        Q_OBJECT
    public:
        ~Response();

    public:
        void addHeader( const HeaderName& header, const HeaderValue& value );
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

    class IAccessLog
    {
    public:
        virtual ~IAccessLog() {}

    public:
        virtual void access( int connectionId, Version version, Method method, const QUrl& url,
                             const QHostAddress& remoteAddr, quint16 remotePort,
                             StatusCode response ) = 0;
    };

    class HTTP_SERVER_API ServerBase : public QObject
    {
        Q_OBJECT
    public:
        ServerBase( QObject* parent = 0 );
        ~ServerBase();

    public:
        bool listen( const QHostAddress& addr, quint16 port = 0 );
        bool listen( quint16 port = 0 );
        void setAccessLog( IAccessLog* log );
        IAccessLog* accessLog() const;
        static QByteArray methodName( Method method );

    signals:
        void newRequest( HTTP::Request* request );

    private slots:
        void newConnection();

    private:
        class Data;
        Data* d;
    };

}

#endif
