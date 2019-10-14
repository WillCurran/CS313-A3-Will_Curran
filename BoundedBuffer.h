#ifndef BoundedBuffer_h
#define BoundedBuffer_h

#include <iostream>
#include <queue>
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
using namespace std;

class BoundedBuffer
{
private:
  	int cap; // capacity remaining
    int full; // size of q. do we need this?
  	queue<vector<char>> q;

	/* mutex to protect the queue from simultaneous producer accesses
	or simultaneous consumer accesses of q*/
	mutex mtx;
    /* mutex to use with slot_available*/
    mutex m_sa;
    /* mutex to use with data_available*/
    mutex m_da;
	
	/* condition that tells the consumers that some data is there */
	condition_variable data_available;
	/* condition that tells the producers that there is some slot available */
	condition_variable slot_available;

public:
	BoundedBuffer(int _cap):cap(_cap){
        full = 0;
	}
	~BoundedBuffer(){

	}

	void push(vector<char> data){
        unique_lock<mutex> l1(m_sa);
        slot_available.wait(l1, [this]{return cap > 0;}); // wait until space to push
        cap--;
        full++;
        l1.unlock();
        
        unique_lock<mutex> l2(mtx);
        l2.lock();
        q.push(data);
        l2.unlock();
        data_available.notify_one(); // wake up one thread to pop
	}

	vector<char> pop(){
		vector<char> temp;
        unique_lock<mutex> l1(m_da);
        data_available.wait(l1, [this]{return full > 0;}); // wait until we can pop (full = q.size()
        cap++;
        full--;
        l1.unlock();
        
        unique_lock<mutex> l2(mtx);
        l2.lock();
        temp = q.front();
        q.pop();
        l2.unlock();
        slot_available.notify_one(); // wake up one thread to push
		return temp;  
	}
};

#endif /* BoundedBuffer_ */
