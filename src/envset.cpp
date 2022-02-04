#include "envset.h"

#include <QProcessEnvironment>
#include <QSettings>
#include <QFile>

EnvSet EnvSet::m_envSet;

EnvSet::EnvSet() :
    m_env( QFile::exists( ".env" ) ) {}

QVariant EnvSet::value( const QString& key, const QVariant& defaultValue ) {
    QVariant value = defaultValue;

    if ( QProcessEnvironment::systemEnvironment().contains( key ) ) {
        value = QProcessEnvironment::systemEnvironment().value( key );
    } else if ( m_envSet.m_env ) {
        QSettings env( ".env", QSettings::IniFormat );
        value = env.value( key );
    }

    return value;
}
