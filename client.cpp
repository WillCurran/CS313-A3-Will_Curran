#include "common.h"
#include "BoundedBuffer.h"
#include "Histogram.h"
#include "common.h"
#include "HistogramCollection.h"
#include "FIFOreqchannel.h"
using namespace std;


struct PatientData {
    BoundedBuffer &b;
    bool file_transfer;
    int patient;
    const char* filename;
    int num_requests;
    FIFORequestChannel* master;
    PatientData(BoundedBuffer &bb, bool ft, int p, const char* fn, int nr, FIFORequestChannel* m) :
        b(bb), file_transfer(ft), patient(p), filename(fn), num_requests(nr), master(m) {}
};

void *patient_function(BoundedBuffer* b, int patient, int num_requests)
{
//    int block_size = sizeof(filemsg) + sizeof(pd.filename);
//    char* block = new char[block_size];
//    if(pd.file_transfer) {
//        // first, send 1st message.
//        filemsg* getFileLength = (filemsg*) block; // set pointers
//        char* filename_to_server = block + sizeof(filemsg); // this pointer's location is constant
//        *getFileLength = filemsg(0, 0); // write the data
//        strcpy(filename_to_server, pd.filename); // won't need to ever change this if same file
//
//
//        // DO WE NEED TO DO THIS FROM HERE OR DO FROM WORKER? - From here because the portion of the block which has the filename in it is important
//        pd.master->cwrite((char *)getFileLength, block_size);
//        char* buf = pd.master->cread();
//        __int64_t* len = (__int64_t*) buf; // cast char ptr to int ptr
//        __int64_t len_remaining = *len;
//        // figure out how many more to send and do it
//        int buf_size = MAX_MESSAGE;
//        filemsg* msg = (filemsg*) block; // re-use same block of memory
//        while(len_remaining > 0) { // keep writing until nothing left
//            if(len_remaining < MAX_MESSAGE) // last portion
//                buf_size = (int) len_remaining;
//            *msg = filemsg(*len - len_remaining, buf_size);
//            vector<char> buf((char*)msg, (char*)msg + sizeof(filemsg)); // copy message to vector<char> format
//            pd.b.push(buf);
//            len_remaining -= MAX_MESSAGE;
//        }
//    } else {
        double time = 0.000;
        for(int i = 0; i < num_requests; i++) {
            datamsg d = datamsg(1, 0.000, 1); // same message every time
            vector<char> buf((char*)&d, (char*)&d + sizeof(d));
            b->push(buf);
            time += 0.004;
        }
//    }
    
//    delete[] block;
}

void *worker_function(BoundedBuffer* b, FIFORequestChannel* w_chan)
{
    
    // check to see if this channel works. it does.
//    datamsg d = datamsg(5, 43.992, 0);
//    w_chan->cwrite((char *)&d, sizeof (d));
//    char* buf = w_chan->cread();
//    double* reply = (double*) buf;
//    cout << *reply << endl;
//    assert(*reply == -0.19);

    // work on the input datamsg or (part of a?) filemsg
    int count = 1;
    while(true) {
        // pop from bdd buf and do the work through chan
//        cout << "waiting to pop." << endl;
        vector<char> popped = b->pop();
//        char* mydata = popped.data();
//        cout << "popped" << endl;
//        cout << "about to cast datamsg" << endl;
//        cout << popped.size() << endl;
//        fwrite(mydata, 1, 1, stdout);
        datamsg* d = (datamsg *)reinterpret_cast<char*>(popped.data());
        datamsg q = *d;
//        cout << "datamsg of type: " << q.mtype << endl;
        
        if(d->mtype == QUIT_MSG) {
//            cout << "worker quitting." << endl;
            b->push(popped); // for other workers to use
            break;
        } else if (d->mtype == DATA_MSG) {
//            cout << "Got data message: " << endl;
//            cout << "person = " << d->person << endl;
//            cout << "secs = " << d->seconds << endl;
//            cout << "ecgno = " << d->ecgno << endl;
            cout << "count is " << count << " out of 2000" << endl << endl;
//            cout << "writing data to server." << endl;
//            w_chan->cwrite((char *)d, sizeof (d));
//            char* buf =  w_chan->cread();
//            double* reply = (double*) buf;
//            cout << *reply << endl; // why is this same every time??
        } else { // file msg

        }
        // add to histogram or to file depending on request
        count++;
    }
    
    MESSAGE_TYPE q = QUIT_MSG;
    w_chan->cwrite ((char *) &q, sizeof (MESSAGE_TYPE));
    cout << "Worker killed." << endl;
}
int main(int argc, char *argv[])
{
    int n = 1000;    //default number of requests per "patient"
    int p = 2;     // number of patients [1,15] 10
    int w = 1;    //default number of worker threads 100
    int b = 100000; 	// default capacity of the request buffer, you should change this default
	int m = MAX_MESSAGE; 	// default capacity of the file buffer
    MESSAGE_TYPE ncm = NEWCHANNEL_MSG;
    MESSAGE_TYPE q = QUIT_MSG;
    srand(time_t(NULL));
    
    int pid = fork();
    if (pid == 0){
		// modify this to pass along m
        execl ("dataserver", "dataserver", (char *)NULL);
        
    }
    
	FIFORequestChannel* chan = new FIFORequestChannel("control", FIFORequestChannel::CLIENT_SIDE);
    BoundedBuffer request_buffer(b);
	HistogramCollection hc;
    
    thread* patients = new thread[p];
    thread* workers = new thread[w];
	
    struct timeval start, end;
    gettimeofday (&start, 0);

    /* Start all threads here */
    for(int i = 1; i <= p; i++) {
//        PatientData pd (request_buffer, false, i, "", 10, chan);
        patients[i-1] = thread(patient_function, &request_buffer, i, n);
    }
    
    for(int i = 0; i < w; i++) { // create channels, create worker threads
        chan->cwrite((char *)&ncm, sizeof (ncm));
        char* buf = chan->cread();
        string name = buf;
        cout << "channel " << name << " created for w" << i << endl;
        FIFORequestChannel* w_chan = new FIFORequestChannel(name, FIFORequestChannel::CLIENT_SIDE);
        workers[i] = thread(worker_function, &request_buffer, w_chan);
    }

	/* Join all threads here */
    for(int i = 0; i < p; i++)
        patients[i].join();
    vector<char> quit_data((char*)&q, (char*)&q + sizeof(q));
    request_buffer.push(quit_data); // patients are done, signal for workers to finish
//    request_buffer.print();
    for(int i = 0; i < w; i++)
        workers[i].join();
    
    delete[] patients;
    delete[] workers;
    
    gettimeofday (&end, 0);
	hc.print ();
    int secs = (end.tv_sec * 1e6 + end.tv_usec - start.tv_sec * 1e6 - start.tv_usec)/(int) 1e6;
    int usecs = (int)(end.tv_sec * 1e6 + end.tv_usec - start.tv_sec * 1e6 - start.tv_usec)%((int) 1e6);
    cout << "Took " << secs << " seconds and " << usecs << " micro seconds" << endl;

    
    chan->cwrite ((char *) &q, sizeof (MESSAGE_TYPE));
    cout << "All Done!!!" << endl;
    delete chan;
    
}
