#ifndef MONGOCLIENT_H
#define MONGOCLIENT_H

#include <QString>
#include <QJsonDocument>

class MongoClient {
    struct Data;

public:
    explicit MongoClient( void* mongo, const QString& database, QString& collection );
    ~MongoClient();
    MongoClient( MongoClient& ) = delete;
    MongoClient operator=( MongoClient ) = delete;

    int lastError();
    QString lastErrorString();

    void setCollection( const QString& database, const QString& collection );

    bool find( const QJsonDocument& filter, const QJsonDocument& opts = {} );
    QJsonDocument next();

    bool insertOne( const QJsonDocument& document );
    bool updateOne( const QJsonDocument& filter, const QJsonDocument& document, const QJsonDocument& opts = {} );
    bool removeOne( const QJsonDocument& filter, const QJsonDocument& opts = {} );
    qint64 count( const QJsonDocument& filter, const QJsonDocument& opts = {} );

private:
    Data* d;
};

#endif // MONGOCLIENT_H
