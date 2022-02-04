#include "jsondtohandler.h"

const QMetaObject* JsonDtoHandler::getQObject() {
#if ( QT_VERSION >= QT_VERSION_CHECK( 6, 0, 0 ) )
    const QMetaType metaType( QMetaType::fromName( "QObject" ) );
#else
    const QMetaType metaType( QMetaType::type( "QObject" ) );
#endif

    return metaType.metaObject();
}

void JsonDtoHandler::convertToDTO( const QJsonObject& json, void* object, const QMetaObject& metaObject ) {
    for ( int i = 0; i < metaObject.propertyCount(); ++i ) {
        QMetaProperty metaProperty = metaObject.property( i );

        if ( metaProperty.isWritable() ) {
            QString name = metaProperty.name();
            if ( json.contains( name ) ) {
                QVariant value = json.value( name );
                value.convert( metaProperty.type() );

                if ( metaObject.inherits( getQObject() ) ) {
                    metaProperty.write( static_cast<QObject*>( object ), value );
                } else {
                    metaProperty.writeOnGadget( object, value );
                }
            }
        }
    }
}

void JsonDtoHandler::convertToJson( void* object, QJsonObject& json, const QMetaObject& metaObject ) {
    for ( int i = 0; i < metaObject.propertyCount(); ++i ) {
        QMetaProperty metaProperty = metaObject.property( i );

        if ( metaProperty.isReadable() ) {
            QString name = metaProperty.name();

            QVariant value;
            if ( metaObject.inherits( getQObject() ) ) {
                value = metaProperty.read( static_cast<QObject*>( object ) );
            } else {
                value = metaProperty.readOnGadget( object );
            }

            json.insert( name, value.toJsonValue() );
        }
    }
}
