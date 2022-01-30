/**
   @file
   @author Carlos Alves
 */

#include "router.h"

#include <map>

#include <QRegularExpression>

namespace {

inline Router::Method getMethod( const QString& method ) {
    const QMap<QString, Router::Method> string2method {
        { "GET", Router::Method::GET },
        { "POST", Router::Method::POST },
        { "PUT", Router::Method::PUT },
        { "PATCH", Router::Method::PATCH },
        { "DELETE", Router::Method::DELETE, },
    };

    return string2method.value( method, Router::Method::ALL );
}

struct __route_t {
    QRegularExpression regex;
    function_t func;
};
typedef std::pair<QString, __route_t> __route_pair_t;
typedef std::map<QString, __route_t> __route_map_t;
typedef std::map<Router::Method, __route_map_t > __method_map_t;

}

struct Router::RouterData {
    __method_map_t path;
};

Router::Router( QObject* parent ) :
    stefanfrings::HttpRequestHandler{ parent },
    _d{ new RouterData } {}

Router::~Router() {
    delete _d;
}

void Router::service( stefanfrings::HttpRequest& request, stefanfrings::HttpResponse& response ) {
    QString newPath { request.getPath() };
    if ( !_usePath.isEmpty() ) {
        newPath.remove( 0, newPath.indexOf( _usePath ) + _usePath.size() );
    }

    const Method method = getMethod( request.getMethod() );

    function_t exec = findRoute( { Method::USE, Method::ALL, method }, newPath );
    if ( !exec ) {
        response.setStatus( 404, "not found" );
        response.write( "404 not found", true );
        return;
    }

    exec( request, response );
}

QString Router::pathParam( const QString& path ) {
    auto match = QRegularExpression( "[^\\/]+$" ).match( path );
    return match.captured();
}

void Router::insert( Method method, const QString& path, function_t func ) {
    QString regex { path };
    regex.prepend( '^' );

    if ( method == Method::USE ) {
        regex.append( '/' );

    } else {
        if ( path == "/" ) {
            regex.append( '$' );

        } else if ( path.contains( "/:" ) ) {
            regex.replace( "/:", "\\S+$" );

        } else {
            regex.append( "$|^" ).append( path ).append( "/$" );
        }
    }
    regex.replace( '/', "\\/" );

    _d->path[method].emplace( path, __route_t( { QRegularExpression( regex ), func } ) );
}

function_t Router::findRoute( const std::array<Method, 3>& methods, const QString& path ) {
    for ( auto method : methods ) {
        auto methodMap { _d->path.find( method ) };
        if ( methodMap == _d->path.end() ) {
            continue;
        }

        const auto& list { methodMap->second };

        auto find = std::find_if( list.cbegin(), list.cend(), [&path]( const __route_pair_t& current ) {
            return path.contains( current.second.regex );
        } );

        if ( find != list.cend() ) {
            return find->second.func;
        }

    }
    return {};
}
