#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include "ESP8266SMTPClient.h"

#define USE_SERIAL Serial

ESP8266WiFiMulti WiFiMulti;


#define ESSID "SSID"
#define WPA_PASS "PASSWORD"

/* If you are using gmail, you will probably have to enable access for less secure applications at
 * https://www.google.com/settings/security/lesssecureapps
 * A throwaway account for sending is recommended anyway 
 */ 

const char * smtp_server = "smtp.gmail.com"; 
const int smtp_port = 465;
const char * smtp_user = "user@gmail.com";
const char * smtp_pass = "YOURPASSWORD"; 

/* SMTP fingerprint is not needed if you accept the risk of MitM attack, if you use it please note
 * it may change in future. Anyway, if you want to obtain it, you may use following command on Linux:
 * openssl s_client -connect smtp.gmail.com:465 < /dev/null 2>/dev/null | openssl x509 -fingerprint -noout | cut -d'=' -f2 | tr ':' ' '
 */

const char * smtp_fingerprint = "53 3B BD 55 96 61 A9 88 22 7D 82 2A 20 F5 B4 20 13 BE 91 20";

const char * from = "sender@mydomain.example";
const char * to = "recipient@otherdomain.example";
const char * message = "Hello world!\r\nEnd of test message.";
const char * subject = "Test subject";

void setup() {
    USE_SERIAL.begin(115200);
    // USE_SERIAL.setDebugOutput(true);

    USE_SERIAL.println();

    //WiFiMulti.addAP("SSID", "PASSWORD");
    WiFiMulti.addAP(ESSID, WPA_PASS);
}

void loop() {
    static boolean finished = false;
    // wait for WiFi connection
    if(!finished && (WiFiMulti.run() == WL_CONNECTED)) {
      SMTPClient smtp;    
      
      smtp.begin(smtp_server, smtp_port); // Server identity is not verified
      //smtp.begin(smtp_server, smtp_port, smtp_fingerprint); // Use this one for fingerprint checking 
      
      if (smtp_user && smtp_user[0] && smtp_pass && smtp_pass[0]) {
        smtp.setAuthorization(smtp_user, smtp_pass);
        Serial.println("[SMTP] Setting autorization");
      }
      Serial.printf("[SMTP] Sender: %s\n", smtp_user);
  
      /* Send message as is with no extra headers to given recipient */
      int result = smtp.sendMessage(smtp_user, message, strlen(message), to, subject);
      Serial.printf("[SMTP] Message send result: %i %s %s\n", result, smtp.getErrorMessage(), smtp.errorToString(result).c_str());
      smtp.end();
      smtp.disconnect();
      Serial.println("[SMTP] Disconnecting from SMTP server");  
      finished=true;
    }    
    delay(1000);
}
