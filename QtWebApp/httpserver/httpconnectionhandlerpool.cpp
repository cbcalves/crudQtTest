#ifndef QT_NO_SSL
    #include <QSslSocket>
    #include <QSslKey>
    #include <QSslCertificate>
    #include <QSslConfiguration>
#endif
#include <QDir>
#include "httpconnectionhandlerpool.h"

using namespace stefanfrings;

HttpConnectionHandlerPool::HttpConnectionHandlerPool( const QSettings* settings, HttpRequestHandler* requestHandler ) :
    QObject(),
    m_settings( settings ),
    m_requestHandler( requestHandler ),
    m_sslConfiguration( nullptr ) {

    Q_ASSERT( settings!=0 );
    loadSslConfig();

    m_cleanupTimer.start( settings->value( "cleanupInterval", 1000 ).toInt() );
    connect( &m_cleanupTimer, &QTimer::timeout, this, &HttpConnectionHandlerPool::cleanup );
}

HttpConnectionHandlerPool::~HttpConnectionHandlerPool() {
    // delete all connection handlers and wait until their threads are closed
    qDeleteAll( m_pool );
    delete m_sslConfiguration;
#ifdef SUPERVERBOSE
    qDebug( "HttpConnectionHandlerPool (%p): destroyed", this );
#endif
}

HttpConnectionHandler* HttpConnectionHandlerPool::getConnectionHandler() {
    HttpConnectionHandler* freeHandler = nullptr;
    m_mutex.lock();
    // find a free handler in pool
    for ( HttpConnectionHandler* handler : qAsConst( m_pool ) ) {
        if ( !handler->isBusy() ) {
            freeHandler = handler;
            freeHandler->setBusy();
            break;
        }
    }

    // create a new handler, if necessary
    if ( !freeHandler ) {
        int maxConnectionHandlers = m_settings->value( "maxThreads", 100 ).toInt();
        if ( m_pool.count() < maxConnectionHandlers ) {
            freeHandler = new HttpConnectionHandler( m_settings, m_requestHandler, m_sslConfiguration );
            freeHandler->setBusy();
            m_pool.append( freeHandler );
        }
    }
    m_mutex.unlock();
    return freeHandler;
}

void HttpConnectionHandlerPool::cleanup() {
    int maxIdleHandlers = m_settings->value( "minThreads", 1 ).toInt();
    int idleCounter = 0;
    m_mutex.lock();
    for ( HttpConnectionHandler* handler : qAsConst( m_pool ) ) {
        if ( !handler->isBusy() ) {
            if ( ++idleCounter > maxIdleHandlers ) {
                delete handler;
                m_pool.removeOne( handler );
#ifdef SUPERVERBOSE
                long int poolSize = (long int)m_pool.size();
                qDebug( "HttpConnectionHandlerPool: Removed connection handler (%p), pool size is now %li", handler, poolSize );
#endif
                break; // remove only one handler in each interval
            }
        }
    }
    m_mutex.unlock();
}

void HttpConnectionHandlerPool::loadSslConfig() {
    // If certificate and key files are configured, then load them
    QString sslKeyFileName = m_settings->value( "sslKeyFile", "" ).toString();
    QString sslCertFileName = m_settings->value( "sslCertFile", "" ).toString();
    if ( !sslKeyFileName.isEmpty() && !sslCertFileName.isEmpty() ) {
#ifdef QT_NO_SSL
        qWarning( "HttpConnectionHandlerPool: SSL is not supported" );
#else
        // Convert relative fileNames to absolute, based on the directory of the config file.
        QFileInfo configFile( m_settings->fileName() );
#ifdef Q_OS_WIN32
        if ( QDir::isRelativePath( sslKeyFileName ) && settings->format()!=QSettings::NativeFormat ) {
#else
        if ( QDir::isRelativePath( sslKeyFileName ) ) {
#endif // Q_OS_WIN32
            sslKeyFileName = QFileInfo( configFile.absolutePath(), sslKeyFileName ).absoluteFilePath();
        }
#ifdef Q_OS_WIN32
        if ( QDir::isRelativePath( sslCertFileName ) && settings->format()!=QSettings::NativeFormat ) {
#else
        if ( QDir::isRelativePath( sslCertFileName ) ) {
#endif // Q_OS_WIN32
            sslCertFileName = QFileInfo( configFile.absolutePath(), sslCertFileName ).absoluteFilePath();
        }

        // Load the SSL certificate
        QFile certFile( sslCertFileName );
        if ( !certFile.open( QIODevice::ReadOnly ) ) {
            qCritical( "HttpConnectionHandlerPool: cannot open sslCertFile %s", qPrintable( sslCertFileName ) );
            return;
        }
        QSslCertificate certificate( &certFile, QSsl::Pem );
        certFile.close();

        // Load the key file
        QFile keyFile( sslKeyFileName );
        if ( !keyFile.open( QIODevice::ReadOnly ) ) {
            qCritical( "HttpConnectionHandlerPool: cannot open sslKeyFile %s", qPrintable( sslKeyFileName ) );
            return;
        }
        QSslKey sslKey( &keyFile, QSsl::Rsa, QSsl::Pem );
        keyFile.close();

        // Create the SSL configuration
        m_sslConfiguration = new QSslConfiguration();
        m_sslConfiguration->setLocalCertificate( certificate );
        m_sslConfiguration->setPrivateKey( sslKey );
        m_sslConfiguration->setPeerVerifyMode( QSslSocket::VerifyNone );
        m_sslConfiguration->setProtocol( QSsl::AnyProtocol );

#ifdef SUPERVERBOSE
        qDebug( "HttpConnectionHandlerPool: SSL settings loaded" );
#endif

#endif // QT_NO_SSL
    }
}
