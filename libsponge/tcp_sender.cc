#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity)
    , _timer{0}
    , _RTO{retx_timeout}{}

uint64_t TCPSender::bytes_in_flight() const { return _flying_bytes; }

void TCPSender::fill_window() {
	//if(windowsize<1)return;
	//
	TCPSegment segment;

	WrappingInt32 nxt_seqno=wrap(_next_seqno,_isn);
	segment.header().seqno=nxt_seqno;

	size_t seq_capacity=TCPConfig::MAX_PAYLOAD_SIZE;
	//syn flag=true in the begining
	if(_next_seqno==0){
		segment.header().syn=true;
		seq_capacity=0;
		_next_seqno++;
	}

	size_t zp_window_edge=windowedge+(windowsize==0&&windowedge==_next_seqno);
	while(seq_capacity==0||zp_window_edge>_next_seqno){
		seq_capacity=min(seq_capacity,zp_window_edge-_next_seqno);

		segment.payload()=Buffer(_stream.read(seq_capacity));
		seq_capacity=min(seq_capacity,segment.payload().size());
		_next_seqno+=seq_capacity;

		//no data to be sent and receiver have window to recieve fin
		if(_stream.eof()&&_next_seqno<zp_window_edge&&fin_seq==0){
			segment.header().fin=true;
			fin_seq=_next_seqno;
			_next_seqno++;

		}		
		_flying_bytes+=segment.length_in_sequence_space();
		if(segment.length_in_sequence_space()==0)return;
		
		_segments_out.push(segment);
		_flying_seg.push(segment);
		if(seq_capacity==0||_stream.buffer_empty()||windowsize==0)break;
		segment.header().syn=false;
		segment.header().fin=false;
		segment.header().seqno=wrap(_next_seqno,_isn);
		seq_capacity=TCPConfig::MAX_PAYLOAD_SIZE;
	}
	if(!_timer_started){
		_timer_started=true;
		_timer=_RTO;
	}
	
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
	uint64_t abs_ackno=unwrap(ackno,_isn,_next_seqno);
	if(abs_ackno>_next_seqno)return;//fault consequnce
	
	windowsize=window_size;
	windowedge=abs_ackno+windowsize;

	if(abs_ackno<=_abs_ackno)return;//already acknowledged

	_RTO=_initial_retransmission_timeout;
	_timer=_RTO;
	_consecutive_retransmissions=0;

	auto acked_size=abs_ackno-_abs_ackno;
	_flying_bytes-=acked_size;
	
	if(_flying_bytes==0)_timer_started=false;

	while(!_flying_seg.empty()){
		TCPSegment frontseg=_flying_seg.front();
		WrappingInt32 flyingseq=frontseg.header().seqno;
		size_t seglength=frontseg.length_in_sequence_space();
		if(abs_ackno>unwrap(flyingseq,_isn,_next_seqno)+seglength-1)
			_flying_seg.pop();
		else break;
	}
	
	_abs_ackno=abs_ackno;


}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) { 
     if(!_timer_started)return;
     _timer-=ms_since_last_tick;
     if(_timer>0)return;

     _segments_out.push(_flying_seg.front());
     if(windowsize>0){
        _consecutive_retransmissions++;
        _RTO*=2;
     }
     _timer=_RTO;
     
}

unsigned int TCPSender::consecutive_retransmissions() const { return _consecutive_retransmissions; }

void TCPSender::send_empty_segment() {
	TCPSegment segment;
	segment.header().seqno=wrap(_next_seqno,_isn);
	_segments_out.push(segment);
}
