#include "byte_stream.hh"
#include<iostream>
// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`
template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity):
    _capacity(capacity), _used(0),_data(""),
    _input_end(false),_byte_wt(0),_byte_rd(0),_eof(false){}//DUMMY_CODE(capacity); }

size_t ByteStream::write(const string &data) {
    //DUMMY_CODE(data);
    size_t r_capacity=remaining_capacity();
    if(r_capacity==0)return 0;
    size_t write_size=min(r_capacity,data.length());
    //for(size_t i=0;i<write_size;i++)
        //_data.push_back(data[i]);
    _data.append(data,0,write_size);
    _byte_wt+=write_size;
    _used+=write_size;
    return write_size;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    //DUMMY_CODE(len);
    size_t rd_size=min(len,_used);
    std::string rd_str=_data.substr(0,rd_size);
    return rd_str;
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) {
    // DUMMY_CODE(len); }
    _data=_data.substr(len);
    _used-=len;
    _byte_rd+=len;
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    //DUMMY_CODE(len);
    const auto ret=peek_output(len);
    //size_t length=result.length();
    pop_output(ret.length());
    return ret;
}

void ByteStream::end_input() { _input_end=true;}

bool ByteStream::input_ended() const { return _input_end; }

size_t ByteStream::buffer_size() const { return _used; }

bool ByteStream::buffer_empty() const { return _used==0; }

bool ByteStream::eof() const { return buffer_empty()&&input_ended(); }

size_t ByteStream::bytes_written() const { return _byte_wt; }

size_t ByteStream::bytes_read() const { return _byte_rd; }

size_t ByteStream::remaining_capacity() const { return _capacity-_used; }
