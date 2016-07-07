# ESP8266SMTPClient
ESP8266 Arduino SMTP client

A simple library inspired by ESP8266HTTPClient, which allows to send multiple messages through a single SMTP connection.

Supported features:
* Basic SMTP client command set
* Authorization (AUTH LOGIN)
* Works both with SMTP and SMTPS servers (does not support STARTTLS)
* Sets a configurable X-Mailer header
* Allows to set multiple recipients (BCC is also supported)
* Allows to set custom headers
* Correct handling of \n. sequence inside of E-mail
* UTF-8 encoded Subject

It is possible to enable debugging output by defining  DEBUG_ESP_SMTP_CLIENT and DEBUG_ESP_PORT (or uncomment the code in library)
