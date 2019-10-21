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
    int full; // size of q, we can't check q.size() or it would be a race condition
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
//        cout << "wait for slot to be open." << endl;
        slot_available.wait(l1, [this]{return cap > 0;}); // wait until space to push
//        cout << "slot is open." << endl;
        cap--;
        full++;
//        cout << "full == " << full << endl;
        l1.unlock();
//        cout << "authorized push waiting for q access..." << endl;
        mtx.lock();
//        cout << "q lock acquired." << endl;
        q.push(data);
//        cout << "q size after pushed = " << q.size() << endl;
        mtx.unlock();
//        cout << "q lock released." << endl;
        data_available.notify_one(); // wake up one thread to pop
	}

	vector<char> pop(){
		vector<char> temp;
        unique_lock<mutex> l1(m_da);
        cout << "wait until poppable..." << endl;
        data_available.wait(l1, [this]{return full > 0;}); // wait until we can pop (full = q.size()
        cout << "popping" << endl;
        cap++;
        full--;
        l1.unlock();
        
        mtx.lock();
        temp = q.front();
        q.pop();
//        cout << "popped." << endl;
        mtx.unlock();
        slot_available.notify_one(); // wake up one thread to push
		return temp;  
	}
    
    void print() {
        while(!q.empty()) {
            vector<char> popped = pop();
            cout << "popped" << endl;
            cout << "about to cast datamsg" << endl;
            datamsg* d = (datamsg *)reinterpret_cast<char*>(popped.data());
            if(d->mtype == QUIT_MSG) {
                cout << "Got quit message." << endl;
                break;
            } else if (d->mtype == DATA_MSG) {
                cout << "Got data message: " << endl;
                cout << "person = " << d->person << endl;
                cout << "secs = " << d->seconds << endl;
                cout << "ecgno = " << d->ecgno << endl;
                cout << "writing data to server." << endl;
            }
        }
    }
};

#endif /* BoundedBuffer_ */
