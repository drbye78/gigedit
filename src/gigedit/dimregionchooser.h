/*                                                         -*- c++ -*-
 * Copyright (C) 2006-2017 Andreas Persson
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with program; see the file COPYING. If not, write to the Free
 * Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA.
 */

#ifndef GIGEDIT_DIMREGIONCHOOSER_H
#define GIGEDIT_DIMREGIONCHOOSER_H

#include <gtkmm/drawingarea.h>
#include <gtkmm/menu.h>
#include <gdkmm/window.h>

#include "compat.h"

#ifdef LIBGIG_HEADER_FILE
# include LIBGIG_HEADER_FILE(gig.h)
#else
# include <gig.h>
#endif

#if USE_GTKMM_BUILDER
# include <gtkmm/builder.h>
#else
# include <gtkmm/uimanager.h> // deprecated in gtkmm >= 3.21.4
#endif

#include <set>
#include <map>

#include "global.h"

class DimRegionChooser : public Gtk::DrawingArea
{
public:
    DimRegionChooser(Gtk::Window& window);
    virtual ~DimRegionChooser();

    void set_region(gig::Region* region);

    sigc::signal<void>& signal_dimregion_selected();
    sigc::signal<void>& signal_region_changed();

    gig::DimensionRegion* get_main_dimregion() const;
    void get_dimregions(const gig::Region* region, bool stereo,
                        std::set<gig::DimensionRegion*>& dimregs) const;
    bool select_dimregion(gig::DimensionRegion* dimrgn);
    void select_next_dimzone(bool add = false);
    void select_prev_dimzone(bool add = false);
    void select_next_dimension();
    void select_prev_dimension();

    // those 3 are ATM only relevant when resizing custom dimension region zones
    // and for painting those auto selected zones with gray hatched pattern
    void setModifyBothChannels(bool b);
    void setModifyAllDimensionRegions(bool b);
    void setModifyAllRegions(bool b);

protected:
#if (GTKMM_MAJOR_VERSION == 2 && GTKMM_MINOR_VERSION < 90) || GTKMM_MAJOR_VERSION < 2
    virtual bool on_expose_event(GdkEventExpose* e);
#else
    virtual bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr);
#endif
    virtual bool on_button_press_event(GdkEventButton* event);
    virtual bool on_button_release_event(GdkEventButton* event);
    virtual bool on_motion_notify_event(GdkEventMotion* event);
    virtual bool on_focus(Gtk::DirectionType direction);
#if GTKMM_MAJOR_VERSION > 3 || (GTKMM_MAJOR_VERSION == 3 && (GTKMM_MINOR_VERSION > 91 || (GTKMM_MINOR_VERSION == 91 && GTKMM_MICRO_VERSION >= 2))) // GTKMM >= 3.91.2
    bool onKeyPressed(Gdk::EventKey& key);
    bool onKeyReleased(Gdk::EventKey& key);
#else
    bool onKeyPressed(GdkEventKey* key);
    bool onKeyReleased(GdkEventKey* key);
#endif
    void refresh_all();
    void on_show_tooltips_changed();
    void split_dimension_zone();
    void delete_dimension_zone();
    void resetSelectedZones();
    void select_dimzone_by_dir(int dir, bool add = false);
    void drawIconsFor(gig::dimension_t dimension, uint zone,
                      const Cairo::RefPtr<Cairo::Context>& cr,
                      int x, int y, int w, int h);

    Gdk::RGBA red, blue, black, white;
    Glib::RefPtr<Gdk::Pixbuf> blueHatchedPatternARGB;
    Cairo::RefPtr<Cairo::SurfacePattern> blueHatchedSurfacePattern;
    Glib::RefPtr<Gdk::Pixbuf> blueHatchedPattern2ARGB;
    Cairo::RefPtr<Cairo::SurfacePattern> blueHatchedSurfacePattern2;
    Glib::RefPtr<Gdk::Pixbuf> grayBlueHatchedPatternARGB;
    Cairo::RefPtr<Cairo::SurfacePattern> grayBlueHatchedSurfacePattern;

    gig::Instrument* instrument;
    gig::Region* region;

    sigc::signal<void> dimregion_selected;
    sigc::signal<void> region_changed;

    // those 3 are ATM only relevant when resizing custom dimension region zones
    // and for painting those auto selected zones with gray hatched pattern
    bool modifybothchannels;
    bool modifyalldimregs;
    bool modifyallregions;

    int focus_line;
    std::map<gig::dimension_t, std::set<int> > dimzones; ///< Reflects which zone(s) of the individual dimension are currently selected.
    int label_width;
    bool labels_changed;
    int nbDimensions;

    // the "main" dimension region is the one that is used to i.e. evaluate the
    // precise custom velocity splits (could also be interpreted for focus stuff,
    // i.e. keyboard arrow key navigation)
    // NOTE: these may *not* necessarily currently be selected !
    gig::dimension_t maindimtype;
    DimensionCase maindimcase;
    int maindimregno;

    // information needed during a resize
    struct {
        bool active;
        enum {
            none,
            left,
            right
        } selected;
        int pos;
        int min;
        int max;
        int dimension;
        gig::dimension_def_t dimensionDef;
        int zone;
    } resize;

    bool multiSelectKeyDown;
    bool primaryKeyDown; // on Mac: Cmd key, on all other OSs: Ctrl key
    bool shiftKeyDown;

    bool cursor_is_resize;
    bool is_in_resize_zone(double x, double y);
    void update_after_resize();

    int h;

    Glib::RefPtr<ActionGroup> actionGroup;
#if USE_GTKMM_BUILDER
    Glib::RefPtr<Gtk::Builder> uiManager;
#else
    Glib::RefPtr<Gtk::UIManager> uiManager;
#endif
    Gtk::Menu* popup_menu_inside_dimregion;
    Gtk::Menu* popup_menu_outside_dimregion;

private:
    Glib::RefPtr<Action> actionDeleteDimZone;
    Glib::RefPtr<Action> actionSplitDimZone;
};

#endif
