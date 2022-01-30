/**
   @file
   @author Stefan Frings
 */

#include "httpresponse.h"

using namespace stefanfrings;

HttpResponse::HttpResponse( QTcpSocket* socket ) :
    m_socket( socket ),
    m_statusCode( 200 ),
    m_statusText( "OK" ),
    m_sentHeaders( false ),
    m_sentLastPart( false ),
    m_chunkedMode( false ) {}

void HttpResponse::setHeader( const QByteArray& name, const QByteArray& value ) {
    Q_ASSERT( m_sentHeaders == false );
    m_headers.insert( name, value );
}

void HttpResponse::setHeader( const QByteArray& name, int value ) {
    Q_ASSERT( m_sentHeaders == false );
    m_headers.insert( name, QByteArray::number( value ) );
}

QMap<QByteArray, QByteArray>& HttpResponse::getHeaders() {
    return m_headers;
}

void HttpResponse::setStatus( int statusCode, const QByteArray& description ) {
    m_statusCode = statusCode;
    m_statusText = description;
}

int HttpResponse::getStatusCode() const {
    return m_statusCode;
}

void HttpResponse::writeHeaders() {
    Q_ASSERT( m_sentHeaders == false );
    QByteArray buffer;
    buffer.append( "HTTP/1.1 " );
    buffer.append( QByteArray::number( m_statusCode ) );
    buffer.append( ' ' );
    buffer.append( m_statusText );
    buffer.append( "\r\n" );

    for ( auto header = m_headers.cbegin(); header != m_headers.cend(); ++header ) {
        buffer.append( header.key() );
        buffer.append( ": " );
        buffer.append( m_headers.value( header.key() ) );
        buffer.append( "\r\n" );

    }

    for ( auto cookie = m_cookies.cbegin(); cookie != m_cookies.cend(); ++cookie ) {
        buffer.append( "Set-Cookie: " );
        buffer.append( cookie.value().toByteArray() );
        buffer.append( "\r\n" );
    }

    buffer.append( "\r\n" );
    writeToSocket( buffer );
    m_socket->flush();
    m_sentHeaders = true;
}

bool HttpResponse::writeToSocket( const QByteArray& data ) const {
    int remaining = data.size();
    const char* ptr = data.data();
    while ( m_socket->isOpen() && remaining>0 ) {
        // If the output buffer has become large, then wait until it has been sent.
        if ( m_socket->bytesToWrite() > 16384 ) {
            m_socket->waitForBytesWritten( -1 );
        }

        qint64 written = m_socket->write( ptr, remaining );
        if ( written == -1 ) {
            return false;
        }
        ptr += written;
        remaining -= written;
    }
    return true;
}

void HttpResponse::write( const QByteArray& data, bool lastPart ) {
    Q_ASSERT( m_sentLastPart == false );

    // Send HTTP headers, if not already done (that happens only on the first call to write())
    if ( m_sentHeaders == false ) {
        // If the whole response is generated with a single call to write(), then we know the total
        // size of the response and therefore can set the Content-Length header automatically.
        if ( lastPart ) {
            // Automatically set the Content-Length header
            m_headers.insert( "Content-Length", QByteArray::number( data.size() ) );
        } else {
            // else if we will not close the connection at the end, them we must use the chunked mode.
            QByteArray connectionValue = m_headers.value( "Connection", m_headers.value( "connection" ) );
            bool connectionClose = QString::compare( connectionValue, "close", Qt::CaseInsensitive )==0;
            if ( !connectionClose ) {
                m_headers.insert( "Transfer-Encoding", "chunked" );
                m_chunkedMode = true;
            }
        }

        writeHeaders();
    }

    // Send data
    if ( data.size() > 0 ) {
        if ( m_chunkedMode ) {
            if ( data.size() > 0 ) {
                QByteArray size = QByteArray::number( data.size(), 16 );
                writeToSocket( size );
                writeToSocket( "\r\n" );
                writeToSocket( data );
                writeToSocket( "\r\n" );
            }
        } else {
            writeToSocket( data );
        }
    }

    // Only for the last chunk, send the terminating marker and flush the buffer.
    if ( lastPart ) {
        if ( m_chunkedMode ) {
            writeToSocket( "0\r\n\r\n" );
        }
        m_socket->flush();
        m_sentLastPart = true;
    }
}

bool HttpResponse::hasSentLastPart() const {
    return m_sentLastPart;
}

void HttpResponse::setCookie( const HttpCookie& cookie ) {
    Q_ASSERT( m_sentHeaders == false );
    if ( !cookie.getName().isEmpty() ) {
        m_cookies.insert( cookie.getName(), cookie );
    }
}

QMap<QByteArray, HttpCookie>& HttpResponse::getCookies() {
    return m_cookies;
}

void HttpResponse::redirect( const QByteArray& url ) {
    setStatus( 303, "See Other" );
    setHeader( "Location", url );
    write( "Redirect", true );
}

void HttpResponse::flush() const {
    m_socket->flush();
}

bool HttpResponse::isConnected() const {
    return m_socket->isOpen();
}
