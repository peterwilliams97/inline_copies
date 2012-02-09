#include <iostream>
#include "BinString.h"

using namespace std;

static const byte *dup_data(const byte *data, size_t len)
{
    byte *dup = new byte[len];
    return (byte *)memcpy(dup, data, len*sizeof(byte));
}

BinString::BinString(const byte *data, size_t len):
    _len(len),
    _data(dup_data(data, len))
{   
    if (_len != len) {
        cerr << "Cannot happen!" << endl;  
    }
    for (size_t i = 0; i < len; i++) {
        if (_data[i] != data[i]) {
            cerr << "Cannot happen!" << endl;
        }
    }
}

BinString::BinString(const BinString &b):
    _len(b.get_len()),
    _data(dup_data(b.get_data(), b.get_len()))
{
}

BinString::BinString():
    _len(0),
    _data(0)
{}



const vector<byte> BinString::get_as_vector() const 
{
    vector<byte> v = vector<byte>(_len);
    for (size_t i = 0; i < _len; i++) {
        v[i] = _data[i];
    }
    return v;
}
