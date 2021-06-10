#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.
/*
template <typename... Targs>
void DUMMY_CODE(Targs &&...  unused ) {}*/

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    TCPHeader header=seg.header();
    Buffer payload=seg.payload();
    const string data= payload.copy();
    if(!read_isn&&header.syn){
	    isn=header.seqno;
            read_isn=true;
    }
    
    if(read_isn&&(this->ackno()||last_assem==0)){
	    const uint64_t abs_seqno=unwrap(header.seqno+header.syn, isn, last_assem);
	    if(header.fin)fin_abs_seq=abs_seqno+seg.length_in_sequence_space()-header.syn-header.fin; 
	    _reassembler.push_substring(data, abs_seqno-1, header.fin);
	    if(this->abs_ackno()==abs_seqno){
		    last_assem=_reassembler.stream_out().bytes_written()+header.fin;//abs_seqno+seg.length_in_sequence_space()-header.syn-1;
                    if(last_assem+1==fin_abs_seq)last_assem++;
	    }
    }
    /*
    if(read_isn&&header.seqno>this->ackno()){
        const uint64_t absolute_seqno=unwrap(header.seqno+header.syn,isn,last_assem);   

    }
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
