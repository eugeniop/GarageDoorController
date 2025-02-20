#ifndef REQUEST_H
#define REQUEST_H

#include <ArduinoJson.h>

#include "HTTPResponse.h"
#include "streamPrintf.h"
#include "wifiHelper.h"
#include "StringStream.h"
#include "Config.h"
#include "TraceLogger.h"

#define MAGIC_DELAY 1000    //This seems to be the smallest delay (a value of 500 fails)
#define MAX_RETRIES 5

class HTTPRequest {
 public:
  HTTPResponse response;
 private:
    WiFiSSLClient client;
    const char * server;
    const int port;
        
    typedef enum { HEADERS, BODY, DONE } HTTP_RX_STATE;

    void parseHeader(char * line){

      if(strncmp(line, "HTTP/1.1", 8)==0){
        //Parse status code like: "HTTP-Version SP Status-Code SP Reason-Phrase CRLF" e.g. "HTTP/1.1 200 OK\r\n"
        char * s = strchr((char *)line, ' ');
        s++;
        response.statusCode =  (*s++ - '0') * 100;
        response.statusCode += (*s++ - '0') * 10;
        response.statusCode += (*s++ - '0');
        return;
      }

      /*
        * For downloads: Content-Disposition: attachment; filename="filename.jpg"
                         1234567890123456789012345678901234567890123
      */ 
      if(strncmp(line, "Content-Disposition", 19)==0){
        strtok(line, "\"");
        const char * fileName = strtok(NULL, "\"");
        trace.log("HTTP", "File download: ", fileName);
        strcpy(response.fileName, fileName);
        response.file = 1;
        return; 
      }
      
      if(strncmp(line, "Content-Length", 14)==0){
        char * l = strchr((char *)line, ' ');
        response.length = atoi(l);
        trace.log("HTTP", "Response length: ", response.length);
        return;
       }

       //In some cases, payload might come "chunked"
       if(strncmp(line, "Transfer-Encoding: chunked", 26)==0){
        response.chunked = 1;
        trace.log("HTTP", "Response chunked");
        return;
       }

       //Capture contentType
       if(strncmp(line, "Content-Type:", 13)==0){
        char * ct = strchr((char *)line, ' ');
        strcpy(response.contentType, ct);
        trace.log("HTTP", "Response Content-Type: ", response.contentType);
        return;
       }
    }

    /*
      In chunked responses, the body is sent in multiple parts that need to be concatenated.
      There's no Content-Length header, so the TOTAL size of the payload is not known.
      If an out Stream is supplied we just write there. If no out Stream is supplied
      we write to the HTTResponse buffer until it runs out of space. If that happens, we
      fail and return NULL.
      The expectation is that a Stream to a File would unlikely fail (unless we run out of space on the SD)
      That is very unlikely though.
    */

    int normalizeHex(int c){
      if(c >= 'A' && c <= 'F') return (c - 'A') + 10;
      if(c >= 'a' && c <= 'f') return (c - 'a') + 10;
      if(c >= '0' && c <= '9') return (c - '0');
      return -1;
    }
    
    // Converts HEX string into an int (to avoid calling sscanf)
    // will return -1 if invalid hex number
    int computeLength(const char * hexLength){
        // EF1 -> E*16^2 + F*16^1 + 1 = 3825
        int length = 0;
        for(int i=strlen(hexLength)-1, j = 0; i>=0; i--, j+=4){
          int v = normalizeHex(hexLength[i]);   //We should check invalid Hex digits, but...that is VERY unlikely
          if(v < 0){
            //Invalid Hex char
            return v;
          }
          length += v * (1 << j);
        }
        return length;
    }


    /*
     * In chunked responses, body is split in chunks with a length header
     * Something like this:
     * 
        4\r\n        (bytes to send)
        Wiki\r\n     (data)
        6\r\n        (bytes to send)
        pedia \r\n   (data)
        E\r\n        (bytes to send)
        in \r\n
        \r\n
        chunks.\r\n  (data)
        0\r\n        (final byte - 0)
        \r\n         (end message)
     * 
     */

    HTTPResponse * processChunkedResponse(){
      
      int totalSize = 0;

      FixedSizeCharStream output(response.data, sizeof(response.data));

      int retries = 0;
      while(retries <= MAX_RETRIES){
        while(client.available()){
          char length[10]; // This stores just the chunk length
          unsigned int chunkLength = 0;
          length[client.readBytesUntil('\r', length, sizeof(length))] = '\0'; //Read length
          client.read(); //Discard '\n'

          chunkLength = computeLength(length);

          if(chunkLength<0){
            return NULL;   
          }

          if(chunkLength > 0){
            //We are writing to the buffer and we ran out iof space
            if(sizeof(response.data) - totalSize - chunkLength <= 0){
              response.reset();
              return NULL;
            }

            for(int b = 0; b < chunkLength; b++){
              output.write(client.read());
            }

            //Discard '\r\n'
            client.read();
            client.read();
            totalSize += chunkLength;
          } else {
            //Final chunk (chunkLength == 0)
            retries = MAX_RETRIES;
            break;
          }
        }
        delay(1000L);
      }

      output.write((uint8_t)'\0');
      response.length = totalSize;
      
      return &response;
    }

    //Normal processing - response is written to the buffer or to the stream
    HTTPResponse * processNormalResponse(){
      //Content-Length is present
      //response.length indicates the size of the data payload
      if(sizeof(response.data) < response.length + 1){
        response.length = 0;
        return NULL;
      }

      //If no out Stream is supplied, we just write t the Response buffer
      FixedSizeCharStream output(response.data, sizeof(response.data));

      int bytesRead = 0;
      int retries = 0;
      while(retries < MAX_RETRIES){
        while(client.available()){
          int c = client.read();
          output.write(c);
          bytesRead++;
        }
        if(bytesRead == response.length){
          //we are done!
          break;
        }
        delay(1000L);
      }

      trace.logHex("HTTP", "ProcessResponse", response.data, response.length);
      
      output.write((uint8_t)'\0');   //Terminate response so all str functions can work directly.
      
      return &response;
    }

    HTTPResponse * processBody(){

      if(response.length == 0 && response.chunked == 0){
        trace.log("HTTP", "No content");
        return NULL;
      }

      //Check if we are downloading a file
      if(response.file){
        trace.log("HTTP", "Processing file download. Not supported");
        return NULL;
      }
        
      //Response might be "chunked"
      if(response.chunked){
        trace.log("HTTP", "Processing chunked response");
        return processChunkedResponse();
      }

      //Process a regular response
      return processNormalResponse();
    }

    /*
      Main response processor
    */
    HTTPResponse * processResponse(){

      HTTPResponse * resp = NULL;
      HTTP_RX_STATE state = HEADERS;

      //Resets the response object
      response.reset();

      //We give some time for the bytes to arrive, but only a few
      int retries = 0;
      while(retries <= MAX_RETRIES){
        if(client.available()) break;
        delay(MAGIC_DELAY);
        retries++;
      }

      //If no data is available....we give up
      if(!client.available()){
        client.stop();
        return resp;
      }

      retries = 0;
      while(retries <= MAX_RETRIES){
        retries++;
        while(client.available()){
          switch(state){
            case HEADERS:
              {
                // Reusing the buffer we already have on the Response object
                int bytes = client.readBytesUntil('\n', response.data, sizeof(response.data));
                response.data[bytes] = '\0';
                //trace.logHex("HTTP", "Data", response.data, bytes);
                if(strlen(response.data) == 1){
                  state = BODY;
                } else {
                  parseHeader(response.data);
                }
              }
              break;
            case BODY:
            {
              resp = processBody();
              state = DONE;
              retries = MAX_RETRIES;
            }
            break;
          }
        }
      }
      client.stop();
      return resp;
    }

    int ConnectServer(){

      int status = WiFi_ConnectWithParams(WIFI_SSID, WIFI_PWD, 3);
      
      if(status == WL_CONNECTED){
        // Connected to WiFi - connect to server
        int retries = 3;
        while(retries--){
          auto r = client.connect(server, port);
          //Watchdog.enable(WDT_TIMEOUT);

          if(r){
            return WL_CONNECTED;
          }
          
          delay(MAGIC_DELAY); // Magic delay
        }
        error.log("HTTP", "Connect with server failed after 3 tries");
      } else {
        error.log("HTTP", "WiFi Connection failed");
      }

      client.stop();
      WiFi_Close();

      return WL_CONNECT_FAILED;
    }

    void sendHTTPHeaders(Stream & s, const char * verb, const char * route, const char * contentType, int length, const char * access_token){
      s_printf(&s, "%s %s HTTP/1.1\r\n", verb, route);
      s_printf(&Serial, "%s %s HTTP/1.1\r\n", verb, route);
      s_printf(&s, "Host: %s\r\n", server);
      s_printf(&Serial, "Host: %s\r\n", server);
      
      if(access_token && strlen(access_token) > 0){
        s.print("Authorization: ");  //s_printf has a limited buffer. Tokens can be long
        s.println(access_token);     
        Serial.print("Authorization: ");  //s_printf has a limited buffer. Tokens can be long
        Serial.println(access_token);          
      }
      
      if(contentType && strlen(contentType)>0){
        s_printf(&s, "Content-Type: %s\r\n", contentType);
        s_printf(&Serial, "Content-Type: %s\r\n", contentType);
      }
      
      if(length>0){
        s_printf(&s, "Content-Length: %d\r\n", length);
        s_printf(&Serial, "Content-Length: %d\r\n", length);
      }
      
      s.println("Connection: close");
      s.println();
    };
    
    HTTPResponse * httpPutPost(const char * route, const char * verb, const char * contentType, const char * access_token){
      if(ConnectServer() != WL_CONNECTED){
        return NULL;
      }

      const int length = strlen(response.data);

      trace.logHex("HTTP", verb, response.data, length);
      
      sendHTTPHeaders(client, verb, route, contentType, length, access_token);
      client.print(response.data);
      return processResponse(); 
    };
    
  public:
    HTTPRequest(const char * server, int port) : server(server), port(port){
    };

    char * dataBuffer(){
      return response.data;
    };
   
    HTTPResponse * postJSON(const char * route, const char * access_token){
      return httpPutPost(route, "POST", "application/json", access_token);
    };

    HTTPResponse * putJSON(const char * route, const char * access_token){
      return httpPutPost(route, "PUT", "application/json", access_token);
    };
   
    HTTPResponse * get(const char * route, const char * access_token){
      if(ConnectServer() != WL_CONNECTED){
        return NULL;
      }
      sendHTTPHeaders(client, "GET", route, NULL, 0, access_token);

      return processResponse();
    }
};

#endif
