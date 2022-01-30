/**
   @file
   @author Stefan Frings
 */

#include "httpsessionstore.h"
#include <QDateTime>
#include <QUuid>

using namespace stefanfrings;

HttpSessionStore::HttpSessionStore( const QSettings* settings, QObject* parent ) :
    QObject( parent ),
    m_settings( settings ),
    m_cookieName( settings->value( "cookieName", "sessionid" ).toByteArray() ),
    m_expirationTime( settings->value( "expirationTime", 3600000 ).toInt() ) {

    connect( &m_cleanupTimer, SIGNAL(timeout()), this, SLOT(sessionTimerEvent()) );
    m_cleanupTimer.start( 60000 );

#ifdef SUPERVERBOSE
    qDebug( "HttpSessionStore: Sessions expire after %i milliseconds", m_expirationTime );
#endif
}

HttpSessionStore::~HttpSessionStore() {
    m_cleanupTimer.stop();
}

QByteArray HttpSessionStore::getSessionId( const HttpRequest& request, HttpResponse& response ) {
    // The session ID in the response has priority because this one will be used in the next request.
    m_mutex.lock();
    // Get the session ID from the response cookie
    QByteArray sessionId = response.getCookies().value( m_cookieName ).getValue();
    if ( sessionId.isEmpty() ) {
        // Get the session ID from the request cookie
        sessionId = request.getCookie( m_cookieName );
    }
    // Clear the session ID if there is no such session in the storage.
    if ( !sessionId.isEmpty() ) {
        if ( !sessions.contains( sessionId ) ) {
#ifdef SUPERVERBOSE
            qDebug( "HttpSessionStore: received invalid session cookie with ID %s", sessionId.data() );
#endif
            sessionId.clear();
        }
    }
    m_mutex.unlock();
    return sessionId;
}

HttpSession HttpSessionStore::getSession( const HttpRequest& request, HttpResponse& response, bool allowCreate ) {
    QByteArray sessionId = getSessionId( request, response );
    m_mutex.lock();
    if ( !sessionId.isEmpty() ) {
        HttpSession session = sessions.value( sessionId );
        if ( !session.isNull() ) {
            m_mutex.unlock();
            // Refresh the session cookie
            QByteArray cookieName = m_settings->value( "cookieName", "sessionid" ).toByteArray();
            QByteArray cookiePath = m_settings->value( "cookiePath" ).toByteArray();
            QByteArray cookieComment = m_settings->value( "cookieComment" ).toByteArray();
            QByteArray cookieDomain = m_settings->value( "cookieDomain" ).toByteArray();
            response.setCookie( HttpCookie( cookieName, session.getId(), m_expirationTime / 1000,
                                            cookiePath, cookieComment, cookieDomain, false, false, "Lax" ) );
            session.setLastAccess();
            return session;
        }
    }
    // Need to create a new session
    if ( allowCreate ) {
        QByteArray cookieName = m_settings->value( "cookieName", "sessionid" ).toByteArray();
        QByteArray cookiePath = m_settings->value( "cookiePath" ).toByteArray();
        QByteArray cookieComment = m_settings->value( "cookieComment" ).toByteArray();
        QByteArray cookieDomain = m_settings->value( "cookieDomain" ).toByteArray();
        HttpSession session( true );
#ifdef SUPERVERBOSE
        qDebug( "HttpSessionStore: create new session with ID %s", session.getId().data() );
#endif
        sessions.insert( session.getId(), session );
        response.setCookie( HttpCookie( cookieName, session.getId(), m_expirationTime / 1000,
                                        cookiePath, cookieComment, cookieDomain, false, false, "Lax" ) );
        m_mutex.unlock();
        return session;
    }
    // Return a null session
    m_mutex.unlock();
    return HttpSession();
}

HttpSession HttpSessionStore::getSession( const QByteArray& id ) {
    m_mutex.lock();
    HttpSession session = sessions.value( id );
    m_mutex.unlock();
    session.setLastAccess();
    return session;
}

void HttpSessionStore::sessionTimerEvent() {
    m_mutex.lock();
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    QMap<QByteArray, HttpSession>::iterator i = sessions.begin();
    while ( i != sessions.end() ) {
        QMap<QByteArray, HttpSession>::iterator prev = i;
        ++i;
        HttpSession session = prev.value();
        qint64 lastAccess = session.getLastAccess();
        if ( ( now - lastAccess ) > m_expirationTime ) {
#ifdef SUPERVERBOSE
            qDebug( "HttpSessionStore: session %s expired", session.getId().data() );
#endif
            emit sessionDeleted( session.getId() );
            sessions.erase( prev );
        }
    }
    m_mutex.unlock();
}

/** Delete a session */
void HttpSessionStore::removeSession( const HttpSession& session ) {
    m_mutex.lock();
    emit sessionDeleted( session.getId() );
    sessions.remove( session.getId() );
    m_mutex.unlock();
}
