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

#include "HttpClient.h"

using namespace ADDON;

#ifdef TARGET_WINDOWS
#pragma warning(disable:4005) // Disable "warning C4005: '_WINSOCKAPI_' : macro redefinition"
#include <winsock2.h>
#pragma warning(default:4005)
#define close(s) closesocket(s)
#else
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#endif

#define SEND_RQ(MSG) \
  /*cout<<send_str;*/ \
  send(sock,MSG,strlen(MSG),0);

HttpRequest::HttpRequest( const std::string& url, const std::string& method )
    : m_url( url ), m_method( method ), m_contentType( "" ), m_requestData( "" )
{
}

HttpRequest::~HttpRequest()
{
}

std::string& HttpRequest::GetUrl()
{
    return m_url;
}

std::string& HttpRequest::GetMethod()
{
    return m_method;
}

std::string& HttpRequest::GetContentType()
{
    return m_contentType;
}

std::string& HttpRequest::GetRequestData()
{
    return m_requestData;
}

void HttpRequest::SetRequestData( const std::string& data, const std::string& contentType )
{
    m_contentType = contentType;
    m_requestData = data;
}

HttpResponse::HttpResponse( const int statusCode, const std::string& responseData )
    : m_statusCode( statusCode ),
    m_responseData( responseData )
{
}

HttpResponse::~HttpResponse()
{

}

int HttpResponse::GetStatusCode()
{
    return m_statusCode;
}

std::string& HttpResponse::GetResponseData()
{
    return m_responseData;
}

HttpClient::HttpClient( const std::string& server, const int serverport )
{
    m_server = server;
    m_serverport = serverport;
}

bool HttpClient::Request( HttpRequest& request )
{
    m_lastReqeuestErrorCode = SendRequest( request );
    return m_lastReqeuestErrorCode == 200;
}

int HttpClient::SendRequest(HttpRequest& request)
{
    int ret_code = -100;
    std::string buffer;
    std::string message;
    char content_header[256];
    const std::string& method = request.GetMethod();

    sprintf( content_header, "%s %s HTTP/1.0\r\n", method.c_str(), request.GetUrl().c_str() );
    buffer.append( content_header );
    sprintf( content_header, "Host: %s:%d\r\n", m_server.c_str(), (int) m_serverport );
    buffer.append( content_header );
    //buffer.append( "Range: bytes=0-\r\n" );
    buffer.append( "Accept: */*\r\n" );
    //buffer.append( "Accept-Charset: UTF-8,*;q=0.8\r\n" );

    if ( ( method == HTTP_POST_METHOD ) || ( method == HTTP_PUT_METHOD ) )
    {
        const std::string& data = request.GetRequestData();
        sprintf( content_header, "Content-Type: %s\r\n", request.GetContentType().c_str() );
        buffer.append( content_header );
        sprintf( content_header, "Content-Length: %ld\r\n", data.size() );
        buffer.append( content_header );
        buffer.append( "\r\n" );
        buffer.append( data );
    }
    else
    {
        buffer.append( "\r\n" );
    }

#ifdef TARGET_WINDOWS
    {
        WSADATA WsaData;
        WSAStartup(0x0101, &WsaData);
    }
#endif

    sockaddr_in sin;
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1)
    {
        return -100;
    }
    sin.sin_family = AF_INET;
    sin.sin_port = htons((unsigned short) m_serverport);

    struct hostent * host_addr = gethostbyname(m_server.c_str());
    if (host_addr == NULL)
    {
        return -103;
    }
    sin.sin_addr.s_addr = *((int*) *host_addr->h_addr_list);

    if (connect(sock, (const struct sockaddr *) &sin, sizeof(sockaddr_in)) == -1)
    {
        return -101;
    }

    SEND_RQ(buffer.c_str());

    const int read_buffer_size = 4096;
    char read_buffer[read_buffer_size];
    int read_size = 0;
    std::string response;
    while ((read_size = recv(sock, read_buffer, read_buffer_size, 0)) > 0)
        response.append(read_buffer, read_buffer + read_size);

    close(sock);

    if (response.size() > 0)
    {
        //header
        std::string::size_type n = response.find("\r\n");
        if (n != std::string::npos)
        {
            std::string header = response.substr(0, n);
            if (header.find("200 OK") != std::string::npos)
                ret_code = 200;
            if (header.find("401 Unauthorized") != std::string::npos)
                ret_code = -401;

            if (ret_code == 200)
            {
                //body
                const char* body_sep = "\r\n\r\n";
                n = response.find(body_sep);
                if (n != std::string::npos)
                {
                    m_responseData.assign(response.c_str() + n + strlen(body_sep));
                }
                else
                {
                    ret_code = -105;
                }
            }
        }
        else
        {
            ret_code = -104;
        }
    }
    else
    {
        ret_code = -102;
    }

    return ret_code;
}

HttpResponse* HttpClient::GetResponse()
{
    if (m_lastReqeuestErrorCode != 200)
    {
        return NULL;
    }
    HttpResponse* response = new HttpResponse(200, m_responseData);
    return response;
}

int HttpClient::GetLastError()
{
    return m_lastReqeuestErrorCode;
}
