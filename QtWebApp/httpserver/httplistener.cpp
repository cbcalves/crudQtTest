/**
   @file
   @author Stefan Frings
 */

#include "httplistener.h"
#include "httpconnectionhandler.h"
#include "httpconnectionhandlerpool.h"
#include <QCoreApplication>

using namespace stefanfrings;

HttpListener::HttpListener( const QSettings* settings, HttpRequestHandler* requestHandler, QObject* parent ) :
    QTcpServer( parent ),
    m_settings( settings ),
    m_requestHandler( requestHandler ),
    m_pool( nullptr ) {

    Q_ASSERT( settings != nullptr );
    Q_ASSERT( requestHandler != nullptr );
    // Reqister type of socketDescriptor for signal/slot handling
    qRegisterMetaType<stefanfrings::tSocketDescriptor>( "stefanfrings::tSocketDescriptor" );
    // Start listening
    listen();
}

HttpListener::~HttpListener() {
    close();
#ifdef SUPERVERBOSE
    qDebug( "HttpListener: destroyed" );
#endif
}

void HttpListener::listen() {
    if ( !m_pool ) {
        m_pool = new HttpConnectionHandlerPool( m_settings, m_requestHandler );
    }
    QString host = m_settings->value( "host" ).toString();
    quint16 port = m_settings->value( "port" ).toUInt() & 0xFFFF;
    QTcpServer::listen( host.isEmpty() ? QHostAddress::Any : QHostAddress( host ), port );
    if ( !isListening() ) {
        qCritical( "HttpListener: Cannot bind on port %i: %s", port, qPrintable( errorString() ) );
        return;
    }

    qDebug( "HttpListener: Listening on port %i", port );
}

void HttpListener::close() {
    QTcpServer::close();
    qDebug( "HttpListener: closed" );
    if ( m_pool ) {
        delete m_pool;
        m_pool = nullptr;
    }
}

void HttpListener::incomingConnection( tSocketDescriptor socketDescriptor ) {
#ifdef SUPERVERBOSE
    qDebug( "HttpListener: New connection" );
#endif

    HttpConnectionHandler* freeHandler = nullptr;
    if ( m_pool ) {
        freeHandler = m_pool->getConnectionHandler();
    }

    // Let the handler process the new connection.
    if ( freeHandler ) {
        // The descriptor is passed via event queue because the handler lives in another thread
        QMetaObject::invokeMethod( freeHandler, "handleConnection", Qt::QueuedConnection, Q_ARG( stefanfrings::tSocketDescriptor, socketDescriptor ) );

        return;
    }

    // Reject the connection
    qDebug( "HttpListener: Too many incoming connections" );
    QTcpSocket* socket = new QTcpSocket( this );
    socket->setSocketDescriptor( socketDescriptor );
    connect( socket, &QTcpSocket::disconnected, socket, &QTcpSocket::deleteLater );
    socket->write( "HTTP/1.1 503 too many connections\r\nConnection: close\r\n\r\nToo many connections\r\n" );
    socket->disconnectFromHost();
}
