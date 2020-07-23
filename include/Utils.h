#ifndef UTILS_H
#define UTILS_H

#include <gtk/gtk.h> 

class Utils
{
public:

  static void createLabel(const char *textDisplay,
                          const char *boxDisplay,
                          GdkColor textForeground,
                          GdkColor textBackground,  
                          GtkWidget **label,
                          GtkWidget **value,
                          bool isEditable,
                          int size,
                          bool smallFont = true);                   
  static gint setImage(void *imageFile_);
  
  static void playSound(const char *soundFile_);
  
};

#endif
