/**
   @file
   @author Stefan Frings
 */

#include "httprequest.h"
#include <QList>
#include <QDir>
#include "httpcookie.h"

using namespace stefanfrings;

HttpRequest::HttpRequest( const QSettings* settings ) :
    m_status( WAIT_FOR_REQUEST ),
    m_maxSize( settings->value( "maxRequestSize", "16000" ).toInt() ),
    m_maxMultiPartSize( settings->value( "maxMultiPartSize", "1000000" ).toInt() ),
    m_currentSize( 0 ),
    m_expectedBodySize( 0 ),
    m_tempFile( nullptr ) {}

void HttpRequest::readRequest( QTcpSocket* socket ) {
#ifdef SUPERVERBOSE
    qDebug( "HttpRequest: read request" );
#endif
    int toRead = m_maxSize - m_currentSize + 1; // allow one byte more to be able to detect overflow
    QByteArray dataRead = socket->readLine( toRead );
    m_currentSize += dataRead.size();
    m_lineBuffer.append( dataRead );
    if ( !m_lineBuffer.contains( "\r\n" ) ) {
#ifdef SUPERVERBOSE
        qDebug( "HttpRequest: collecting more parts until line break" );
#endif
        return;
    }
    QByteArray newData = m_lineBuffer.trimmed();
    m_lineBuffer.clear();
    if ( !newData.isEmpty() ) {
#ifdef SUPERVERBOSE
        qDebug( "HttpRequest: from %s: %s", qPrintable( socket->peerAddress().toString() ), newData.data() );
#endif
        QList<QByteArray> list = newData.split( ' ' );
        if ( list.count()!=3 || !list.at( 2 ).contains( "HTTP" ) ) {
            qWarning( "HttpRequest: received broken HTTP request, invalid first line" );
            m_status = ABORT;
        }else {
            m_method = list.at( 0 ).trimmed();
            m_path = list.at( 1 );
            m_version = list.at( 2 );
            m_peerAddress = socket->peerAddress();
            m_status = WAIT_FOR_HEADER;
        }
    }
}

void HttpRequest::readHeader( QTcpSocket* socket ) {
    int toRead=m_maxSize - m_currentSize + 1; // allow one byte more to be able to detect overflow
    QByteArray dataRead = socket->readLine( toRead );
    m_currentSize += dataRead.size();
    m_lineBuffer.append( dataRead );
    if ( !m_lineBuffer.contains( "\r\n" ) ) {
#ifdef SUPERVERBOSE
        qDebug( "HttpRequest: collecting more parts until line break" );
#endif
        return;
    }
    QByteArray newData = m_lineBuffer.trimmed();
    m_lineBuffer.clear();
    int colon = newData.indexOf( ':' );
    if ( colon > 0 ) {
        // Received a line with a colon - a header
        m_currentHeader = newData.left( colon ).toLower();
        QByteArray value = newData.mid( colon + 1 ).trimmed();
        m_headers.insert( m_currentHeader, value );
#ifdef SUPERVERBOSE
        qDebug( "HttpRequest: received header %s: %s", currentHeader.data(), value.data() );
#endif
        return;
    }

    if ( !newData.isEmpty() ) {
        // received another line - belongs to the previous header
#ifdef SUPERVERBOSE
        qDebug( "HttpRequest: read additional line of header" );
#endif
        // Received additional line of previous header
        if ( m_headers.contains( m_currentHeader ) ) {
            m_headers.insert( m_currentHeader, m_headers.value( m_currentHeader ) + " " + newData );
        }
        return;
    }

    // received an empty line - end of headers reached
#ifdef SUPERVERBOSE
    qDebug( "HttpRequest: headers completed" );
#endif
    // Empty line received, that means all headers have been received
    // Check for multipart/form-data
    QByteArray contentType = m_headers.value( "content-type" );
    if ( contentType.startsWith( "multipart/form-data" ) ) {
        int posi = contentType.indexOf( "boundary=" );
        if ( posi >= 0 ) {
            m_boundary=contentType.mid( posi + 9 );
            if ( m_boundary.startsWith( '"' ) && m_boundary.endsWith( '"' ) ) {
                m_boundary = m_boundary.mid( 1, m_boundary.length() - 2 );
            }
        }
    }
    QByteArray contentLength = m_headers.value( "content-length" );
    if ( !contentLength.isEmpty() ) {
        m_expectedBodySize = contentLength.toInt();
    }

    if ( m_expectedBodySize == 0 ) {
#ifdef SUPERVERBOSE
        qDebug( "HttpRequest: expect no body" );
#endif
        m_status = COMPLETE;
    } else if ( m_boundary.isEmpty() && m_expectedBodySize + m_currentSize>m_maxSize ) {
        qWarning( "HttpRequest: expected body is too large" );
        m_status = ABORT;
    } else if ( !m_boundary.isEmpty() && m_expectedBodySize>m_maxMultiPartSize ) {
        qWarning( "HttpRequest: expected multipart body is too large" );
        m_status = ABORT;
    } else {
#ifdef SUPERVERBOSE
        qDebug( "HttpRequest: expect %i bytes body", expectedBodySize );
#endif
        m_status = WAIT_FOR_BODY;
    }
}

void HttpRequest::readBody( QTcpSocket* socket ) {
    Q_ASSERT( m_expectedBodySize != 0 );
    if ( m_boundary.isEmpty() ) {
        // normal body, no multipart
#ifdef SUPERVERBOSE
        qDebug( "HttpRequest: receive body" );
#endif
        int toRead = m_expectedBodySize - m_bodyData.size();
        QByteArray newData = socket->read( toRead );
        m_currentSize += newData.size();
        m_bodyData.append( newData );
        if ( m_bodyData.size() >= m_expectedBodySize ) {
            m_status = COMPLETE;
        }

        return;
    }

    // multipart body, store into temp file
#ifdef SUPERVERBOSE
    qDebug( "HttpRequest: receiving multipart body" );
#endif
    // Create an object for the temporary file, if not already present
    if ( m_tempFile == nullptr ) {
        m_tempFile = new QTemporaryFile;
    }
    if ( !m_tempFile->isOpen() ) {
        m_tempFile->open();
    }
    // Transfer data in 64kb blocks
    qint64 fileSize = m_tempFile->size();
    qint64 toRead = m_expectedBodySize - fileSize;
    if ( toRead > 65536 ) {
        toRead = 65536;
    }
    fileSize += m_tempFile->write( socket->read( toRead ) );
    if ( fileSize >= m_maxMultiPartSize ) {
        qWarning( "HttpRequest: received too many multipart bytes" );
        m_status = ABORT;
    } else if ( fileSize >= m_expectedBodySize ) {
#ifdef SUPERVERBOSE
        qDebug( "HttpRequest: received whole multipart body" );
#endif
        m_tempFile->flush();
        if ( m_tempFile->error() ) {
            qCritical( "HttpRequest: Error writing temp file for multipart body" );
        }
        parseMultiPartFile();
        m_tempFile->close();
        m_status = COMPLETE;
    }
}

void HttpRequest::decodeRequestParams() {
#ifdef SUPERVERBOSE
    qDebug( "HttpRequest: extract and decode request parameters" );
#endif
    // Get URL parameters
    QByteArray rawParameters;
    int questionMark = m_path.indexOf( '?' );
    if ( questionMark >= 0 ) {
        rawParameters=m_path.mid( questionMark + 1 );
        m_path=m_path.left( questionMark );
    }
    // Get request body parameters
    QByteArray contentType = m_headers.value( "content-type" );
    if ( !m_bodyData.isEmpty() && ( contentType.isEmpty() || contentType.startsWith( "application/x-www-form-urlencoded" ) ) ) {
        if ( !rawParameters.isEmpty() ) {
            rawParameters.append( '&' );
            rawParameters.append( m_bodyData );
        } else {
            rawParameters = m_bodyData;
        }
    }
    // Split the parameters into pairs of value and name
    const QList<QByteArray> list = rawParameters.split( '&' );
    for ( const QByteArray& part : list ) {
        int posi = part.indexOf( '=' );
        QByteArray name = posi ? part.left( posi ).trimmed() : part;
        QByteArray value = posi ? part.mid( posi + 1 ).trimmed() : "";
        m_parameters.insert( urlDecode( name ), urlDecode( value ) );
    }
}

void HttpRequest::extractCookies() {
#ifdef SUPERVERBOSE
    qDebug( "HttpRequest: extract cookies" );
#endif
    const auto cookies = m_headers.values( "cookie" );
    for ( const QByteArray& cookieStr : cookies ) {
        const QList<QByteArray> list = HttpCookie::splitCSV( cookieStr );

        for ( const QByteArray& part : list ) {
#ifdef SUPERVERBOSE
            qDebug( "HttpRequest: found cookie %s", part.data() );
#endif                // Split the part into name and value
            int posi = part.indexOf( '=' );
            QByteArray name = posi ? part.left( posi ).trimmed() : part.trimmed();
            QByteArray value = posi ? part.mid( posi + 1 ).trimmed() : "";
            m_cookies.insert( name, value );
        }
    }
    m_headers.remove( "cookie" );
}

void HttpRequest::readFromSocket( QTcpSocket* socket ) {
    Q_ASSERT( m_status != COMPLETE );
    if ( m_status == WAIT_FOR_REQUEST ) {
        readRequest( socket );

    } else if ( m_status == WAIT_FOR_HEADER ) {
        readHeader( socket );

    } else if ( m_status == WAIT_FOR_BODY ) {
        readBody( socket );
    }

    if ( ( m_boundary.isEmpty() && m_currentSize>m_maxSize ) || ( !m_boundary.isEmpty() && m_currentSize>m_maxMultiPartSize ) ) {
        qWarning( "HttpRequest: received too many bytes" );
        m_status = ABORT;
    }
    if ( m_status == COMPLETE ) {
        // Extract and decode request parameters from url and body
        decodeRequestParams();
        // Extract cookies from headers
        extractCookies();
    }
}

HttpRequest::RequestStatus HttpRequest::getStatus() const {
    return m_status;
}

const QByteArray& HttpRequest::getMethod() const {
    return m_method;
}

QByteArray HttpRequest::getPath() const {
    return urlDecode( m_path );
}

const QByteArray& HttpRequest::getRawPath() const {
    return m_path;
}

QByteArray HttpRequest::getVersion() const {
    return m_version;
}

QByteArray HttpRequest::getHeader( const QByteArray& name ) const {
    return m_headers.value( name.toLower() );
}

QList<QByteArray> HttpRequest::getHeaders( const QByteArray& name ) const {
    return m_headers.values( name.toLower() );
}

const QMultiMap<QByteArray, QByteArray>& HttpRequest::getHeaderMap() const {
    return m_headers;
}

QByteArray HttpRequest::getParameter( const QByteArray& name ) const {
    return m_parameters.value( name );
}

QList<QByteArray> HttpRequest::getParameters( const QByteArray& name ) const {
    return m_parameters.values( name );
}

const QMultiMap<QByteArray, QByteArray>& HttpRequest::getParameterMap() const {
    return m_parameters;
}

const QByteArray& HttpRequest::getBody() const {
    return m_bodyData;
}

QByteArray HttpRequest::urlDecode( const QByteArray& source ) {
    if (source.isEmpty()) {
        return source;
    }

    QByteArray buffer( source );
    buffer.replace( '+', ' ' );
    int percentChar = buffer.indexOf( '%' );
    while ( percentChar >= 0 ) {
        bool ok;
        int hexCode = buffer.mid( percentChar + 1, 2 ).toInt( &ok, 16 );
        if ( ok ) {
            char c = char(hexCode);
            buffer.replace( percentChar, 3, &c, 1 );
        }
        percentChar = buffer.indexOf( '%', percentChar + 1 );
    }
    return buffer;
}

void HttpRequest::parseMultiPartFile() {
    qDebug( "HttpRequest: parsing multipart temp file" );
    m_tempFile->seek( 0 );
    bool finished = false;
    while ( !m_tempFile->atEnd() && !finished && !m_tempFile->error() ) {
#ifdef SUPERVERBOSE
        qDebug( "HttpRequest: reading multpart headers" );
#endif
        QByteArray fieldName;
        QByteArray fileName;
        while ( !m_tempFile->atEnd() && !finished && !m_tempFile->error() ) {
            QByteArray line=m_tempFile->readLine( 65536 ).trimmed();
            if ( line.startsWith( "Content-Disposition:" ) ) {
                if ( line.contains( "form-data" ) ) {
                    int start = line.indexOf( " name=\"" );
                    int end = line.indexOf( "\"", start + 7 );
                    if ( start >= 0 && end >= start ) {
                        fieldName=line.mid( start + 7, end - start - 7 );
                    }
                    start = line.indexOf( " filename=\"" );
                    end = line.indexOf( "\"", start + 11 );
                    if ( start >= 0 && end >= start ) {
                        fileName=line.mid( start + 11, end - start - 11 );
                    }
#ifdef SUPERVERBOSE
                    qDebug( "HttpRequest: multipart field=%s, filename=%s", fieldName.data(), fileName.data() );
#endif
                } else {
#ifdef SUPERVERBOSE
                    qDebug( "HttpRequest: ignoring unsupported content part %s", line.data() );
#endif
                }
            } else if ( line.isEmpty() ) {
                break;
            }
        }

#ifdef SUPERVERBOSE
        qDebug( "HttpRequest: reading multpart data" );
#endif
        QTemporaryFile* uploadedFile=nullptr;
        QByteArray fieldValue;
        while ( !m_tempFile->atEnd() && !finished && !m_tempFile->error() ) {
            QByteArray line = m_tempFile->readLine( 65536 );
            if ( line.startsWith( "--" + m_boundary ) ) {
                // Boundary found. Until now we have collected 2 bytes too much,
                // so remove them from the last result
                if ( fileName.isEmpty() && !fieldName.isEmpty() ) {
                    // last field was a form field
                    fieldValue.remove( fieldValue.size() - 2, 2 );
                    m_parameters.insert( fieldName, fieldValue );
#ifdef SUPERVERBOSE
                    qDebug( "HttpRequest: set parameter %s=%s", fieldName.data(), fieldValue.data() );
#endif
                } else if ( !fileName.isEmpty() && !fieldName.isEmpty() ) {
                    // last field was a file
                    if ( uploadedFile ) {
#ifdef SUPERVERBOSE
                        qDebug( "HttpRequest: finishing writing to uploaded file" );
#endif
                        uploadedFile->resize( uploadedFile->size() - 2 );
                        uploadedFile->flush();
                        uploadedFile->seek( 0 );
                        m_parameters.insert( fieldName, fileName );
                        qDebug( "HttpRequest: set parameter %s=%s", fieldName.data(), fileName.data() );
                        m_uploadedFiles.insert( fieldName, uploadedFile );
#ifdef SUPERVERBOSE
                        long int fileSize=(long int) uploadedFile->size();
                        qDebug( "HttpRequest: uploaded file size is %li", fileSize );
#endif
                    } else {
                        qWarning( "HttpRequest: format error, unexpected end of file data" );
                    }
                }
                if ( line.contains( m_boundary + "--" ) ) {
                    finished = true;
                }
                break;
            } else {
                if ( fileName.isEmpty() && !fieldName.isEmpty() ) {
                    // this is a form field.
                    m_currentSize += line.size();
                    fieldValue.append( line );
                } else if ( !fileName.isEmpty() && !fieldName.isEmpty() ) {
                    // this is a file
                    if ( !uploadedFile ) {
                        uploadedFile = new QTemporaryFile();
                        uploadedFile->open();
                    }
                    uploadedFile->write( line );
                    if ( uploadedFile->error() ) {
                        qCritical( "HttpRequest: error writing temp file, %s", qPrintable( uploadedFile->errorString() ) );
                    }
                }
            }
        }
    }
    if ( m_tempFile->error() ) {
        qCritical( "HttpRequest: cannot read temp file, %s", qPrintable( m_tempFile->errorString() ) );
    }
#ifdef SUPERVERBOSE
    qDebug( "HttpRequest: finished parsing multipart temp file" );
#endif
}

HttpRequest::~HttpRequest() {
    for ( auto uploadedFile = m_uploadedFiles.cbegin(); uploadedFile != m_uploadedFiles.cend(); ++uploadedFile ) {
        QTemporaryFile* file = m_uploadedFiles.value( uploadedFile.key() );
        if ( file->isOpen() ) {
            file->close();
        }
        delete file;
    }
    if ( m_tempFile != nullptr ) {
        if ( m_tempFile->isOpen() ) {
            m_tempFile->close();
        }
        delete m_tempFile;
    }
}

QTemporaryFile* HttpRequest::getUploadedFile( const QByteArray& fieldName ) const {
    return m_uploadedFiles.value( fieldName );
}

QByteArray HttpRequest::getCookie( const QByteArray& name ) const {
    return m_cookies.value( name );
}

/** Get the map of cookies */
const QMap<QByteArray, QByteArray>& HttpRequest::getCookieMap() {
    return m_cookies;
}

/**
   Get the address of the connected client.
   Note that multiple clients may have the same IP address, if they
   share an internet connection (which is very common).
 */
const QHostAddress& HttpRequest::getPeerAddress() const {
    return m_peerAddress;
}
