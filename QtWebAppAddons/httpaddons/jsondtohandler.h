/**
   @file
   @author Carlos Alves
 */

#ifndef JASONDTOHANDLER_H
#define JASONDTOHANDLER_H

#include <QObject>
#include <QMetaProperty>
#include <QJsonObject>

class JsonDtoHandler {
public:
    JsonDtoHandler();

    template<typename DTO>
    DTO* toDTO( const QJsonObject& json ) {
        DTO* dto = new DTO;
        toDTO( json, dto );
        return dto;
    }

    template<typename DTO>
    void toDTO( const QJsonObject& json, DTO* dto ) {
        const QMetaObject metaObject = DTO::staticMetaObject;
        convertToDTO( json, dto, metaObject );
    }

    template<typename DTO>
    QJsonObject toJson( DTO* dto ) {
        QJsonObject json;
        toJson( dto, json );
        return json;
    }

    template<typename DTO>
    void toJson( DTO* dto, QJsonObject& json ) const {
        const QMetaObject metaObject = DTO::staticMetaObject;
        convertToJson( dto, json, metaObject );
    }

private:
    const QMetaObject* getQObject() const;

    void convertToDTO( const QJsonObject& json, void* object, const QMetaObject& metaObject ) const;
    void convertToJson( void* object, QJsonObject& json, const QMetaObject& metaObject ) const;
};

#endif // JASONDTOHANDLER_H
