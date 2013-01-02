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

#ifndef HTTP_HTTP_PRIVATE_HPP
#define HTTP_HTTP_PRIVATE_HPP

#include "libHttpServer/Http.hpp"

namespace HTTP
{

    #define CRLF "\r\n"

    #define HTTP_DO_DBG if( 1 ) qDebug
    #define HTTP_NO_DBG if( 0 ) qDebug

    #define HTTP_PARSER_DBG HTTP_NO_DBG

    #define HTTP_DBG HTTP_NO_DBG

}

#endif
