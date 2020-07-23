#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/select.h>
#include <QueueThread.h>

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
template<class QueueEntry> QueueThread<QueueEntry>::QueueThread()
{
  _isThreadStarted = false;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
template<class QueueEntry> QueueThread<QueueEntry>::~QueueThread()
{
  stopThread();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
template<class QueueEntry> void QueueThread<QueueEntry>::startThread(void)
{
  if (!_isThreadStarted)
  {
    pthread_mutex_init(&_mutex, NULL);
    pipe(_pipe);
    pthread_create(&_tid, NULL, &QueueThread::queueThread, this);
    _isThreadStarted = true;
  }
  else
  {
    printf("ERROR: QueueThread already started\n");
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
template<class QueueEntry> void QueueThread<QueueEntry>::stopThread(void)
{
  if (_isThreadStarted)
  {
    // make sure nobody is trying to push anything into
    // our queue as we are trying to destroy it
    pthread_mutex_lock(&_mutex);
    pthread_cancel(_tid);
    close(_pipe[0]);
    close(_pipe[1]);
    _isThreadStarted = false;
    pthread_mutex_unlock(&_mutex);
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
template<class QueueEntry> void QueueThread<QueueEntry>::push(QueueEntry entry_)
{
  char writeBuffer[1];
  if (_isThreadStarted)
  {
    printf("entry pushed\n");
    pthread_mutex_lock(&_mutex);
    _queue.push(entry_);
    pthread_mutex_unlock(&_mutex);
    // write one dummy byte to our pipe to signal our thread to wakeup
    write(_pipe[1], writeBuffer, 1);
  }
  else
  {
    printf("ERROR: QueueThread not started\n");
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
template<class QueueEntry> void *QueueThread<QueueEntry>::queueThread(void *this_)
{
  fd_set rfds;
  struct timeval tv;
  unsigned waitTime;
  char readBuffer[1];
  QueueThread *_this = (QueueThread *)this_;

  for (;;)
  {
    
    // watch the pipe fd to see when it has input
    FD_ZERO(&rfds);
    FD_SET(_this->_pipe[0], &rfds);

    if (_this->_timer.getNumTimers() > 0)
    {

      // returns the wait time in mSec
      waitTime = _this->_timer.getWaitTime();

      // get the number of seconds
      tv.tv_sec = waitTime/1000;

      // get the number of uSeconds
      tv.tv_usec = (waitTime%1000)*1000;

      ::select(_this->_pipe[1]+1, &rfds, NULL, NULL, &tv);

    }
    else
    {
      // we have no active timers, wait forever
      // until someone registers a timer, we will
      // be signalled to wake up
      ::select(_this->_pipe[1]+1, &rfds, NULL, NULL, NULL);
    }

    // process any registered timer callback functions
    _this->_timer.processTimers();

    // see if we have have something to read
    if (FD_ISSET(_this->_pipe[0], &rfds))
    {
      // got something to read, read it to clear out the pipe,
      // we don't actually do anything with this one byte of
      // data, we just use it to wake up our thread
      read(_this->_pipe[0], readBuffer, 1);

      // lock the queue mutex so we can process an entry
      pthread_mutex_lock(&_this->_mutex);

      // make sure we have something in the queue
      if (!_this->_queue.empty())
      {
        QueueEntry entry = _this->_queue.front();
        _this->_queue.pop();
        // release the mutex while we process this entry
        pthread_mutex_unlock(&_this->_mutex);
        _this->processEntry(entry);
      }
      else
      {
        // this should never happen, but in case it does,
        // release our queue mutex before we go to sleep
        pthread_mutex_unlock(&_this->_mutex);
      }
    }
  }
  return (NULL);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

#ifdef UNIT_TEST

// two example derived QueueThread classes, note that we can only use
// object types that are copyable in the C++ container classes, hence
// we can only use the actual object and and object pointer, not an
// object reference, if the object type is used and it is a complex
// class, the user needs to be sure it supports the copy constructor
// and that the "shallow copy" problem is addressed, if the object type
// is a pointer, the user needs to ensure the integrity of the pointer
// (i.e. valid and in scope) at the time it is popped and processed

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
class IntQueueThread : public QueueThread<int>
{
  public:
    IntQueueThread() : _privateMember(5) {};
    ~IntQueueThread(){};
    void registerTimer(void);
  private:
    void processEntry(int entry_){printf("IntQueueThread: Processing queue entry: %d\n", entry_);};
    static void timerCallback(const char *name_, unsigned id_, unsigned iteration_, void *parms_);
    int _privateMember;
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void IntQueueThread::registerTimer(void)
{
  _timer.startTimer(timerCallback,
                    "timerCallback",
                    3,
                    this,
                    1000*Timer::ONE_MSEC,
                    5);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void IntQueueThread::timerCallback(const char *name_, unsigned id_, unsigned iteration_, void *parms_)
{
  printf("timerCallback: name: %s, id: %d, iteration: %d, privateMember: %d\n",
         name_,
         id_,
         iteration_,
         ((IntQueueThread*)parms_)->_privateMember);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
class IntPtrQueueThread : public QueueThread<int*>
{
  public:
    IntPtrQueueThread(){};
    ~IntPtrQueueThread(){};
  private:
    void processEntry(int *entry_){printf("IntPtrQueueThread: Processing queue entry: %d\n", *entry_);};
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

// 'main' to test our timer threads
int main(int argc, char *argv[])
{
  int value1 = 1;
  int value2 = 2;
  int value3 = 3;
  int value;
  IntQueueThread intQueueThread;
  intQueueThread.startThread();
  //intQueueThread.push(1);
  //intQueueThread.push(2);
  //intQueueThread.push(3);
  IntPtrQueueThread intPtrQueueThread;
  intPtrQueueThread.startThread();
  value = 1;
  //intPtrQueueThread.push(&value1);
  value = 2;
  //intPtrQueueThread.push(&value2);
  value = 3;
  //intPtrQueueThread.push(&value3);
  //intQueueThread.registerTimer();
  //sleep(5);
  char command[80];
  for (;;)
  {
    cout<<"Enter command: "<<flush;
    cin>>command;
    if (strcmp(command, "timer") == 0)
    {
      intQueueThread.registerTimer();
    }
    else if (strcmp(command, "push") == 0)
    {
      intQueueThread.push(5);
    }
    else
    {
      cout<<"Valid commands: 'timer', 'push'"<<endl;
    }
  }
  return (0);
}

#endif
