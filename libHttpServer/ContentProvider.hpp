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

#ifndef HTTP_CONTENT_PROVIDER_HPP
#define HTTP_CONTENT_PROVIDER_HPP

#include <QRegExp>

#include "libHttpServer/Request.hpp"

namespace HTTP
{

    class HTTP_SERVER_API ContentProvider : public QObject
    {
        Q_OBJECT
    public:
        ContentProvider( QObject* parent );

    public:
        virtual bool canHandle( const QUrl& url ) const = 0;
        virtual void newRequest( Request* request ) = 0;
    };

    class HTTP_SERVER_API StaticContentProvider : public ContentProvider
    {
    public:
        StaticContentProvider( const QString& prefix, const QString& basePath, QObject* parent );

    public:
        virtual bool canHandle( const QUrl& url ) const;
        virtual void newRequest( Request* request );

    public:
        void addMimeType( const QRegExp& regEx, const QByteArray& mimeType );

    private:
        struct MimeTypeInfo
        {
            QRegExp     mRegEx;
            QByteArray  mMimeType;
        };

        typedef QList< MimeTypeInfo > MimeTypes;
        QString     mPrefix;
        QString     mBasePath;
        MimeTypes   mMimeTypes;
    };

}

#endif
