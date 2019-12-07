#include "builtinpix.h"
#include "../compat.h"

Glib::RefPtr<Gdk::Pixbuf> redDot;
Glib::RefPtr<Gdk::Pixbuf> yellowDot;
Glib::RefPtr<Gdk::Pixbuf> blackLoop;
Glib::RefPtr<Gdk::Pixbuf> blueHatchedPattern;
Glib::RefPtr<Gdk::Pixbuf> blueHatchedPattern2;
Glib::RefPtr<Gdk::Pixbuf> grayBlueHatchedPattern;
Glib::RefPtr<Gdk::Pixbuf> grayLoop;

extern const unsigned char red_dot_rgba[];
extern const unsigned char yellow_dot_rgba[];
extern const unsigned char black_loop_rgba[];
extern const unsigned char blue_hatched_pattern_rgba[];
extern const unsigned char blue_hatched_pattern2_rgba[];
extern const unsigned char grayblue_hatched_pattern_rgba[];
extern const unsigned char gray_loop_rgba[];

extern const int red_dot_rgba_size;
extern const int yellow_dot_rgba_size;
extern const int black_loop_rgba_size;
extern const int blue_hatched_pattern_rgba_size;
extern const int blue_hatched_pattern2_rgba_size;
extern const int grayblue_hatched_pattern_rgba_size;
extern const int gray_loop_rgba_size;

static struct _BuiltInPixMap {
    Glib::RefPtr<Gdk::Pixbuf>* pixbuf;
    const unsigned char* raw;
    int size;
} builtInPixMap[] = {
    { &redDot, red_dot_rgba, red_dot_rgba_size },
    { &yellowDot, yellow_dot_rgba, yellow_dot_rgba_size },
    { &blackLoop, black_loop_rgba, black_loop_rgba_size },
    { &blueHatchedPattern, blue_hatched_pattern_rgba, blue_hatched_pattern_rgba_size },
    { &blueHatchedPattern2, blue_hatched_pattern2_rgba, blue_hatched_pattern2_rgba_size },
    { &grayBlueHatchedPattern, grayblue_hatched_pattern_rgba, grayblue_hatched_pattern_rgba_size },
    { &grayLoop, gray_loop_rgba, gray_loop_rgba_size },
};

void loadBuiltInPix() {
    if (*builtInPixMap[0].pixbuf) return;
    const int n = sizeof(builtInPixMap) / sizeof(_BuiltInPixMap);
    for (int i = 0; i < n; ++i) {
# if GTKMM_MAJOR_VERSION > 3 || (GTKMM_MAJOR_VERSION == 3 && (GTKMM_MINOR_VERSION > 89 || (GTKMM_MINOR_VERSION == 89 && GTKMM_MICRO_VERSION >= 2))) // GTKMM >= 3.89.2
        GdkPixbuf* pPixbuf = gdk_pixbuf_new_from_inline(
            builtInPixMap[i].size,
            builtInPixMap[i].raw,
            false,
            NULL
        );
        *builtInPixMap[i].pixbuf = Glib::wrap(pPixbuf);
#else
        *builtInPixMap[i].pixbuf = Gdk::Pixbuf::create_from_inline(
            builtInPixMap[i].size,
            builtInPixMap[i].raw
        );
#endif
    }
}
