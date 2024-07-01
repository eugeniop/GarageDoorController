#ifndef _CHAR_STREAM_H_
#define _CHAR_STREAM_H_

#include <Stream.h>

class DevNullStream : public Stream {

public:

    int bytesWritten;
    
    DevNullStream(){
      bytesWritten = 0;
    }
    // Stream methods
    virtual int available(){ 
      return 0;
    }
    
    virtual int read(){
      return -1;
    }
    
    virtual int peek(){ 
      return read();
    }
    
    virtual void flush() { 
      bytesWritten = 0;
    }
    
    // Print methods
    virtual size_t write(uint8_t c){
      bytesWritten++;
      return 1; 
    }
};

class FixedSizeCharStream : public Stream {
  
  char * s;
  int length;
  int w_position;
  int r_position;
  int max;
  int truncated = 0;  //Signals that we ran out of space attempting to write
  
public:

    //The underlying buffer
    FixedSizeCharStream(char * s, int max) : w_position(0), r_position(0) {
      this->s = s;
      this->max = max;
    }

    int isTruncated(){
      return truncated;
    }

    // Stream methods
    virtual int available(){ 
      return w_position - r_position;
    }
    
    virtual int read(){
      if(r_position == w_position) return -1;   //No data
      return s[r_position++];
    }
    
    virtual int peek(){ 
      if(r_position == w_position) return -1;   //No data
      return s[r_position];
    }
    
    virtual void flush() { 
      r_position=0; w_position = 0; 
    }
    
    // Print methods
    virtual size_t write(uint8_t c){

      if(w_position==max-1){
        truncated = 1;
        return -1;
      }

      s[w_position++] = (char) c;
      
      return 1; 
    }

    virtual void end(){
      s[w_position] = '\0';
    }
};

class urlEncodedStream : public Stream {

 const char * hex = "0123456789abcdef";
 Stream * s;
 
 public:
    urlEncodedStream(Stream * s) {
      this->s = s;
    }

    virtual int read(){ 
      int c = s->read();
      if(c == '%'){
        char code[3] = { (char)s->read(), (char)s->read(), '\0' };
        return strtol(code, NULL, 16);
      } else {
        return c;
      }
    }
    
    virtual size_t write(uint8_t c){
      if (('a' <= c && c <= 'z')
        || ('A' <= c && c <= 'Z')
        || ('0' <= c && c <= '9')) {
       return s->write(c);
      } else {
        if( s->write('%') == -1 ) return -1;
        if( s->write(hex[c >> 4]) == -1) return -1;
        return s->write(hex[c & 15]);
      }   
    };    

    // Stream methods
    virtual int available(){ 
      return s->available(); 
    }

    virtual int peek(){ 
      return s->peek(); 
    }
    
    virtual void flush() { s-> flush(); };
};

#endif // _CHAR_STREAM_H_
