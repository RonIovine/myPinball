#include <Utils.h> 
#include <Rules.h>
#include <TraceLog.h>

extern GtkWidget *screenImage;
extern char currentImage[300];
extern char currentSound[300];

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
const char *ModuleEvents::getType(Type type_)
{
  switch (type_)
  {
    case ANY:
      return ("ANY");
    case GROUP:
      return ("GROUP");    
    case SEQUENCE:
      return ("SEQUENCE");
    default:
      return ("UNKNOWN");
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void ModuleEvents::addEvent(Module *module_, Type type_)
{
  if (numModules < MAX_EVENTS)
  {
    TRACE_DEBUG("Adding event: module: %s, type: %s", module_->getName(), getType(type_));
    modules[numModules++] = module_;
    type = type_;
  }
  else
  {
    TRACE_ERROR("Max events: %d exceeded, could not add module: %s event", MAX_EVENTS, module_->getName());
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool ModuleEvents::evaluateEvents(Module *module_)
{
  // see if we have any events for this module
  if (numModules > 0)
  {
    for (int i = 0; i < numModules; i++)
    {
      if (modules[i] == module_)
      {
        return (true);
      }
    }
    return (false);
  }
  else
  {
    return (true);
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void ModuleQueries::addQuery(Module *module_, Module::Query query_, int value_)
{
  if (numQueries < MAX_QUERIES)
  {
    if (query_ == Module::GET_STATE)
    {
      TRACE_DEBUG("Adding query: %s, value: %s:%d to module: %s", module_->getQuery(query_), module_->getCommand(value_), value_, module_->getName());
    }
    else
    {
      TRACE_DEBUG("Adding query: %s, value: %d to module: %s", module_->getQuery(query_), value_, module_->getName());
    }
    queries[numQueries].module = module_;
    queries[numQueries].query = query_;
    queries[numQueries].value = value_;
    numQueries++;
  }
  else
  {
    TRACE_ERROR("Max queries: %d exceeded, could not add module: %s query", MAX_QUERIES, module_->getName());
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool ModuleQueries::evaluateStates(void)
{
  for (int i = 0; i < numQueries; i++)
  {
    if (queries[i].module->queryState(queries[i].query) != queries[i].value)
    {
      return (false);
    }
  }
  return (true);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void ModuleActions::addAction(Module *module_, Module::Action action_, int value_, int type_)
{
  if (numActions <MAX_ACTIONS)
  {
    if (action_ == Module::RUN_COMMAND)
    {
      TRACE_DEBUG("Adding action: %s, value: %s:%d, type: %s to module: %s",
                  module_->getAction(action_),
                  module_->getCommand(value_),
                  value_,
                  ((type_ == PERSISTENT) ? "PERSISTENT" : "TRANSIENT"),
                  module_->getName());
    }
    else
    {
      TRACE_DEBUG("Adding action: %s, value: %d, type: %s to module: %s",
                  module_->getAction(action_),
                  value_,
                  ((type_ == PERSISTENT) ? "PERSISTENT" : "TRANSIENT"),
                  module_->getName());
    }
    actions[numActions].module = module_;
    actions[numActions].action = action_;
    actions[numActions].value = value_;
    actions[numActions].type = type_;
    numActions++;
  }
  else
  {
    TRACE_ERROR("Max actions: %d exceeded, could not add action: %s, to module: %s",
                MAX_ACTIONS, module_->getAction(action_), module_->getName());
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void ModuleActions::performActions(void)
{
  for (int i = 0; i < numActions; i++)
  {
    actions[i].module->performAction(actions[i].action, actions[i].value, actions[i].type);
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void GlobalData::resetGlobal(void)
{
  _shootAgain = false;
  _drainMultiplier = 0;
  _drainMultiplierType = TRANSIENT;
  _drainBonus = 0;
  _drainBonusType = TRANSIENT;
  _gameOverMultiplier = 0;
  _gameOverBonus = 0;
  _multiBall = 0;
  _imageFileType = TRANSIENT;
  strcpy(_imageFile, _defaultImageFile);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void GlobalData::resetBall(void)
{
  if (_drainMultiplierType == TRANSIENT)
  {
    _drainMultiplier = 0;
  }
  if (_drainBonusType == TRANSIENT)
  {
    _drainBonus = 0;
  }
  if (_imageFileType == TRANSIENT)
  {
    strcpy(_imageFile, _defaultImageFile);
  }
  resetMultiBall();
  _shootAgain = false;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
const char *GlobalActions::getAction(int action_)
{
  switch (action_)
  {
    case SHOOT_AGAIN:
      return ("shoot-again");
    case DRAIN_MULTIPLIER:
      return ("drain-multiplier");
    case DRAIN_BONUS:
      return ("drain-bonus");
    case GAME_OVER_MULTIPLIER:
      return ("game-over-multiplier");
    case GAME_OVER_BONUS:
      return ("game-over-bonus");
    case MULTI_BALL:
      return ("multi-ball");
    case IMAGE_FILE:
      return ("image-file");
    case SOUND_FILE:
      return ("sound-file");
    default:
      return ("unknown");
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void GlobalActions::addAction(int action_, int value_, int type_)
{
  if (numActions < MAX_ACTIONS)
  {
    TRACE_DEBUG("Adding GLOBAL action: %s, value: %d, type: %s",
                getAction(action_),
                value_,
                ((type_ == PERSISTENT) ? "PERSISTENT" : "TRANSIENT"));
    actions[numActions].action = action_;
    actions[numActions].value = value_;
    actions[numActions].type = type_;
    numActions++;
  }
  else
  {
    TRACE_ERROR("Max GLOBAL actions: %d exceeded, could not add action: %s", MAX_ACTIONS, getAction(action_));
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void GlobalActions::addAction(int action_, const char *imageFile_,int type_)
{
  if (numActions < MAX_ACTIONS)
  {
    TRACE_DEBUG("Adding GLOBAL action: %s, value: %s, type: %s",
                getAction(action_),
                imageFile_,
                ((type_ == PERSISTENT) ? "PERSISTENT" : "TRANSIENT"));
    actions[numActions].action = action_;
    strcpy(actions[numActions].imageFile, imageFile_);
    actions[numActions].type = type_;
    numActions++;
  }
  else
  {
    TRACE_ERROR("Max GLOBAL actions: %d exceeded, could not add action: %s", MAX_ACTIONS, getAction(action_));
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void GlobalActions::addSound(int action_, const char *soundFile_,int type_)
{
  if (numActions < MAX_ACTIONS)
  {
    TRACE_DEBUG("Adding GLOBAL action: %s, value: %s, type: %s",
                getAction(action_),
                soundFile_,
                ((type_ == PERSISTENT) ? "PERSISTENT" : "TRANSIENT"));
    actions[numActions].action = action_;
    strcpy(actions[numActions].soundFile, soundFile_);
    actions[numActions].type = type_;
    numActions++;
  }
  else
  {
    TRACE_ERROR("Max GLOBAL actions: %d exceeded, could not add action: %s", MAX_ACTIONS, getAction(action_));
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void GlobalData::setImageFile(const char *imageFile_, int type_)
{
  _imageFileType = type_;
  gdk_threads_add_idle(Utils::setImage, (void *)imageFile_);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void GlobalActions::performActions(GlobalData &global_)
{
  for (int i = 0; i < numActions; i++)
  {
    switch (actions[i].action)
    {
      case SHOOT_AGAIN:
        TRACE_INFO("Setting %s: module: GLOBAL, value: %d", getAction(actions[i].action), actions[i].value);
        global_.setShootAgain(actions[i].value);
        break;
      case DRAIN_MULTIPLIER:
        TRACE_INFO("Setting %s: module: GLOBAL, value: %d, type: %s",
                   getAction(actions[i].action),
                   actions[i].value,
                   ((actions[i].type == PERSISTENT) ? "PERSISTENT" : "TRANSIENT"));
        global_.setDrainMultiplier(actions[i].value, actions[i].type);
        break;
      case DRAIN_BONUS:
        TRACE_INFO("Setting %s: module: GLOBAL, value: %d, type: %s",
                   getAction(actions[i].action),
                   actions[i].value,
                   ((actions[i].type == PERSISTENT) ? "PERSISTENT" : "TRANSIENT"));
        global_.setDrainBonus(actions[i].value, actions[i].type);
        break;
      case GAME_OVER_MULTIPLIER:
        TRACE_INFO("Setting %s: module: GLOBAL, value: %d", getAction(actions[i].action), actions[i].value);
        global_.setGameOverMultiplier(actions[i].value);
        break;
      case GAME_OVER_BONUS:
        TRACE_INFO("Setting %s: module: GLOBAL, value: %d", getAction(actions[i].action), actions[i].value);
        global_.setGameOverBonus(actions[i].value);
        break;
      case MULTI_BALL:
        TRACE_INFO("Setting %s: module: GLOBAL, value: %d", getAction(actions[i].action), actions[i].value);
        global_.setMultiBall(actions[i].value);
        break;
         case IMAGE_FILE:
        TRACE_INFO("Setting %s: module: GLOBAL, value: %s, type: %s",
                   getAction(actions[i].action),
                   actions[i].imageFile,
                   ((actions[i].type == PERSISTENT) ? "PERSISTENT" : "TRANSIENT"));
        global_.setImageFile(actions[i].imageFile, actions[i].type);
        break;
   case SOUND_FILE:
        if (strcmp(currentSound, actions[i].soundFile) != 0)
        {
          TRACE_INFO("Setting %s: module: GLOBAL, value: %s, type: %s",
                     getAction(actions[i].action),
                     actions[i].soundFile,
                     ((actions[i].type == PERSISTENT) ? "PERSISTENT" : "TRANSIENT"));
          Utils::playSound(actions[i].soundFile);
        }
        break;
      default:
        break;
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool Rules::addRule(const char *name_)
{
  if (_numRules < MAX_RULES)
  {
    TRACE_RULE("Adding new rule: '%s'", name_);
    strcpy(_rules[_numRules++].name, name_);
    return (true);
  }
  else
  {
    TRACE_ERROR("Max rules: %d, exceeded, not adding rule: '%s'", MAX_RULES, name_);
    return (false);
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void Rules::processRules(Module *module_)
{
  for (int i = 0; i < _numRules; i++)
  {
    if (_rules[i].moduleEvents.evaluateEvents(module_) && _rules[i].moduleQueries.evaluateStates())
    {
      TRACE_RULE("Rule: %s, passed, performing actions", _rules[i].name);
      _rules[i].moduleActions.performActions();
      _rules[i].globalActions.performActions(_globalData);
    }
  }
}
