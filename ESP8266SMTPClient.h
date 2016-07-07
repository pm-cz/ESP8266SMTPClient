/**
 * ESP8266SMTPClient.h
 *
 * Created on: 04.07.2016
 *
 * Copyright (c) 2016 Pavel Moravec. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <StreamString.h>
#include <base64.h>

#ifndef ESP8266SMTPClient_H_
#define ESP8266SMTPClient_H_

//#define DEBUG_ESP_SMTP_CLIENT
//#define DEBUG_ESP_PORT Serial

#ifdef DEBUG_ESP_SMTP_CLIENT
#ifdef DEBUG_ESP_PORT
#define DEBUG_SMTPCLIENT(...) DEBUG_ESP_PORT.printf( __VA_ARGS__ )
#endif
#endif

#ifndef DEBUG_SMTPCLIENT
#define DEBUG_SMTPCLIENT(...)
#endif

#define SMTPCLIENT_DEFAULT_TCP_TIMEOUT (5000)

#define SMTPC_ERROR_CONNECTION_REFUSED  (-1)
#define SMTPC_ERROR_SEND_HEADER_FAILED  (-2)
#define SMTPC_ERROR_SEND_PAYLOAD_FAILED (-3)
#define SMTPC_ERROR_NOT_CONNECTED       (-4)
#define SMTPC_ERROR_CONNECTION_LOST     (-5)
#define SMTPC_ERROR_NO_STREAM           (-6)
#define SMTPC_ERROR_NO_SMTP_SERVER      (-7)
#define SMTPC_ERROR_TOO_LESS_RAM        (-8)
#define SMTPC_ERROR_STREAM_WRITE        (-10)
#define SMTPC_ERROR_READ_TIMEOUT        (-11)
#define SMTPC_ERROR_UNAUTHORIZED        (-12)
#define SMTPC_ERROR_INVALID_SENDER      (-13)
#define SMTPC_ERROR_INVALID_RECIPIENT   (-14)
#define SMTPC_ERROR_INVALID_ENVELOPE    (-15)

class SMTPClient {
    public:
        SMTPClient();
        ~SMTPClient();

        void begin(const char *server, uint16_t port, const char * smtpsFingerprint = "");
        void begin(String host, uint16_t port, String smtpsFingerprint = "");
        void end(void);

        bool connected(void);

        void setAuthorization(const char * user, const char * password);
        void setTimeout(uint16_t timeout);
        void setMailer(const char * mailer);

        int sendMessage(const char * from, const char * payload, size_t size, const char* to=NULL, const char * subject = NULL);
        int sendMessage(const char * from, String payload, const char* to=NULL, const char * subject = NULL);

        void addHeader(const String& name, const String& value, bool first = false);
        void addRecipient(const String& to);
        void addRecipient(const char* to);
        inline void clearHeaders() { _Headers = ""; }
        inline void clearRecipients() { _Recipients = ""; }
        void disconnect();

        WiFiClient * getStreamPtr(void);
        const char * getErrorMessage() { return _ErrorMessage.c_str(); }
        static String errorToString(int error);

    protected:
        const char * nl = "\r\n";

/*        struct RequestArgument {
          String key;
          String value;
        };
*/

        WiFiClient * _tcp;
        WiFiClientSecure * _tcps;

        /// request handling
        String _host;
        uint16_t _port;
        uint16_t _tcpTimeout;

        bool _smtps;
        String _smtpsFingerprint;

        String _Headers;
        String _ErrorMessage;
        String _Recipients;
        String _mailer;
        String _base64User;
        String _base64Pass;

        /// Response handling
        int _returnCode;

        int returnError(int error);
        bool connect(void);
        bool sendHeaders();
        int sendRequest(const char * request);
        int sendRequest(String &request);
        int sendAddress(String &cmd, String &address);
        int sendAddress(const char *cmd, const char * address);
        int handleResponse();
        void addRecipients(const char* to);
        void addRecipients(String& to);
};


#endif /* ESP8266SMTPClient_H_ */
