#include "mongoclient.h"

#include <memory>

#include <QJsonObject>

#include <mongoc/mongoc.h>

#define BSON_FREE( var ) if ( var ) bson_free( var )

struct MongoClient::Data {
    bson_error_t error;
    mongoc_client_pool_t* pool = nullptr;
    mongoc_client_t* client = nullptr;
    mongoc_collection_t* collection = nullptr;
    mongoc_cursor_t* cursor = nullptr;
};

MongoClient::MongoClient( void* mongo, const QString& database, QString& collection ) :
    d( new Data ) {
    d->pool = static_cast<mongoc_client_pool_t*>( mongo );
    d->client = mongoc_client_pool_pop( d->pool );

    if ( !database.isEmpty() && !collection.isEmpty() ) {
        d->collection = mongoc_client_get_collection( d->client, database.toUtf8(), collection.toUtf8() );
    }
}

int MongoClient::lastError() {
    return d->error.domain;
}

QString MongoClient::lastErrorString() {
    return d->error.message;
}

MongoClient::~MongoClient() {
    if ( d->cursor ) {
        mongoc_cursor_destroy( d->cursor );
    }

    if ( d->collection ) {
        mongoc_collection_destroy( d->collection );
    }

    if ( d->client ) {
        mongoc_client_pool_push( d->pool, d->client );
    }

    delete d;
}

void MongoClient::setCollection( const QString& database, const QString& collection ) {
    if ( d->collection ) {
        mongoc_collection_destroy( d->collection );
    }

    d->collection = mongoc_client_get_collection( d->client, database.toUtf8(), collection.toUtf8() );
}

bool MongoClient::find( const QJsonDocument& filter, const QJsonDocument& opts ) {
    if ( !d->collection ) {
        return false;
    }

    QByteArray jsonFilter = filter.toJson( QJsonDocument::Compact );
    QByteArray jsonOpts = opts.toJson( QJsonDocument::Compact );

    bson_t* bsonFilter( bson_new_from_json( reinterpret_cast<uint8_t*>( jsonFilter.data() ), jsonFilter.size(), &d->error ) );
    if ( !bsonFilter ) {
        return false;
    }

    bson_t* bsonOpts( bson_new_from_json( reinterpret_cast<uint8_t*>( jsonOpts.data() ), -1, &d->error ) );

    if ( d->cursor ) {
        mongoc_cursor_destroy( d->cursor );
        d->cursor = nullptr;
    }

    d->cursor = mongoc_collection_find_with_opts( d->collection, bsonFilter, bsonOpts, nullptr );
    bson_free( bsonFilter );
    BSON_FREE( bsonOpts );

    return static_cast<bool>( d->cursor );
}

QJsonDocument MongoClient::next() {
    const bson_t* document;

    if ( !d->cursor ) {
        return {};
    }

    if ( !mongoc_cursor_next( d->cursor, &document ) || !document ) {
        mongoc_cursor_error( d->cursor, &d->error );
        return {};
    }

    char* json = ( bson_as_json( document, nullptr ) );
    QByteArray jsonDocument( json );
    BSON_FREE( json );

    return QJsonDocument::fromJson( jsonDocument );
}

bool MongoClient::insertOne( const QJsonDocument& document ) {
    if ( !d->collection ) {
        return false;
    }

    QByteArray jsonDocument = document.toJson( QJsonDocument::Compact );
    bson_t* bsonDocument = bson_new_from_json( reinterpret_cast<uint8_t*>( jsonDocument.data() ), jsonDocument.size(), &d->error );

    bool resp = bsonDocument && mongoc_collection_insert_one( d->collection, bsonDocument, nullptr, nullptr, &d->error );
    BSON_FREE( bsonDocument );

    return resp;
}

bool MongoClient::updateOne( const QJsonDocument& filter, const QJsonDocument& document, const QJsonDocument& opts ) {
    if ( !d->collection ) {
        return false;
    }

    QByteArray jsonFilter = filter.toJson( QJsonDocument::Compact );
    QByteArray jsonDocument = document.toJson( QJsonDocument::Compact );
    QByteArray jsonOpts = opts.toJson( QJsonDocument::Compact );

    bson_t* bsonFilter = bson_new_from_json( reinterpret_cast<uint8_t*>( jsonFilter.data() ), jsonFilter.size(), &d->error );
    bson_t* bsonDocument = bson_new_from_json( reinterpret_cast<uint8_t*>( jsonDocument.data() ), jsonDocument.size(), &d->error );
    bson_t* bsonOpts = bson_new_from_json( reinterpret_cast<uint8_t*>( jsonOpts.data() ), jsonOpts.size(), &d->error );

    bool resp = bsonFilter && bsonDocument && mongoc_collection_update_one( d->collection, bsonFilter, bsonDocument, bsonOpts, nullptr, &d->error );
    BSON_FREE( bsonFilter );
    BSON_FREE( bsonDocument );
    BSON_FREE( bsonOpts );

    return resp;
}

bool MongoClient::removeOne( const QJsonDocument& filter, const QJsonDocument& opts ) {
    if ( !d->collection ) {
        return false;
    }

    QByteArray jsonFilter = filter.toJson( QJsonDocument::Compact );
    QByteArray jsonOpts = opts.toJson( QJsonDocument::Compact );

    bson_t* bsonFilter = bson_new_from_json( reinterpret_cast<uint8_t*>( jsonFilter.data() ), jsonFilter.size(), &d->error );
    bson_t* bsonOpts = bson_new_from_json( reinterpret_cast<uint8_t*>( jsonOpts.data() ), jsonOpts.size(), &d->error );

    bool resp = bsonFilter && mongoc_collection_delete_one( d->collection, bsonFilter, bsonOpts, nullptr, &d->error );
    BSON_FREE( bsonFilter );
    BSON_FREE( bsonOpts );

    return resp;

}

qint64 MongoClient::count( const QJsonDocument& filter, const QJsonDocument& opts ) {
    if ( !d->collection ) {
        return false;
    }

    QByteArray jsonFilter = filter.toJson( QJsonDocument::Compact );
    QByteArray jsonOpts = opts.toJson( QJsonDocument::Compact );

    bson_t* bsonFilter = bson_new_from_json( reinterpret_cast<uint8_t*>( jsonFilter.data() ), jsonFilter.size(), &d->error );
    bson_t* bsonOpts = bson_new_from_json( reinterpret_cast<uint8_t*>( jsonOpts.data() ), jsonOpts.size(), &d->error );

    bool resp = bsonFilter && mongoc_collection_count_documents( d->collection, bsonFilter, bsonOpts, nullptr, nullptr, &d->error );
    BSON_FREE( bsonFilter );
    BSON_FREE( bsonOpts );

    return resp;
}
