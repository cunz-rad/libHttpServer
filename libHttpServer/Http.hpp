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

#ifndef HTTP_HTTP_HPP
#define HTTP_HTTP_HPP

#include <QObject>
#include <QMetaType>
#include <QByteArray>
#include <QHash>

#ifdef HttpServer_EXPORTS
#define HTTP_SERVER_API Q_DECL_EXPORT
#else
#define HTTP_SERVER_API Q_DECL_IMPORT
#endif

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

    HTTP_SERVER_API QByteArray toRfc1123date( const QDateTime& dt );
    HTTP_SERVER_API QDateTime fromRfc1123date( const QByteArray& data );

}

#endif
