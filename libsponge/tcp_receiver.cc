#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.
/*
template <typename... Targs>
void DUMMY_CODE(Targs &&...  unused ) {}*/

using namespace std;

bool TCPReceiver::segment_received(const TCPSegment &seg) {
    uint64_t old_abs_ackno = 0;
    if (read_isn) {
        old_abs_ackno = abs_ackno();
    }
    TCPHeader header=seg.header();
    Buffer payload=seg.payload();
    const string data= payload.copy();
    if(!read_isn&&header.syn){
	    isn=header.seqno;
            read_isn=true;
    }

    if (!read_isn) {
        return false;
    }

    
    const uint64_t abs_seqno=unwrap(header.seqno+header.syn, isn, last_assem);
    uint64_t old_window_size = window_size();
    // ACK after FIN should be received
    if (fin_abs_seq && abs_seqno >= fin_abs_seq && seg.length_in_sequence_space() == 0) {
        return true;
    }

    if (!(abs_seqno < old_abs_ackno + old_window_size && abs_seqno + seg.length_in_sequence_space() > old_abs_ackno)) {
        // Not overlap with the window. but if it's a ack only, it's accepted.
        return seg.length_in_sequence_space() == 0 && abs_seqno == old_abs_ackno;
    }
    
    bool all_fill = abs_seqno + seg.length_in_sequence_space() <= old_abs_ackno + old_window_size;

    if (all_fill && seg.header().fin) {  // only when fin also fall in the window
        fin_abs_seq = abs_seqno + seg.length_in_sequence_space()-header.syn-header.fin;
    }

    if(read_isn&&(old_abs_ackno||last_assem==0)){
        _reassembler.push_substring(data, abs_seqno-1, header.fin);
    }

    if(abs_ackno()>=abs_seqno){
		    last_assem=_reassembler.stream_out().bytes_written()+header.fin;//abs_seqno+seg.length_in_sequence_space()-header.syn-1;
            if(last_assem+1==fin_abs_seq)last_assem++;
	}

    return true;
    /*
    if(read_isn&&(this->ackno()||last_assem==0)){
	    
	    if(header.fin)fin_abs_seq=abs_seqno+seg.length_in_sequence_space()-header.syn-header.fin; 
	    _reassembler.push_substring(data, abs_seqno-1, header.fin);
	    if(this->abs_ackno()==abs_seqno){
		    last_assem=_reassembler.stream_out().bytes_written()+header.fin;//abs_seqno+seg.length_in_sequence_space()-header.syn-1;
            if(last_assem+1==fin_abs_seq)last_assem++;
	    }
        return true;
    }

    return false;
    */
}

uint64_t TCPReceiver::abs_ackno()const{
	return last_assem+1;
}
optional<WrappingInt32> TCPReceiver::ackno() const { 
	if(!read_isn)return nullopt;
        else return make_optional<WrappingInt32>(wrap(last_assem+1,isn));	
}

size_t TCPReceiver::window_size() const { 
	return _capacity-stream_out().buffer_size(); 
}
