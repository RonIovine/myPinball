#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <Utils.h> 
#include <TraceLog.h>
#include <Playfield.h>

bool Playfield::_isDatabaseLoaded = false;
int Playfield::_numDatabaseModules = 0;
Playfield::Database Playfield::_moduleDatabase[MAX_MODULES];

extern GtkWidget *screenScore;
extern GtkWidget *playerLabel;
extern GtkWidget *currentBall;
extern GtkWidget *shootAgain;

extern GdkColor green;
extern GdkColor yellow;
extern GdkColor red;
extern GdkColor black;
extern GdkColor grey;

extern GtkWidget *screenImage;
extern char currentImage[300];
extern char currentSound[300];

extern const char *INSTALL_PATH;
extern const char *CONFIG_PATH;
extern const char *GAMEPLAY_PATH;

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Playfield::Playfield()
{
  _ballScore = 0;
  _totalScore = 0;
  _ballEvents = 0;
  _totalEvents = 0;
  _isInitialized = false;
  _eventNum = 0;
  _playerName[0] = 0;
  _currBall = 0;
  _currPlayer = false;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Playfield::~Playfield()
{
  cleanup();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void Playfield::cleanup(void)
{
  if (_isInitialized)
  {
    for (int i = 0; i < _numEntries; i++)
    {
      if (_masterPlayfield[i] != NULL)
      {
        delete _masterPlayfield[i];
      }
    }
    delete [] _masterPlayfield;
    delete [] _activePlayfield;
  }
  _numTargets = 0;
  _numEntries = 0;
  _ballScore = 0;
  _totalScore = 0;
  _ballEvents = 0;
  _totalEvents = 0;
  _isInitialized = false;
  _eventNum = 0;
  _currBall = 0;
  _currPlayer = false;

  for (int i = 0; i < MAX_MODULES; i++)
  {
    _moduleDatabase[i].numModules = 0;
  }

  _rules.resetGlobal();

}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void Playfield::init(void)
{
  cleanup();
  loadDatabase();
  loadPlayfield();
  loadProfile();
  loadGameplay();
  initModules();
  updateDisplay();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void Playfield::initModules(void)
{
  for (int i = 0; i < _numTargets; i++)
  {
    _activePlayfield[i]->init();
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void Playfield::loadDatabase(void)
{
  char line[180];
  char file[300];
  FILE *fp;
  Tokens token1;
  Tokens token2;
  Tokens token3;
  int moduleId = -1;
  int commandId = -1;
  if (!_isDatabaseLoaded)
  {
    sprintf(file, "%s/%s/database.conf", INSTALL_PATH, CONFIG_PATH);
    if ((fp = fopen(file, "r")) != NULL)
    {
      _numDatabaseModules = 0;
      while (fgets(line, sizeof(line), fp) != NULL)
      {
        // NULL terminate
        line[strlen(line)-1] = 0;
        // ignore comment or blank lines 
        if ((line[0] != '#') && (strlen(line) > 0))
        {
          tokenize(line, token1, "=");
          if ((token1.numTokens == 2) && isEqual(token1.tokens[0], "id"))
          {
            TRACE_DEBUG("Database Module: id: %s", token1.tokens[1]);
            moduleId = getInt(token1.tokens[1]);
          }
          else if ((token1.numTokens == 2) && (moduleId >= 0) && (moduleId < MAX_MODULES))
          {
            if (isEqual(token1.tokens[0], "name"))
            {
              TRACE_INFO("Adding Module: name: %s, id: %d, into master database", token1.tokens[1], moduleId);
              strcpy(_moduleDatabase[moduleId].name, token1.tokens[1]);
              _moduleDatabase[moduleId].numCommands = 0;
              _numDatabaseModules++;
            }
            else if (isEqual(token1.tokens[0], "commands"))
            {
              tokenize(token1.tokens[1], token2, ",");
              for (int i = 0; i < token2.numTokens; i++)
              {
                tokenize(token2.tokens[i], token3, ":");
                if (token3.numTokens == 3)
                {
                  commandId = getInt(token3.tokens[1]);
                  TRACE_DEBUG("Loading Command: name: %s, id: %s, type: %s", token3.tokens[0], token3.tokens[1], token3.tokens[2]);
                  strcpy(_moduleDatabase[moduleId].commands[_moduleDatabase[moduleId].numCommands].name, token3.tokens[0]);
                  _moduleDatabase[moduleId].commands[_moduleDatabase[moduleId].numCommands].id = commandId;
                  if (isEqual(token3.tokens[2], "transient"))
                  {
                    _moduleDatabase[moduleId].commands[_moduleDatabase[moduleId].numCommands].type = TRANSIENT;
                  }
                  else if (isEqual(token3.tokens[2], "persistent"))
                  {
                    _moduleDatabase[moduleId].commands[_moduleDatabase[moduleId].numCommands].type = PERSISTENT;
                  }
                  _moduleDatabase[moduleId].numCommands++;
                }
              }
            }
          }
        }
      }
      _isDatabaseLoaded = true;
    }
    else
    {
      TRACE_ERROR("Cannot open database file: 'Database.conf'");
    }  
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void Playfield::loadPlayfield(void)
{
  char line[180];
  char file[300];
  FILE *fp;
  Tokens module;
  Tokens tuple;
  int tuple1;
  int tuple2;
  int index;
  cleanup();
  sprintf(file, "%s/%s/playfield.conf", INSTALL_PATH, CONFIG_PATH);
  if ((fp = fopen(file, "r")) != NULL)
  {
    while (fgets(line, sizeof(line), fp) != NULL)
    {
      // NULL terminate
      line[strlen(line)-1] = 0;
      // ignore comment or blank lines 
      if ((line[0] != '#') && (strlen(line) > 0))
      {
        // start parsing line
        tokenize(line, module, "=");
        if (module.numTokens == 2)
        {
          tokenize(module.tokens[1], tuple, ",");
          if ((tuple.numTokens == 2) && isNumeric(tuple.tokens[0]) && isNumeric(tuple.tokens[1]))
          {
            tuple1 = getInt(tuple.tokens[0]);
            tuple2 = getInt(tuple.tokens[1]);
            if (isEqual(module.tokens[0], "playfield"))
            {
              _numRows = tuple1;
              _numCols = tuple2;
              _numEntries = _numRows*_numCols;
              _numTargets = 0;
              _masterPlayfield = new Module*[_numRows*_numCols];
              _activePlayfield = new Module*[_numRows*_numCols];
              TRACE_INFO("Creating playfield, numRows: %d, numCols: %d", _numRows, _numCols);
              for (int i = 0; i < _numEntries; i++)
              {
                _masterPlayfield[i] = NULL;
                _activePlayfield[i] = NULL;
              }
              _isInitialized = true;
            }
            else if (!isValidModule(module.tokens[0]))
            {
              TRACE_ERROR("Invalid Module: %s, row: %d, col: %d", module.tokens[0], tuple1, tuple2);
            }
            else if ((index = getIndex(tuple1, tuple2)) == INVALID_INDEX)
            {
              TRACE_ERROR("Invalid Position: row: %d, col: %d, module: %s", tuple1, tuple2, module.tokens[0]);
            }
            else
            {
              TRACE_INFO("Adding Module: %s, row: %d, col: %d, into master playfield", module.tokens[0], tuple1, tuple2);
              _masterPlayfield[index] = new Module(module.tokens[0], tuple1, tuple2);
            }
          }
          else
          {
            TRACE_ERROR("Improperly formatted tuple: '%s'", module.tokens[1]);
          }
        }
      }
    }
  }
  else
  {
    TRACE_ERROR("Cannot open playfield file: 'Playfield.conf'");
  }  
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void Playfield::loadProfile(void)
{
  char line[180];
  char name[80];
  char file[300];
  FILE *fp;
  Tokens rowsAndCols;
  Tokens tuple;
  Module *module = NULL;
  Command *command = NULL;
  Database *database = NULL;
  sprintf(file, "%s/%s/profile.conf", INSTALL_PATH, GAMEPLAY_PATH);
  if ((fp = fopen(file, "r")) != NULL)
  {
    while (fgets(line, sizeof(line), fp) != NULL)
    {
      // NULL terminate
      line[strlen(line)-1] = 0;
      // ignore comment or blank lines 
      if ((line[0] != '#') && (strlen(line) > 0))
      {
        // start parsing line
        tokenize(line, tuple, "=");
        if (tuple.numTokens == 2)
        {
          if (isEqual(tuple.tokens[0], "position"))
          {
            // found new position, load the previous module into our active playfield
            if (module != NULL)
            {
              TRACE_INFO("Adding Profile: %s: %s, row: %d, col: %d, into active playfield", module->getType(), module->getName(), module->getRow(), module->getCol());
              _activePlayfield[_numTargets++] = module;
              addCommands(module);
            }
            tokenize(tuple.tokens[1], rowsAndCols, ",");
            module = getModule(getInt(rowsAndCols.tokens[0]), getInt(rowsAndCols.tokens[1]));
            database = getDatabase(module);
            sprintf(name, "%s%d", database->name, ++(database->numModules));
            module->setName(name);
          }
          else if ((module != NULL) && isEqual(tuple.tokens[0], "default-points"))
          {
            TRACE_DEBUG("Adding default points: %s, to %s: %s", tuple.tokens[1], module->getType(), module->getName());
            module->setDefaultPoints(getInt(tuple.tokens[1]));
          }
          else if ((module != NULL) && isEqual(tuple.tokens[0], "default-active"))
          {
            if (isEqual(tuple.tokens[1], "yes"))
            {
              module->setDefaultActive(true);
            }
            else if (isEqual(tuple.tokens[1], "no"))
            {
              module->setDefaultActive(false);
            }
          }
          else if ((module != NULL) && isEqual(tuple.tokens[0], "default-action-delay"))
          {
            module->setDefaultActionDelay(getInt(tuple.tokens[1]));
          }
          else if ((module != NULL) && isEqual(tuple.tokens[0], "default-action"))
          {
            if ((command = findCommand(module->getType(), tuple.tokens[1])) != NULL)
            {
               TRACE_INFO("Adding default action: %s, id: %d, to %s: %s",
                          command->name,
                          command->id,
                          module->getType(),
                          module->getName());
               module->setDefaultAction(command);
            }
          }
          else if ((module != NULL) && isEqual(tuple.tokens[0], "initial-command"))
          {
            if ((command = findCommand(module->getType(), tuple.tokens[1])) != NULL)
            {
               TRACE_INFO("Adding initial command: %s, id: %d, to %s: %s",
                          command->name,
                          command->id,
                          module->getType(),
                          module->getName());
               module->setInitialCommand(command);
            }
          }
        }
      }
    }
    // finished, make sure we get our last module
    if (module != NULL)
    {
      TRACE_INFO("Adding Profile: %s: %s, row: %d, col: %d, into active playfield", module->getType(), module->getName(), module->getRow(), module->getCol());
      _activePlayfield[_numTargets++] = module;
    }    
  }
  else
  {
    TRACE_ERROR("Cannot open playfield file: 'Profile.conf'");
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void Playfield::loadGameplay(void)
{
  char line[300];
  char file[300];
  FILE *fp;
  Tokens token1;
  Tokens token2;
  Tokens token3;
  Tokens token4;
  Module *module;
  Command *command;
  int type;
  bool addRule = true;
  sprintf(file, "%s/%s/gameplay.conf", INSTALL_PATH, GAMEPLAY_PATH);
  if ((fp = fopen(file, "r")) != NULL)
  {
    while (fgets(line, sizeof(line), fp) != NULL)
    {
      // NULL terminate
      line[strlen(line)-1] = 0;
      // ignore comment or blank lines 
      if ((line[0] != '#') && (strlen(line) > 0))
      {
        tokenize(line, token1, "=");
        if (isEqual(token1.tokens[0], "rule"))
        {
          addRule = _rules.addRule(token1.tokens[1]);
        }
        else if (isEqual(token1.tokens[0], "events") && (addRule == true))
        {
          tokenize(token1.tokens[1], token2, ",");
          for (int i = 0; i < token2.numTokens; i++)
          {
            if ((module = getModule(token2.tokens[i])) != NULL)
            {
              _rules.addEvent(module, ModuleEvents::ANY);
            }
            else
            {
              TRACE_ERROR("Could not find Module: %s, not adding event", token2.tokens[i]);
            }
          }
        }
        else if (isEqual(token1.tokens[0], "group") && (addRule == true))
        {
          tokenize(token1.tokens[1], token2, ",");
          for (int i = 0; i < token2.numTokens; i++)
          {
            if ((module = getModule(token2.tokens[i])) != NULL)
            {
              _rules.addEvent(module, ModuleEvents::GROUP);
            }
            else
            {
              TRACE_ERROR("Could not find Module: %s, not adding event", token2.tokens[i]);
            }
          }
        }
        else if (isEqual(token1.tokens[0], "sequence") && (addRule == true))
        {
          tokenize(token1.tokens[1], token2, ",");
          for (int i = 0; i < token2.numTokens; i++)
          {
            if ((module = getModule(token2.tokens[i])) != NULL)
            {
              _rules.addEvent(module, ModuleEvents::SEQUENCE);
            }
            else
            {
              TRACE_ERROR("Could not find Module: %s, not adding event", token2.tokens[i]);
            }
          }
        }
        else if (isEqual(token1.tokens[0], "states") && (addRule == true))
        {
          tokenize(token1.tokens[1], token2, ",");
          for (int i = 0; i < token2.numTokens; i++)
          {
            tokenize(token2.tokens[i], token3, ".");
            if ((module = getModule(token3.tokens[0])) != NULL)
            {
              tokenize(token3.tokens[1], token4, ":");
              if (isEqual(token4.tokens[0], "ball-events"))
              {
                _rules.addQuery(module, Module::GET_BALL_EVENTS, getInt(token4.tokens[1]));
              }
              else if (isEqual(token4.tokens[0], "ball-score"))
              {
                _rules.addQuery(module, Module::GET_BALL_SCORE, getInt(token4.tokens[1]));
              }
              else if (isEqual(token4.tokens[0], "total-events"))
              {
                _rules.addQuery(module, Module::GET_TOTAL_EVENTS, getInt(token4.tokens[1]));
              }
              else if (isEqual(token4.tokens[0], "total-score"))
              {
                _rules.addQuery(module, Module::GET_TOTAL_SCORE, getInt(token4.tokens[1]));
              }
              else if (isEqual(token4.tokens[0], "active"))
              {
                _rules.addQuery(module, Module::GET_ACTIVE, true);
              }
              else if (isEqual(token4.tokens[0], "inactive"))
              {
                _rules.addQuery(module, Module::GET_ACTIVE, false);
              }
              else if (isEqual(token4.tokens[0], "state"))
              {
                if ((command = module->getCommand(token4.tokens[1])) != NULL)
                {
                  _rules.addQuery(module, Module::GET_STATE, command->id);
                }
                else
                {
                  TRACE_ERROR("Could not find state: %s, for module: %s", token4.tokens[1], module->getName());
                }
              }
            }
            else
            {
              TRACE_ERROR("Could not find Module: %s, not adding query", token3.tokens[0]);              
            }
          }
        }
        else if (isEqual(token1.tokens[0], "actions") && (addRule == true))
        {
          tokenize(token1.tokens[1], token2, ",");
          for (int i = 0; i < token2.numTokens; i++)
          {
            tokenize(token2.tokens[i], token4, ":");
            tokenize(token4.tokens[0], token3, ".");
            // figure out is this action is transient or persistent
            if (isEqual(token4.tokens[token4.numTokens-1], "transient"))
            {
              type = TRANSIENT;
            }
            else if (isEqual(token4.tokens[token4.numTokens-1], "persistent"))
            {
              type = PERSISTENT;
            }
            // figure out the action they want to perform
            if ((module = getModule(token3.tokens[0])) != NULL)
            {
              if (isEqual(token4.tokens[0], "active"))
              {
                _rules.addAction(module, Module::SET_ACTIVE, true, type);
              }
              else if (isEqual(token4.tokens[0], "inactive"))
              {
                _rules.addAction(module, Module::SET_ACTIVE, false, type);
              }
              else if (isEqual(token4.tokens[0], "points"))
              {
                _rules.addAction(module, Module::SET_POINTS, getInt(token4.tokens[1]), type);
              }
              else if (isEqual(token4.tokens[0], "action"))
              {
                _rules.addAction(module, Module::SET_ACTION, getInt(token4.tokens[1]), type);
              }
              else if (isEqual(token4.tokens[0], "action-delay"))
              {
                _rules.addAction(module, Module::SET_ACTION_DELAY, getInt(token4.tokens[1]), type);
              }
              else if (isEqual(token4.tokens[0], "command"))
              {
                if ((command = module->getCommand(token4.tokens[1])) != NULL)
                {
                  _rules.addAction(module, Module::RUN_COMMAND, command->id, type);
                }
                else
                {
                  TRACE_ERROR("Could not find command: %s, for module: %s", token4.tokens[1], module->getName());
                }
                
              }
            }
            else if (isEqual(token3.tokens[0], "global"))
            {
              if (isEqual(token3.tokens[1], "shoot-again"))
              {
                _rules.addAction(GlobalActions::SHOOT_AGAIN, true);
              }
              else if (isEqual(token3.tokens[1], "drain-multiplier"))
              {
                _rules.addAction(GlobalActions::DRAIN_MULTIPLIER, getInt(token4.tokens[1]), type);
              }
              else if (isEqual(token3.tokens[1], "drain-bonus"))
              {
                _rules.addAction(GlobalActions::DRAIN_BONUS, getInt(token4.tokens[1]), type);
              }
              else if (isEqual(token3.tokens[1], "game-over-multiplier"))
              {
                _rules.addAction(GlobalActions::GAME_OVER_MULTIPLIER, getInt(token4.tokens[1]));
              }
              else if (isEqual(token3.tokens[1], "game-over-bonus"))
              {
                _rules.addAction(GlobalActions::GAME_OVER_BONUS, getInt(token4.tokens[1]));
              }
              else if (isEqual(token3.tokens[1], "multi-ball"))
              {
                _rules.addAction(GlobalActions::MULTI_BALL, getInt(token4.tokens[1]));
              }
              else if (isEqual(token3.tokens[1], "image-file"))
              {
                _rules.addAction(GlobalActions::IMAGE_FILE, token4.tokens[1], type);
              }
              else if (isEqual(token3.tokens[1], "sound-file"))
              {
                _rules.addSound(GlobalActions::SOUND_FILE, token4.tokens[1], type);
              }
            }
            else
            {
              TRACE_ERROR("Could not find Module: %s, not adding action", token3.tokens[0]);              
            }
          }
        }
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void Playfield::addModule(int row_, int col_, Module *module_)
{
  int index;
  if ((index = getIndex(row_,col_)) != INVALID_INDEX)
  {
    if (_masterPlayfield == NULL)
    {
      _masterPlayfield[index] = module_;
    }
    else
    {
      TRACE_ERROR("Cannot add module for row: %d, col: %d, module already exists", row_, col_);
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void Playfield::processEvent(int row_, int col_)
{
  Module *module;
  int pointsPerEvent;
  if ((module = getModule(row_, col_)) != NULL)
  {
    if (module->isActive())
    {
      module->setEventNum(_eventNum++);
      pointsPerEvent = module->processEvent();
      _ballScore += pointsPerEvent;
      _totalScore += pointsPerEvent;
      _ballEvents++;
      _totalEvents++;
      _rules.processRules(module);
      updateDisplay();
    }
    else
    {
      TRACE_INFO("%s: not active, not processing event", module->getName());
    }
  }
  else
  {
    TRACE_ERROR("No module found for row: %d, col: %d", row_, col_);
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void Playfield::processEvent(int position_)
{
  if (position_ < _numTargets)
  {
    if (_activePlayfield[position_] != NULL)
    {
      processEvent(_activePlayfield[position_]->getRow(), _activePlayfield[position_]->getCol());
    }
    else
    {
      TRACE_ERROR("No module found in active playfield for position: %d", position_);
    }
  }
  else
  {
    TRACE_ERROR("No position: %d our of bounds of active playfield", position_);
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void Playfield::performAction(int row_, int col_)
{
  Module *module;
  if ((module = getModule(row_, col_)) != NULL)
  {
    module->performAction();
  }
  else
  {
    TRACE_ERROR("No module found for row: %d, col: %d", row_, col_);
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void Playfield::drainBall(void)
{
  _totalScore += getBallScore()*_rules.getDrainMultiplier();
  _totalScore += _rules.getDrainBonus();
  TRACE_EVENT("Drain: ballEvents: %d, ballScore: %llu, totalEvents: %d, totalScore: %llu",
              getBallEvents(),
              getBallScore(),
              getTotalEvents(),
              getTotalScore());
  updateDisplay();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void Playfield::resetBall(void)
{
  for (int i = 0; i < _numTargets; i++)
  {
    _activePlayfield[i]->resetBall();
  }
  _ballScore = 0;
  _ballEvents = 0;
  _rules.resetBall();
  updateDisplay();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void Playfield::resetGame(void)
{
  for (int i = 0; i < _numTargets; i++)
  {
    _activePlayfield[i]->resetGame();
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void Playfield::endGame(void)
{
  _totalScore *= _rules.getGameOverMultiplier();
  _totalScore += _rules.getGameOverBonus();
  _currPlayer = false;
  _currBall = 0;
  TRACE_INFO("Game Over: totalScore: %llu", getTotalScore());
  updateDisplay();
  gtk_widget_modify_bg(_guiPlayerFrame, GTK_STATE_NORMAL, &grey);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Playfield::Database *Playfield::getDatabase(Module *module_)
{
  if (_isDatabaseLoaded)
  {
    for (int i = 0; i < _numDatabaseModules; i++)
    {
      if (isEqual(module_->getType(), _moduleDatabase[i].name))
      {
        return (&_moduleDatabase[i]);
      }
    }
  }
  return (NULL);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Module *Playfield::getModule(int row_, int col_)
{
  int index;
  Module *module = NULL;
  if ((index = getIndex(row_, col_)) != INVALID_INDEX)
  {
    module = _masterPlayfield[index];
  }
  return (module);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Module *Playfield::getModule(int position_)
{
  Module *module = NULL;
  if (position_ < _numEntries)
  {
    module = _masterPlayfield[position_];
  }
  return (module);  
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Module *Playfield::getModule(const char *name_)
{
  for (int i = 0; i < _numTargets; i++)
  {
    if (isEqual(name_, _activePlayfield[i]->getName()))
    {
      return (_activePlayfield[i]);
    }
  }
  return(NULL);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
int Playfield::getIndex(int row_, int col_)
{
  int index = INVALID_INDEX;
  if ((row_ < _numRows) && (col_ < _numCols))
  {
    index = (row_*_numRows)+col_;
  }
  //TRACE_DEBUG("numRows: %d, numCols: %d, numEntries: %d, index: %d", _numRows,  _numCols, _numEntries, index);
  return (index);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void Playfield::tokenize(char *inputLine_, Tokens &tokens_, const char *delimiter_)
{
  char *token;
  tokens_.numTokens = 0;
  strcpy(tokens_.line, inputLine_);
  if ((token = strtok(tokens_.line, delimiter_)) != NULL)
  {
    tokens_.tokens[tokens_.numTokens++] = token;
    while (((token = strtok(NULL, delimiter_)) != NULL) && (tokens_.numTokens < MAX_TOKENS))
    {
      tokens_.tokens[tokens_.numTokens++] = token;
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool Playfield::isEqual(const char *string1_, const char *string2_)
{
  if ((string1_ == NULL) && (string2_ == NULL))
  {
    return (true);
  }
  else if ((string1_ != NULL) && (string2_ != NULL))
  {
    return (strcasecmp(string1_, string2_) == 0);
  }
  else
  {
    return (false);
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool Playfield::isNumeric(const char *string_)
{
  unsigned i;
  unsigned length;
  if (string_ != NULL)
  {
    if ((length = strlen(string_)) > 0)
    {
      for (i = 0; i < length; i++)
      {
        if (!isdigit(string_[i]))
        {
          return (false);
        }
      }
      return (true);
    }
    else
    {
      return (false);
    }
  }
  else
  {
    return (false);
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool Playfield::isValidModule(const char *module_)
{
  return (true);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Command *Playfield::findCommand(const char *type_, const char *command_)
{
  for (int i = 0; i < MAX_MODULES; i++)
  {
    if (isEqual(type_, _moduleDatabase[i].name))
    {
      for (int j = 0; j < MAX_COMMANDS; j++)
      {
        if (isEqual(command_, _moduleDatabase[i].commands[j].name))
        {
          return (&_moduleDatabase[i].commands[j]);
        }
      }
    }
  }
  return (NULL);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void Playfield::addCommands(Module *module_)
{
  for (int i = 0; i < MAX_MODULES; i++)
  {
    if (isEqual(module_->getType(), _moduleDatabase[i].name))
    {
      for (int j = 0; j < _moduleDatabase[i].numCommands; j++)
      {
        module_->addCommand(&_moduleDatabase[i].commands[j]);
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
gint guiUpdateDisplay(void *this_)
{
  char value[80];

  sprintf(value, "%llu", ((Playfield *)this_)->getTotalScore());
  gtk_entry_set_text(GTK_ENTRY(((Playfield *)this_)->getGuiTotalScore()), value);

  gtk_entry_set_text(GTK_ENTRY(screenScore), value);
  
  sprintf(value, "%llu", ((Playfield *)this_)->getBallScore());
  gtk_entry_set_text(GTK_ENTRY(((Playfield *)this_)->getGuiBallScore()), value);

  if (((Playfield *)this_)->getCurrPlayer())
  {
    sprintf(value, "%s", "Yes");
    gtk_widget_modify_text(((Playfield *)this_)->getGuiCurrPlayer(), GTK_STATE_NORMAL, &green);
    gtk_widget_modify_bg(((Playfield *)this_)->getGuiPlayerFrame(), GTK_STATE_NORMAL, &green);
  }
  else
  {
    sprintf(value, "%s", "No");
    gtk_widget_modify_text(((Playfield *)this_)->getGuiCurrPlayer(), GTK_STATE_NORMAL, &red);
    gtk_widget_modify_bg(((Playfield *)this_)->getGuiPlayerFrame(), GTK_STATE_NORMAL, &red);
  }
  
  gtk_entry_set_text(GTK_ENTRY(((Playfield *)this_)->getGuiCurrPlayer()), value);

  sprintf(value, "%d", ((Playfield *)this_)->getCurrBall());
  gtk_entry_set_text(GTK_ENTRY(((Playfield *)this_)->getGuiCurrBall()), value);

  sprintf(value, "%d", ((Playfield *)this_)->getDrainMultiplier());
  gtk_entry_set_text(GTK_ENTRY(((Playfield *)this_)->getGuiDrainMultiplier()), value);
  
  sprintf(value, "%d", ((Playfield *)this_)->getDrainBonus());
  gtk_entry_set_text(GTK_ENTRY(((Playfield *)this_)->getGuiDrainBonus()), value);
  
  sprintf(value, "%d", ((Playfield *)this_)->getGameOverMultiplier());
  gtk_entry_set_text(GTK_ENTRY(((Playfield *)this_)->getGuiGameOverMultiplier()), value);
  
  sprintf(value, "%d", ((Playfield *)this_)->getDrainBonus());
  gtk_entry_set_text(GTK_ENTRY(((Playfield *)this_)->getGuiGameOverBonus()), value);
  
  if (((Playfield *)this_)->getShootAgain())
  {
    sprintf(value, "%s", "Yes");
    gtk_widget_modify_text(((Playfield *)this_)->getGuiShootAgain(), GTK_STATE_NORMAL, &green);
    gtk_entry_set_text(GTK_ENTRY(shootAgain), "Shoot Again");
  }
  else
  {
    sprintf(value, "%s", "No");
    gtk_widget_modify_text(((Playfield *)this_)->getGuiShootAgain(), GTK_STATE_NORMAL, &red);
    gtk_entry_set_text(GTK_ENTRY(shootAgain), "");
  }
  gtk_entry_set_text(GTK_ENTRY(((Playfield *)this_)->getGuiShootAgain()), value);
  
  sprintf(value, "%d", ((Playfield *)this_)->getMultiBall());
  gtk_entry_set_text(GTK_ENTRY(((Playfield *)this_)->getGuiMultiBall()), value);
  return (FALSE);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
gint guiSetCurrPlayer(void *this_)
{
  char value[80];
	gtk_entry_set_text(GTK_ENTRY(playerLabel), ((Playfield *)this_)->getPlayerName());
  sprintf(value, "Ball %d", ((Playfield *)this_)->getCurrBall());
  gtk_entry_set_text(GTK_ENTRY(currentBall), value);
  Utils::setImage((void *)((Playfield *)this_)->getDefaultImageFile());
  return (guiUpdateDisplay(this_));
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void Playfield::setCurrPlayer(bool player_)
{
  char currPlayerSound[300];
  _currPlayer = player_;
  if (_currPlayer == true)
  {
    _currBall++;
    sprintf(currPlayerSound, "player-%d.mp3", _playerId);
    Utils::playSound(currPlayerSound);
  }
  gdk_threads_add_idle(guiSetCurrPlayer, (void *)this);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void Playfield::updateDisplay(void)
{
  gdk_threads_add_idle(guiUpdateDisplay, (void *)this);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void Playfield::setActive(bool active_)
{
  _isActive = active_;
  if (_isActive)
  {
    gtk_widget_set_sensitive(_guiPlayerFrame, TRUE);
  }
  else
  {
    gtk_widget_set_sensitive(_guiPlayerFrame, FALSE);
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
GtkWidget *Playfield::createPlayer(const char *name_, int id_)
{
  GtkWidget *hboxScore;
  
  GtkWidget *vboxStatus;
  GtkWidget *hboxStatus0;
  GtkWidget *hboxStatus1;
  GtkWidget *hboxStatus2;
  GtkWidget *hboxStatus3;
  
  GtkWidget *totalScoreLabel;  
  GtkWidget *ballScoreLabel;
  GtkWidget *currPlayerLabel;
  GtkWidget *currBallLabel;
  GtkWidget *drainMultiplierLabel;
  GtkWidget *drainBonusLabel;
  GtkWidget *gameOverMultiplierLabel;
  GtkWidget *gameOverBonusLabel;
  GtkWidget *shootAgainLabel;
  GtkWidget *multiBallLabel;
  
  GtkWidget *playerBox;
  GtkWidget *scoreFrame;
  GtkWidget *statusFrame;

  gdk_color_parse("green", &green);
	gdk_color_parse("yellow", &yellow);
	gdk_color_parse("red", &red);
	gdk_color_parse("black", &black);
	gdk_color_parse("grey", &grey);

  int entrySize = 16;
  int boxSpacing = 1;

    // create player frame
  _guiPlayerFrame = gtk_frame_new (name_);
  gtk_frame_set_shadow_type (GTK_FRAME (_guiPlayerFrame), GTK_SHADOW_ETCHED_OUT);
  gtk_frame_set_label_align (GTK_FRAME (_guiPlayerFrame), 0.5, 0.5);
  gtk_widget_show (_guiPlayerFrame);

  // create our score frame
  scoreFrame = gtk_frame_new ("Score");
  gtk_frame_set_shadow_type (GTK_FRAME (scoreFrame), GTK_SHADOW_ETCHED_OUT);
  gtk_widget_show (scoreFrame);

  // create our status frame
  statusFrame = gtk_frame_new ("Status");
  gtk_frame_set_shadow_type (GTK_FRAME (statusFrame), GTK_SHADOW_ETCHED_OUT);
  gtk_widget_show (statusFrame);
  
  // create horizontal box for scores
  hboxScore = gtk_hbox_new (FALSE, boxSpacing);
  gtk_widget_show (hboxScore);  

  Utils::createLabel("Total Score:", "0", green, black, &totalScoreLabel, &_guiTotalScore, false, entrySize);
  gtk_box_pack_start (GTK_BOX (hboxScore), totalScoreLabel, FALSE, FALSE, 0);  
  gtk_box_pack_start (GTK_BOX (hboxScore), _guiTotalScore, TRUE, TRUE, 0);

  Utils::createLabel("Ball Score:", "0", green, black, &ballScoreLabel, &_guiBallScore, false, entrySize);
  gtk_box_pack_start (GTK_BOX (hboxScore), ballScoreLabel, FALSE, FALSE, 0);  
  gtk_box_pack_start (GTK_BOX (hboxScore), _guiBallScore, TRUE, TRUE, 0);

  // add the score box into our score frame
  gtk_container_add (GTK_CONTAINER (scoreFrame), hboxScore);

  // create top level status box
  vboxStatus = gtk_vbox_new (FALSE, boxSpacing);
  gtk_widget_show (vboxStatus);

  // create first horizontal status box
  hboxStatus0 = gtk_hbox_new (FALSE, boxSpacing);
  gtk_widget_show (hboxStatus0);  

  Utils::createLabel("Current Player:", "No", red, black, &currPlayerLabel, &_guiCurrPlayer, false, entrySize);
  gtk_box_pack_start (GTK_BOX (hboxStatus0), currPlayerLabel, FALSE, FALSE, 0);  
  gtk_box_pack_start (GTK_BOX (hboxStatus0), _guiCurrPlayer, TRUE, TRUE, 0);

  Utils::createLabel("Current Ball:", "0", green, black, &currBallLabel, &_guiCurrBall, false, entrySize);
  gtk_box_pack_start (GTK_BOX (hboxStatus0), currBallLabel, FALSE, FALSE, 0);  
  gtk_box_pack_start (GTK_BOX (hboxStatus0), _guiCurrBall, TRUE, TRUE, 0);

  hboxStatus1 = gtk_hbox_new (FALSE, boxSpacing);
  gtk_widget_show (hboxStatus1);  

  Utils::createLabel("Drain Multiplier:", "0", green, black, &drainMultiplierLabel, &_guiDrainMultiplier, false, entrySize);
  gtk_box_pack_start (GTK_BOX (hboxStatus1), drainMultiplierLabel, FALSE, FALSE, 0);  
  gtk_box_pack_start (GTK_BOX (hboxStatus1), _guiDrainMultiplier, TRUE, TRUE, 0);

  Utils::createLabel("Drain Bonus:", "0", green, black, &drainBonusLabel, &_guiDrainBonus, false, entrySize);
  gtk_box_pack_start (GTK_BOX (hboxStatus1), drainBonusLabel, FALSE, FALSE, 0);  
  gtk_box_pack_start (GTK_BOX (hboxStatus1), _guiDrainBonus, TRUE, TRUE, 0);

  // create second horizontal status box
  hboxStatus2 = gtk_hbox_new (FALSE, boxSpacing);
  gtk_widget_show (hboxStatus2);  

  Utils::createLabel("Game Over Multiplier:", "0", green, black, &gameOverMultiplierLabel, &_guiGameOverMultiplier, false, entrySize);
  gtk_box_pack_start (GTK_BOX (hboxStatus2), gameOverMultiplierLabel, FALSE, FALSE, 0);  
  gtk_box_pack_start (GTK_BOX (hboxStatus2), _guiGameOverMultiplier, TRUE, TRUE, 0);

  Utils::createLabel("Game Over Bonus:", "0", green, black, &gameOverBonusLabel, &_guiGameOverBonus, false, entrySize);
  gtk_box_pack_start (GTK_BOX (hboxStatus2), gameOverBonusLabel, FALSE, FALSE, 0);  
  gtk_box_pack_start (GTK_BOX (hboxStatus2), _guiGameOverBonus, TRUE, TRUE, 0);

  // create third horizontal status box
  hboxStatus3 = gtk_hbox_new (FALSE, boxSpacing);
  gtk_widget_show (hboxStatus3);  

  Utils::createLabel("Shoot Again:", "No", red, black, &shootAgainLabel, &_guiShootAgain, false, entrySize);
  gtk_box_pack_start (GTK_BOX (hboxStatus3), shootAgainLabel, FALSE, FALSE, 0);  
  gtk_box_pack_start (GTK_BOX (hboxStatus3), _guiShootAgain, TRUE, TRUE, 0);

  Utils::createLabel("Multi-Ball:", "0", green, black, &multiBallLabel, &_guiMultiBall, false, entrySize);
  gtk_box_pack_start (GTK_BOX (hboxStatus3), multiBallLabel, FALSE, FALSE, 0);  
  gtk_box_pack_start (GTK_BOX (hboxStatus3), _guiMultiBall, TRUE, TRUE, 0);

  // pack horizontal status boxes into their vertical status box
  gtk_box_pack_start (GTK_BOX (vboxStatus), hboxStatus0, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vboxStatus), hboxStatus1, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vboxStatus), hboxStatus2, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vboxStatus), hboxStatus3, FALSE, FALSE, 0);

  // add the vertical status box into the status frame
  gtk_container_add (GTK_CONTAINER (statusFrame), vboxStatus);
  
  // create top-level player box
  playerBox = gtk_vbox_new (FALSE, boxSpacing);
  gtk_widget_show (playerBox);

  // pack our two sub-frames in the top level player box
  gtk_box_pack_start (GTK_BOX (playerBox), scoreFrame, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (playerBox), statusFrame, FALSE, FALSE, 0);

  // set our degault frame colors
  gtk_widget_modify_bg(scoreFrame, GTK_STATE_NORMAL, &grey);
  gtk_widget_modify_bg(statusFrame, GTK_STATE_NORMAL, &grey);
  gtk_widget_modify_bg(_guiPlayerFrame, GTK_STATE_NORMAL, &grey);
  
  // add the player box to the player frame
  gtk_container_add (GTK_CONTAINER (_guiPlayerFrame), playerBox);

  // player remains inactive until we start a new game
  setActive(false);

  strcpy(_playerName, name_);

  _playerId = id_;

  return (_guiPlayerFrame);
}
