#ifndef MONGO_H
#define MONGO_H

#include <QString>

class MongoClient;
class Mongo {
    struct Data;

public:
    ~Mongo();
    Mongo( Mongo& ) = delete;
    Mongo operator=( Mongo ) = delete;

    static inline Mongo& instance() { return mongo; }

    bool start( const QString& uri, const QString& database = "", const QString& collection = "" );

    unsigned lastError() const;
    QString lastErrorString() const;

    MongoClient* getClient();
private:
    static Mongo mongo;
    Data* d;

    Mongo();

};

#endif // MONGO_H
