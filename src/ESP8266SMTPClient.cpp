/**
 * ESP8266SMTPClient.cpp
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

#include "ESP8266SMTPClient.h"

/**
 * constractor
 */
SMTPClient::SMTPClient() {
    _tcp = NULL;
    _tcps = NULL;
    _port = 0;
    _tcpTimeout = SMTPCLIENT_DEFAULT_TCP_TIMEOUT;
    _smtps = false;
    _mailer = "ESP8266SMTPClient";
    _returnCode = 0;
}

/**
 * deconstractor
 */
SMTPClient::~SMTPClient() {
    if(_tcps) {
        _tcps->stop();
        delete _tcps;
        _tcps = NULL;
        _tcp = NULL;
    } else if(_tcp) {
        _tcp->stop();
        delete _tcp;
        _tcp = NULL;
    }
}

/**
 * begin
 * @param host const char *
 * @param port uint16_t
 * @param url  const char *
 * @param smtps bool
 * @param smtpsFingerprint const char *
 */
void SMTPClient::begin(const char *host, uint16_t port, const char * smtpsFingerprint) {

    DEBUG_SMTPCLIENT("[SMTP-Client][begin] host: %s port:%d smtps: %d smtpsFingerprint: %s\n", host, port, (port == 465), smtpsFingerprint);

    _host = host;
    _port = port;
    _smtps = (port == 465);
    _smtpsFingerprint = smtpsFingerprint;

    _returnCode = 0;

    clearHeaders();
    clearRecipients();
}

void SMTPClient::begin(String host, uint16_t port, String smtpsFingerprint) {
    begin(host.c_str(), port, smtpsFingerprint.c_str());
}

/**
 * end
 * called after the payload is handled
 */
void SMTPClient::end(void) {
    if(connected()) {
        if(_tcp->available() > 0) {
            DEBUG_SMTPCLIENT("[SMTP-Client][end] still data in buffer (%d), clean up.\n", _tcp->available());
            while(_tcp->available() > 0) {
                _tcp->read();
            }
        }
    } else {
        DEBUG_SMTPCLIENT("[SMTP-Client][end] tcp is closed\n");
    }
}

/**
 * connected
 * @return connected status
 */
bool SMTPClient::connected() {
    if(_tcp) {
        return (_tcp->connected() || (_tcp->available() > 0));
    }
    return false;
}

/**
 * set X-mailer name
 * @param mailer const char *
 */
void SMTPClient::setMailer(const char * mailer) {
    _mailer = mailer;
}

/**
 * set the Authorizatio for the smtp request
 * @param user const char *
 * @param password const char *
 */
void SMTPClient::setAuthorization(const char * user, const char * password) {
    if(user && password) {
        _base64User = base64::encode(user);
        _base64Pass = base64::encode(password);
    }
}

/**
 * set the timeout for the TCP connection
 * @param timeout unsigned int
 */
void SMTPClient::setTimeout(uint16_t timeout) {
    _tcpTimeout = timeout;
    if(connected()) {
        _tcp->setTimeout(timeout);
    }
}

/**
 * sendMessage
 * @param from const char *     Sender E-mail address
 * @param payload String        data for the message body
 * @param to const char *     	Recepient E-mail address (if NULL, will use pre-set recepients)
 * @param subject const char *  Message subject (if NULL, will use pre-set header or not send subject at all)
 * @return -1 on connection error, status code when message sending
 */
int SMTPClient::sendMessage(const char * from, String & payload, const char * to, const char * subject)  {
    return sendMessage(from, (char *) payload.c_str(), payload.length(), to, subject);
}

/**
 * sendMessage
 * @param from const char *     Sender E-mail address
 * @param payload uint8_t *     data for the message body
 * @param size size_t           size for the message body if 0 not send
 * @param to const char *     	Recepient E-mail address (if NULL, will use pre-set recepients)
 * @param subject const char *  Message subject (if NULL, will use pre-set header or not send subject at all)
 * @return -1 if no info or > 0 when Content-Length is set by server
 */
int SMTPClient::sendMessage(const char * from, const char * payload, size_t size, const char * to, const char * subject) {    
    String command;
    if (size==0) { size = strlen(payload); } 

    if (!connected()) {
      if(!connect()) {
          return returnError(SMTPC_ERROR_CONNECTION_REFUSED);
      }     
    }

    addHeader("From", from);
    if (subject) { 
      String subj2 = subject;
      subj2 = "=?UTF-8?B?" + base64::encode(subj2) + "?=";
      addHeader("Subject", subj2); 
    }
    addHeader("X-Mailer", _mailer);
    if (to) { 
      addHeader("To", to);
      addRecipients(to); 
    }

    _returnCode = sendAddress("MAIL FROM: ", from);
    if (_returnCode < 0 || _returnCode >= 400) { 
      return SMTPC_ERROR_INVALID_SENDER;
    }

    size_t startP = 0, endP=_Recipients.length(), splitP=_Recipients.indexOf('\n');
    do {
       String s2;
       if (splitP < 0) {
         s2 = _Recipients.substring(startP);
         startP =-1;
       } else {
         s2 = _Recipients.substring(startP, splitP);        
         startP=splitP+1;
         splitP=_Recipients.indexOf('\n', startP);       
       }
       s2.trim();
       if (s2.indexOf('<') >=0) { s2 = s2.substring(s2.indexOf('<')); }
       if (! s2.length()) { continue; }
       _returnCode = sendAddress("RCPT TO: ", s2.c_str());
       if (_returnCode < 0 || _returnCode >= 400) { 
         return SMTPC_ERROR_INVALID_RECIPIENT;
       }           
    } while (startP > 0);
    
    _returnCode = sendRequest("DATA");
    if (_returnCode < 0 || _returnCode >= 400) { 
     return SMTPC_ERROR_INVALID_ENVELOPE;
    }           

    // send Header
    
    DEBUG_SMTPCLIENT("[SMTP-Client][sendMessage] headers: '%s'\n", _Headers.c_str());
    if(!sendHeaders()) {
        return returnError(SMTPC_ERROR_SEND_HEADER_FAILED);
    }

    // send Payload if needed
    DEBUG_SMTPCLIENT("[SMTP-Client][sendMessage] message: '%s'\n", payload);
    if(payload && size > 0) {
        char * ptr;
        static const char * alternative = "\n..";
        const int alen = strlen(alternative);
        while (ptr=strstr(payload,"\n.")) {
          size_t pos = ptr-payload;
          size -= pos + alen;
          if(_tcp->write(&payload[0], pos) != pos) {
              return returnError(SMTPC_ERROR_SEND_PAYLOAD_FAILED);
          }
          if(_tcp->write(alternative, 4) != 4) {
              return returnError(SMTPC_ERROR_SEND_PAYLOAD_FAILED);
          }
          payload = ptr + alen;
        }
        if(_tcp->write(&payload[0], size) != size) {
            return returnError(SMTPC_ERROR_SEND_PAYLOAD_FAILED);
        }
    }
	_returnCode = sendRequest("\r\n.");
    
    /* Reset recepients after sending them */
    clearRecipients();
    clearHeaders();
    return _returnCode;

}

int SMTPClient::sendAddress(String &cmd, String &address) {
  return sendAddress(cmd.c_str(), address.c_str());
}

int SMTPClient::sendAddress(const char *cmd, const char *address) {
    String command = cmd;
    String a2 = address;
    a2.trim();
    if (strchr(address, '<')) {
      command += a2;
    } else {
      command += '<';
      command += a2;
      command += '>';
    }
    _returnCode = sendRequest(command);
    if (_returnCode < 0 || _returnCode >= 400) { 
      DEBUG_SMTPCLIENT("[SMTP-Client][sendAddress] failed command: '%s'\n", command.c_str());      
      return _returnCode;
    }
  
}


/**
 * returns the stream of the tcp connection
 * @return WiFiClient *
 */
WiFiClient * SMTPClient::getStreamPtr(void) {
    if(connected()) {
        return _tcp;
    }

    DEBUG_SMTPCLIENT("[SMTP-Client] no stream to return!?\n");
    return NULL;
}

/**
 * converts error code to String
 * @param error int
 * @return String
 */
String SMTPClient::errorToString(int error) {
    switch(error) {
        case SMTPC_ERROR_CONNECTION_REFUSED:
            return String("connection refused");
        case SMTPC_ERROR_SEND_HEADER_FAILED:
            return String("send header failed");
        case SMTPC_ERROR_SEND_PAYLOAD_FAILED:
            return String("send payload failed");
        case SMTPC_ERROR_NOT_CONNECTED:
            return String("not connected");
        case SMTPC_ERROR_CONNECTION_LOST:
            return String("connection lost");
        case SMTPC_ERROR_NO_STREAM:
            return String("no stream");
        case SMTPC_ERROR_NO_SMTP_SERVER:
            return String("no SMTP server");
        case SMTPC_ERROR_TOO_LESS_RAM:
            return String("too few ram");
        case SMTPC_ERROR_STREAM_WRITE:
            return String("Stream write error");
        case SMTPC_ERROR_READ_TIMEOUT:
            return String("read Timeout");
        case SMTPC_ERROR_UNAUTHORIZED:
            return String("login failed");
        case SMTPC_ERROR_INVALID_SENDER:
            return String("invalid sender address");        
        case SMTPC_ERROR_INVALID_RECIPIENT:
            return String("invalid recepient address");        
        case SMTPC_ERROR_INVALID_ENVELOPE:
            return String("error in E-mail envelope");        
        default:
            return String();
    }
}

/**
 * adds Header to the request
 * @param name
 * @param value
 * @param first
 */
void SMTPClient::addHeader(const String& name, const String& value, bool first) {
    String headerLine = name;
    headerLine += ": ";
	headerLine += value;
	headerLine += "\r\n";

	if(first) {
		_Headers = headerLine + _Headers;
	} else {
		_Headers += headerLine;
	}
}


/**
 * adds Header to the request
 * @param to String - recepients to split
 */
void SMTPClient::addRecipients(String & to) {
	addRecipients(to.c_str());
}

/**
 * adds Header to the request
 * @param name
 * @param value
 * @param first
 */
void SMTPClient::addRecipients(const char * to) {
	uint16_t len = strlen(to);
	char * to2 = strdup(to);
	boolean mode=0;
	String line="";
	for (uint16_t i=0; i < len; i++) {
		if (to2[i] == '"') { /* Quoted string */
			mode ^= 1;
    } else if (((mode & 1) == 0) && (to2[i] == '<')) { /* Start of <>*/
      mode |= 2;
    } else if (((mode & 3) == 2) && (to2[i] == '>')) { /* End of <>*/
      mode ^= 2;
		} else if ((mode & 1) && (to2[i] == '\\')) { /* Escape characters */
			i++;
		} else if (((mode & 3) == 0) && (to2[i] == ',')) { /* Comma outside of "" and <> splits addresses */
			to2[i]='\n';
		}
	}
	_Recipients += to2;
  free(to2);
}

/**
 * adds recepients to the list
 * @param to String - recepient
 */
void SMTPClient::addRecipient(const char * to) {
  String headerLine = to;
  headerLine += "\n";
  _Recipients += headerLine;
}
/**
 * adds recepients to the list
 * @param to String - recepient
 */
void SMTPClient::addRecipient(const String& to) {
  addRecipient(to.c_str());
}

/**
 * terminate TCP connection
 * @return true if connection is ok
 */
void SMTPClient::disconnect() {
    if (connected()) {_returnCode = sendRequest("QUIT");}
    _tcp->stop();
}

/**
 * init TCP connection and handle ssl verify if needed
 * @return true if connection is ok
 */
bool SMTPClient::connect(void) {

    if(connected()) {
        DEBUG_SMTPCLIENT("[SMTP-Client] connect. already connected!\n");
        while(_tcp->available() > 0) {
            _tcp->read();
        }
        return true;
    }

    if(_smtps) {
        DEBUG_SMTPCLIENT("[SMTP-Client] connect smtps...\n");
        if(_tcps) {
            delete _tcps;
            _tcps = NULL;
            _tcp = NULL;
        }
        _tcps = new WiFiClientSecure();
        _tcp = _tcps;
    } else {
        DEBUG_SMTPCLIENT("[SMTP-Client] connect smtp...\n");
        if(_tcp) {
            delete _tcp;
            _tcp = NULL;
        }
        _tcp = new WiFiClient();
    }

    if(!_tcp->connect(_host.c_str(), _port)) {
        DEBUG_SMTPCLIENT("[SMTP-Client] failed connect to %s:%u\n", _host.c_str(), _port);
        return false;
    }

    DEBUG_SMTPCLIENT("[SMTP-Client] connected to %s:%u\n", _host.c_str(), _port);

    if(_smtps && _smtpsFingerprint.length() > 0) {
        if(_tcps->verify(_smtpsFingerprint.c_str(), _host.c_str())) {
            DEBUG_SMTPCLIENT("[SMTP-Client] smtps certificate matches\n");
        } else {
            DEBUG_SMTPCLIENT("[SMTP-Client] smtps certificate doesn't match!\n");
            _tcp->stop();
            return false;
        }
    }

    // set Timeout for readBytesUntil and readStringUntil
    _tcp->setTimeout(_tcpTimeout);

#ifdef ESP8266
    _tcp->setNoDelay(true);
#endif
    _returnCode = handleResponse();
    if (_returnCode == SMTPC_ERROR_NO_SMTP_SERVER) {
      _tcp->stop();
      return false;
    }

    _returnCode = sendRequest("HELO localhost");    

    if (_returnCode >= 0 && _base64User.length() && _base64Pass.length()) {
      _returnCode = sendRequest("AUTH LOGIN");    
      if (_returnCode > 0 && _returnCode < 400) {
        _returnCode = sendRequest(_base64User);    
        _returnCode = sendRequest(_base64Pass);
      }
      if (_returnCode < 0 || _returnCode > 400) {
        returnError(SMTPC_ERROR_UNAUTHORIZED);     
      }
    }
    
    return connected();
}

/**
 * sends SMTP request header
 * @return status
 */
bool SMTPClient::sendHeaders() {
    if(!connected()) {
        return false;
    }

    _Headers += nl;

    return (_tcp->write(_Headers.c_str(), _Headers.length()) == _Headers.length());
}

int SMTPClient::sendRequest(String &request) {
  return sendRequest(request.c_str());
}

int SMTPClient::sendRequest(const char * request) {
    size_t len = strlen(request);
		DEBUG_SMTPCLIENT("[SMTP-Client][sendReqest] request: '%s'\n", request);
    if(_tcp->write(request, len) != len) {
        return returnError(SMTPC_ERROR_SEND_PAYLOAD_FAILED);
    }
    if(_tcp->write(nl, 2) != 2) {
        return returnError(SMTPC_ERROR_SEND_PAYLOAD_FAILED);
    }
		// handle Server Response (Header)
		return returnError(handleResponse());
}

/**
 * reads the response from the server
 * @return int smtp code
 */
int SMTPClient::handleResponse() {

    if(!connected()) {
        return SMTPC_ERROR_NOT_CONNECTED;
    }

    _returnCode = -1;
    unsigned long lastDataTime = millis();

    while(connected()) {
        size_t len = _tcp->available();
        if(len > 0) {
            String headerLine = _tcp->readStringUntil('\n');
            headerLine.trim(); // remove \r

            lastDataTime = millis();

            DEBUG_SMTPCLIENT("[SMTP-Client][handleResponse] RX: '%s'\n", headerLine.c_str());
      			_returnCode = headerLine.substring(0, 3).toInt();
            _ErrorMessage = headerLine.substring(3);
            while(headerLine[3] == '-') { 
      				headerLine = _tcp->readStringUntil('\n'); 
              _ErrorMessage += '\n' + headerLine.substring(3);
      				DEBUG_SMTPCLIENT("[SMTP-Client][handleResponse] RX_line: '%s'\n", headerLine.c_str());
      			}
      			DEBUG_SMTPCLIENT("[SMTP-Client][handleResponse] code: %d\n", _returnCode);
      
      			if(_returnCode) {
      				return _returnCode;
      			} else {
      				DEBUG_SMTPCLIENT("[SMTP-Client][handleResponse] Error - invalid response from SMTP Server!");
      				return SMTPC_ERROR_NO_SMTP_SERVER;
      			}
        } else {
            if((millis() - lastDataTime) > _tcpTimeout) {
                return SMTPC_ERROR_READ_TIMEOUT;
            }
            delay(0);
        }
    }

    return SMTPC_ERROR_CONNECTION_LOST;
}

/**
 * called to handle error return, may disconnect the connection if still exists
 * @param error
 * @return error
 */
int SMTPClient::returnError(int error) {
    if(error < 0) {
        DEBUG_SMTPCLIENT("[SMTP-Client][returnError] error(%d): %s\n", error, errorToString(error).c_str());
        if(connected()) {
            DEBUG_SMTPCLIENT("[SMTP-Client][returnError] tcp stop\n");
            _tcp->stop();
        }
    }
    return error;
}
