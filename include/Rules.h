#ifndef RULES_H
#define RULES_H

#include <stdlib.h>
#include <MyPinball.h>
#include <Module.h>

#define MAX_QUERIES 100
#define MAX_EVENTS 100
#define MAX_ACTIONS 100
#define MAX_RULES 200

class ModuleEvents
{
  public:
    ModuleEvents(){init();};
    ~ModuleEvents(){};
    enum Type
    {
      ANY,
      GROUP,
      SEQUENCE
    };
    void init(void){numModules = 0;};
    void addEvent(Module *module_, Type type_);
    bool evaluateEvents(Module *module_);
  private:
    const char *getType(Type type_);
    Type type;
    int numModules;
    Module *modules[MAX_EVENTS];
};

class ModuleQueries
{
  public:
    ModuleQueries(){init();};
    ~ModuleQueries(){};
    void init(void){numQueries = 0;};
    void addQuery(Module *module_, Module::Query query_, int value_);
    bool evaluateStates(void);
  private:

    struct Query
    {
      Module *module;
      Module::Query query;
      int value;
    };

    int numQueries;
    Query queries[MAX_QUERIES];
};

class ModuleActions
{
  public:
    ModuleActions(){init();};
    ~ModuleActions(){};
    void init(void){numActions = 0;};
    void addAction(Module *module_, Module::Action action_, int value_, int type_ = 0);
    void performActions(void);
  private:
  
    struct Action
    {
      Module *module;
      Module::Action action;
      int value;
      int type;
    };

    int numActions;
    Action actions[MAX_ACTIONS];
};

class GlobalData
{
  public:
  
    GlobalData(){};
    ~GlobalData(){};

    void resetGlobal(void);
    void resetBall(void);
    
    bool getShootAgain(void){return (_shootAgain);};
    void setShootAgain(bool value_){_shootAgain = value_;};
    
    int getDrainMultiplier(void){return (_drainMultiplier);};
    int getDrainMultiplierType(void){return (_drainMultiplierType);};
    void setDrainMultiplier(int value_, int type_){_drainMultiplier = value_;_drainMultiplierType = type_;};
    
    int getDrainBonus(void){return (_drainBonus);};
    int getDrainBonusType(void){return (_drainBonusType);};
    void setDrainBonus(int value_, int type_){_drainBonus = value_;_drainBonusType = type_;};
    
    int getGameOverMultiplier(void){return (_gameOverMultiplier);};
    void setGameOverMultiplier(int value_){_gameOverMultiplier = value_;};
    
    int getGameOverBonus(void){return (_gameOverBonus);};
    void setGameOverBonus(int value_){_gameOverBonus = value_;};
    
    int getMultiBall(void){return (_multiBall);};
    void setMultiBall(int value_){_multiBall = value_;};
    
    void resetMultiBall(void){_multiBall = 0;};

    void setImageFile(const char *imageFile_, int type_);
    int getImageFileType(void){return (_imageFileType);};
    void setDefaultImageFile(const char *imageFile_){strcpy(_defaultImageFile, imageFile_);strcpy(_imageFile, imageFile_);};
    const char *getImageFile(void){return(_imageFile);};
    const char *getDefaultImageFile(void){return(_defaultImageFile);};
    
  private:
  
    bool _shootAgain;
    int _drainMultiplier;
    int _drainMultiplierType;
    int _drainBonus;
    int _drainBonusType;
    int _gameOverMultiplier;
    int _gameOverBonus;
    int _multiBall;
    int _imageFileType;
    char _defaultImageFile[300];
    char _imageFile[300];
    
};

class GlobalActions
{
  public:
  
    GlobalActions(){init();};
    ~GlobalActions(){};

    enum
    {
      SHOOT_AGAIN = Module::RUN_COMMAND+1,
      DRAIN_MULTIPLIER,
      DRAIN_BONUS,
      GAME_OVER_MULTIPLIER,
      GAME_OVER_BONUS,
      MULTI_BALL,
      IMAGE_FILE,
      SOUND_FILE
    };
    
    void init(void){numActions = 0;};
    void addAction(int action_, int value_, int type_ = 0);
    void addAction(int action_, const char *imageFile_, int type_ = 0);
    void addSound(int action_, const char *soundFile_, int type_ = 0);
    void performActions(GlobalData &global_);
    
  private:

    const char *getAction(int action_);
  
    struct Action
    {
      int action;
      int value;
      int type;
      char imageFile[300];
      char soundFile[300];
    };

    int numActions;
    Action actions[MAX_ACTIONS];

};

class Rules
{
  public:
  
    Rules(){init();};
    ~Rules(){};
    
    void init(void){_numRules = 0;};
    
    void processRules(Module *module_);
    
    bool addRule(const char *name_);
    
    void addEvent(Module *module_, ModuleEvents::Type type_){_rules[_numRules-1].moduleEvents.addEvent(module_, type_);};
    void addQuery(Module *module_, Module::Query query_, int value_){_rules[_numRules-1].moduleQueries.addQuery(module_, query_, value_);};
    void addAction(Module *module_, Module::Action action_, int value_, int type_ = 0){_rules[_numRules-1].moduleActions.addAction(module_, action_, value_, type_);};
    void addAction(int action_, int value_, int type_ = 0){_rules[_numRules-1].globalActions.addAction(action_, value_, type_);};
    void addAction(int action_, const char *imageFile_, int type_ = 0){_rules[_numRules-1].globalActions.addAction(action_, imageFile_, type_);};
    void addSound(int action_, const char *soundFile_, int type_ = 0){_rules[_numRules-1].globalActions.addSound(action_, soundFile_, type_);};;
    
    void resetGlobal(void){_globalData.resetGlobal();};
    void resetBall(void){_globalData.resetBall();};
    void resetMultiBall(void){_globalData.resetMultiBall();};
    
    bool getShootAgain(void){return (_globalData.getShootAgain());};    
    int getDrainMultiplier(void){return (_globalData.getDrainMultiplier());};    
    int getDrainBonus(void){return (_globalData.getDrainBonus());};    
    int getGameOverMultiplier(void){return (_globalData.getGameOverMultiplier());};    
    int getGameOverBonus(void){return (_globalData.getGameOverBonus());};    
    int getMultiBall(void){return (_globalData.getMultiBall());};

    void setDefaultImageFile(const char *imageFile_){_globalData.setDefaultImageFile(imageFile_);};
    const char *getImageFile(void){return (_globalData.getImageFile());};
    const char *getDefaultImageFile(void){return (_globalData.getDefaultImageFile());};
   
  private:
  
    struct Rule
    {
      char name[STRING_SIZE];
      ModuleEvents moduleEvents;
      ModuleQueries moduleQueries;
      ModuleActions moduleActions;
      GlobalActions globalActions;
    };

    int _numRules;
    Rule _rules[MAX_RULES];
    GlobalData _globalData;
    
};

#endif
