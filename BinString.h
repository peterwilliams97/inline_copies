#ifndef BIN_STRING_H
#define BIN_STRING_H

#include <vector>

typedef unsigned char byte;

class Regex;
class RegexResults;

// Binary data will be processed.
class BinString
{
    const size_t _len;
    byte *_data;
public:
    BinString();
    BinString(size_t len);
    BinString(const byte *data, size_t len);
    BinString(const BinString &b);
 
    ~BinString() { 
        delete[] _data; 
    }
    size_t get_len() const  { return _len; }
    byte *get_data() const { return _data; }
    const std::vector<byte> get_as_vector() const;
};

#endif