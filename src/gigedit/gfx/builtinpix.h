#ifndef GIGEDIT_BUILTIN_PIX
#define GIGEDIT_BUILTIN_PIX

#include <gdkmm/pixbuf.h>

void loadBuiltInPix();

extern Glib::RefPtr<Gdk::Pixbuf> redDot;
extern Glib::RefPtr<Gdk::Pixbuf> yellowDot;
extern Glib::RefPtr<Gdk::Pixbuf> blackLoop;
extern Glib::RefPtr<Gdk::Pixbuf> blueHatchedPattern;
extern Glib::RefPtr<Gdk::Pixbuf> blueHatchedPattern2;
extern Glib::RefPtr<Gdk::Pixbuf> grayBlueHatchedPattern;
extern Glib::RefPtr<Gdk::Pixbuf> grayLoop;

#endif // GIGEDIT_BUILTIN_PIX
