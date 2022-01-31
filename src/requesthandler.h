#ifndef REQUESTHANDLER_H
#define REQUESTHANDLER_H

#include <httpserver/httprequesthandler.h>

#include "httpaddons/router.h"

using stefanfrings::HttpRequest;
using stefanfrings::HttpResponse;

class RequestHandler : public stefanfrings::HttpRequestHandler {
    Q_OBJECT

public:
    RequestHandler( QObject* parent = nullptr );

    void service( HttpRequest& request, HttpResponse& response ) override;

    void get( HttpRequest& request, HttpResponse& response );
    void address( HttpRequest& request, HttpResponse& response );
    void name ( HttpRequest& request, HttpResponse& response );
    void findName ( HttpRequest& request, HttpResponse& response );

private:
    Router _router;
    Router _testApi;
    Router _homeApi;

};

#endif // REQUESTHANDLER_H
