/**
   @file
   @author Stefan Frings
 */

#include "httpsession.h"
#include <QDateTime>
#include <QUuid>

using namespace stefanfrings;

HttpSession::HttpSession( bool canStore ) {
    if ( canStore ) {
        m_dataPtr = new HttpSessionData();
        m_dataPtr->refCount=1;
        m_dataPtr->lastAccess=QDateTime::currentMSecsSinceEpoch();
        m_dataPtr->id=QUuid::createUuid().toString().toLocal8Bit();
#ifdef SUPERVERBOSE
        qDebug( "HttpSession: (constructor) new session %s with refCount=1", m_dataPtr->id.constData() );
#endif
    } else {
        m_dataPtr = nullptr;
    }
}

HttpSession::HttpSession( const HttpSession& other ) {
    m_dataPtr = other.m_dataPtr;
    if ( m_dataPtr ) {
        m_dataPtr->lock.lockForWrite();
        m_dataPtr->refCount++;
#ifdef SUPERVERBOSE
        qDebug( "HttpSession: (constructor) copy session %s refCount=%i", m_dataPtr->id.constData(), m_dataPtr->refCount );
#endif
        m_dataPtr->lock.unlock();
    }
}

HttpSession& HttpSession::operator= ( const HttpSession& other ) {
    HttpSessionData* oldPtr = m_dataPtr;
    m_dataPtr = other.m_dataPtr;
    if ( m_dataPtr ) {
        m_dataPtr->lock.lockForWrite();
        m_dataPtr->refCount++;
#ifdef SUPERVERBOSE
        qDebug( "HttpSession: (operator=) session %s refCount=%i", m_dataPtr->id.constData(), m_dataPtr->refCount );
#endif
        m_dataPtr->lastAccess=QDateTime::currentMSecsSinceEpoch();
        m_dataPtr->lock.unlock();
    }
    if ( oldPtr ) {
        int refCount;
        oldPtr->lock.lockForWrite();
        refCount = --oldPtr->refCount;
#ifdef SUPERVERBOSE
        qDebug( "HttpSession: (operator=) session %s refCount=%i", oldPtr->id.constData(), oldPtr->refCount );
#endif
        oldPtr->lock.unlock();
        if ( refCount == 0 ) {
            qDebug( "HttpSession: deleting old data" );
            delete oldPtr;
        }
    }
    return *this;
}

HttpSession::~HttpSession() {
    if ( m_dataPtr ) {
        int refCount;
        m_dataPtr->lock.lockForWrite();
        refCount = --m_dataPtr->refCount;
#ifdef SUPERVERBOSE
        qDebug( "HttpSession: (destructor) session %s refCount=%i", dataPtr->id.constData(), dataPtr->refCount );
#endif
        m_dataPtr->lock.unlock();
        if ( refCount == 0 ) {
#ifdef SUPERVERBOSE
            qDebug( "HttpSession: deleting data" );
#endif
            delete m_dataPtr;
        }
    }
}

QByteArray HttpSession::getId() const {
    if ( m_dataPtr ) {
        return m_dataPtr->id;
    }

    return QByteArray();
}

bool HttpSession::isNull() const {
    return m_dataPtr == nullptr;
}

void HttpSession::set( const QByteArray& key, const QVariant& value ) {
    if ( m_dataPtr ) {
        m_dataPtr->lock.lockForWrite();
        m_dataPtr->values.insert( key, value );
        m_dataPtr->lock.unlock();
    }
}

void HttpSession::remove( const QByteArray& key ) {
    if ( m_dataPtr ) {
        m_dataPtr->lock.lockForWrite();
        m_dataPtr->values.remove( key );
        m_dataPtr->lock.unlock();
    }
}

QVariant HttpSession::get( const QByteArray& key ) const {
    QVariant value;
    if ( m_dataPtr ) {
        m_dataPtr->lock.lockForRead();
        value = m_dataPtr->values.value( key );
        m_dataPtr->lock.unlock();
    }

    return value;
}

bool HttpSession::contains( const QByteArray& key ) const {
    bool found=false;
    if ( m_dataPtr ) {
        m_dataPtr->lock.lockForRead();
        found = m_dataPtr->values.contains( key );
        m_dataPtr->lock.unlock();
    }

    return found;
}

QMap<QByteArray, QVariant> HttpSession::getAll() const {
    QMap<QByteArray, QVariant> values;
    if ( m_dataPtr ) {
        m_dataPtr->lock.lockForRead();
        values = m_dataPtr->values;
        m_dataPtr->lock.unlock();
    }

    return values;
}

qint64 HttpSession::getLastAccess() const {
    qint64 value = 0;
    if ( m_dataPtr ) {
        m_dataPtr->lock.lockForRead();
        value = m_dataPtr->lastAccess;
        m_dataPtr->lock.unlock();
    }

    return value;
}

void HttpSession::setLastAccess() {
    if ( m_dataPtr ) {
        m_dataPtr->lock.lockForWrite();
        m_dataPtr->lastAccess = QDateTime::currentMSecsSinceEpoch();
        m_dataPtr->lock.unlock();
    }
}
