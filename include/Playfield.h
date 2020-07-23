#ifndef PLAYFIELD_H
#define PLAYFIELD_H

#include <stdlib.h>
#include <gtk/gtk.h> 
#include <Module.h>
#include <Rules.h>

#define INVALID_INDEX -1
#define MAX_QUERIES 100
#define MAX_EVENTS 100
#define MAX_ACTIONS 100
#define MAX_RULES 200
#define MAX_MODULES 300

class Playfield
{
  public:
  
    Playfield();
    ~Playfield();

    void init(void);

    void loadDatabase(void);
    void loadPlayfield(void);
    void loadProfile(void);
    void loadModules(void);
    void loadGameplay(void);
    
    void addModule(int row_, int col_, Module *module_);
    void processEvent(int row_, int col_);
    void processEvent(int target_);
    void performAction(int row_, int col_);
    
    void drainBall(void);
    void resetBall(void);
    void resetGame(void);
    void endGame(void);

    int getIndex(int row_, int col_);

    bool isValidModule(const char *module_);
    int getNumTargets(void){return (_numTargets);};
    int getNumRows(void){return (_numRows);};
    int getNumCols(void){return (_numCols);};

    int getTotalEvents(void){return (_totalEvents);};
    unsigned long long getTotalScore(void){return (_totalScore);};
    int getBallEvents(void){return (_ballEvents);};
    unsigned long long getBallScore(void){return (_ballScore);};

    void incrementCurrBall(void){_currBall++;updateDisplay();};
    int getCurrBall(void){return (_currBall);};

    bool isActive(void){return (_isActive);};
    void setActive(bool active_);
    
    void setCurrPlayer(bool player_);
    bool getCurrPlayer(void){return (_currPlayer);};

    bool getShootAgain(void){return (_rules.getShootAgain());};    
    int getDrainMultiplier(void){return (_rules.getDrainMultiplier());};    
    int getDrainBonus(void){return (_rules.getDrainBonus());};    
    int getGameOverMultiplier(void){return (_rules.getGameOverMultiplier());};    
    int getGameOverBonus(void){return (_rules.getGameOverBonus());};    
    int getMultiBall(void){return (_rules.getMultiBall());};

    bool isInitialized(void){return (_isInitialized);};

    GtkWidget *createPlayer(const char *name_, int id_);
    const char *getPlayerName(void){return (_playerName);};

    GtkWidget *getGuiTotalScore(void){return (_guiTotalScore);};
    GtkWidget *getGuiBallScore(void){return (_guiBallScore);};
    GtkWidget *getGuiCurrPlayer(void){return (_guiCurrPlayer);};
    GtkWidget *getGuiCurrBall(void){return (_guiCurrBall);};
    GtkWidget *getGuiDrainMultiplier(void){return (_guiDrainMultiplier);};
    GtkWidget *getGuiDrainBonus(void){return (_guiDrainBonus);};
    GtkWidget *getGuiGameOverMultiplier(void){return (_guiGameOverMultiplier);};
    GtkWidget *getGuiGameOverBonus(void){return (_guiGameOverBonus);};
    GtkWidget *getGuiShootAgain(void){return (_guiShootAgain);};
    GtkWidget *getGuiMultiBall(void){return (_guiMultiBall);};
    GtkWidget *getGuiPlayerFrame(void){return (_guiPlayerFrame);};

    void setDefaultImageFile(const char *imageFile_){_rules.setDefaultImageFile(imageFile_);};
    const char *getImageFile(void){return(_rules.getImageFile());};
    const char *getDefaultImageFile(void){return(_rules.getDefaultImageFile());};

  private:

    #define MAX_TOKENS 64
    struct Tokens
    {
      int numTokens;
      char *tokens[MAX_TOKENS];
      char line[300];
    };

    class Database
    {
      public:
        Database():numCommands(0),numModules(0){};
        ~Database(){};
        char name[STRING_SIZE];
        Command commands[MAX_COMMANDS];
        int numCommands;
        int numModules;
    };

    void tokenize(char *inputLine_, Tokens &tokens_, const char *delimiter_ = " ");
    bool isEqual(const char *string1_, const char *string2_);
    bool isNumeric(const char *string_);
    int getInt(const char *string_){return (atoi(string_));};
    Module *getModule(int row_, int col_);
    Module *getModule(int position_);
    Module *getModule(const char *name_);
    Command *findCommand(Module *module_, const char *name_);
    Database *getDatabase(Module *module_);
    void cleanup(void);
    Command *findCommand(const char *type_, const char *command_);
    void addCommands(Module *module_);
    void initModules(void);
    void updateDisplay(void);

    Module **_masterPlayfield;
    Module **_activePlayfield;
    int _numRows;
    int _numCols;
    int _numEntries;
    int _numTargets;
    int _totalEvents;
    unsigned long long _totalScore;
    int _ballEvents;
    unsigned long long _ballScore;
    bool _isInitialized;
    bool _isActive;
    Rules _rules;
    int _eventNum;
    int _currBall;
    bool _currPlayer;
    char _playerName[STRING_SIZE];
    int _playerId;

    // GUI based member variables
    GtkWidget *_guiTotalScore;
    GtkWidget *_guiBallScore;
    GtkWidget *_guiCurrPlayer;
    GtkWidget *_guiCurrBall;
    GtkWidget *_guiDrainMultiplier;
    GtkWidget *_guiDrainBonus;
    GtkWidget *_guiGameOverMultiplier;
    GtkWidget *_guiGameOverBonus;
    GtkWidget *_guiShootAgain;
    GtkWidget *_guiMultiBall;
    GtkWidget *_guiPlayerFrame;

    static Database _moduleDatabase[MAX_MODULES];
    static bool _isDatabaseLoaded;
    static int _numDatabaseModules;
    
};

#endif
