#ifndef TIMER_HH
#define TIMER_HH

#include <iostream>
#include <sys/time.h>
#include <pthread.h>

using namespace std;

class Semaphore
{
public:
  Semaphore(){::pthread_mutex_init(&_mutex, NULL);};
  ~Semaphore(){};
  void lock(void){::pthread_mutex_lock(&_mutex);};
  bool trylock(void){return (::pthread_mutex_trylock(&_mutex) == 0);};
  void unlock(void){::pthread_mutex_unlock(&_mutex);};
private:
  pthread_mutex_t _mutex; 
};

// timer function callback prototype
typedef void (*TimerFunction_t) (const char *name_, unsigned id_, unsigned iteration_, void *parms_);

// high resolution time tick counter
#ifndef hrtime_t
typedef long long hrtime_t;
#else
#define HRTIME
#endif

/////////////////////////////////////////////
//
// Timer class 
// 
/////////////////////////////////////////////

#define TIMESTAMP_SIZE 80

class TimerEntry;

class Timer
{

public:

  // public enums

  // used for the elapsed and delta times,
  // also used to specify the desired units
  // for "getWaitTime" and "getDuration"

  enum Units
  {
    NSEC,
    USEC,
    MSEC,
    SECOND,
    MINUTE,
    HOUR
  };

  // used for setting the timer durations, can't use enums
  // because we need 64 bits for the proper resolution
  // (enums are only 32 bits)

  // Durations "enum"

  static const hrtime_t ONE_NSEC;
  static const hrtime_t ONE_USEC;
  static const hrtime_t ONE_MSEC;
  static const hrtime_t ONE_SECOND;
  static const hrtime_t ONE_MINUTE;
  static const hrtime_t ONE_HOUR;

  // used for setting the nuber of iterations

  enum Iterations
  {
    FOREVER,
    SINGLE
  };

  // make some friends

  friend ostream& operator<<(ostream &stream_, Timer &timer_);
  friend class TimerEntry;

  // public constructor/destructor

  Timer(Units units_ = MSEC);
  ~Timer();

  // initialiaze the timer list class for stand alone timers,
  // i.e. it uses its own thread context

  void initStandAlone(unsigned priority_ = 0);

  // start a timer

  void startTimer(TimerFunction_t function_,
                  const char *name_,
                  unsigned id_,
                  hrtime_t duration_,
                  unsigned numIterations_ = SINGLE);  
  
  void startTimer(TimerFunction_t function_,
                  const char *name_,
                  unsigned id_,
                  void *parms_,
                  hrtime_t duration_,
                  unsigned numIterations_ = SINGLE);
             
  // stop a timer

  void stopTimer(TimerFunction_t function_, unsigned id_);

  // return the time (in the specified Units) 
  // until the next timer is ready to go, use 
  // this as the value to pass into a "select" call

  hrtime_t getWaitTime(Units units_ = MSEC);

  // process all timers ready to go, call this
  // after popping out of the select call
  
  void processTimers(void);

  // get the number of currently active timers, use this
  // to determine whether to block forever in the "select"
  // call or not

  unsigned getNumTimers(void){return (_numTimers);};

  // test functions

  static void timerTestFunction(const char *name_, unsigned id_, unsigned iteration_, void *parms_);

  // blocking inline delay function for the speficied duration,
  // NOTE: Use the "Durations enum" (i.e. static const variables) 
  // above to specify the durations

  static void sleep(hrtime_t duration_);

  // return system timestamp in default format: MM/DD/YYYY HH:MM:SS
  // can override default format with user specified format

  char *getTimeStamp(const char *format_ = NULL);

  // returns the elapsed time (in Units) since
  // an arbitrary system start time

  hrtime_t getElapsedTime(void){return (getHrTime()/getConversion(_units));};

  // returns the delta time (in Units) since
  // the previous call to this same function

  hrtime_t getDeltaTime(void);

  // set/get the internal time units

  void setUnits(Units units_){_units = units_;};
  Units getUnits(void){return (_units);};
  const char *getUnits(Units units_);  // returns a string representation
  
private:
  
  enum Conversions
  {
    NSEC_PER_SEC = 1000000000,
    NSEC_PER_MSEC = 1000000,
    NSEC_PER_USEC = 1000
  };

  static hrtime_t getHrTime(void);
  static hrtime_t getConversion(Units units_);

  // don't allow these

  Timer(const Timer&);
  Timer &operator=(const Timer&);

  // returns true if there is a timer (at the front of the list)
  // to be processed, false otherwise
  bool isTimerExpired(void){return (!((_list == NULL) || (getWaitTime() > 0)));};

  void addTimer(TimerEntry *timer_);
  TimerEntry *removeTimer(TimerFunction_t function_, unsigned id_);

  // stand-alone timer methods
  static void *startTimerThread(void *timer_);
  void timerThread(void);

  // private member data

  // endure integrity of list
  Semaphore _semaphore; 
  unsigned _numTimers;
  // acutal timer entry list
  TimerEntry *_list;
  // used for stand-alone timers
  pthread_t _tid; 
  bool _standAlone;
  int _pipeFds[2];
  // used for the delta time utility functions
  hrtime_t _prevTime;
  hrtime_t _currTime;
  Units _units;
  // holds the time of day timestamp
  char _timeStampBuffer[TIMESTAMP_SIZE];

};

inline ostream& operator<<(ostream& stream_, Timer::Units units_)
{
  switch (units_)
    {
    case Timer::NSEC:
      return (stream_<<"nSec");
    case Timer::USEC:
      return (stream_<<"uSec");
    case Timer::MSEC:
      return (stream_<<"mSec");
    case Timer::SECOND:
      return (stream_<<"Sec");
    case Timer::MINUTE:
      return (stream_<<"Min");
    case Timer::HOUR:
      return (stream_<<"Hour");
    default:
      return (stream_<<"???");
    }
}

/////////////////////////////////////////////
//
// TimerList class inline functions
// 
/////////////////////////////////////////////

inline const char *Timer::getUnits(Units units_)
{
  switch (units_)
    {
    case Timer::NSEC:
      return ("nSec");
    case Timer::USEC:
      return ("uSec");
    case Timer::MSEC:
      return ("mSec");
    case Timer::SECOND:
      return ("Sec");
    case Timer::MINUTE:
      return ("Min");
    case Timer::HOUR:
      return ("Hour");
    default:
      return ("???");
    }
}

/////////////////////////////////////////////
//
// TimerEntry class 
// 
/////////////////////////////////////////////

class TimerEntry
{

public:

  // make some friends

  friend class Timer;
  friend ostream& operator<<(ostream& stream_, Timer &timer_);

private:

  // don't allow these, only the Timer class can create a TimerEntry

  TimerEntry();
  ~TimerEntry(){};

  TimerEntry(const TimerEntry &);
  TimerEntry &operator=(const TimerEntry &);

  // constructors/destructors

  TimerEntry(TimerFunction_t function_,
             const char *name_,
             unsigned id_,
             void *parms_,
             hrtime_t duration_,
             unsigned numIterations_ = Timer::SINGLE); 

  // accessor methods

  TimerFunction_t getFunction(void){return (_function);};
  const char *getName(void){return (_name);};
  unsigned getId(void){return (_id);};
  // returns time (in the specified Units) until 
  // the timer will expire value will be negative 
  // if timer is past due
  hrtime_t getWaitTime(Timer::Units units_ = Timer::MSEC){return ((getExpirationTime() - Timer::getHrTime())/Timer::getConversion(units_));};
  unsigned getIterationCount(void){return (_iterationCount);};
  hrtime_t getExpirationTime(void){return (_expirationTime);};

  hrtime_t getDuration(Timer::Units units_ = Timer::MSEC){return (_duration);};
  void setDuration(hrtime_t duration_){_duration = duration_;_expirationTime = Timer::getHrTime() + duration_;};

  void *getParms(void){return (_parms);};
  void setParms(void *parms_){_parms = parms_;};

  unsigned getNumIterations(void){return (_numIterations);};
  void setNumIterations(unsigned numIteratons_){_numIterations = _iterationCount = numIteratons_;_isPersistent = (numIteratons_ == Timer::FOREVER);};

  TimerEntry *getNext(void){return (_next);};
  void setNext(TimerEntry *timerEntry_){_next = timerEntry_;};

  bool isKill(void){return ((!isPersistent() && (getIterationCount() == 0)));};
  bool isPersistent(void){return (_isPersistent);};

  
  void dispatch(void);

  // member data

  TimerFunction_t _function;
  unsigned _id;
  void *_parms;
  hrtime_t _duration;
  unsigned _numIterations;
  unsigned _iterationCount;
  bool _isPersistent;
  hrtime_t _expirationTime;
  char _name[80];
  TimerEntry *_next;

};

#endif
