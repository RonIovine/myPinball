#ifndef MODULE_H
#define MODULE_H

#include <string.h>
#include <MyPinball.h>

class Module
{
  public:

    enum Query
    {
      GET_BALL_EVENTS,
      GET_BALL_SCORE,
      GET_TOTAL_EVENTS,
      GET_TOTAL_SCORE,
      GET_ACTIVE,
      GET_STATE
    };

    enum Action
    {
      SET_POINTS,
      SET_ACTIVE,
      SET_ACTION,
      SET_ACTION_DELAY,
      RUN_COMMAND
    };

    Module(const char *type_, int row_,int col_);
    ~Module(){};

    int getRow(void){return(_row);};
    int getCol(void){return(_col);};

    void setType(const char *type_){strcpy(_type, type_);};
    char *getType(void){return (_type);};
    
    void setName(const char *name_){strcpy(_name, name_);};
    char *getName(void){return (_name);};

    void setEventNum(int eventNum_){_eventNum = eventNum_;};
    int getEventNum(void){return (_eventNum);};

    void init(void);
    int processEvent(void);
    void performAction(void);

    void resetBall(void);
    void resetGame(void);

    int queryState(Query query_);
    const char *getQuery(Query query_);
    void performAction(Action action_, int value_, int type_);
    const char *getAction(Action action_);

    int getTotalEvents(void){return (_totalEvents);};
    int getTotalScore(void){return (_totalScore);};
    int getBallEvents(void){return (_ballEvents);};
    int getBallScore(void){return (_ballScore);};
    
    void setInitialCommand(Command *command_){_initialCommand = command_;};
    void addCommand(Command *command_);
    void runCommand(const char *command_);
    void runCommand(int index_);
    Command *getCommand(const char *command_);
    const char *getCommand(int id_);

    void setDefaultPoints(int points_){_defaultPoints = points_;_points = points_;};
    void setOverridePoints(int points_, int type_){_points = points_;_pointsType = type_;};
    int getPoints(void){return (_points);};

    void setDefaultAction(Command *action_){_defaultAction = action_;_action = action_;};
    void setOverrideAction(Command *action_, int type_){_action = action_; _actionType = type_;};
    
    void setDefaultActionDelay(int delay_){_defaultActionDelay = delay_;_actionDelay = delay_;};
    void setOverrideActionDelay(int delay_, int type_){_actionDelay = delay_;_actionDelayType = type_;};

    void setDefaultActive(bool isActive_){_isDefaultActive = isActive_;_isActive = isActive_;};
    void setOverrideActive(bool isActive_, int type_){_isActive = isActive_;_activeType = type_;};
    bool isActive(void){return (_isActive);};

    int getState(void){return (_state);};
 
  private:

    void runCommand(Command *command_);

    // type and name as defined in the profile configuration file
    char _type[STRING_SIZE];
    char _name[STRING_SIZE];
  
    // position of this module on the playfield
    int _row;
    int _col;

    // the initial command to run for this module upon game startup
    Command *_initialCommand;

    // this is the points scored per ball event
    int _pointsType;
    int _points;
    int _defaultPoints;

    // this is the action to perform upon a ball event
    int _actionDelayType;
    int _actionDelay;
    int _defaultActionDelay;
    int _actionType;   
    Command *_action;
    Command *_defaultAction;

    // scoring member variables
    int _totalEvents;
    int _totalScore;
    int _ballEvents;
    int _ballScore;
    
    bool _isActive;
    bool _isDefaultActive;
    int _activeType;

    int _state;

    int _eventNum;

    int _numCommands;
    Command *_commands[MAX_COMMANDS];
    
};

#endif
