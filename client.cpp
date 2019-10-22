#include "common.h"
#include "BoundedBuffer.h"
#include "Histogram.h"
#include "common.h"
#include "HistogramCollection.h"
#include "FIFOreqchannel.h"
#include <mutex>
#define MIN_HIST -8.5
#define MAX_HIST 8.5
#define NUM_BUCKETS 25
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

void *patient_function(BoundedBuffer* b, int patient, int num_requests, bool isFT, int file_size_bytes)
{
    if(isFT) {
        int buf_size = MAX_MESSAGE;
        char* block = new char[MAX_MESSAGE];
        filemsg* msg = (filemsg*) block;
        int len_remaining = file_size_bytes;
        while(len_remaining > 0) { // keep writing until nothing left
            if(len_remaining < MAX_MESSAGE) // last portion
                buf_size = (int) len_remaining;
            *msg = filemsg(file_size_bytes - len_remaining, buf_size);
            vector<char> buf((char*)msg, (char*)msg + sizeof(filemsg)); // copy message to vector<char> format
            b->push(buf);
            len_remaining -= MAX_MESSAGE;
        }
        delete[] block;
    } else {
        double time = 0.000;
        for(int i = 0; i < num_requests; i++) {
            datamsg d = datamsg(patient, time, 1); // same message every time
            vector<char> buf((char*)&d, (char*)&d + sizeof(d));
            b->push(buf);
            time += 0.004;
        }
    }
}

void *worker_function(BoundedBuffer* b, FIFORequestChannel* w_chan, HistogramCollection* hc, mutex hc_mtx[], string filename)
{
    while(true) {
        vector<char> popped = b->pop();
        datamsg* d = (datamsg *)reinterpret_cast<char*>(popped.data());
//        datamsg q = *d;
        if(d->mtype == QUIT_MSG) {
            b->push(popped); // for other workers to use
            break;
        } else if (d->mtype == DATA_MSG) {
//            cout << "Got data message: " << endl;
//            cout << "person = " << d->person << endl;
//            cout << "secs = " << d->seconds << endl;
//            cout << "ecgno = " << d->ecgno << endl;
//            cout << "writing data to server." << endl;
            w_chan->cwrite((char *)d, sizeof (*d));
            char* buf =  w_chan->cread();
            double* reply = (double*) buf;
//            cout << *reply << endl; // why is this same every time??
            hc_mtx[d->person - 1].lock();
            hc->getHist(d->person)->update(*reply);
            hc_mtx[d->person - 1].unlock();
        } else { // file msg
            // open file
            int fd;
            string new_file = "./received/" + filename;
            if((fd = open(new_file.c_str(), O_RDWR|O_CREAT)) < 0) {
                perror("open");
                _exit(1);
            }
            filemsg* f = (filemsg *)reinterpret_cast<char*>(popped.data());
            int block_size = sizeof(filemsg) + sizeof(filename.c_str());
            w_chan->cwrite((char *)f, block_size);
            char* buf = w_chan->cread();
            lseek(fd, f->offset, SEEK_SET); // advance to where we want to write
            if(write(fd, buf, f->length) < 0) {
                perror("write");
                _exit(1);
            }
            // close file
            if(close(fd) < 0) {
                perror("close");
                exit(1);
            }
        }
    }
    
    MESSAGE_TYPE q = QUIT_MSG;
    w_chan->cwrite ((char *) &q, sizeof (MESSAGE_TYPE));
//    cout << "Worker killed." << endl;
}

int main(int argc, char *argv[])
{
    int n = 15000;    //default number of requests per "patient"
    int p = 2;     // number of patients [1,15] 10
    int w = 500;    //default number of worker threads 100
    int b = 50; 	// default capacity of the request buffer, you should change this default
	int m = MAX_MESSAGE; 	// default capacity of the file buffer
    char* filename;
    MESSAGE_TYPE ncm = NEWCHANNEL_MSG;
    MESSAGE_TYPE q = QUIT_MSG;
    
    int option;
    int errflag = 0;
    extern char *optarg;
    extern int optind, optopt;
    bool isFT = false;

    while ((option = getopt(argc, argv, "n:p:w:b:f:")) != -1) {
        switch(option) {
            case 'n':
                if(isdigit(optarg[0])) {
                    n = atoi(optarg);
                }
                else
                    errflag++;
                break;
            case 'p':
                if(isdigit(optarg[0])) {
                    p = atoi(optarg);
                }
                else
                    errflag++;
                break;
            case 'w':
                if(isdigit(optarg[0]))
                    w = atoi(optarg);
                else
                    errflag++;
                break;
            case 'b':
                if(isdigit(optarg[0]))
                    b = atoi(optarg);
                else
                    errflag++;
                break;
            case 'f':
                filename = optarg;
                isFT = true;
                break;
            case '?':
                errflag++;
        }
    }
    if (errflag) {
        fprintf(stderr, "usage: client [-n <datapoints>] [-p <people>] [-w <worker threads>] OR client [-w <worker threads>] [-f <filename>]\n");
        return 1;
    }
    
    srand(time_t(NULL));
    
    int pid = fork();
    if (pid == 0){
		// modify this to pass along m
        execl ("dataserver", "dataserver", (char *)NULL);
        
    }
    
	FIFORequestChannel* chan = new FIFORequestChannel("control", FIFORequestChannel::CLIENT_SIDE);
    BoundedBuffer request_buffer(b);
    HistogramCollection hc;
    mutex* hc_mtx = new mutex[p];
    thread* workers = new thread[w];
	
    struct timeval start, end;
    gettimeofday (&start, 0);

    if(isFT) {
        string file_str = filename;
        // prepare the first file message
        int block_size = sizeof(filemsg) + sizeof(filename);
        char* block = new char[block_size];
        filemsg* getFileLength = (filemsg*) block;
        char* filename_to_server = block + sizeof(filemsg);
        *getFileLength = filemsg(0, 0);
        strcpy(filename_to_server, filename);
        // send the first file message
        chan->cwrite((char *)getFileLength, block_size);
        char* buf = chan->cread();
        __int64_t* len = (__int64_t*) buf;
        // pre-allocate our file to the designated length
        int fd;
        string new_file = "./received/" + file_str;
        if((fd = open(new_file.c_str(), O_RDWR|O_CREAT)) < 0) {
            perror("open");
            _exit(1);
        }
        lseek(fd, *len, SEEK_SET);
        if(close(fd) < 0) {
            perror("close");
            exit(1);
        }
        // create patient thread
        thread pt(patient_function, &request_buffer, 0, 0, true, *len);
        // create channels, create worker threads
        for(int i = 0; i < w; i++) {
                chan->cwrite((char *)&ncm, sizeof (ncm));
                char* buf = chan->cread();
                string name = buf;
                FIFORequestChannel* w_chan = new FIFORequestChannel(name, FIFORequestChannel::CLIENT_SIDE);
                workers[i] = thread(worker_function, &request_buffer, w_chan, &hc, hc_mtx, file_str);
        }
        // join threads
        pt.join();
        vector<char> quit_data((char*)&q, (char*)&q + sizeof(q));
        request_buffer.push(quit_data); // patient is done, signal for workers to finish
        for(int i = 0; i < w; i++)
            workers[i].join();
        
        delete[] block;
        gettimeofday (&end, 0);
    } else {
        thread* patients = new thread[p];
        
        /* Start all threads here */
        for(int i = 1; i <= p; i++) {
            patients[i-1] = thread(patient_function, &request_buffer, i, n, false, 0);
            Histogram* h = new Histogram(NUM_BUCKETS, MIN_HIST, MAX_HIST); // one for each person
            hc.add(h);
    //        hc_mtx[i-1] = mutex();
        }
        
        for(int i = 0; i < w; i++) { // create channels, create worker threads
            chan->cwrite((char *)&ncm, sizeof (ncm));
            char* buf = chan->cread();
            string name = buf;
    //        cout << "channel " << name << " created for w" << i << endl;
            FIFORequestChannel* w_chan = new FIFORequestChannel(name, FIFORequestChannel::CLIENT_SIDE);
            workers[i] = thread(worker_function, &request_buffer, w_chan, &hc, hc_mtx, "");
        }

        /* Join all threads here */
        for(int i = 0; i < p; i++)
            patients[i].join();
        vector<char> quit_data((char*)&q, (char*)&q + sizeof(q));
        request_buffer.push(quit_data); // patients are done, signal for workers to finish
        for(int i = 0; i < w; i++)
            workers[i].join();
        delete[] patients;
        
        gettimeofday (&end, 0);
        hc.print ();
    }
    
    
    int secs = (end.tv_sec * 1e6 + end.tv_usec - start.tv_sec * 1e6 - start.tv_usec)/(int) 1e6;
    int usecs = (int)(end.tv_sec * 1e6 + end.tv_usec - start.tv_sec * 1e6 - start.tv_usec)%((int) 1e6);
    cout << "Took " << secs << " seconds and " << usecs << " micro seconds" << endl;

    
    chan->cwrite ((char *) &q, sizeof (MESSAGE_TYPE));
    cout << "All Done!!!" << endl;
    delete chan;
    delete[] workers;
    delete[] hc_mtx;
}
