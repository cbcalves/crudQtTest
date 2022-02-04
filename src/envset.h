#ifndef ENVSET_H
#define ENVSET_H

#include <QVariant>

class EnvSet {

public:
    EnvSet( const EnvSet& ) = delete;
    EnvSet& operator=( const EnvSet& ) = delete;

    static QVariant value( const QString& key, const QVariant& defaultValue = {} );

private:
    static EnvSet m_envSet;
    bool m_env;

    EnvSet();

};

#endif // ENVSET_H
