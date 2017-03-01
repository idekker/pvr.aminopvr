/*
*  This file is part of AminoPVR.
*  Copyright (C) 2012-2017  Ino Dekker
*
*  This program is free software: you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation, either version 3 of the License, or
*  (at your option) any later version.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
*/

#pragma once

#include "libXBMC_addon.h"

/**
* A constant string representing the HTTP method for requests.
*/
const std::string HTTP_GET_METHOD = "GET";
const std::string HTTP_POST_METHOD = "POST";
const std::string HTTP_PUT_METHOD = "PUT";
const std::string HTTP_DELETE_METHOD = "DELETE";

/**
* A constant string representing the name of the HTTP header Accept.
*/
const std::string HTTP_HEADER_ACCEPT = "Accept";

/**
* A constant string representing the name of the HTTP header Accept-Charset.
*/
const std::string HTTP_HEADER_ACCEPT_CHARSET = "Accept-Charset";

/**
* A constant string representing the name of the HTTP header Content-Type.
*/
const std::string HTTP_HEADER_CONTENT_TYPE = "Content-Type";
const std::string HTTP_CONTENT_TYPE_APPLICATION_JSON = "application/json";

/**
* Class for defining a HTTP web request.
* This is used as input parameter for the HttpClient::SendRequest() method.
* @see HttpClient::SendRequest()
*/
class HttpRequest
{
public:
    /**
    * Initializes a new instance of the dvblinkremotehttp::HttpWebRequest class.
    * @param url A constant string representing the URL for which the request will be sent.
    */
    HttpRequest( const std::string& url, const std::string& method = HTTP_GET_METHOD );

    /**
    * Destructor for cleaning up allocated memory.
    */
    ~HttpRequest();

    /**
    * Gets the URL.
    * @return A string reference
    */
    std::string& GetUrl();

    /**
    * Gets the method.
    * @return A string reference
    */
    std::string& GetMethod();

    /**
    * Gets the data to be sent in the request.
    * @return A string reference
    */
    std::string& GetRequestData();

    /**
    * Gets the content type to be set in the request.
    * @return A string reference
    */
    std::string& GetContentType();

    /**
    * Sets the data to be sent in the request (using POST method).
    * @param data A constant string reference representing the data to be sent in the request.
    */
    void SetRequestData( const std::string& data, const std::string& contentType = HTTP_CONTENT_TYPE_APPLICATION_JSON );

private:
    std::string m_url;
    std::string m_method;
    std::string m_contentType;
    std::string m_requestData;
};

/**
* Class for defining a HTTP web response.
* This is used as return parameter for the HttpClient::GetResponse method.
* @see HttpClient::GetResponse()
*/
class HttpResponse
{
public:
    /**
    * Initializes a new instance of the dvblinkremotehttp::HttpWebResponse class.
    * @param statusCode   A constant integer representing the HTTP response code.
    * @param responseData A constant string reference representing the HTTP response data.
    */
    HttpResponse( const int statusCode, const std::string& responseData );

    /**
    * Destructor for cleaning up allocated memory.
    */
    ~HttpResponse();

    /**
    * Gets the HTTP response code.
    * @return An integer value
    */
    int GetStatusCode();

    /**
    * Gets the HTTP response data.
    * @return A string reference
    */
    std::string& GetResponseData();

private:
    int m_statusCode;
    std::string m_responseData;
};

class HttpClient
{
    public:
        HttpClient( const std::string& server, const int serverport );

        bool Request( HttpRequest& request );
        HttpResponse* GetResponse();
        int GetLastError();

    private:
        int SendRequest( HttpRequest& request );
        std::string m_server;
        long m_serverport;
        ADDON::CHelper_libXBMC_addon *XBMC;
        std::string m_responseData;
        int m_lastReqeuestErrorCode;

};
