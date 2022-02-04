#include "mongo.h"

#include <QList>

#include <mongoc/mongoc.h>

#include "mongoclient.h"

Mongo Mongo::mongo;

struct Mongo::Data {
    bson_error_t error;
    mongoc_uri_t* uri = nullptr;
    mongoc_client_pool_t* pool = nullptr;
    QString database;
    QString collection;
    QList<MongoClient*> clients;
};

Mongo::Mongo() :
    d( nullptr ) {}

Mongo::~Mongo() {
    qDeleteAll( d->clients );
    if ( d->uri ) {
        mongoc_client_pool_destroy( d->pool );
        mongoc_uri_destroy( d->uri );
    }

    mongoc_cleanup();
    delete d;
}

bool Mongo::start( const QString& uri, const QString& database, const QString& collection ) {
    if ( !d ) {
        d = new Data;
        mongoc_init();
    }

    if ( d->uri ) {
        mongoc_client_pool_destroy( d->pool );
        mongoc_uri_destroy( d->uri );
    }

    d->uri = mongoc_uri_new_with_error( uri.toUtf8(), &d->error );
    if ( d->uri ) {
        d->database = database;
        d->collection = collection;
        d->pool = mongoc_client_pool_new( d->uri );
        if ( d->pool ) {
            mongoc_client_pool_set_error_api( d->pool, 2 );
        }
    }

    return static_cast<bool>( d->uri ) && static_cast<bool>( d->pool );
}

unsigned Mongo::lastError() const {
    return d->error.code;
}

QString Mongo::lastErrorString() const {
    return d->error.message;
}

MongoClient* Mongo::getClient() {
    if ( !d->uri ) {
        return nullptr;
    }

    d->clients.append( new MongoClient( d->pool, d->database, d->collection ) );
    return d->clients.last();
}
