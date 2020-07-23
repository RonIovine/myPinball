#include <sstream>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include <Utils.h> 
#include <PshellServer.h>
#include <Playfield.h>
#include <Timer.h>

#define MAX_PLAYERS 4
Playfield playfield[MAX_PLAYERS];
unsigned numPlayers = 0;
unsigned currPlayer = 0;

int prevEvent = -1;
int prevSleepTime = 0;
FILE *logFile;
struct SimTarget
{
  int row;
  int col;
};
SimTarget *simTargets;

enum GameMode
{
  AUTO,
  MANUAL
};

GameMode gameMode = AUTO;

// GTK GUI widgets
GtkWidget *numEvents;
GtkWidget *currPlayerBox;
GtkWidget *gameModeBox;

GtkWidget *debugScrolledWindow;
GtkWidget *debugFrame;
GtkTextBuffer *debugBuffer;
GtkTextIter debugTextIter;
GtkWidget *debugTextView;
GtkWidget *window = NULL;
GtkWidget *machineWindow = NULL;
gint debugOnWindowWidth;
gint debugOnWindowHeight;
gint debugOffWindowWidth;
gint debugOffWindowHeight;

GtkWidget *screenScore;
GtkWidget *playerLabel;
GtkWidget *currentBall;
GtkWidget *shootAgain;
GtkWidget *screenImage;
GtkWidget *screenBox;  

GdkColor green;
GdkColor yellow;
GdkColor red;
GdkColor black;
GdkColor grey;

bool showDebug = false;

pthread_mutex_t traceMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t simMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t simCond = PTHREAD_COND_INITIALIZER;

char currentImage[300];
char currentSound[300];

int  numIterations;
int  currIteration;

const char *INSTALL_PATH;
const char *CONFIG_PATH = "config";
const char *LOG_PATH = "log";
const char *GAMEPLAY_PATH = "config/gameplay";
const char *SOUNDS_PATH = "config/gameplay/sounds";
const char *IMAGES_PATH = "config/gameplay/images";

Timer timerThread(Timer::SECOND);
  
// simulation functions

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void milliSleep(void)
{
  int currSleepTime;
  struct timespec sleepTime;
  // get a random sleep time between 100 and 1000 mSec,
  // loop so we get a different sleep time than last time
  while (((currSleepTime = (rand()%900+100)) == prevEvent));
  // sleep time is in mSec
  sleepTime.tv_sec = currSleepTime/1000;
  sleepTime.tv_nsec = (currSleepTime%1000)*1000000;
  nanosleep(&sleepTime, NULL);
  prevSleepTime = currSleepTime;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
int getRow(void)
{
  return (rand()%playfield[currPlayer].getNumRows());
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
int getCol(void)
{
  return (rand()%playfield[currPlayer].getNumRows());
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
int getEvent(void)
{
  int event;
  // get a random event, loop so we get a different event than last time
  while (((event = rand()%playfield[currPlayer].getNumTargets()) == prevEvent));
  prevEvent = event;
  return (event);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
gint guiTraceLogFunction(void *outputString_)
{
  // write to our GUI window
  GtkTextMark *mark = gtk_text_buffer_get_insert(debugBuffer);
  gtk_text_buffer_get_iter_at_mark(debugBuffer, &debugTextIter, mark);
  gtk_text_buffer_insert(debugBuffer, &debugTextIter, (const char *)outputString_ , -1);
  gtk_text_view_scroll_mark_onscreen(GTK_TEXT_VIEW(debugTextView), mark);
  free(outputString_);
  return (FALSE);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void traceLogFunction(const char *outputString_)
{

  // write to stdout 
  //printf("%s", outputString_);
  
  // write to logfile
  if (logFile != NULL)
  {
    fprintf(logFile, "%s", outputString_);
  }

  // write to GUI
  char *strDup = strdup(outputString_);
  gdk_threads_add_idle(guiTraceLogFunction, (void *)strDup);

}

// graphical output display functions

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void getCurrPlayer(void)
{
  if (gameMode == MANUAL)
  {
    char *string;
    string = (char *)gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(currPlayerBox)->entry));
    TRACE_DEBUG("Current Player Selection: %s", string);
    for (unsigned i = 0; i < MAX_PLAYERS; i++)
    {
      if (strcmp(string, playfield[i].getPlayerName()) == 0)
      {
        playfield[currPlayer].setCurrPlayer(false);
        currPlayer = i;
        playfield[currPlayer].setCurrPlayer(true);
        break;
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void getGameMode(void)
{
  char *string;
  string = (char *)gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(gameModeBox)->entry));
  TRACE_DEBUG("Current Game Mode Selection: %s", string);
  if (strcmp(string, "AUTO") == 0)
  {
    gameMode = AUTO;
    gtk_widget_set_sensitive(currPlayerBox, FALSE);
  }
  else if (strcmp(string, "MANUAL") == 0)
  {
    gameMode = MANUAL;
    gtk_widget_set_sensitive(currPlayerBox, TRUE);
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
gint showCurrIteration(void *unused_)
{
  char entries[20];
  sprintf(entries, "%d", currIteration);
  gtk_entry_set_text (GTK_ENTRY (numEvents), entries);
  return (FALSE);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void simulateEvents(void)
{
  getCurrPlayer();
  if (playfield[currPlayer].isInitialized())
  {
    playfield[currPlayer].resetBall();
    for (currIteration = numIterations; currIteration > 0; currIteration--)
    {
      playfield[currPlayer].processEvent(getEvent());
      milliSleep();
      gdk_threads_add_idle(showCurrIteration, NULL);
    }
    playfield[currPlayer].drainBall();
    if ((gameMode == AUTO) && (!playfield[currPlayer].getShootAgain()))
    {
      playfield[currPlayer].setCurrPlayer(false);
      currPlayer = (currPlayer+1)%numPlayers;
      playfield[currPlayer].setCurrPlayer(true);
    }
    currIteration = numIterations;
    gdk_threads_add_idle(showCurrIteration, NULL);
  }
  else
  {
    TRACE_ERROR("Player: %s, simulation not initialized, press 'New Game' button to initialize", playfield[currPlayer].getPlayerName());
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
static void debugCallBack(GtkWidget *widget, gpointer data)
{
  if (showDebug == false)
  {
    gtk_window_get_size (GTK_WINDOW (window), &debugOffWindowWidth, &debugOffWindowHeight);
    gtk_widget_show (debugFrame);
    gtk_window_resize (GTK_WINDOW (window), debugOnWindowWidth, debugOnWindowHeight);
    showDebug = true;
  }
  else
  {
    gtk_window_get_size (GTK_WINDOW (window), &debugOnWindowWidth, &debugOnWindowHeight);
    gtk_widget_hide (debugFrame);
    gtk_window_resize (GTK_WINDOW (window), debugOffWindowWidth, debugOffWindowHeight);
    showDebug = false;
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
static void gameOverCallBack(GtkWidget *widget, gpointer data)
{
  if (gameMode == MANUAL)
  {
    getCurrPlayer();
    playfield[currPlayer].endGame();
    playfield[currPlayer].setActive(false);
    TRACE_INFO("%s Game over!!", playfield[currPlayer].getPlayerName());
  }
  else
  {
    for (unsigned i = 0; i < numPlayers; i++)
    {
      playfield[i].endGame();
      playfield[i].setActive(false);
      TRACE_INFO("%s Game over!!", playfield[i].getPlayerName());
    }
    numPlayers = 0;
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
static void getCurrPlayerCallBack(GtkWidget *widget, gpointer data)
{
  getCurrPlayer();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
static void getGameModeCallBack(GtkWidget *widget, gpointer data)
{
  getGameMode();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
static void exitProgramCallBack(GtkWidget *widget, gpointer data)
{
  exit(0);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
static void simulateEventsCallBack(GtkWidget *widget, gpointer data)
{
  // signal our simulation thread to wake up and process events,
  // we do this so we don't block our gtk_main thread during the
  // event simulation
  TRACE_INFO("Signalling simulation thread");
  const gchar *entry_text;
  entry_text = gtk_entry_get_text (GTK_ENTRY (numEvents));
  numIterations = atoi(entry_text);
  pthread_cond_signal(&simCond);
  TRACE_INFO("Signalling simulation thread");
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
static void newGameCallBack(GtkWidget *widget, gpointer data)
{
  // initialize our playfield
  if (gameMode == MANUAL)
  {
    playfield[currPlayer].init();
    getCurrPlayer();
    playfield[currPlayer].setActive(true);
  }
  else
  {
    playfield[numPlayers].init();
    playfield[numPlayers].setActive(true);
    numPlayers++;    
    if (playfield[0].getCurrPlayer() != true)
    {
      currPlayer = 0;
      playfield[currPlayer].setCurrPlayer(true);
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void createMachine(void)
{
  
  GtkWidget *topBox;

  GtkWidget *screenFrame;  
  GtkWidget *playfieldFrame;  
  GtkWidget *playfieldBox;

  GtkWidget *statusHbox1;
  GtkWidget *statusHbox2;
  GtkWidget *statusVbox;

  GtkWidget *playfieldImage;

  char homeScreenImage[300];
  char defaultPlayfieldImage[300];

  int boxSpacing = 0;

  strcpy(currentImage, "home-screen.gif");
  playfield[0].setDefaultImageFile("home-screen.gif");
  playfield[1].setDefaultImageFile("home-screen.gif");
  playfield[2].setDefaultImageFile("home-screen.gif");
  playfield[3].setDefaultImageFile("home-screen.gif");

  sprintf(homeScreenImage, "%s/%s/home-screen.gif", INSTALL_PATH, IMAGES_PATH);
  sprintf(defaultPlayfieldImage, "%s/%s/playfield.gif", INSTALL_PATH, IMAGES_PATH);

  // open images
  screenImage  = gtk_image_new_from_file(homeScreenImage);
  gtk_widget_show (screenImage);

  // open images
  playfieldImage  = gtk_image_new_from_file(defaultPlayfieldImage);
  gtk_widget_show (playfieldImage);

  // create top level window
  machineWindow = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_widget_show (machineWindow);
  g_signal_connect (machineWindow, "destroy", G_CALLBACK (gtk_main_quit), NULL);
  gtk_window_set_title (GTK_WINDOW (machineWindow), "myPinball Machine");
  gtk_container_set_border_width (GTK_CONTAINER (machineWindow), 5);
  
  // create top-level vertical box
  topBox = gtk_vbox_new (FALSE, boxSpacing);
  gtk_container_add (GTK_CONTAINER (machineWindow), topBox);
  gtk_widget_show (topBox);

  // create our screen frame
  screenFrame = gtk_frame_new ("Play Screen");
  //screenFrame = gtk_aspect_frame_new ("Play Screen", 0.5, 0.5, 2, FALSE);
  gtk_frame_set_shadow_type (GTK_FRAME (screenFrame), GTK_SHADOW_ETCHED_OUT);
  gtk_frame_set_label_align (GTK_FRAME (screenFrame), 0.5, 0.5);
  gtk_widget_modify_bg(screenFrame, GTK_STATE_NORMAL, &grey);
  gtk_box_pack_start (GTK_BOX (topBox), screenFrame, FALSE, FALSE, 0);
  gtk_widget_show (screenFrame);
  
  // create our screen box
  screenBox = gtk_vbox_new (FALSE, boxSpacing);
  gtk_container_add (GTK_CONTAINER (screenFrame), screenBox);
  gtk_container_add (GTK_CONTAINER (screenBox), screenImage);
  gtk_widget_show (screenBox);

  // creat our status boxes
  statusVbox = gtk_vbox_new (FALSE, boxSpacing);
  gtk_widget_modify_bg(statusVbox, GTK_STATE_NORMAL, &black);
  gtk_widget_show (statusVbox);

  statusHbox1 = gtk_hbox_new (FALSE, boxSpacing);
  gtk_widget_modify_bg(statusHbox1, GTK_STATE_NORMAL, &black);
  gtk_widget_show (statusHbox1);

  statusHbox2 = gtk_hbox_new (FALSE, boxSpacing);
  gtk_widget_modify_bg(statusHbox2, GTK_STATE_NORMAL, &black);
  gtk_widget_show (statusHbox2);

  // create our playfield frame
  playfieldFrame = gtk_frame_new ("Play Field");
  gtk_frame_set_shadow_type (GTK_FRAME (playfieldFrame), GTK_SHADOW_ETCHED_OUT);
  gtk_frame_set_label_align (GTK_FRAME (playfieldFrame), 0.5, 0.5);
  gtk_widget_modify_bg(playfieldFrame, GTK_STATE_NORMAL, &grey);
  gtk_box_pack_start (GTK_BOX (topBox), playfieldFrame, FALSE, FALSE, 0);
  gtk_widget_show (playfieldFrame);
  
  // create our playfield box
  playfieldBox = gtk_vbox_new (FALSE, boxSpacing);
  gtk_container_add (GTK_CONTAINER (playfieldFrame), playfieldBox);
  gtk_container_add (GTK_CONTAINER (playfieldBox), playfieldImage);
  gtk_widget_show (playfieldBox);

  // create global score label
  playerLabel = gtk_entry_new();
  gtk_entry_set_width_chars (GTK_ENTRY (playerLabel), 10);
  gtk_widget_modify_base(playerLabel, GTK_STATE_NORMAL, &black);
  gtk_widget_modify_text(playerLabel, GTK_STATE_NORMAL, &yellow);
  gtk_editable_set_editable(GTK_EDITABLE(playerLabel), FALSE);
	GTK_WIDGET_UNSET_FLAGS(playerLabel, GTK_CAN_FOCUS);
  gtk_widget_show (playerLabel);
  gtk_box_pack_start (GTK_BOX (statusHbox1), playerLabel, TRUE, TRUE, 0);

  shootAgain = gtk_entry_new();
  gtk_entry_set_width_chars (GTK_ENTRY (shootAgain), 10);
  gtk_widget_modify_base(shootAgain, GTK_STATE_NORMAL, &black);
  gtk_widget_modify_text(shootAgain, GTK_STATE_NORMAL, &yellow);
  gtk_editable_set_editable(GTK_EDITABLE(shootAgain), FALSE);
	GTK_WIDGET_UNSET_FLAGS(shootAgain, GTK_CAN_FOCUS);
  gtk_misc_set_alignment (GTK_MISC (shootAgain), 0.5, 0.0);
  gtk_widget_show (shootAgain);
  gtk_box_pack_start (GTK_BOX (statusHbox1), shootAgain, TRUE, TRUE, 0);

  gtk_box_pack_start (GTK_BOX (statusVbox), statusHbox1, FALSE, FALSE, 0);

  // create global score label
  screenScore = gtk_entry_new();
  gtk_entry_set_width_chars (GTK_ENTRY (screenScore), 10);
  gtk_widget_modify_base(screenScore, GTK_STATE_NORMAL, &black);
  gtk_widget_modify_text(screenScore, GTK_STATE_NORMAL, &yellow);
  gtk_editable_set_editable(GTK_EDITABLE(screenScore), FALSE);
	GTK_WIDGET_UNSET_FLAGS(screenScore, GTK_CAN_FOCUS);
  gtk_widget_show (screenScore);
  gtk_box_pack_start (GTK_BOX (statusHbox2), screenScore, TRUE, TRUE, 0);

  currentBall = gtk_entry_new();
  gtk_entry_set_width_chars (GTK_ENTRY (currentBall), 10);
  gtk_widget_modify_base(currentBall, GTK_STATE_NORMAL, &black);
  gtk_widget_modify_text(currentBall, GTK_STATE_NORMAL, &yellow);
  gtk_editable_set_editable(GTK_EDITABLE(currentBall), FALSE);
	GTK_WIDGET_UNSET_FLAGS(currentBall, GTK_CAN_FOCUS);
  gtk_misc_set_alignment (GTK_MISC (currentBall), 0.5, 0.0);
  gtk_widget_show (currentBall);
  gtk_box_pack_start (GTK_BOX (statusHbox2), currentBall, TRUE, TRUE, 0);

  gtk_box_pack_start (GTK_BOX (statusVbox), statusHbox2, FALSE, FALSE, 0);

  gtk_box_pack_start (GTK_BOX (screenBox), statusVbox, FALSE, FALSE, 0);

}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void createMonitor(void)
{
  
  GtkWidget *topBox;
  
  GtkWidget *playerHbox1;
  GtkWidget *playerHbox2;
  GtkWidget *numEventsHBox;
  GtkWidget *hbox1Control;
  GtkWidget *hbox2Control;
  GtkWidget *vboxControl;
  GtkWidget *currPlayerHBox;
  GtkWidget *gameModeHBox;

  GtkWidget *numEventsLabel;
  GtkWidget *currPlayerLabel;
  GtkWidget *gameModeLabel;
  
  GtkWidget *exitButton;
  GtkWidget *simulateButton;
  GtkWidget *newGameButton;
  GtkWidget *gameOverButton;
  GtkWidget *debugButton;

  GtkWidget *controlFrame;  

  GList *currPlayerList = NULL;
  GList *gameModeList = NULL;

  int boxSpacing = 1;

  PangoFontDescription *debugFont;

  // create top level window
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_widget_show (window);
  g_signal_connect (window, "destroy", G_CALLBACK (gtk_main_quit), NULL);
  gtk_window_set_title (GTK_WINDOW (window), "myPinball Simulation Manager");
  gtk_container_set_border_width (GTK_CONTAINER (window), 5);
  
  // create top-level vertical box
  topBox = gtk_vbox_new (FALSE, boxSpacing);
  gtk_container_add (GTK_CONTAINER (window), topBox);
  gtk_widget_show (topBox);

  // create our two player boxes
  playerHbox1 = gtk_hbox_new (FALSE, boxSpacing);
  gtk_widget_show (playerHbox1);
  
  playerHbox2 = gtk_hbox_new (FALSE, boxSpacing);
  gtk_widget_show (playerHbox2);

  // create our control frame
  controlFrame = gtk_frame_new ("Game Control");
  gtk_frame_set_shadow_type (GTK_FRAME (controlFrame), GTK_SHADOW_ETCHED_OUT);
  gtk_frame_set_label_align (GTK_FRAME (controlFrame), 0.5, 0.5);
  gtk_widget_show (controlFrame);
  
  // add the player frames to our main box
  gtk_box_pack_start (GTK_BOX (playerHbox1), playfield[0].createPlayer("Player 1", 1), FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (playerHbox1), playfield[1].createPlayer("Player 2", 2), FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (topBox), playerHbox1, FALSE, FALSE, 0);
  
  gtk_box_pack_start (GTK_BOX (playerHbox2), playfield[2].createPlayer("Player 3", 3), FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (playerHbox2), playfield[3].createPlayer("Player 4", 4), FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (topBox), playerHbox2, FALSE, FALSE, 0);

  // create debug our stuff
  debugScrolledWindow = gtk_scrolled_window_new(NULL, NULL);
  debugTextView = gtk_text_view_new();
  debugFrame = gtk_frame_new ("Debug");
  gtk_frame_set_shadow_type (GTK_FRAME (debugFrame), GTK_SHADOW_IN);
  gtk_frame_set_label_align (GTK_FRAME (debugFrame), 0.5, 0.5);
  debugBuffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW (debugTextView));
  debugFont = pango_font_description_from_string ("monospace 10");
  //pango_font_description_set_weight(debugFont, PANGO_WEIGHT_BOLD);
  gtk_widget_modify_font (debugTextView, debugFont);
  
  gtk_text_buffer_get_start_iter(debugBuffer, &debugTextIter);
    
  gtk_container_add(GTK_CONTAINER (debugScrolledWindow), debugTextView);
      
  gtk_widget_show(debugTextView);
  gtk_widget_show(debugScrolledWindow);
  //gtk_widget_show (debugFrame);
    
  gtk_container_add (GTK_CONTAINER (debugFrame), debugScrolledWindow);
  gtk_widget_modify_bg(debugFrame, GTK_STATE_NORMAL, &grey);
  gtk_box_pack_start (GTK_BOX (topBox), debugFrame, TRUE, TRUE, 0);

  // create vertical box for player control
  vboxControl = gtk_vbox_new (FALSE, boxSpacing);
  gtk_widget_show (vboxControl);  

  // create horizontal box1 for player control
  hbox1Control = gtk_hbox_new (FALSE, boxSpacing);
  gtk_widget_show (hbox1Control);  

  // create horizontal box2 for player control
  hbox2Control = gtk_hbox_new (FALSE, boxSpacing);
  gtk_widget_show (hbox2Control);  

  // Exit button
  exitButton = gtk_button_new_with_label ("Exit");
  gtk_widget_show (exitButton);
  g_signal_connect (exitButton, "clicked", G_CALLBACK (exitProgramCallBack), NULL);
  gtk_box_pack_start (GTK_BOX (hbox2Control), exitButton, TRUE, TRUE, 0);

  // New Game button
  newGameButton = gtk_button_new_with_label ("New Game");
  gtk_widget_show (newGameButton);
  g_signal_connect (newGameButton, "clicked", G_CALLBACK (newGameCallBack), NULL);
  gtk_box_pack_start (GTK_BOX (hbox2Control), newGameButton, TRUE, TRUE, 0);

  // Game over button
  gameOverButton = gtk_button_new_with_label ("Game Over");
  gtk_widget_show (gameOverButton);
  g_signal_connect (gameOverButton, "clicked", G_CALLBACK (gameOverCallBack), NULL);
  gtk_box_pack_start (GTK_BOX (hbox2Control), gameOverButton, TRUE, TRUE, 0);

  // Debug button
  debugButton = gtk_button_new_with_label ("Debug");
  gtk_widget_show (debugButton);
  g_signal_connect (debugButton, "clicked", G_CALLBACK (debugCallBack), NULL);
  gtk_box_pack_start (GTK_BOX (hbox2Control), debugButton, TRUE, TRUE, 0);

  // create button to start the simulation
  numEventsHBox = gtk_hbox_new (FALSE, boxSpacing);
  gtk_widget_show (numEventsHBox);
  simulateButton = gtk_button_new_with_label ("Simulate Events");
  gtk_widget_show (simulateButton);
  g_signal_connect (simulateButton, "clicked", G_CALLBACK (simulateEventsCallBack), NULL);
  gtk_box_pack_start (GTK_BOX (hbox1Control), simulateButton, TRUE, TRUE, 0);

  Utils::createLabel("Num Events:", "10", yellow, black, &numEventsLabel, &numEvents, true, 11, false);
  gtk_box_pack_start (GTK_BOX (numEventsHBox), numEventsLabel, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (numEventsHBox), numEvents, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox1Control), numEventsHBox, FALSE, FALSE, 0);

  // game mode player box
  gameModeHBox = gtk_hbox_new (FALSE, boxSpacing);
  gtk_widget_show (gameModeHBox);

  // game mode box
	gameModeLabel = gtk_label_new (NULL);
	gtk_label_set_markup(GTK_LABEL (gameModeLabel), "Game Mode:");
	gtk_label_set_justify(GTK_LABEL (gameModeLabel), GTK_JUSTIFY_LEFT);
  gtk_label_set_width_chars(GTK_LABEL (gameModeLabel), 10);
	gtk_misc_set_alignment (GTK_MISC (gameModeLabel), 0, 0.5);
	gtk_widget_show (gameModeLabel);
  gtk_box_pack_start (GTK_BOX (gameModeHBox), gameModeLabel, FALSE, FALSE, 0);
  
  // game mode combo box
  gameModeBox = gtk_combo_new();
  gtk_widget_show (gameModeBox);
  gameModeList = g_list_append (gameModeList, (void *)"AUTO");
  gameModeList = g_list_append (gameModeList, (void *)"MANUAL");
  gtk_combo_set_popdown_strings (GTK_COMBO (gameModeBox), gameModeList);
  gtk_combo_set_use_arrows (GTK_COMBO (gameModeBox), TRUE);
  gtk_combo_set_use_arrows_always (GTK_COMBO (gameModeBox), TRUE);
  gtk_box_pack_start (GTK_BOX (gameModeHBox), gameModeBox, FALSE, FALSE, 0);
  gtk_signal_connect(GTK_OBJECT(GTK_COMBO(gameModeBox)->entry), "changed", GTK_SIGNAL_FUNC (getGameModeCallBack), NULL);

  gtk_box_pack_start (GTK_BOX (hbox1Control), gameModeHBox, FALSE, FALSE, 0);

  // current player box
  currPlayerHBox = gtk_hbox_new (FALSE, boxSpacing);
  gtk_widget_show (currPlayerHBox);

  // current player box
	currPlayerLabel = gtk_label_new (NULL);
	gtk_label_set_markup(GTK_LABEL (currPlayerLabel), "Current Player:");
	gtk_label_set_justify(GTK_LABEL (currPlayerLabel), GTK_JUSTIFY_LEFT);
  gtk_label_set_width_chars(GTK_LABEL (currPlayerLabel), 12);
	gtk_misc_set_alignment (GTK_MISC (currPlayerLabel), 0, 0.5);
	gtk_widget_show (currPlayerLabel);
  gtk_box_pack_start (GTK_BOX (currPlayerHBox), currPlayerLabel, FALSE, FALSE, 0);
  
  // current player combo box
  currPlayerBox = gtk_combo_new();
  gtk_widget_show (currPlayerBox);
  for (unsigned i = 0; i < MAX_PLAYERS; i++)
  {
    currPlayerList = g_list_append (currPlayerList, (void *)playfield[i].getPlayerName());
  }
  gtk_combo_set_popdown_strings (GTK_COMBO (currPlayerBox), currPlayerList);
  gtk_combo_set_use_arrows (GTK_COMBO (currPlayerBox), TRUE);
  gtk_combo_set_use_arrows_always (GTK_COMBO (currPlayerBox), TRUE);
  gtk_box_pack_start (GTK_BOX (currPlayerHBox), currPlayerBox, FALSE, FALSE, 0);
  gtk_signal_connect(GTK_OBJECT(GTK_COMBO(currPlayerBox)->entry), "changed", GTK_SIGNAL_FUNC (getCurrPlayerCallBack), NULL);
  gtk_widget_set_sensitive(currPlayerBox, FALSE);

  gtk_box_pack_start (GTK_BOX (hbox1Control), currPlayerHBox, FALSE, FALSE, 0);

  // pack the 2 horizontal control boxes into the verticle control box
  gtk_box_pack_start (GTK_BOX (vboxControl), hbox1Control, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vboxControl), hbox2Control, FALSE, FALSE, 0);

  // add the control box to the control frame
  gtk_container_add (GTK_CONTAINER (controlFrame), vboxControl);
  gtk_widget_modify_bg(controlFrame, GTK_STATE_NORMAL, &grey);

  gtk_box_pack_start (GTK_BOX (topBox), controlFrame, FALSE, FALSE, 0);
  
}

// simulation threads so we don't block the gtk main

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
static void *simulationiThread(void*)
{
  for (;;)
  {
    pthread_cond_wait(&simCond, &simMutex);
    TRACE_INFO("Woke up simulation thread, running events");
    simulateEvents();
    TRACE_INFO("Woke up simulation thread, events done");
  }
  return (NULL);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void createSimulationiThread(void)
{
  pthread_t simulationThreadTID;
  pthread_create(&simulationThreadTID , NULL, simulationiThread, NULL);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

// pshell diagnostics functions

void timerTest(int argc, char *argv[])
{
  if ((argc == 1) && pshell_isSubString("show", argv[0], 2))
  {
    stringstream stream;
    stream<<timerThread;
    pshell_printf("%s", stream.str().c_str());
  }
  else if ((argc == 2) && pshell_isSubString("stop", argv[0], 3))
  {
    timerThread.stopTimer(Timer::timerTestFunction, pshell_getUnsigned(argv[1]));
  }
  else if ((argc == 5) && pshell_isSubString("start", argv[0], 3))
  {
    timerThread.startTimer(Timer::timerTestFunction,
                           argv[1],
                           pshell_getUnsigned(argv[2]),
                           pshell_getUnsigned(argv[3])*Timer::ONE_SECOND,
                           pshell_getUnsigned(argv[4]));
  }
  else
  {
    pshell_showUsage();
  }
}

#define MY_PINBALL_PORT 9090

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
int main(int argc, char *argv[])
{

  char *str;

  if ((INSTALL_PATH = getenv("MY_PINBALL_INSTALL")) == NULL)
  {
    if ((str = strstr(argv[0], "/bin/myPinball")) != NULL)
    {
      str[0] = 0;
      INSTALL_PATH = argv[0];
    }
    else
    {
      INSTALL_PATH = "/usr/share/myPinball";
    }
  }

  // start our timer service thread in stand-alone mode
  timerThread.initStandAlone();

  // register our standard trace log levels with the trace filter mechanism
  trace_registerLevels();

  // register our program specific trace log levels with the trace filter mechanism
  // format of call is "name", level, isDefault, isMaskable
  tf_addLevel("EVENT", TL_EVENT, true, true);
  tf_addLevel("COMMAND", TL_COMMAND, true, true);
  tf_addLevel("RULE", TL_RULE, true, true);

  // set our trace log prefix string
  trace_setLogPrefix("my_pinball");
  //trace_setLogPrefix(NULL);

  // don't show file, function, line information in trace logs
  trace_showLocation(false);
  
  /* open syslog with our program name */
  char logFilePath[300];
  sprintf(logFilePath, "%s/%s/myPinball.log", INSTALL_PATH, LOG_PATH);
  logFile = fopen(logFilePath, "w+");
  
  /* register our log function */
  trace_registerLogFunction(traceLogFunction);

  // initialize our trace filter machanism, need to do this before we
  // can issue any traces
  tf_init();

  // turn on our trace filtering
  pshell_runCommand("trace global +event command rule info debug");

  // register any pshell commands here
  pshell_addCommand(timerTest,                                                // function
                    "timer",                                                  // command
                    "call to test the timer subsystem",                       // description
                    "{show | {stop <id>} | {start <name> <id> <duration> <iterations>}}",   // usage
                    1,                                                        // minArgs
                    5,                                                        // maxArgs
                    true);                                                    // showUsage on "?"

  // start our pshell server for program control   
  pshell_startServer("myPinball", PSHELL_TCP_SERVER, PSHELL_NON_BLOCKING, "anyhost", MY_PINBALL_PORT);

  // create our simulation thread
  createSimulationiThread();

  // Initialise GTK 
  gtk_init (&argc, &argv);

  gdk_color_parse("green", &green);
	gdk_color_parse("yellow", &yellow);
	gdk_color_parse("red", &red);
	gdk_color_parse("black", &black);
	gdk_color_parse("grey", &grey);

  // create our monitor output window
  createMonitor();

  // create our mochine output window
  createMachine();

  gtk_main();

  return (0);
  
}
