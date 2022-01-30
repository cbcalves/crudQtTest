/**
   @file
   @author Carlos Alves
 */

#include "datadto.h"

DataDTO::DataDTO() :
    m_idObject( 0 ) {}

const QString& DataDTO::name() const {
    return m_name;
}

void DataDTO::setName( const QString& newName ) {
    m_name = newName;
}

int DataDTO::idObject() const {
    return m_idObject;
}

void DataDTO::setIdObject( int idObject ) {
    m_idObject = idObject;
}
