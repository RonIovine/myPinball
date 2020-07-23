////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
#include <string.h>
#include <stdlib.h>

#include <TraceLog.h>
#include <Utils.h>

extern GtkWidget *screenImage;
extern char currentImage[300];
extern char currentSound[300];

extern const char *INSTALL_PATH;
extern const char *IMAGES_PATH;
extern const char *SOUNDS_PATH;

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void Utils::createLabel(const char *textDisplay,
                        const char *boxDisplay,
                        GdkColor textForeground,
                        GdkColor textBackground,
                        GtkWidget **label,
                        GtkWidget **value,
                        bool isEditable,
                        int size,
                        bool smallFont)
{
  GtkStyle *style;
  PangoFontDescription *font;

  // get default font
  font = pango_font_description_from_string ("system 9");

  // create text label
  *label = gtk_label_new (NULL);
  gtk_label_set_markup(GTK_LABEL (*label), textDisplay);
  gtk_label_set_justify(GTK_LABEL (*label), GTK_JUSTIFY_LEFT);
  gtk_label_set_width_chars(GTK_LABEL (*label), size);
  gtk_misc_set_alignment (GTK_MISC (*label), 0, 0.5);
  if (smallFont)
  {
    gtk_widget_modify_font (*label, font);
  }
  gtk_widget_show (*label);

  // create non-editable entry box value
  *value = gtk_entry_new();
  gtk_entry_set_width_chars (GTK_ENTRY (*value), 10);
  gtk_widget_modify_base(*value, GTK_STATE_NORMAL, &textBackground);
  gtk_widget_modify_text(*value, GTK_STATE_NORMAL, &textForeground);
  style = gtk_widget_get_style(*value);
  pango_font_description_set_weight(style->font_desc, PANGO_WEIGHT_BOLD);
  font = pango_font_description_from_string ("sans 9 bold");
  if (smallFont)
  {
    gtk_widget_modify_font (*value, font);
  }
  else
  {
    gtk_widget_modify_font(*value, style->font_desc);
  }
  if (!isEditable)
  {
    gtk_editable_set_editable(GTK_EDITABLE(*value), FALSE);
    GTK_WIDGET_UNSET_FLAGS(*value, GTK_CAN_FOCUS);
  }
  gtk_entry_set_text(GTK_ENTRY(*value), boxDisplay);
  gtk_widget_show (*value);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
gint Utils::setImage(void *imageFile_)
{
  char imageFile[300];
  if (strcmp(currentImage, (const char *)imageFile_) != 0)
  {
    TRACE_INFO("Setting image file: %s", imageFile_);
    strcpy(currentImage, (const char *)imageFile_);
    sprintf(imageFile, "%s/%s/%s", INSTALL_PATH, IMAGES_PATH, (const char *)imageFile_);
    gtk_image_set_from_file(GTK_IMAGE (screenImage), (const char *)imageFile);
  }
  return (FALSE);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void Utils::playSound(const char *soundFile_)
{
  char soundCommand[300];
  sprintf(soundCommand, "gst-launch-1.0 playbin uri=file://%s/%s/%s", INSTALL_PATH, SOUNDS_PATH, soundFile_);
  TRACE_INFO("Playing sound file: %s", soundFile_);
  strcpy(currentSound, soundFile_);
  system(soundCommand);
}



