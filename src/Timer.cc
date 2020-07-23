#include <string.h>
#include <iomanip>
#include <ctype.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <sys/times.h>

#include <TraceLog.h>
#include <Timer.h>

/////////////////////////////////////////////
//
// Timer class 
// 
/////////////////////////////////////////////

// static data members, initialization

// Durations "enum"

const hrtime_t Timer::ONE_NSEC = 1;
const hrtime_t Timer::ONE_USEC = Timer::ONE_NSEC*1000;
const hrtime_t Timer::ONE_MSEC = Timer::ONE_USEC*1000;
const hrtime_t Timer::ONE_SECOND = Timer::ONE_MSEC*1000;
const hrtime_t Timer::ONE_MINUTE = Timer::ONE_SECOND*60;
const hrtime_t Timer::ONE_HOUR = Timer::ONE_MINUTE*60;

// constructor

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Timer::Timer(Units units_) :
  _numTimers(0),
  _list(NULL),
  _standAlone(false),
  _units(units_)
{
  _prevTime = getHrTime();
}

// destructor

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Timer::~Timer()
{
  // nuke the whole list
  if (_list != NULL)
  {
    TimerEntry *current = _list;
    TimerEntry *next = _list->getNext();
    while (current != NULL)
    {
      delete (current);
      current = next;
      if (next != NULL)
      {
        next = next->getNext();
      }
    }
  }
}

// static functions

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
hrtime_t Timer::getHrTime(void)
{
#ifdef HRTIME
  // we have true nsec resolution hrtime
  return (::gethrtime());
#else
  struct timeval timeBuffer;
  ::gettimeofday(&timeBuffer, NULL);
  return (((hrtime_t)timeBuffer.tv_sec)*NSEC_PER_SEC + ((hrtime_t)timeBuffer.tv_usec)*NSEC_PER_USEC);
#endif
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
hrtime_t Timer::getConversion(Units units_)
{
  switch (units_)
  {
  case Timer::NSEC:
    return (ONE_NSEC);
  case Timer::USEC:
    return (ONE_USEC);
  case Timer::MSEC:
    return (ONE_MSEC);
  case Timer::SECOND:
    return (ONE_SECOND);
  case Timer::MINUTE:
    return (ONE_MINUTE);
  case Timer::HOUR:
    return (ONE_HOUR);
  default:
    return (ONE_NSEC);
  }
}

// member functions

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void Timer::sleep (hrtime_t duration_)
{
  timespec t1, t2; 
  t1.tv_sec = duration_/NSEC_PER_SEC;
  t1.tv_nsec = duration_%NSEC_PER_SEC;
  ::nanosleep(&t1, &t2);
}

// returns the delta time (in Units) since
// the previous call to this same function

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
hrtime_t Timer::getDeltaTime(void)
{
  hrtime_t deltaTime;
  _currTime = getHrTime();
  deltaTime = _currTime - _prevTime;
  _prevTime = _currTime;
  return ((hrtime_t)(deltaTime/getConversion(_units)));
}

// return system timestamp in default format: MM/DD/YYYY HH:MM:SS
// can override default format with user specified format

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
char *Timer::getTimeStamp(const char *format_)
{

  struct tm *timeBufferPtr;
  time_t time = ::time(0);

#ifdef localtime_r
  struct tm timeBuffer;
  ::localtime_r(&tm, &timeBuffer);
  timeBufferPtr = &timeBuffer;
#else
  timeBufferPtr = ::localtime(&time);
#endif

  if (format_ ==  NULL)
  {
    // Default timestamp format: MM/DD/YYYY HH:MM:SS
    ::strftime(_timeStampBuffer, TIMESTAMP_SIZE, "%D %T", timeBufferPtr); 
  }
  else
  {
    // Use the format passed in
    ::strftime(_timeStampBuffer, TIMESTAMP_SIZE, format_, timeBufferPtr); 
  }
  return (_timeStampBuffer);
}

// returns the wait time in the specified units (default to mSec)
// for the top timer on the list if timer is past due it will return 0

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
hrtime_t Timer::getWaitTime(Timer::Units units_)
{
  hrtime_t waitTime = _list->getWaitTime(units_);
  return ((waitTime <= 0) ? 0 : waitTime);
}

// add a timer to the list in sorted order 
// (lowest to highest expiration time)

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void Timer::addTimer(TimerEntry *timer_)
{

  _semaphore.lock();

  TimerEntry *current = _list;
  TimerEntry *previous = NULL;

  while (current != NULL)
  {
    if (current->getExpirationTime() > timer_->getExpirationTime())
    {
      break;
    }
    previous = current;
    current = current->getNext();
  }

  // link it up in in its  proper position
  
  timer_->setNext(current);

  if (previous != NULL)
  {
    previous->setNext(timer_);
  }
  else
  {
    _list = timer_;
  }

  _numTimers++;

  _semaphore.unlock();

}

// remove timer from list (if found) and return its pointer (NULL if not found)

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
TimerEntry *Timer::removeTimer(TimerFunction_t function_, unsigned id_)
{

  _semaphore.lock();

  TimerEntry *current = _list;
  TimerEntry *previous = NULL;

  // look for the current entry and remove it if found
  while (current != NULL)
  {
    if ((current->getFunction() == function_) && (current->getId() == id_))
    {
      if (previous != NULL)
      {
        previous->setNext(current->getNext());
      }
      else
      {
        _list = current->getNext();
      }
      _numTimers--;
      _semaphore.unlock();
      return (current);
    }
    previous = current;
    current = current->getNext();
  }

  _semaphore.unlock();
  // not found, return NULL
  return (NULL);

}

// start a new timer or restart an existing timer

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void Timer::startTimer(TimerFunction_t function_,
                       const char *name_,
                       unsigned id_,
                       void *parms_,
                       hrtime_t duration_,
                       unsigned numIterations_)
{
  
  TimerEntry *timer = NULL;

  if ((timer = removeTimer(function_, id_)) != NULL)
  {
    // timer found, reset paramaters and re-add to list
    timer->setDuration(duration_);
    timer->setNumIterations(numIterations_);
    addTimer(timer);
  }
  else
  {
    // timer not found, create a new one and add it to list
    TimerEntry *timer = new TimerEntry(function_,
                                       name_,
                                       id_,
                                       parms_,
                                       duration_,
                                       numIterations_);
    addTimer(timer);
  }

  // we are running in stand alone mode, signal the timer
  // thread to wake up because we need to (possibly) change
  // our wait time to take into account the newly added timer

  if (_standAlone)
  {
    // write one byte to the pipe to wake up the thread
    char writeBuffer[1];
    ::write(_pipeFds[1], writeBuffer, 1);
  }

}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void Timer::startTimer(TimerFunction_t function_,
                       const char *name_,
                       unsigned id_,
                       hrtime_t duration_,
                       unsigned numIterations_)
{
  startTimer(function_, name_, id_, NULL, duration_, numIterations_);
}
  
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void Timer::stopTimer(TimerFunction_t function_, unsigned id_)
{
  TimerEntry *timer;
  if ((timer = removeTimer(function_, id_)) != NULL)
  {
    delete (timer);
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void Timer::processTimers(void)
{

  TimerEntry *timer;
  TimerFunction_t function;
  unsigned id;

  // dispatch all the timers that are ready to go
  while (isTimerExpired())
  {
     
    // save function and id because we will need it later 
    function = _list->getFunction();
    id = _list->getId();

    // dispatch the first function on the list
    _list->dispatch();

    // see if timer is still on the list, 
    // our callback could have stopped it
    if ((timer = removeTimer(function, id)) != NULL)
    {
      // it's still on the list,
      // see if we need to kill it
      if (timer->isKill())
      {
        delete (timer);
      }
      else
      {
        // if we don't need to kill it add the timer
        // back to the list in its proper position
        addTimer(timer);
      }
    }

  }
  
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void Timer::initStandAlone(unsigned priority_)
{
  if (!_standAlone)
  {
    ::pipe(_pipeFds);
    ::pthread_create(&_tid, NULL, Timer::startTimerThread, (void *)this);
    _standAlone = true;
  }
}
  
void *Timer::startTimerThread(void *timer_)
{
   ((Timer *)timer_)->timerThread();
   return (NULL);
}

void Timer::timerThread(void)
{

  fd_set rfds;
  struct timeval tv;
  unsigned waitTime;
  char readBuffer[1];

  TRACE_INFO("Stand alone timer thread started...");
	  
  for (;;)
  {
    // Watch the pipe fd to see when it has input
    FD_ZERO(&rfds);
    FD_SET(_pipeFds[0], &rfds);

    if (getNumTimers() > 0)
    {

      // returns the wait time in mSec
      waitTime = getWaitTime();

      // get the number of seconds
      tv.tv_sec = waitTime/1000;

      // get the number of uSeconds
      tv.tv_usec = (waitTime%1000)*1000;

      ::select(_pipeFds[1]+1, &rfds, NULL, NULL, &tv);

    }
    else
    {
      // we have no active timers, wait forever
      // until someone registers a timer, we will
      // be signalled to wake up
      ::select(_pipeFds[1]+1, &rfds, NULL, NULL, NULL);
    }

    // see if we have timed out or have something to read
    if (FD_ISSET(_pipeFds[0], &rfds))
    {
      // got something to read, read it
      ::read(_pipeFds[0], readBuffer, 1);
    }

    // process any timers
    processTimers();
      
  }

}
  
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void Timer::timerTestFunction(const char *name_, unsigned id_, unsigned iteration_, void *parms_)
{
  TRACE_DEBUG("timerTestFunction called, name: %s, id: %d, iteration: %d", name_, id_, iteration_);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
ostream& operator<<(ostream& stream_, Timer &timer_)
{

  timer_._semaphore.lock();

  TimerEntry *current = timer_._list;

  stream_<<"Timer Name            Id          Duration  Persistent  Iterations  Count  Expiration Time"<<endl;
  stream_<<"--------------------  ----------  --------  ----------  ----------  -----  ---------------"<<endl;

  stream_<<setfill(' ');
  stream_.setf(ios::left);

  while (current != NULL)
  {

    stream_<<setw(20)<<current->getName()<<"  "
           <<setw(10)<<current->getId()<<"  "
           <<setw(8)<<current->getDuration()<<"  ";

    if (current->isPersistent())
    {
      stream_<<setw(10)<<"YES"<<"  "<<setw(10)<<"N/A"<<"  "
             <<setw(5)<<current->getIterationCount()<<"  ";
    }
    else
    {
      stream_<<setw(10)<<"NO"<<"  "<<setw(10)<<current->getNumIterations()<<"  "
             <<setw(5)<<current->getIterationCount()<<"  ";
    }
      
    stream_<<setw(15)<<current->getWaitTime()<<endl;

    current = current->getNext();

  }

  timer_._semaphore.unlock();

  return (stream_);
}
  
/////////////////////////////////////////////
//
// TimerEntry class 
// 
/////////////////////////////////////////////

// constructors

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
TimerEntry::TimerEntry(TimerFunction_t function_,
		       const char *name_,
		       unsigned id_,
           void *parms_,
		       hrtime_t duration_,
		       unsigned numIterations_) :
  _function(function_),
  _id(id_),
  _parms(parms_),
  _next(NULL)
{
  setNumIterations(numIterations_);
  setDuration(duration_);
  ::strcpy(_name, name_);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void TimerEntry::dispatch(void)
{
  
  // reset the duration and expiration time 
  // in case we need to keep this timer going

  setDuration(getDuration());

  if (!isPersistent())
  {      
    // decrement iteration count
    _iterationCount--;
  }
  else
  {
    // increment iteration count
    _iterationCount++;
  }

  TRACE_DEBUG("Calling Timer: %s, id: %d, duration: %llu, iteration: %d",
              getName(), getId(), getDuration(), getIterationCount());

  // dispatch function
  getFunction()(getName(), getId(), getIterationCount(), getParms());
  
}

