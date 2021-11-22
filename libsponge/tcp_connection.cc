#include "tcp_connection.hh"

#include <iostream>

// Dummy implementation of a TCP connection

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.

using namespace std;

size_t TCPConnection::remaining_outbound_capacity() const { return _sender.stream_in().remaining_capacity(); }

size_t TCPConnection::bytes_in_flight() const { return _sender.bytes_in_flight(); }

size_t TCPConnection::unassembled_bytes() const { return _receiver.unassembled_bytes(); }

size_t TCPConnection::time_since_last_segment_received() const { return _time_since_last_segment_received; }

void TCPConnection::segment_received(const TCPSegment &seg) { 

    _time_since_last_segment_received = 0;

    if(seg.header().rst){
        _sender.stream_in().set_error();
        _receiver.stream_out().set_error();
        return;
    }
    bool invalid_ack = false;
    // deliver ack to sender
    if (seg.header().ack) {
        invalid_ack = !_sender.ack_received(seg.header().ackno, seg.header().win);
    }
    if (invalid_ack && _sender.next_seqno_absolute() == 0) {
        return;
    }

    bool segment_received = _receiver.segment_received(seg);

    // reset linger_after_streams_finish if remote EOF before inbound EOF
    if (_receiver.stream_out().eof() && !_sender.stream_in().eof()) {
        _linger_after_streams_finish = false;
    }

    bool send=updateSender();
    if(!send&&(seg.header().fin||invalid_ack||!segment_received||seg.length_in_sequence_space())&&(_sender.bytes_in_flight() != _sender.next_seqno_absolute())){
        _sender.send_empty_segment();
        sendSegments();
    }

}

bool TCPConnection::active() const { 
    if (_receiver.stream_out().error() || _sender.stream_in().error()) {
        return false;
    }
    if (_receiver.stream_out().eof() && _sender.stream_in().eof() && _sender.bytes_in_flight() == 0 &&!has_new_ackno()) {
        if (_linger_after_streams_finish && _time_since_last_segment_received < 10 * _cfg.rt_timeout) {
            return true;
        }
        return false;
    }
    return true;
 }

size_t TCPConnection::write(const string &data) {
    size_t nwrite=_sender.stream_in().write(data);
    updateSender();
    return nwrite;
}
bool TCPConnection::updateSender(){
    size_t nsegment=_sender.segments_out().size();
    size_t newnsegment=nsegment;

    //fill window as much as possible
    while(true){
        _sender.fill_window();
        newnsegment=_sender.segments_out().size();
        if(newnsegment==nsegment)break;
        nsegment=newnsegment;
    }

    if(nsegment==0&&has_new_ackno()){
        _sender.send_empty_segment();
    }

    if(_sender.segments_out().size()){
        sendSegments();
        return true;
    }
    return false;
}

void TCPConnection::sendSegments(){
    if(!active())return;
    while(_sender.segments_out().size()){
        auto seg=_sender.segments_out().front();
        if(_receiver.ackno().has_value()){
            seg.header().ack=true;
            seg.header().ackno=_receiver.ackno().value();
            _last_ackno_sent=_receiver.ackno();
        }
        seg.header().win=_receiver.window_size();
        _segments_out.push(seg);
        _sender.segments_out().pop();
    }
}

bool TCPConnection::has_new_ackno()const {
    return _receiver.ackno().has_value()&&(!_last_ackno_sent.has_value()||(_last_ackno_sent.value()!=_receiver.ackno().value()));
}
//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) { 
    _sender.tick(ms_since_last_tick);
    _time_since_last_segment_received += ms_since_last_tick;
    if (_sender.consecutive_retransmissions() > TCPConfig::MAX_RETX_ATTEMPTS) {
        reset();
    }

    if (_time_since_last_segment_received >= 10 * _cfg.rt_timeout && !active()) {
        _linger_after_streams_finish = false;
    }

    if (_sender.next_seqno_absolute() == 0) {  // should not send segment(SYN) when stream is not started.
        return;
    }
    updateSender();
}

void TCPConnection::end_input_stream() {
    _sender.stream_in().end_input();
    updateSender();
}

void TCPConnection::connect() {
    updateSender();
}

void TCPConnection::reset(){
    TCPSegment segment;
    segment.header().seqno = _sender.next_seqno();
    segment.header().rst = true;
    _segments_out.push(segment);
    _sender.stream_in().set_error();
    _receiver.stream_out().set_error();
}

TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";

            // Your code here: need to send a RST segment to the peer
            reset();
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}
