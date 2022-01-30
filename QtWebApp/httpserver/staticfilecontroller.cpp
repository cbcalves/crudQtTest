/**
   @file
   @author Stefan Frings
 */

#include "staticfilecontroller.h"
#include <QFileInfo>
#include <QDir>
#include <QDateTime>

using namespace stefanfrings;

StaticFileController::StaticFileController( const QSettings* settings, QObject* parent ) :
    HttpRequestHandler( parent ),
    m_encoding( settings->value( "encoding", "UTF-8" ).toString() ),
    m_docroot( settings->value( "path", "." ).toString() ),
    m_maxAge( settings->value( "maxAge", "60000" ).toInt() ),
    m_cacheTimeout( settings->value( "cacheTime", "60000" ).toInt() ),
    m_maxCachedFileSize( settings->value( "maxCachedFileSize", "65536" ).toInt() ) {

    if ( !( m_docroot.startsWith( ":/" ) || m_docroot.startsWith( "qrc://" ) ) ) {
        // Convert relative path to absolute, based on the directory of the config file.
        #ifdef Q_OS_WIN32
        if ( QDir::isRelativePath( docroot ) && settings->format() != QSettings::NativeFormat )
        #else
        if ( QDir::isRelativePath( m_docroot ) )
        #endif
        {
            QFileInfo configFile( settings->fileName() );
            m_docroot=QFileInfo( configFile.absolutePath(), m_docroot ).absoluteFilePath();
        }
    }
    m_cache.setMaxCost( settings->value( "cacheSize", "1000000" ).toInt() );

#ifdef SUPERVERBOSE
    qDebug( "StaticFileController: docroot=%s, encoding=%s, maxAge=%i", qPrintable( m_docroot ), qPrintable( m_encoding ), m_maxAge );
    long int cacheMaxCost=(long int)m_cache.maxCost();
    qDebug( "StaticFileController: cache timeout=%i, size=%li", m_cacheTimeout, cacheMaxCost );
#endif
}

void StaticFileController::service( HttpRequest& request, HttpResponse& response ) {
    QByteArray path = request.getPath();
    // Check if we have the file in cache
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    m_mutex.lock();
    CacheEntry* entry = m_cache.object( path );
    if ( entry && ( m_cacheTimeout == 0 || entry->created>now - m_cacheTimeout ) ) {
        QByteArray document = entry->document; //copy the cached document, because other threads may destroy the cached entry immediately after mutex unlock.
        QByteArray filename = entry->filename;
        m_mutex.unlock();
#ifdef SUPERVERBOSE
        qDebug( "StaticFileController: Cache hit for %s", path.data() );
#endif
        setContentType( filename, response );
        response.setHeader( "Cache-Control", "max-age=" + QByteArray::number( m_maxAge / 1000 ) );
        response.write( document, true );

        return;
    }

    m_mutex.unlock();
    // The file is not in cache.
#ifdef SUPERVERBOSE
    qDebug( "StaticFileController: Cache miss for %s", path.data() );
#endif
    // Forbid access to files outside the docroot directory
    if ( path.contains( "/.." ) ) {
        qWarning( "StaticFileController: detected forbidden characters in path %s", path.data() );
        response.setStatus( 403, "forbidden" );
        response.write( "403 forbidden", true );
        return;
    }
    // If the filename is a directory, append index.html.
    if ( QFileInfo( m_docroot + path ).isDir() ) {
        path += "/index.html";
    }
    // Try to open the file
    QFile file( m_docroot + path );
#ifdef SUPERVERBOSE
    qDebug( "StaticFileController: Open file %s", qPrintable( file.fileName() ) );
#endif
    if ( file.open( QIODevice::ReadOnly ) ) {
        setContentType( path, response );
        response.setHeader( "Cache-Control", "max-age=" + QByteArray::number( m_maxAge / 1000 ) );
        response.setHeader( "Content-Length", QByteArray::number( file.size() ) );
        if ( file.size() <= m_maxCachedFileSize ) {
            // Return the file content and store it also in the cache
            entry = new CacheEntry();
            while ( !file.atEnd() && !file.error() ) {
                QByteArray buffer = file.read( 65536 );
                response.write( buffer, false );
                entry->document.append( buffer, false );
            }
            entry->created = now;
            entry->filename = path;
            m_mutex.lock();
            m_cache.insert( request.getPath(), entry, entry->document.size() );
            m_mutex.unlock();

        } else {
            // Return the file content, do not store in cache
            while ( !file.atEnd() && !file.error() ) {
                response.write( file.read( 65536 ) );
            }
        }
        file.close();

        return;
    }

    if ( file.exists() ) {
        qWarning( "StaticFileController: Cannot open existing file %s for reading", qPrintable( file.fileName() ) );
        response.setStatus( 403, "forbidden" );
        response.write( "403 forbidden", true );

        return;
    }

    response.setStatus( 404, "not found" );
    response.write( "404 not found", true );
}

void StaticFileController::setContentType( const QString& fileName, HttpResponse& response ) const {

    // Todo: add all of your content types
    const QMap<QString, QString> contentTypeMap = {
        { ".png", "image/png" },
        { ".jpg", "image/jpeg" },
        { ".gif", "image/gif" },
        { ".pdf", "application/pdf" },
        { ".txt", "text/plain; charset=" + m_encoding },
        { ".html", "text/html; charset=" + m_encoding },
        { ".htm", "text/html; charset=" + m_encoding },
        { ".css", "text/css" },
        { ".js", "text/javascript" },
        { ".svg", "image/svg+xml" },
        { ".woff", "font/woff" },
        { ".woff2", "font/woff2" },
        { ".ttf", "application/x-font-ttf" },
        { ".eot", "application/vnd.ms-fontobject" },
        { ".otf", "application/font-otf" },
        { ".json", "application/json" },
        { ".xml", "text/xml" },
    };

    for ( auto contentType = contentTypeMap.cbegin(); contentType != contentTypeMap.cend(); ++contentType ) {
        if ( fileName.endsWith( contentType.key() ) ) {
            response.setHeader( "Content-Type", qPrintable( contentType.value() ) );
            return;
        }
    }

    qWarning( "StaticFileController: unknown MIME type for filename '%s'", qPrintable( fileName ) );
}
