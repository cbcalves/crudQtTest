/**
   @file
   @author Stefan Frings
 */

#include "httpconnectionhandler.h"
#include "httpresponse.h"

using namespace stefanfrings;

HttpConnectionHandler::HttpConnectionHandler( const QSettings* settings, HttpRequestHandler* requestHandler, const QSslConfiguration* sslConfiguration ) :
    QObject(),
    m_settings( settings ),
    m_socket( nullptr ),
    m_thread( new QThread ),
    m_currentRequest( nullptr ),
    m_requestHandler( requestHandler ),
    m_busy( false ),
    m_sslConfiguration( sslConfiguration ) {

    Q_ASSERT( settings != nullptr );
    Q_ASSERT( requestHandler != nullptr );

    // execute signals in a new thread
    m_thread->start();
#ifdef SUPERVERBOSE
    qDebug( "HttpConnectionHandler (%p): thread started", static_cast<void*>( this ) );
#endif
    moveToThread( m_thread );
    m_readTimer.moveToThread( m_thread );
    m_readTimer.setSingleShot( true );

    // Create TCP or SSL socket
    createSocket();
    m_socket->moveToThread( m_thread );

    // Connect signals
    connect( m_socket, &QTcpSocket::readyRead, this, &HttpConnectionHandler::read );
    connect( m_socket, &QTcpSocket::disconnected, this, &HttpConnectionHandler::disconnected );
    connect( &m_readTimer, &QTimer::timeout, this, &HttpConnectionHandler::readTimeout );
    connect( m_thread, &QThread::finished, this, &HttpConnectionHandler::thread_done );

#ifdef SUPERVERBOSE
    qDebug( "HttpConnectionHandler (%p): constructed", static_cast<void*>( this ) );
#endif
}

void HttpConnectionHandler::thread_done() {
    m_readTimer.stop();
    m_socket->close();
    delete m_socket;
#ifdef SUPERVERBOSE
    qDebug( "HttpConnectionHandler (%p): thread stopped", static_cast<void*>( this ) );
#endif
}

HttpConnectionHandler::~HttpConnectionHandler() {
    m_thread->quit();
    m_thread->wait();
    m_thread->deleteLater();
#ifdef SUPERVERBOSE
    qDebug( "HttpConnectionHandler (%p): destroyed", static_cast<void*>( this ) );
#endif
}

void HttpConnectionHandler::createSocket() {
    // If SSL is supported and configured, then create an instance of QSslSocket
#ifndef QT_NO_SSL
    if ( m_sslConfiguration ) {
        QSslSocket* sslSocket = new QSslSocket();
        sslSocket->setSslConfiguration( *m_sslConfiguration );
        m_socket = sslSocket;
#ifdef SUPERVERBOSE
        qDebug( "HttpConnectionHandler (%p): SSL is enabled", static_cast<void*>( this ) );
#endif
        return;
    }
#endif
    // else create an instance of QTcpSocket
    m_socket = new QTcpSocket();
}

void HttpConnectionHandler::handleConnection( tSocketDescriptor socketDescriptor ) {
    qDebug( "HttpConnectionHandler (%p): handle new connection", static_cast<void*>( this ) );
    m_busy = true;
    Q_ASSERT( m_socket->isOpen() == false ); // if not, then the handler is already busy

    //UGLY workaround - we need to clear writebuffer before reusing this socket
    //https://bugreports.qt-project.org/browse/QTBUG-28914
    m_socket->connectToHost( "", 0 );
    m_socket->abort();

    if ( !m_socket->setSocketDescriptor( socketDescriptor ) ) {
        qCritical( "HttpConnectionHandler (%p): cannot initialize socket: %s",
                   static_cast<void*>( this ), qPrintable( m_socket->errorString() ) );
        return;
    }

#ifndef QT_NO_SSL
    // Switch on encryption, if SSL is configured
    if ( m_sslConfiguration ) {
        qDebug( "HttpConnectionHandler (%p): Starting encryption", static_cast<void*>( this ) );
        ( static_cast<QSslSocket*>( m_socket ) )->startServerEncryption();
    }
#endif

    // Start timer for read timeout
    int readTimeout = m_settings->value( "readTimeout", 10000 ).toInt();
    m_readTimer.start( readTimeout );
    // delete previous request
    delete m_currentRequest;
    m_currentRequest = nullptr;
}

bool HttpConnectionHandler::isBusy() {
    return m_busy;
}

void HttpConnectionHandler::setBusy() {
    this->m_busy = true;
}

void HttpConnectionHandler::readTimeout() {
#ifdef SUPERVERBOSE
    qDebug( "HttpConnectionHandler (%p): read timeout occured", static_cast<void*>( this ) );
#endif

    //Commented out because QWebView cannot handle this.
    //socket->write("HTTP/1.1 408 request timeout\r\nConnection: close\r\n\r\n408 request timeout\r\n");

    while ( m_socket->bytesToWrite() ) m_socket->waitForBytesWritten();
    m_socket->disconnectFromHost();
    delete m_currentRequest;
    m_currentRequest = nullptr;
}

void HttpConnectionHandler::disconnected() {
    qDebug( "HttpConnectionHandler (%p): disconnected", static_cast<void*>( this ) );
    m_socket->close();
    m_readTimer.stop();
    m_busy = false;
}

void HttpConnectionHandler::read() {
    // The loop adds support for HTTP pipelinig
    while ( m_socket->bytesAvailable() ) {
#ifdef SUPERVERBOSE
        qDebug( "HttpConnectionHandler (%p): read input", static_cast<void*>( this ) );
#endif

        // Create new HttpRequest object if necessary
        if ( !m_currentRequest ) {
            m_currentRequest = new HttpRequest( m_settings );
        }

        // Collect data for the request object
        while ( m_socket->bytesAvailable() && m_currentRequest->getStatus() != HttpRequest::COMPLETE && m_currentRequest->getStatus() != HttpRequest::ABORT ) {
            m_currentRequest->readFromSocket( m_socket );
            if ( m_currentRequest->getStatus() == HttpRequest::WAIT_FOR_BODY ) {
                // Restart timer for read timeout, otherwise it would
                // expire during large file uploads.
                int readTimeout = m_settings->value( "readTimeout", 10000 ).toInt();
                m_readTimer.start( readTimeout );
            }
        }

        // If the request is aborted, return error message and close the connection
        if ( m_currentRequest->getStatus() == HttpRequest::ABORT ) {
            m_socket->write( "HTTP/1.1 413 entity too large\r\nConnection: close\r\n\r\n413 Entity too large\r\n" );
            while ( m_socket->bytesToWrite() ) m_socket->waitForBytesWritten();
            m_socket->disconnectFromHost();
            delete m_currentRequest;
            m_currentRequest=nullptr;
            return;
        }

        // If the request is complete, let the request mapper dispatch it
        if ( m_currentRequest->getStatus() == HttpRequest::COMPLETE ) {
            m_readTimer.stop();
            qDebug( "HttpConnectionHandler (%p): received request", static_cast<void*>( this ) );

            // Copy the Connection:close header to the response
            HttpResponse response( m_socket );
            bool closeConnection = QString::compare( m_currentRequest->getHeader( "Connection" ), "close", Qt::CaseInsensitive ) == 0;
            if ( closeConnection ) {
                response.setHeader( "Connection", "close" );
            } else {
                // In case of HTTP 1.0 protocol add the Connection:close header.
                // This ensures that the HttpResponse does not activate chunked mode, which is not spported by HTTP 1.0.
                bool http1_0 = QString::compare( m_currentRequest->getVersion(), "HTTP/1.0", Qt::CaseInsensitive ) == 0;
                if ( http1_0 ) {
                    closeConnection = true;
                    response.setHeader( "Connection", "close" );
                }
            }

            // Call the request mapper
            try{
                m_requestHandler->service( *m_currentRequest, response );

            } catch ( ... ) {
                qCritical( "HttpConnectionHandler (%p): An uncatched exception occured in the request handler",
                           static_cast<void*>( this ) );
            }

            // Finalize sending the response if not already done
            if ( !response.hasSentLastPart() ) {
                response.write( QByteArray(), true );
            }

#ifdef SUPERVERBOSE
            qDebug( "HttpConnectionHandler (%p): finished request", static_cast<void*>( this ) );
#endif

            // Find out whether the connection must be closed
            if ( !closeConnection ) {
                // Maybe the request handler or mapper added a Connection:close header in the meantime
                bool closeResponse = QString::compare( response.getHeaders().value( "Connection" ), "close", Qt::CaseInsensitive ) == 0;
                if ( closeResponse == true ) {
                    closeConnection = true;
                } else {
                    // If we have no Content-Length header and did not use chunked mode, then we have to close the
                    // connection to tell the HTTP client that the end of the response has been reached.
                    bool hasContentLength = response.getHeaders().contains( "Content-Length" );
                    if ( !hasContentLength ) {
                        bool hasChunkedMode = QString::compare( response.getHeaders().value( "Transfer-Encoding" ), "chunked", Qt::CaseInsensitive ) == 0;
                        if ( !hasChunkedMode ) {
                            closeConnection = true;
                        }
                    }
                }
            }

            // Close the connection or prepare for the next request on the same connection.
            if ( closeConnection ) {
                while ( m_socket->bytesToWrite() ) m_socket->waitForBytesWritten();
                m_socket->disconnectFromHost();
            } else {
                // Start timer for next request
                int readTimeout = m_settings->value( "readTimeout", 10000 ).toInt();
                m_readTimer.start( readTimeout );
            }
            delete m_currentRequest;
            m_currentRequest = nullptr;
        }
    }
}
