/**
   @file
   @author Stefan Frings
 */

#include "httpcookie.h"

using namespace stefanfrings;

HttpCookie::HttpCookie() :
    m_maxAge( 0 ),
    m_secure( false ),
    m_httpOnly( false ),
    m_version( 1 ) {}

HttpCookie::HttpCookie( const QByteArray& name, const QByteArray& value, const int maxAge, const QByteArray& path,
                        const QByteArray& comment, const QByteArray& domain, const bool secure, const bool httpOnly,
                        const QByteArray& sameSite ) :
    m_name( name ),
    m_value( value ),
    m_comment( comment ),
    m_domain( domain ),
    m_maxAge( maxAge ),
    m_path( path ),
    m_secure( secure ),
    m_httpOnly( httpOnly ),
    m_sameSite( sameSite ),
    m_version( 1 ) {}

HttpCookie::HttpCookie( const QByteArray& source ) :
    m_maxAge( 0 ),
    m_secure( false ),
    m_httpOnly( false ),
    m_version( 1 ) {

    QList<QByteArray> list=splitCSV( source );
    foreach( QByteArray part, list )
    {

        // Split the part into name and value
        int posi=part.indexOf( '=' );
        QByteArray name = posi ? part.left( posi ).trimmed() : part.trimmed();
        QByteArray value = posi ? part.mid( posi + 1 ).trimmed() : "";

        // Set fields
        if ( name == "Comment" ) {
            m_comment = value;

        } else if ( name == "Domain" ) {
            m_domain = value;

        } else if ( name == "Max-Age" ) {
            m_maxAge = value.toInt();

        } else if ( name == "Path" ) {
            m_path = value;

        } else if ( name == "Secure" ) {
            m_secure = true;

        } else if ( name == "HttpOnly" ) {
            m_httpOnly = true;

        } else if ( name == "SameSite" ) {
            m_sameSite = value;

        } else if ( name == "Version" ) {
            m_version = value.toInt();

        } else {
            if ( m_name.isEmpty() ) {
                m_name = name;
                m_value = value;
            } else {
                qWarning( "HttpCookie: Ignoring unknown %s=%s", name.data(), value.data() );
            }
        }
    }
}

QByteArray HttpCookie::toByteArray() const {
    QByteArray buffer( m_name );
    buffer.append( '=' );
    buffer.append( m_value );
    if ( !m_comment.isEmpty() ) {
        buffer.append( "; Comment=" ).append( m_comment );
    }
    if ( !m_domain.isEmpty() ) {
        buffer.append( "; Domain=" ).append( m_domain );
    }
    if ( m_maxAge != 0 ) {
        buffer.append( "; Max-Age=" ).append( QByteArray::number( m_maxAge ) );
    }
    if ( !m_path.isEmpty() ) {
        buffer.append( "; Path=" ).append( m_path );
    }
    if ( m_secure ) {
        buffer.append( "; Secure" );
    }
    if ( m_httpOnly ) {
        buffer.append( "; HttpOnly" );
    }
    if ( !m_sameSite.isEmpty() ) {
        buffer.append( "; SameSite=" ).append( m_sameSite );
    }
    buffer.append( "; Version=" ).append( QByteArray::number( m_version ) );
    return buffer;
}

void HttpCookie::setName( const QByteArray& name ) {
    m_name = name;
}

void HttpCookie::setValue( const QByteArray& value ) {
    m_value = value;
}

void HttpCookie::setComment( const QByteArray& comment ) {
    m_comment = comment;
}

void HttpCookie::setDomain( const QByteArray& domain ) {
    m_domain = domain;
}

void HttpCookie::setMaxAge( const int maxAge ) {
    m_maxAge = maxAge;
}

void HttpCookie::setPath( const QByteArray& path ) {
    m_path = path;
}

void HttpCookie::setSecure( const bool secure ) {
    m_secure = secure;
}

void HttpCookie::setHttpOnly( const bool httpOnly ) {
    m_httpOnly = httpOnly;
}

void HttpCookie::setSameSite( const QByteArray& sameSite ) {
    m_sameSite = sameSite;
}

const QByteArray& HttpCookie::getName() const {
    return m_name;
}

const QByteArray& HttpCookie::getValue() const {
    return m_value;
}

const QByteArray& HttpCookie::getComment() const {
    return m_comment;
}

const QByteArray& HttpCookie::getDomain() const {
    return m_domain;
}

int HttpCookie::getMaxAge() const {
    return m_maxAge;
}

const QByteArray& HttpCookie::getPath() const {
    return m_path;
}

bool HttpCookie::getSecure() const {
    return m_secure;
}

bool HttpCookie::getHttpOnly() const {
    return m_httpOnly;
}

const QByteArray& HttpCookie::getSameSite() const {
    return m_sameSite;
}

int HttpCookie::getVersion() const {
    return m_version;
}

QList<QByteArray> HttpCookie::splitCSV( const QByteArray& source ) {
    bool inString = false;
    QList<QByteArray> list;
    QByteArray buffer;
    for ( int i = 0; i < source.size(); ++i ) {
        char c = source.at( i );
        if ( inString == false ) {
            if ( c == '\"' ) {
                inString = true;
            } else if ( c == ';' ) {
                QByteArray trimmed=buffer.trimmed();
                if ( !trimmed.isEmpty() ) {
                    list.append( trimmed );
                }
                buffer.clear();
            } else {
                buffer.append( c );
            }
        } else {
            if ( c == '\"' ) {
                inString=false;
            } else {
                buffer.append( c );
            }
        }
    }
    QByteArray trimmed = buffer.trimmed();
    if ( !trimmed.isEmpty() ) {
        list.append( trimmed );
    }
    return list;
}
