#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.
/*
template <typename... Targs>
void DUMMY_CODE(Targs &&...  unused ) {}*/

using namespace std;

bool TCPReceiver::segment_received(const TCPSegment &seg) {/*
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

    uint64_t old_window_size = window_size();
    const uint64_t abs_seqno=unwrap(header.seqno+header.syn, isn, last_assem);
    if (!(abs_seqno < old_abs_ackno + old_window_size && abs_seqno + seg.length_in_sequence_space() > old_abs_ackno)) {
        // Not overlap with the window. but if it's a ack only, it's accepted.
        return seg.length_in_sequence_space() == 0 && abs_seqno == old_abs_ackno;
    }

    if(read_isn&&(this->ackno()||last_assem==0)){
	    
	    if(header.fin)fin_abs_seq=abs_seqno+seg.length_in_sequence_space()-header.syn-header.fin; 
	    _reassembler.push_substring(data, abs_seqno-1, header.fin);
	    if(this->abs_ackno()==abs_seqno){
		    last_assem=_reassembler.stream_out().bytes_written()+header.fin;//abs_seqno+seg.length_in_sequence_space()-header.syn-1;
                    if(last_assem+1==fin_abs_seq)last_assem++;
	    }
    }
    /////
    if(read_isn&&header.seqno>this->ackno()){
        const uint64_t absolute_seqno=unwrap(header.seqno+header.syn,isn,last_assem);   

    }
    
    return true;*/
    uint64_t old_abs_ackno = 0;
    if (read_isn) {
        old_abs_ackno = abs_ackno();
    }
    // Deal with the first SYN
    if (seg.header().syn && !read_isn) {
        isn = seg.header().seqno;
    }

    if (!read_isn) {
        return false;
    }

    // absolute sequence number
    uint64_t checkpoint = _reassembler.stream_out()
                              .bytes_written();  // bytes_written is a little bit small, but good enough for checkpoint
    uint64_t abs_seq = unwrap(seg.header().seqno, isn, checkpoint);


    uint64_t old_window_size = window_size();

    // ACK after FIN should be received
    if (fin_abs_seq && abs_seq >= fin_abs_seq && seg.length_in_sequence_space() == 0) {
        return true;
    }

    if (!(abs_seq < old_abs_ackno + old_window_size && abs_seq + seg.length_in_sequence_space() > old_abs_ackno)) {
        // Not overlap with the window. but if it's a ack only, it's accepted.
        return seg.length_in_sequence_space() == 0 && abs_seq == old_abs_ackno;
    }
    bool all_fill = abs_seq + seg.length_in_sequence_space() <= old_abs_ackno + old_window_size;

    if (all_fill && seg.header().fin) {  // only when fin also fall in the window
        fin_abs_seq = abs_seq + seg.length_in_sequence_space();
    }

    uint64_t stream_indices = abs_seq > 0 ? abs_seq - 1 : 0;
    std::string payload(seg.payload().copy());
    _reassembler.push_substring(payload, stream_indices, stream_indices + seg.payload().size() + 2 == fin_abs_seq);

    return true;

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
