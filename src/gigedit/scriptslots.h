/*
    Copyright (c) 2014 - 2017 Christian Schoenebeck
    
    This file is part of "gigedit" and released under the terms of the
    GNU General Public License version 2.
*/

#ifndef GIGEDIT_SCRIPTSLOTS_H
#define GIGEDIT_SCRIPTSLOTS_H

#ifdef LIBGIG_HEADER_FILE
# include LIBGIG_HEADER_FILE(gig.h)
#else
# include <gig.h>
#endif

#ifdef GTKMM_HEADER_FILE
# include GTKMM_HEADER_FILE(gtkmm.h)
#else
# include <gtkmm.h>
#endif
#include "compat.h"
#include <vector>
#include "Settings.h"
#include "ManagedWindow.h"

class ScriptSlots : public ManagedWindow {
public:
    ScriptSlots();
   ~ScriptSlots();
    void setInstrument(gig::Instrument* instrument);

    sigc::signal<void, gig::Instrument*>& signal_script_slots_changed();

    // implementation for abstract methods of interface class "ManagedWindow"
    virtual Settings::Property<int>* windowSettingX() { return &Settings::singleton()->scriptSlotsWindowX; }
    virtual Settings::Property<int>* windowSettingY() { return &Settings::singleton()->scriptSlotsWindowY; }
    virtual Settings::Property<int>* windowSettingWidth() { return &Settings::singleton()->scriptSlotsWindowW; }
    virtual Settings::Property<int>* windowSettingHeight() { return &Settings::singleton()->scriptSlotsWindowH; }

protected:
    struct Row {
        int id;
        HBox*        hbox;
        Gtk::Label*  label;
        Gtk::Button* upButton;
        Gtk::Button* downButton;
        Gtk::Button* deleteButton;
        gig::Script* script;
    };

    sigc::signal<void, gig::Instrument*> script_slots_changed_signal;

    VBox m_vbox;
    Gtk::Label m_generalInfoLabel;
    HButtonBox m_buttonBox;
    Gtk::ScrolledWindow m_scrolledWindow;
    VBox m_vboxSlots;
    Gtk::Label m_dragHintLabel;
    Gtk::Button m_closeButton;

    gig::Instrument* m_instrument;
    std::vector<Row> m_slots;

    void onScriptDragNDropDataReceived(
        const Glib::RefPtr<Gdk::DragContext>& context, int, int,
        const Gtk::SelectionData& selection_data, guint, guint time);
    void appendNewSlot(gig::Script* script);
    void moveSlotUp(int slotID);
    void moveSlotDown(int slotID);
    void deleteSlot(int slotID);
    void refreshSlots();
    void onButtonClose();
    void onWindowHide();
    void clearSlots();
};

#endif // GIGEDIT_SCRIPTSLOTS_H
