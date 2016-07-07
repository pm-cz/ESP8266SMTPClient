#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include "ESP8266SMTPClient.h"

#define USE_SERIAL Serial

ESP8266WiFiMulti WiFiMulti;


#define ESSID "SSID"
#define WPA_PASS "PASSWORD"

const char * smtp_server = "smtp.myprovider.example";
const int    smtp_port = 25;
const char * smtp_user = "username";
const char * smtp_pass = ""; /* Password, or empty if you do not use autorization */

const char * from = "sender@mydomain.example";
const char * to = "recipient@otherdomain.example";
const char * message = "Hello world!\r\n.\r\nThird line\r\nSome accented letters: Čšäáćľ, no newline.";
const char * subject = "Test subject, accented letters: ćósÿ";
const char * subject2 = "Second test subject";

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
      
      smtp.begin(smtp_server, smtp_port);
      if (smtp_user && smtp_user[0] && smtp_pass && smtp_pass[0]) {
        smtp.setAuthorization(smtp_user, smtp_pass);
        Serial.println("[SMTP] Setting autorization");
      }
      Serial.printf("[SMTP] Sender: %s\n", from);
  
      /* Add headers for unicode message suppor and custom ID of mailer software */
      smtp.addHeader("Content-Type", "text/plain; charset=UTF-8");
      smtp.addHeader("Content-Transfer-Encoding", "8bit");
      smtp.setMailer("My-Custom-Mailer");

      /* Send first message to given recipient, specified in to with correct UTF-8 charset */
      int result = smtp.sendMessage(from, message, strlen(message), to, subject);
      Serial.printf("[SMTP] Message send result: %i %s %s\n", result, smtp.getErrorMessage(), smtp.errorToString(result).c_str());

      /* Send second message to undisclosed recipients, the recipient being used as blind carbon copy.
       * Recipent's User agent will try to guess the charset. The 8bit encoding is used without correct headers.
       * As a result, the message may be broken when received.
       */
       
      smtp.addHeader("Subject", subject2);
      smtp.addRecipient(to);
      result = smtp.sendMessage(from, message);
      Serial.printf("[SMTP] Message send result: %i %s %s\n", result, smtp.getErrorMessage(), smtp.errorToString(result).c_str());
      
      smtp.end();
      smtp.disconnect();
      Serial.println("[SMTP] Disconnecting from SMTP server");  
      finished=true;
    }    
    delay(1000);
}
