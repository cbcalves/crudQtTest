/**
   @file
   @author Carlos Alves
 */

#include "requesthandler.h"

#include <QJsonDocument>

#include "jsondtohandler.h"
#include "datadto.h"

RequestHandler::RequestHandler( QObject* parent ) :
    HttpRequestHandler( parent ) {

    // Show forward route
    _router.use( "/hom", &_testApi );
    _router.use( "/test", &_testApi );
    _router.use( "/home", &_homeApi );

    // Show route (POST) with parameters ':'
    _router.route( Router::Method::POST, "/wall/:", []( HttpRequest& request, HttpResponse& response ) {
        response.write( Router::pathParam( request.getPath() ).toUtf8(), true );
    } );

    // Show the root route with get request
    _router.getRequest( "/", []( HttpRequest&, HttpResponse& response ) {
        response.write( "main page test", true );
    } );

    //Show /test/get with any request
    _testApi.allRequest( "/get", this, &RequestHandler::get );

    //Show /home/adress with put request
    _homeApi.putRequest( "/address", this, &RequestHandler::address );

    // Using Qt Metatype property to handle json<->dto
    _homeApi.postRequest( "/name", this, &RequestHandler::name );
    _homeApi.getRequest( "/name/:", this, &RequestHandler::findName );
}

void RequestHandler::service( HttpRequest& request, HttpResponse& response ) {
    qDebug() << "RequestHandler::service path   =" << request.getPath();
    qDebug() << "RequestHandler::service method =" << request.getMethod();

    // Set a response header
    response.setHeader( "Content-Type", "text/html; charset=utf-8" );

    _router.service( request, response );
}

void RequestHandler::get( HttpRequest&, HttpResponse& response ) {
    response.write( "<html><body>Hello World!</body></html>", true );

}

void RequestHandler::address( HttpRequest&, HttpResponse& response ) {
    response.write( "<html><body>My home!</body></html>", true );
}

void RequestHandler::name( HttpRequest& request, HttpResponse& response ) {
    QByteArray json = request.getBody();
    QJsonDocument jsonDocument = QJsonDocument::fromJson( json );

    DataDTO data;
    JsonDtoHandler().toDTO( jsonDocument.object(), &data );

    qDebug() << "ID" << data.idObject() << "NAME" << data.name();

    QString text = QStringLiteral( "<html><body>ID=%1<br>Name=%2</body></html>" ).arg( data.idObject() ).arg( data.name() );

    response.write( text.toUtf8(), true );
}

void RequestHandler::findName( stefanfrings::HttpRequest& request, stefanfrings::HttpResponse& response ) {
    DataDTO data;
    data.setIdObject( Router::pathParam( request.getPath() ).toInt() );
    data.setName( "Test Name" );

    response.setHeader( "Content-Type", "text/json; charset=utf-8" );

    auto jsonObject = JsonDtoHandler().toJson( &data );
    response.write( QJsonDocument( jsonObject ).toJson(), true );
}
