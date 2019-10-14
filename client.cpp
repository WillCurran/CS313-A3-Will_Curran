#include "common.h"
#include "BoundedBuffer.h"
#include "Histogram.h"
#include "common.h"
#include "HistogramCollection.h"
#include "FIFOreqchannel.h"
using namespace std;


void *patient_function(void *arg) // why this arg?
{
    // if datapoint transfer, n. n=100 example:
    for(int i = 0; i < 100; i++) {
        // push to bdd buf
        continue;
    }
    
    // if file, sz/MAX_MESSAGE? - depends how worker deals with a chunk of a file req
    for(int i = 0; i < 200; i++) {
        // push to bdd buf
        continue;
    }
}

void *worker_function(void *arg) // why this arg?
{
    // create a channel
    string s = string(1); // some unique string based on which patient in [1, 15]
    FIFORequestChannel* chan = new FIFORequestChannel(s.c_str(), FIFORequestChannel::CLIENT_SIDE);

    // work on the input datamsg or (part of a?) filemsg
    while(true) {
        // pop from bdd buf and do the work through chan
        
        // terminate when bdd buf size is 0? or is this not for sure when?
        // maybe need to get notified by a conditional variable?
        // or does join take care of the waiting part we're talking about
    }
    
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
	

	/* Join all threads here */
    
    gettimeofday (&end, 0);
	hc.print ();
    int secs = (end.tv_sec * 1e6 + end.tv_usec - start.tv_sec * 1e6 - start.tv_usec)/(int) 1e6;
    int usecs = (int)(end.tv_sec * 1e6 + end.tv_usec - start.tv_sec * 1e6 - start.tv_usec)%((int) 1e6);
    cout << "Took " << secs << " seconds and " << usecs << " micor seconds" << endl;

    MESSAGE_TYPE q = QUIT_MSG;
    chan->cwrite ((char *) &q, sizeof (MESSAGE_TYPE));
    cout << "All Done!!!" << endl;
    delete chan;
    
}
