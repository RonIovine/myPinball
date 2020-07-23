#include <time.h>
#include <Module.h>

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Module::Module(const char *type_, int row_,int col_)
{
  _row = row_;
  _col = col_;

  _pointsType = PERSISTENT;
  _points = 0;
  _defaultPoints = 0;

  _initialCommand = NULL;

  _actionDelayType = PERSISTENT;
  _actionDelay = 0;
  _defaultActionDelay = 0;
  _actionType = PERSISTENT;
  _action = NULL;
  _defaultAction = NULL;

  _totalEvents = 0;
  _totalScore = 0;
  _ballEvents = 0;
  _ballScore = 0;
  
  _isActive = true;
  _isDefaultActive = true;
  _activeType = PERSISTENT;

  _state = NO_STATE;
  _numCommands = 0;
  
  _eventNum = 0;
  
  setType(type_);
  setName("none");
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void Module::init(void)
{
  if (_initialCommand != NULL)
  {
    TRACE_INFO("Initializing: %s: %s", _type, _name);
    runCommand(_initialCommand);
  }
  else
  {
    TRACE_ERROR("Initial command not set for module, %s: %s", _type, _name);
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void Module::addCommand(Command *command_)
{
  if (_numCommands < MAX_COMMANDS)
  {
    _commands[_numCommands++] = command_;
    TRACE_DEBUG("Adding command: %s, to module: %s", command_->name, getName());
  }
  else
  {
    TRACE_ERROR("Max commands: %d reached, command: %s not added to module: %s, %s",
                MAX_COMMANDS,
                command_->name,
                _type,
                _name);
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void Module::runCommand(Command *command_)
{
  TRACE_COMMAND("%s: running command: %s", _name, command_->name);
  if (command_->type == PERSISTENT)
  {
    TRACE_DEBUG("%s: setting state: %s", _name, command_->name);
    _state = command_->id;
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
int Module::queryState(Query query_)
{
  switch (query_)
  {
    case GET_BALL_EVENTS:
      return (getBallEvents());
    case GET_BALL_SCORE:
      return (getBallScore());
    case GET_TOTAL_EVENTS:
      return (getTotalEvents());
    case GET_TOTAL_SCORE:
      return (getTotalScore());
    case GET_ACTIVE:
      return (isActive());
    case GET_STATE:
      return (getState());
    default:
      return (-1);
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void Module::performAction(Action action_, int value_, int type_)
{
  switch (action_)
  {
    case SET_POINTS:
      TRACE_INFO("Setting override points: module: %s, value: %d, type: %s", getName(), value_, ((type_ == PERSISTENT) ? "PERSISTENT" : "TRANSIENT"));
      setOverridePoints(value_, type_);
      break;
    case SET_ACTIVE:
      TRACE_INFO("Setting override active: module: %s, value: %d, type: %s", getName(), value_, ((type_ == PERSISTENT) ? "PERSISTENT" : "TRANSIENT"));
      setOverrideActive(value_, type_);
      break;
    case SET_ACTION:
      TRACE_INFO("Setting override action: module: %s, value: %d, type: %s", getName(), value_, ((type_ == PERSISTENT) ? "PERSISTENT" : "TRANSIENT"));
      setOverrideAction(_commands[value_], type_);
      break;
    case SET_ACTION_DELAY:
      TRACE_INFO("Setting override action-delay: module: %s, value: %d, type: %s", getName(), value_, ((type_ == PERSISTENT) ? "PERSISTENT" : "TRANSIENT"));
      setOverrideActionDelay(value_, type_);
      break;
    case RUN_COMMAND:
      runCommand(value_);
      break;
    default:
      break;
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
const char *Module::getQuery(Query query_)
{
  switch (query_)
  {
    case GET_BALL_EVENTS:
      return ("ball-events");
    case GET_BALL_SCORE:
      return ("ball-score");
    case GET_TOTAL_EVENTS:
      return ("total-events");
    case GET_TOTAL_SCORE:
      return ("total-score");
    case GET_ACTIVE:
      return ("active");
    case GET_STATE:
      return ("state");
    default:
      return ("unknown-query");
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
const char *Module::getAction(Action action_)
{
  switch (action_)
  {
    case SET_POINTS:
      return ("points");
    case SET_ACTIVE:
      return ("active");
    case SET_ACTION:
      return ("action");
    case SET_ACTION_DELAY:
      return ("action-delay");
    case RUN_COMMAND:
      return ("command");
    default:
      return ("unknown-action");
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
const char *Module::getCommand(int id_)
{
  for (int i = 0; i < _numCommands; i++)
  {
    if ((_commands[i] != NULL) && (_commands[i]->id == id_))
    {
      return (_commands[i]->name);
    }
  }
  return ("unknown");
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Command *Module::getCommand(const char *command_)
{
  for (int i = 0; i < _numCommands; i++)
  {
    if ((_commands[i] != NULL) && (strcmp(_commands[i]->name, command_) == 0))
    {
      return (_commands[i]);
    }
  }
  return (NULL);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void Module::runCommand(const char *command_)
{
  for (int i = 0; i < _numCommands; i++)
  {
    if ((_commands[i] != NULL) && (strcmp(_commands[i]->name, command_) == 0))
    {
      runCommand(_commands[i]);
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void Module::runCommand(int index_)
{
  if ((index_ < _numCommands) && (_commands[index_] != NULL))
  {
    runCommand(_commands[index_]);
  }
  else
  {
    TRACE_ERROR("No command found for index: %d", index_);
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
int Module::processEvent(void)
{
  if (_isActive)
  {
    _totalEvents++;
    _totalScore += _points;
    _ballEvents++;
    _ballScore += _points;
    TRACE_EVENT("%s: ballEvents: %d, ballScore: %d, totEvents: %d, totScore: %d",
                getName(),
                getBallEvents(),
                getBallScore(),
                getTotalEvents(),
                getTotalScore());
    performAction();
    return (_points);
  }
  else
  {
    TRACE_EVENT("%s: not active", getName());
    return (0);
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void Module::performAction(void)
{
  struct timespec sleepTime;
  if (_action != NULL)
  {
    if (_actionDelay > 0)
    {
      TRACE_INFO("%s: delaying %d mSec before performing action", getName(), _actionDelay);
      // delay is in mSec
      sleepTime.tv_sec = _actionDelay/1000;
      sleepTime.tv_nsec = (_actionDelay%1000)*1000000;
      nanosleep(&sleepTime, NULL);
    }
    runCommand(_action);
  }
  else
  {
    TRACE_INFO("%s: no action specified", getName());
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void Module::resetBall(void)
{
  _ballEvents = 0;
  _ballScore = 0;
  if (_pointsType == TRANSIENT)
  {
    _points = _defaultPoints;
  }
  if (_actionDelayType == TRANSIENT)
  {
    _actionDelay = _defaultActionDelay;
    _action = _defaultAction;
  }
  if (_actionType == TRANSIENT)
  {
    _action = _defaultAction;
  }
  if (_activeType == TRANSIENT)
  {
    _isActive = _isDefaultActive;
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void Module::resetGame(void)
{
  resetBall();
  _totalEvents = 0;
  _totalScore = 0;
  _points = _defaultPoints;
}



