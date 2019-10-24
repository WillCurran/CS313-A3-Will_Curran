This is a project completed by Will Curran, a student at Texas A&M University, for CSCE 313. It is not intended to be a resource for other students of this class.

Objective: Facilitate data transfer between two processes using threads to increase efficiency.


Given info to students:

Your code must also incorporate the following modifications compared to PA2:
• Your client program should accept all the command line arguments: n, p, w, b, f.
Based on whether the f argument was provided, the client chooses to request data or
a file. All the other arguments are optional.
• Start all threads (e.g., p patient threads and w worker threads) and wait for the threads
to finish. Time your program under different setting and collect runtime for each
setting. You need to wait for all threads to finish completely using the thread::join()
function
• For data requests, your client program should call HistogramCollection::print()
function at the end. If you program is working correctly, the final output should show
n for each person.
• Your program should include a functional BoundedBuffer class that is thread-safe and
guarded against over-flow and under-flow.
