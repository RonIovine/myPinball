#ifndef QUEUE_THREAD_H
#define QUEUE_THREAD_H

#include <queue>
#include <pthread.h>
#include <Timer.h>

using namespace std;

// note, when using this class, the application must be linked with -lpthread and -lpshell-server

template <class QueueEntry> class QueueThread
{
  public:


    QueueThread();
    ~QueueThread();

    void startThread(void);    
    void stopThread(void);    
    void push(QueueEntry entry_);

  protected:

    // make the timer member protected so our inherited classes can use it
    Timer _timer;  

  private:
    
    // this is the actual user defined function that is called to
    // process an entry that is popped from the queue, this function
    // should NOT have any infinite loop control mechanism, that is
    // handled by the actual thread function below, this function must
    // be defined in the derived classes of this base class
    virtual void processEntry(QueueEntry entry_) = 0;

    // this is the function that the actual 'pthread_create' is done on,
    // it has the infinite thread loop control structure with the sleep
    static void *queueThread(void*);

    queue<QueueEntry> _queue;
    pthread_mutex_t _mutex;
    pthread_t _tid;
    int _pipe[2];
    bool _isThreadStarted;

};

#endif
