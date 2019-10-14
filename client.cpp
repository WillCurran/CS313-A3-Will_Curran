#include "common.h"
#include "BoundedBuffer.h"
#include "Histogram.h"
#include "common.h"
#include "HistogramCollection.h"
#include "FIFOreqchannel.h"
using namespace std;


void *patient_function(BoundedBuffer &b, bool file_transfer, int patient, const char* filename, int num_requests, FIFORequestChannel* master)
{
    if(file_transfer) {
        // first, send 1st message.
        int block_size = sizeof(filemsg) + sizeof(filename);
        char* block = new char[block_size];
        filemsg* getFileLength = (filemsg*) block; // set pointers
        char* filename_to_server = block + sizeof(filemsg); // this pointer's location is constant
        *getFileLength = filemsg(0, 0); // write the data
        strcpy(filename_to_server, filename); // won't need to ever change this if same file
        
        
        // DO WE NEED TO DO THIS FROM HERE OR DO FROM WORKER? - From here because the portion of the block which has the filename in it is important
        master->cwrite((char *)getFileLength, block_size);
        char* buf = master->cread();
        __int64_t* len = (__int64_t*) buf; // cast char ptr to int ptr
        __int64_t len_remaining = *len;
        // figure out how many more to send and do it
        int buf_size = MAX_MESSAGE;
        filemsg* msg = (filemsg*) block; // re-use same block of memory
        while(len_remaining > 0) { // keep writing until nothing left
            if(len_remaining < MAX_MESSAGE) // last portion
                buf_size = (int) len_remaining;
            *msg = filemsg(*len - len_remaining, buf_size);
            vector<char> buf((char*)msg, (char*)msg + sizeof(filemsg)); // copy message to vector<char> format
            b.push(buf);
            len_remaining -= MAX_MESSAGE;
        }
    } else {
        // if datapoint transfer, n. n=100 example:
        for(int i = 0; i < 100; i++) {
            // push to bdd buf
            continue;
        }
    }
    
    delete[] block;
}

void *worker_function(FIFORequestChannel* master)
{
    // create a channel
    MESSAGE_TYPE m = NEWCHANNEL_MSG;
    master->cwrite((char *)&m, sizeof (m));
    char* buf = master->cread(); // contains name of new channel
    string name = buf;
    FIFORequestChannel* chan = new FIFORequestChannel(name, FIFORequestChannel::CLIENT_SIDE);
    
    // check to see if this channel works. it does.
//    datamsg d = datamsg(5, 43.992, 0);
//    chan->cwrite((char *)&d, sizeof (d));
//    buf = chan->cread();
//    double* reply = (double*) buf;
//    cout << *reply << endl;
//    assert(*reply == -0.19);

//    // work on the input datamsg or (part of a?) filemsg
//    while(true) {
//        // pop from bdd buf and do the work through chan
//        // add to histogram or to file depending on request
//
//        // terminate when bdd buf size is 0? or is this not for sure when?
//        // maybe need to get notified by a conditional variable?
//        // or does join take care of the waiting part we're talking about
//    }
    
    // quit the channel
    MESSAGE_TYPE q = QUIT_MSG;
    chan->cwrite ((char *) &q, sizeof (MESSAGE_TYPE));
}
int main(int argc, char *argv[])
{
    int n = 100;    //default number of requests per "patient"
    int p = 10;     // number of patients [1,15]
    int w = 100;    //default number of worker threads
    int b = 20; 	// default capacity of the request buffer, you should change this default
	int m = MAX_MESSAGE; 	// default capacity of the file buffer
    srand(time_t(NULL));
    
    
    int pid = fork();
    if (pid == 0){
		// modify this to pass along m
        execl ("dataserver", "dataserver", (char *)NULL);
        
    }
    
	FIFORequestChannel* chan = new FIFORequestChannel("control", FIFORequestChannel::CLIENT_SIDE);
    BoundedBuffer request_buffer(b);
	HistogramCollection hc;
	
	
	
    struct timeval start, end;
    gettimeofday (&start, 0);

    /* Start all threads here */
    worker_function(chan);

	/* Join all threads here */
    
    gettimeofday (&end, 0);
	hc.print ();
    int secs = (end.tv_sec * 1e6 + end.tv_usec - start.tv_sec * 1e6 - start.tv_usec)/(int) 1e6;
    int usecs = (int)(end.tv_sec * 1e6 + end.tv_usec - start.tv_sec * 1e6 - start.tv_usec)%((int) 1e6);
    cout << "Took " << secs << " seconds and " << usecs << " micro seconds" << endl;

    MESSAGE_TYPE q = QUIT_MSG;
    chan->cwrite ((char *) &q, sizeof (MESSAGE_TYPE));
    cout << "All Done!!!" << endl;
    delete chan;
    
}
