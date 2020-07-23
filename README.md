MyPinball
====================
The mission of this project is to create the software/firmware to enable the realization
of a Software Defined Pinball physical game.  The idea is to create a standardized pinball
playfield with standardized, interchangable, pluggable modules that can be easily configured
by the novice.  This physical game can then have a gameplay defined that consists of game
rules and modes that define how the game operates.  The gameplay is not hardcoded, but rather
is created by the user via a graphical gameplay builder.  This gameplay builder will create
a series of configuration files that defines how the game operates.  These files are read
in by a common game engine that manages the play of the game.

Getting Started
====================

This package contains binaries to run the myPinball game engine as well as source code to
build it from scratch.  The following are the detailed instructions on how to build, install,
and run the 'myPinball' game engine simulator on a new Linux machine.  This procedure has been
validated on CentOS, Ubuntu/Mint and Raspbian, following instructions Debian specific.

1. Open a command line terminal

2. Ensure you have the latest repository information:

    $ sudo apt-get update
   
3. Install the GTK2.0 development packages:

    $ sudo apt-get install libgtk2.0-dev
   
4. Download the latest pshell package from:

    https://github.com/RonIovine/pshell
   
5. From the top level pshell download directory, install the pshell package

    $ sudo make install
   
6. Download the latest myPinball package from:

    https://bitbucket.org/RonIovine/mypinball

7. From the top level myPinball directory, create the local myPinball environment:

    $ ./installMyPinball -local

8. Source the myPinball environment file into your local shell:

    $ source .mypinballrc

9. Add the .mypinballrc to your .bashrc file to make permanent for all shells:

    source <myPinballPath>/.mypinballrc

10. Build the myPinball image:

    $ buildMyPinball

11. The executable binary image 'myPinball' is now created, you can
    run the executable at the command line.  You can open the debug
    window and add up to 4 players and simulate events and watch the
    results.

    We currently don't have sound, I am using the sound player
    'gst-launch-1.0', which is not installed on raspbian.  I tried some
    of the native raspbian players like 'aplay' and 'omxplayer', but
    could not get any sound out, even with some of the system installed
    sound file examples.  If you are able to get sound from your pi, try
    to modify the file: Utils.cc, function: Utils::playSound to change
    the sound module that is used in the 'sprintf' system command string.

    When the myPinball process is running, there is a backdoor telnet
    interface into the process at port 9090.  You can get their either
    locally via 'telnet localhost 9090' or from any external host via
    'telnet <ipAddres> 9090'.  We can add any diagnostics commands with
    this mechanism, in the current file with the 'C' 'main', 'myPinball.cc',
    look at the function: 'timerTest' to see how to write and register
    pshell commands.  A more comprehensive set of examples can be found in
    the pshell install directory in the file 'demo/pshellServerDemo.c'.

    

    

