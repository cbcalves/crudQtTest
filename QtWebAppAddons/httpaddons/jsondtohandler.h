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
    template<typename DTO>
    static DTO* toDTO( const QJsonObject& json ) {
        DTO* dto = new DTO;
        toDTO( json, dto );
        return dto;
    }

    template<typename DTO>
    static void toDTO( const QJsonObject& json, DTO* dto ) {
        const QMetaObject metaObject = DTO::staticMetaObject;
        convertToDTO( json, dto, metaObject );
    }

    template<typename DTO>
    static QJsonObject toJson( DTO* dto ) {
        QJsonObject json;
        toJson( dto, json );
        return json;
    }

    template<typename DTO>
    static void toJson( DTO* dto, QJsonObject& json ) {
        const QMetaObject metaObject = DTO::staticMetaObject;
        convertToJson( dto, json, metaObject );
    }

private:
    static const QMetaObject* getQObject();

    static void convertToDTO( const QJsonObject& json, void* object, const QMetaObject& metaObject );
    static void convertToJson( void* object, QJsonObject& json, const QMetaObject& metaObject );
};

#endif // JASONDTOHANDLER_H
