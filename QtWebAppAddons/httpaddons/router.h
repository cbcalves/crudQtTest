/**
   @file
   @author Carlos Alves
 */

#ifndef ROUTER_H
#define ROUTER_H

#include <QObject>

#include <httpserver/httprequesthandler.h>

typedef std::function<void ( stefanfrings::HttpRequest& request, stefanfrings::HttpResponse& response )> function_t;

class Router : public stefanfrings::HttpRequestHandler {
    Q_OBJECT
    struct RouterData;
public:
    explicit Router( QObject* parent = nullptr );
    ~Router();

    enum class Method : char {
        USE = -1,
        ALL,
        GET,
        POST,
        PUT,
        PATCH,
        DELETE,
    };

    /**
     * Defines a route.
     * @param method Method to be accessed
     * @param path Incomming request path
     * @param Obj pointer to the class
     * @param member Class method to be called func(HttpRequest& request, HttpResponse& response )
     **/
    template <typename Obj, typename Func>
    inline void route( Router::Method method, const QString& path, Obj* obj, Func&& member ) {
        insert( method, path, std::bind( member, obj, std::placeholders::_1, std::placeholders::_2 ) );
    }

    /**
     * Defines a route
     * @param method Method to be accessed
     * @param path Incomming request path
     * @param function Function to be called func(HttpRequest& request, HttpResponse& response )
     **/
    inline void route( Router::Method method, const QString& path, function_t function ) {
        insert( method, path, function );
    }

    /**
     * Recieve any method.
     * @param path Incomming request path
     * @param Obj pointer to the class
     * @param member Class method to be called func(HttpRequest& request, HttpResponse& response )
     **/
    template <typename Obj, typename Func>
    inline void allRequest( const QString& path, Obj* obj, Func&& member ) {
        insert( Method::ALL, path, std::bind( member, obj, std::placeholders::_1, std::placeholders::_2 ) );
    }
    /**
     * Recieve any method.
     * @param method Method to be accessed
     * @param path Path called
     * @param function Function to be called func(HttpRequest& request, HttpResponse& response )
     **/
    inline void allRequest( const QString& path, function_t function ) {
        insert( Method::ALL, path, function );
    }

    /**
     * Recieve get request.
     * @param path Incomming request path
     * @param Obj pointer to the class
     * @param member Class method to be called func(HttpRequest& request, HttpResponse& response )
     **/
    template <typename Obj, typename Func>
    inline void getRequest( const QString& path, Obj* obj, Func&& member ) {
        insert( Method::GET, path, std::bind( member, obj, std::placeholders::_1, std::placeholders::_2 ) );
    }
    /**
     * Recieve get request.
     * @param method Method to be accessed
     * @param path Incomming request path
     * @param function Function to be called func(HttpRequest& request, HttpResponse& response )
     **/
    inline void getRequest( const QString& path, function_t function ) {
        insert( Method::GET, path, function );
    }

    /**
     * Recieve post request.
     * @param path Incomming request path
     * @param Obj pointer to the class
     * @param member Class method to be called func(HttpRequest& request, HttpResponse& response )
     **/
    template <typename Obj, typename Func>
    inline void postRequest( const QString& path, Obj* obj, Func&& member ) {
        insert( Method::POST, path, std::bind( member, obj, std::placeholders::_1, std::placeholders::_2 ) );
    }
    /**
     * Recieve post request.
     * @param method Method to be accessed
     * @param path Incomming request path
     * @param function Function to be called func(HttpRequest& request, HttpResponse& response )
     **/
    inline void postRequest( const QString& path, function_t function ) {
        insert( Method::POST, path, function );
    }

    /**
     * Recieve put request.
     * @param path Incomming request path
     * @param Obj pointer to the class
     * @param member Class method to be called func(HttpRequest& request, HttpResponse& response )
     **/
    template <typename Obj, typename Func>
    inline void putRequest( const QString& path, Obj* obj, Func&& member ) {
        insert( Method::PUT, path, std::bind( member, obj, std::placeholders::_1, std::placeholders::_2 ) );
    }
    /**
     * Recieve put request.
     * @param method Method to be accessed
     * @param path Incomming request path
     * @param function Function to be called func(HttpRequest& request, HttpResponse& response )
     **/
    inline void putRequest( const QString& path, function_t function ) {
        insert( Method::PUT, path, function );
    }

    /**
     * Recieve patch request.
     * @param path Incomming request path
     * @param Obj pointer to the class
     * @param member Class method to be called func(HttpRequest& request, HttpResponse& response )
     **/
    template <typename Obj, typename Func>
    inline void patchRequest( const QString& path, Obj* obj, Func&& member ) {
        insert( Method::PATCH, path, std::bind( member, obj, std::placeholders::_1, std::placeholders::_2 ) );
    }
    /**
     * Recieve patch request.
     * @param method Method to be accessed
     * @param path Incomming request path
     * @param function Function to be called func(HttpRequest& request, HttpResponse& response )
     **/
    inline void patchRequest( const QString& path, function_t function ) {
        insert( Method::PATCH, path, function );
    }

    /**
     * Recieve delete request.
     * @param path Incomming request path
     * @param Obj pointer to the class
     * @param member Class method to be called func(HttpRequest& request, HttpResponse& response )
     **/
    template <typename Obj, typename Func>
    inline void deleteRequest( const QString& path, Obj* obj, Func&& member ) {
        insert( Method::DELETE, path, std::bind( member, obj, std::placeholders::_1, std::placeholders::_2 ) );
    }
    /**
     * Recieve delete request.
     * @param method Method to be accessed
     * @param path Incomming request path
     * @param function Function to be called func(HttpRequest& request, HttpResponse& response )
     **/
    inline void deleteRequest( const QString& path, function_t function ) {
        insert( Method::DELETE, path, function );
    }

    /**
     * Use another router for the incomming path.
     * @param path Incomming request path
     * @param router Router to be called
     **/
    inline void use( const QString& path, Router* router ) {
        insert( Method::USE, path, std::bind( &Router::service, router, std::placeholders::_1, std::placeholders::_2 ) );
        router->_usePath = _usePath + path;
    }

    /**
     * Process an incoming HTTP request.
     * @param request The received HTTP request
     * @param response Must be used to return the response
     **/
    void service( stefanfrings::HttpRequest& request, stefanfrings::HttpResponse& response ) override;

    /**
     * Extract the last path parameter.
     * @param path The path used to make the request
     **/
    static QString pathParam( const QString& path );

private:
    QString _usePath;
    RouterData* _d;

    void insert( Method method, const QString& path, function_t func );
    function_t findRoute( const std::array<Method, 3>& methods, const QString& path );

};

#endif // ROUTER_H
