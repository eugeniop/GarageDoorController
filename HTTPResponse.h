#ifndef HTTP_RESPONSE
#define HTTP_RESPONSE

#define MAX_HTTP_RESPONSE 2000

#include <Stream.h>

/*
  Simple class to represent the a response to an HTTP Request
  the `data` buffer is used for BOTH sending and receiving information.
  The GET commmand on the request can optionally accept a Stream * to 
  write the result to. Useful when you want to write directly to an SD
  for example. In that case, the data buffer will be empty, but response.length
  will indicate the amount of data transfered.
*/
class HTTPResponse {

  public:
    int statusCode;                 
    unsigned int length;            //Length in data    
    int chunked;                    //The Response is chunked
    int file;                       //The Response is  file
    char contentType[256];          //Content type of response
    char fileName[14];              //Used for downloads 8.3 filenames supported
    char data[MAX_HTTP_RESPONSE];
    
    ~HTTPResponse(){
      reset();
    };

    void reset(){
      file = 0;
      chunked = 0;
      statusCode = 0;
      length = 0;
      data[0] ='\0';
      fileName[0] = '\0';
      contentType[0] = '\0';
    }

    void dump(){
//      trace.log("HTTP Res", "Status: ", statusCode);
//      trace.log("HTTP Res", chunked ? "Chunked" : "Not Chunked");
      if(file){
//        trace.log("HTTP Res", "File Downloaded: ", fileName);
      }
      if(length > 0){
//        trace.log("HTTP Res", "Content-Length: ", length);
//        trace.logHex("HTTP Res", "Data", data, length);
      } else {
//        trace.log("HTTP Res", "No content");
      }
    };
};

#endif
