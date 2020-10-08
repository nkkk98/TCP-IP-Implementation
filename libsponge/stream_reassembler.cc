#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) :leftdata({}),leftindex({}),lefteof({}),_output(capacity), _capacity(capacity),_empty(false),unassem_num(0),needindex(0){}
//Discription:delete characters in unassemble array to store the newest characters could be assembled. if the unassemble num is not enough,cut the present string.
//Parameter: 
//index:index of head unassemble array member that can be erased or cut
//num: count of place that should be delete.
//Return: number of the present string need to be cut
size_t StreamReassembler::iterdelete(const size_t index,const size_t num){
	size_t iter=leftindex.size()-1;
	size_t delnum=num;
	if(index>=leftindex.size())return num;
	while(iter>=index){
		if(delnum>=leftdata[iter].length()){
			delnum-=leftdata[iter].length();
			unassem_num-=leftdata[iter].length();
			leftdata.erase(leftdata.begin()+iter);
			leftindex.erase(leftindex.begin()+iter);
			lefteof.erase(lefteof.begin()+iter);
			iter--;		
		}else{
			size_t needsize=leftdata[iter].length()-delnum;
			leftdata[iter]=leftdata[iter].substr(0,needsize);
			//if the last string with eof=true is cutted,the corresponding eof should be false;
			lefteof[iter]=false;
			unassem_num-=delnum;
			delnum=0;
			break;
		}
		
	}

	return delnum;

}
//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
   // if string has porbability to be assembled,we should judge if this string already included in ByteSream
   if(index<=needindex){
     size_t trueindex=needindex-index;
     if(static_cast<long unsigned int>(trueindex)>=data.length()&&data!="")return;
     bool _eof=eof;
     if(data!=""){
     std::string needdata=data.substr(trueindex);
     size_t maxsize=_capacity-unassem_num-_output.buffer_size();
     size_t needsize=needdata.length();
     size_t lastindex=index+data.length()-1;
     size_t k=0;
     size_t originlength=needsize;
     //delete the overlapping characters between this string and string has been stored in unassembled array.
     while(k<leftindex.size()){
     	if(leftindex[k]>lastindex)break;
	else if(leftindex[k]+leftdata[k].length()-1<=lastindex){
		needsize-=leftdata[k].length();
		unassem_num-=leftdata[k].length();
		leftindex.erase(leftindex.begin());
		leftdata.erase(leftdata.begin());
		lefteof.erase(lefteof.begin());

	}else{
		needsize-=lastindex-leftindex[k]+1;
		unassem_num-=lastindex-leftindex[k]+1;
		leftdata[k]=leftdata[k].substr(lastindex-leftindex[k]+1);
		leftindex[k]=lastindex+1;
		break;
	}
     
     }
     size_t cut;
     //if the space is not enough,delete some characters or cut the string itself.
     if(needsize>maxsize){
     	cut=iterdelete(k,needsize-maxsize);
	if(cut!=0){
		needsize-=cut;
		originlength-=cut;
	}
     }
     needdata=needdata.substr(0,originlength);
     _output.write(needdata);
     needindex+=originlength;
     }
     //if _eof=true and there is no posibility to assemble any characters ,end the input.
     if(_eof){
	if(leftindex.size()==0){_empty=true;
	    _output.end_input();
	}
	else{
	    if(leftindex[0]>needindex){_empty=true;
	    	_output.end_input();
	    }
	    }
     }
     //if is not end,iterately call push_substring function to push unassebled array member
     if(!_empty){
       if(leftindex.size()>0){
         if(leftindex[0]<=needindex){
		 std::string tmpleft=leftdata[0];
		 size_t tmpindex=leftindex[0];
		 bool tmpeof=lefteof[0];
		 leftdata.erase(leftdata.begin());
		 leftindex.erase(leftindex.begin());
		 lefteof.erase(lefteof.begin());
		 unassem_num-=tmpleft.length();
		 push_substring(tmpleft,tmpindex,tmpeof);
       }
       }
     }

   }
   else{
   //store into leftdata
      size_t maxsize=_capacity-_output.buffer_size()-unassem_num;
      size_t needsize=data.length();
      needsize=maxsize>needsize?needsize:maxsize;
      std::string storedata=data;
      if(needsize>0){
	size_t i=0;
      	while(i<leftindex.size()){
	    if(leftindex[i]>=index)break;
	    else i++;
	}
	bool _eof=eof;
	//calculate overlapping characters with earlier member,then push back directly
	if(i==leftindex.size()){
	    size_t newindex=index;
	    if(i>0){
	       newindex=(leftindex[i-1]+leftdata[i-1].length()-1)>=index?(leftindex[i-1]+leftdata[i-1].length()):index;
	       if(index+data.length()-1>=newindex){
		    storedata=data.substr(newindex-index);
		    needsize=needsize>storedata.length()?storedata.length():needsize;
		    if(needsize<storedata.length())_eof=false;
		    storedata=storedata.substr(0,needsize);

	       }else return;
	    }
	    leftindex.push_back(newindex);
	    leftdata.push_back(storedata);
	    lefteof.push_back(_eof);
	    unassem_num+=needsize;
	    return;
	}
	//calculate overlapping characters with earlier and later member
	if(i<leftindex.size()){
		size_t newindex=index;
		if(i>0){
			newindex=leftindex[i-1]+leftdata[i-1].length()-1>=index?leftindex[i-1]+leftdata[i-1].length():index;
			if(index+data.length()-1>=newindex){
			   storedata=data.substr(newindex-index);
			}else return;
		
		}
		size_t lastindex=newindex+storedata.length()-1;
		size_t j=i;
		size_t extra=storedata.length();
		size_t originlength=storedata.length();
		// first: erase leftdatas which is encluded in data
		// second: calculate the eltra place to set the disoverlapping part of data,during this process,if the place account is bigger than maxsize,use function to delete leftdata characters from tile,if these character is less than what we need,we should cut the storedata.
		while(j<leftindex.size()){
			if(leftindex[j]>lastindex)break;
			else if(leftindex[j]+leftdata[j].length()-1<=lastindex){
				//calculate extra place
				extra-=leftdata[j].length();
				//erase the overlapping leftdata
				leftindex.erase(leftindex.begin()+j);
				leftdata.erase(leftdata.begin()+j);
				lefteof.erase(lefteof.begin()+j);
			}else{
				extra-=lastindex-leftindex[j]+1;
				leftdata[j]=leftdata[j].substr(lastindex-leftindex[j]+1);
				leftindex[j]=lastindex+1;
				break;
			}
		}
		size_t cut;
		if(extra>maxsize){
			cut=iterdelete(j,extra-maxsize);
			if(cut!=0){
				extra-=cut;
				originlength-=cut;	
			}
		}
		storedata=storedata.substr(0,originlength);
		leftindex.insert(leftindex.begin()+i,newindex);
		leftdata.insert(leftdata.begin()+i,storedata);
		lefteof.insert(lefteof.begin()+i,_eof);
		unassem_num+=extra;
		return; 
	    }
	}
   }
}

size_t StreamReassembler::unassembled_bytes() const { return unassem_num; }

bool StreamReassembler::empty() const { return _empty;}
