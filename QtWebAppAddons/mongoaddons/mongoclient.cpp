#include "mongoclient.h"

#include <memory>

#include <QJsonObject>

#include <mongoc/mongoc.h>

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
        mongoc_client_pool_push ( d->pool, d->client );
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

    QByteArray jsonOpts = {};
    QByteArray jsonFilter = filter.toJson( QJsonDocument::Compact );
    if ( !opts.isEmpty() ) {
        jsonOpts = opts.toJson( QJsonDocument::Compact );
    }

    bson_t* bsonFilter( bson_new_from_json( reinterpret_cast<uint8_t*>( jsonFilter.data() ), jsonFilter.size(), &d->error ) );
    bson_t* bsonOpts( bson_new_from_json( reinterpret_cast<uint8_t*>( jsonOpts.data() ), -1, &d->error ) );
    if ( !bsonFilter ) {
        return false;
    }

    if ( d->cursor ) {
        mongoc_cursor_destroy( d->cursor );
        d->cursor = nullptr;
    }

    d->cursor = mongoc_collection_find_with_opts( d->collection, bsonFilter, bsonOpts, nullptr );
    bson_free( bsonFilter );
    bson_free( bsonOpts );

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
    bson_free( json );

    return QJsonDocument::fromJson( jsonDocument );
}

bool MongoClient::insertOne( const QJsonDocument& document ) {
    if ( !d->collection ) {
        return false;
    }

    QByteArray jsonDocument = document.toJson( QJsonDocument::Compact );
    std::unique_ptr<_bson_t> bsonDocument( bson_new_from_json( reinterpret_cast<uint8_t*>( jsonDocument.data() ), jsonDocument.size(), &d->error ) );
    if ( !bsonDocument ) {
        return false;
    }

    return mongoc_collection_insert_one( d->collection, bsonDocument.get(), nullptr, nullptr, &d->error );
}

bool MongoClient::updateOne( const QJsonDocument& filter, const QJsonDocument& document, const QJsonDocument& opts ) {
    if ( !d->collection ) {
        return false;
    }

    QByteArray jsonFilter = filter.toJson( QJsonDocument::Compact );
    QByteArray jsonDocument = document.toJson( QJsonDocument::Compact );
    QByteArray jsonOpts = {};
    if ( !opts.isEmpty() ) {
        jsonOpts = opts.toJson( QJsonDocument::Compact );
    }

    std::unique_ptr<_bson_t> bsonFilter( bson_new_from_json( reinterpret_cast<uint8_t*>( jsonFilter.data() ), jsonFilter.size(), &d->error ) );
    std::unique_ptr<_bson_t> bsonDocument( bson_new_from_json( reinterpret_cast<uint8_t*>( jsonDocument.data() ), jsonDocument.size(), &d->error ) );
    std::unique_ptr<_bson_t> bsonOpts( bson_new_from_json( reinterpret_cast<uint8_t*>( jsonOpts.data() ), jsonOpts.size(), &d->error ) );
    if ( !bsonDocument ) {
        return false;
    }

    return mongoc_collection_update_one( d->collection, bsonFilter.get(), bsonDocument.get(), bsonOpts.get(), nullptr, &d->error );
}

bool MongoClient::removeOne( const QJsonDocument& filter, const QJsonDocument& opts ) {
    if ( !d->collection ) {
        return false;
    }

    QByteArray jsonOpts = {};
    QByteArray jsonFilter = filter.toJson( QJsonDocument::Compact );
    if ( !opts.isEmpty() ) {
        jsonOpts = opts.toJson( QJsonDocument::Compact );
    }

    std::unique_ptr<_bson_t> bsonFilter( bson_new_from_json( reinterpret_cast<uint8_t*>( jsonFilter.data() ), jsonFilter.size(), &d->error ) );
    std::unique_ptr<_bson_t> bsonOpts( bson_new_from_json( reinterpret_cast<uint8_t*>( jsonOpts.data() ), jsonOpts.size(), &d->error ) );
    if ( !bsonFilter ) {
        return false;
    }

    return mongoc_collection_delete_one( d->collection, bsonFilter.get(), bsonOpts.get(), nullptr, &d->error );

}

qint64 MongoClient::count( const QJsonDocument& filter, const QJsonDocument& opts ) {
    if ( !d->collection ) {
        return false;
    }

    QByteArray jsonOpts = {};
    QByteArray jsonFilter = filter.toJson( QJsonDocument::Compact );
    if ( !opts.isEmpty() ) {
        jsonOpts = opts.toJson( QJsonDocument::Compact );
    }

    std::unique_ptr<_bson_t> bsonFilter( bson_new_from_json( reinterpret_cast<uint8_t*>( jsonFilter.data() ), jsonFilter.size(), &d->error ) );
    std::unique_ptr<_bson_t> bsonOpts( bson_new_from_json( reinterpret_cast<uint8_t*>( jsonOpts.data() ), jsonOpts.size(), &d->error ) );
    if ( !bsonFilter ) {
        return false;
    }

    return mongoc_collection_count_documents( d->collection, bsonFilter.get(), bsonOpts.get(), nullptr, nullptr, &d->error );
}
