#include <QCoreApplication>
#include <QDir>
#include <QFile>

#include "httpserver/httplistener.h"
#include "mongoaddons/mongo.h"

#include "requesthandler.h"
#include "envset.h"

int main( int argc, char* argv[] ) {

    // Initialize the core application
    QCoreApplication app( argc, argv );
    app.setApplicationName( "Demo2" );

    if ( !Mongo::instance().start( EnvSet::value( "DB_CONNECTION" ).toString(), "quotes", "quotes" ) ) {
        qCritical() << "DB failure";
        return EXIT_FAILURE;
    }
    qDebug() << "DB success";

    // Collect hardcoded configarion settings
    QSettings* settings=new QSettings( &app );
    // settings->setValue("host","192.168.0.100");
    settings->setValue( "port", EnvSet::value( "PORT", "8080" ) );
    settings->setValue( "minThreads", "4" );
    settings->setValue( "maxThreads", "100" );
    settings->setValue( "cleanupInterval", "60000" );
    settings->setValue( "readTimeout", "60000" );
    settings->setValue( "maxRequestSize", "16000" );
    settings->setValue( "maxMultiPartSize", "10000000" );
    // settings->setValue("sslKeyFile","ssl/my.key");
    // settings->setValue("sslCertFile","ssl/my.cert");

    // Configure and start the TCP listener
    new stefanfrings::HttpListener( settings, new RequestHandler( &app ), &app );

    qWarning( "Application has started" );
    app.exec();
    qWarning( "Application has stopped" );
}
