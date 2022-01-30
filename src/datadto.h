/**
   @file
   @author Carlos Alves
 */

#ifndef DATADTO_H
#define DATADTO_H

#include <QObject>

class DataDTO {
    Q_GADGET
    Q_PROPERTY( QString name READ name WRITE setName )
    Q_PROPERTY( int idObject READ idObject WRITE setIdObject )

public:
    DataDTO();

    const QString& name() const;
    void setName( const QString& name );

    int idObject() const;
    void setIdObject( int idObject );

private:
    QString m_name;
    int m_idObject;
};

#endif // DATADTO_H
