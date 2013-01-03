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

#include <QRegExp>
#include <QFile>
#include <QFileInfo>
#include <QString>
#include <QStringBuilder>
#include <QUrl>

#include "libHttpServer/ContentProvider.hpp"
#include "libHttpServer/Request.hpp"
#include "libHttpServer/Response.hpp"

namespace HTTP
{

    ContentProvider::ContentProvider( QObject* parent )
        : QObject( parent )
    {
    }

    StaticContentProvider::StaticContentProvider( const QString& prefix, const QString& basePath,
                                                  QObject* parent )
        : ContentProvider( parent )
        , mPrefix( prefix )
        , mBasePath( basePath )
    {
        addMimeType( QRegExp( QLatin1String( "*.css" ), Qt::CaseInsensitive, QRegExp::Wildcard ),
                     "text/css" );
        addMimeType( QRegExp( QLatin1String( "*.js" ),  Qt::CaseInsensitive, QRegExp::Wildcard ),
                     "application/javascript" );
    }

    bool StaticContentProvider::canHandle( const QUrl& url ) const
    {
        return url.path().startsWith( mPrefix );
    }

    void StaticContentProvider::newRequest( Request* request )
    {
        Response* res = request->response();
        QString path = request->url().path();
        QString fn( mBasePath % path );
        QFileInfo fi( fn );

        if( !fi.exists() )
        {
            res->fixBody();
            res->send( HTTP::NotFound );
            return;
        }

        if( request->hasHeader( "If-Modified-Since" ) )
        {
            QDateTime dt = HTTP::fromRfc1123date( request->header( "If-Modified-Since" ) );

            if( dt <= fi.lastModified() )
            {
                res->fixBody();
                res->send( HTTP::NotModified );
                return;
            }
        }

        QFile f( fn );
        if( !f.open( QFile::ReadOnly ) )
        {
            res->fixBody();
            res->send( HTTP::NotFound );
            return;
        }

        QByteArray a = f.readAll();
        res->addBody( a );
        res->fixBody();

        QByteArray mimeType = "text/plain";

        MimeTypes::ConstIterator it = mMimeTypes.constBegin();
        while( it != mMimeTypes.constEnd() )
        {
            if( it->mRegEx.exactMatch( path ) )
                mimeType = it->mMimeType;
            ++it;
        }

        res->addHeader( "Content-Type", mimeType );
        res->addHeader( "Cache-Control", "max-age=1200" );
        res->addHeader( "Expires", QDateTime::currentDateTimeUtc().addSecs( 1200 ) );
        res->addHeader( "Last-Modified", fi.lastModified() );

        res->send( HTTP::Ok );
        return;
    }

    void StaticContentProvider::addMimeType( const QRegExp& regEx, const QByteArray& mimeType )
    {
        MimeTypeInfo mti;
        mti.mRegEx = regEx;
        mti.mMimeType = mimeType;
        mMimeTypes.append( mti );
    }

}
