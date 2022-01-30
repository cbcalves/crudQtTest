/**
   @file
   @author Stefan Frings
 */

#ifndef HTTPREQUEST_H
#define HTTPREQUEST_H

#include <QByteArray>
#include <QHostAddress>
#include <QTcpSocket>
#include <QMap>
#include <QMultiMap>
#include <QSettings>
#include <QTemporaryFile>
#include <QUuid>
#include "httpglobal.h"

namespace stefanfrings {

/**
   This object represents a single HTTP request. It reads the request
   from a TCP socket and provides getters for the individual parts
   of the request.
   <p>
   The follwing config settings are required:
   <code><pre>
   maxRequestSize=16000
   maxMultiPartSize=1000000
   </pre></code>
   <p>
   MaxRequestSize is the maximum size of a HTTP request. In case of
   multipart/form-data requests (also known as file-upload), the maximum
   size of the body must not exceed maxMultiPartSize.
   The body is always a little larger than the file itself.
 */

class DECLSPEC HttpRequest {
    Q_DISABLE_COPY( HttpRequest )
    friend class HttpSessionStore;

public:
    /** Values for getStatus() */
    enum RequestStatus {WAIT_FOR_REQUEST, WAIT_FOR_HEADER, WAIT_FOR_BODY, COMPLETE, ABORT};

    /**
       Constructor.
       @param settings Configuration settings
     */
    explicit HttpRequest( const QSettings* settings );

    /**
       Destructor.
     */
    virtual ~HttpRequest();

    /**
       Read the HTTP request from a socket.
       This method is called by the connection handler repeatedly
       until the status is RequestStatus::complete or RequestStatus::abort.
       @param socket Source of the data
     */
    void readFromSocket( QTcpSocket* socket );

    /**
       Get the status of this reqeust.
       @see RequestStatus
     */
    RequestStatus getStatus() const;

    /** Get the method of the HTTP request  (e.g. "GET") */
    const QByteArray& getMethod() const;

    /** Get the decoded path of the HTPP request (e.g. "/index.html") */
    QByteArray getPath() const;

    /** Get the raw path of the HTTP request (e.g. "/file%20with%20spaces.html") */
    const QByteArray& getRawPath() const;

    /** Get the version of the HTPP request (e.g. "HTTP/1.1") */
    QByteArray getVersion() const;

    /**
       Get the value of a HTTP request header.
       @param name Name of the header, not case-senitive.
       @return If the header occurs multiple times, only the last
       one is returned.
     */
    QByteArray getHeader( const QByteArray& name ) const;

    /**
       Get the values of a HTTP request header.
       @param name Name of the header, not case-senitive.
     */
    QList<QByteArray> getHeaders( const QByteArray& name ) const;

    /**
     * Get all HTTP request headers. Note that the header names
     * are returned in lower-case.
     */
    const QMultiMap<QByteArray, QByteArray>& getHeaderMap() const;

    /**
       Get the value of a HTTP request parameter.
       @param name Name of the parameter, case-sensitive.
       @return If the parameter occurs multiple times, only the last
       one is returned.
     */
    QByteArray getParameter( const QByteArray& name ) const;

    /**
       Get the values of a HTTP request parameter.
       @param name Name of the parameter, case-sensitive.
     */
    QList<QByteArray> getParameters( const QByteArray& name ) const;

    /** Get all HTTP request parameters. */
    const QMultiMap<QByteArray, QByteArray>& getParameterMap() const;

    /** Get the HTTP request body.  */
    const QByteArray& getBody() const;

    /**
       Decode an URL parameter.
       E.g. replace "%23" by '#' and replace '+' by ' '.
       @param source The url encoded strings
       @see QUrl::toPercentEncoding for the reverse direction
     */
    static QByteArray urlDecode( const QByteArray& source );

    /**
       Get an uploaded file. The file is already open. It will
       be closed and deleted by the destructor of this HttpRequest
       object (after processing the request).
       <p>
       For uploaded files, the method getParameters() returns
       the original fileName as provided by the calling web browser.
     */
    QTemporaryFile* getUploadedFile( const QByteArray& fieldName ) const;

    /**
       Get the value of a cookie.
       @param name Name of the cookie
     */
    QByteArray getCookie( const QByteArray& name ) const;

    /** Get all cookies. */
    const QMap<QByteArray, QByteArray>& getCookieMap();

    /**
       Get the address of the connected client.
       Note that multiple clients may have the same IP address, if they
       share an internet connection (which is very common).
     */
    const QHostAddress& getPeerAddress() const;

private:
    /** Request headers */
    QMultiMap<QByteArray, QByteArray> m_headers;

    /** Parameters of the request */
    QMultiMap<QByteArray, QByteArray> m_parameters;

    /** Uploaded files of the request, key is the field name. */
    QMap<QByteArray, QTemporaryFile*> m_uploadedFiles;

    /** Received cookies */
    QMap<QByteArray, QByteArray> m_cookies;

    /** Storage for raw body data */
    QByteArray m_bodyData;

    /** Request method */
    QByteArray m_method;

    /** Request path (in raw encoded format) */
    QByteArray m_path;

    /** Request protocol version */
    QByteArray m_version;

    /**
       Status of this request. For the state engine.
       @see RequestStatus
     */
    RequestStatus m_status;

    /** Address of the connected peer. */
    QHostAddress m_peerAddress;

    /** Maximum size of requests in bytes. */
    int m_maxSize;

    /** Maximum allowed size of multipart forms in bytes. */
    int m_maxMultiPartSize;

    /** Current size */
    int m_currentSize;

    /** Expected size of body */
    int m_expectedBodySize;

    /** Name of the current header, or empty if no header is being processed */
    QByteArray m_currentHeader;

    /** Boundary of multipart/form-data body. Empty if there is no such header */
    QByteArray m_boundary;

    /** Temp file, that is used to store the multipart/form-data body */
    QTemporaryFile* m_tempFile;

    /** Parse the multipart body, that has been stored in the temp file. */
    void parseMultiPartFile();

    /** Sub-procedure of readFromSocket(), read the first line of a request. */
    void readRequest( QTcpSocket* socket );

    /** Sub-procedure of readFromSocket(), read header lines. */
    void readHeader( QTcpSocket* socket );

    /** Sub-procedure of readFromSocket(), read the request body. */
    void readBody( QTcpSocket* socket );

    /** Sub-procedure of readFromSocket(), extract and decode request parameters. */
    void decodeRequestParams();

    /** Sub-procedure of readFromSocket(), extract cookies from headers */
    void extractCookies();

    /** Buffer for collecting characters of request and header lines */
    QByteArray m_lineBuffer;

};

} // end of namespace

#endif // HTTPREQUEST_H
