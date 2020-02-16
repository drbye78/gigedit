/*
    Copyright (c) MMXX Christian Schoenebeck

    This file is part of "gigedit" and released under the terms of the
    GNU General Public License version 2.
*/

#ifndef SCRIPT_PATCH_VARS_H
#define SCRIPT_PATCH_VARS_H

#ifdef LIBGIG_HEADER_FILE
# include LIBGIG_HEADER_FILE(gig.h)
# include LIBGIG_HEADER_FILE(Serialization.h)
#else
# include <gig.h>
# include <Serialization.h>
#endif

#ifdef GTKMM_HEADER_FILE
# include GTKMM_HEADER_FILE(gtkmm.h)
#else
# include <gtkmm.h>
#endif

#if HAVE_CONFIG_H
# include <config.h>
#endif
#include "compat.h"
#if USE_GTKMM_BUILDER
# include <gtkmm/builder.h>
#else
# include <gtkmm/uimanager.h> // deprecated in gtkmm >= 3.21.4
#endif
#include "wrapLabel.hh"

#include "scripteditor.h" // lazy inclusion of LS script VM header files

/** @brief Editor for real-time instrument script 'patch' variables.
 *
 * 'Patch' variables are special variable types in real-time instrument scripts
 * which allow to override their initial value on a per instrument basis. That
 * allows to share the same script among multiple instruments, while at the same
 * time being able to fine tune certain aspects of the script for a specific
 * instrument.
 *
 * This class implements a window which allows to edit such 'patch' variables
 * for the currently selected instrument. In Gigedit this is currently available
 * under the 'Script' tab on the right hand side of Gigedit's main window.
 */
class ScriptPatchVars : public Gtk::ScrolledWindow {
public:
    ScriptPatchVars();
    void setInstrument(gig::Instrument* pInstrument, bool forceUpdate = false);
    void deleteSelectedRows();

    sigc::signal<void, gig::Instrument*> signal_vars_to_be_changed;
    sigc::signal<void, gig::Instrument*> signal_vars_changed;

    class VarsModel : public Gtk::TreeModel::ColumnRecord {
    public:
        VarsModel() {
            add(m_col_name);
            add(m_col_name_color);
            add(m_col_name_weight);
            add(m_col_type);
            add(m_col_value);
            add(m_col_value_color);
            add(m_col_value_weight);
            add(m_col_value_tooltip);
            add(m_col_slot);
            add(m_col_allowTextEntry);
            add(m_col_editable);
            add(m_col_options);
            add(m_col_script);
        }

        Gtk::TreeModelColumn<Glib::ustring> m_col_name;
        Gtk::TreeModelColumn<Glib::ustring> m_col_name_color;
        Gtk::TreeModelColumn<int>           m_col_name_weight;
        Gtk::TreeModelColumn<Glib::ustring> m_col_type;
        Gtk::TreeModelColumn<Glib::ustring> m_col_value;
        Gtk::TreeModelColumn<Glib::ustring> m_col_value_color;
        Gtk::TreeModelColumn<int>           m_col_value_weight;
        Gtk::TreeModelColumn<Glib::ustring> m_col_value_tooltip;
        Gtk::TreeModelColumn<int> m_col_slot;
        Gtk::TreeModelColumn<bool> m_col_allowTextEntry;
        Gtk::TreeModelColumn<bool> m_col_editable;
        Gtk::TreeModelColumn<Glib::RefPtr<Gtk::ListStore> > m_col_options;
        Gtk::TreeModelColumn<gig::Script*> m_col_script;
    } m_treeModel;

    class VarsTreeStore : public Gtk::TreeStore {
    public:
        static Glib::RefPtr<VarsTreeStore> create(const VarsModel& columns) {
            return Glib::RefPtr<VarsTreeStore>( new VarsTreeStore(columns) );
        }
    protected:
        VarsTreeStore(const VarsModel& columns) : Gtk::TreeStore(columns) {}
    };

    Gtk::TreeView m_treeView;
    Glib::RefPtr<VarsTreeStore> m_treeStore;
    Gtk::CellRendererCombo m_valueCellRenderer;
    bool m_ignoreTreeViewValueChange;

protected:
    void buildTreeViewVar(const Gtk::TreeModel::Row& parentRow, int iScriptSlot,
                          gig::Script* script, const std::string name,
                          const struct PatchVar* var);
    void buildTreeViewSlot(const Gtk::TreeModel::Row& parentRow, int iScriptSlot);
    void reloadTreeView();
    void onValueCellEdited(const Glib::ustring& sPath, const Glib::ustring& text);
    void onTreeViewSelectionChanged();
    bool onQueryTreeViewTooltip(int x, int y, bool keyboardTip,
                                const Glib::RefPtr<Gtk::Tooltip>& tooltip);
#if GTKMM_MAJOR_VERSION > 3 || (GTKMM_MAJOR_VERSION == 3 && (GTKMM_MINOR_VERSION > 91 || (GTKMM_MINOR_VERSION == 91 && GTKMM_MICRO_VERSION >= 2))) // GTKMM >= 3.91.2
    bool onTreeViewKeyRelease(Gdk::EventKey& key);
#else
    void onTreeViewKeyRelease(GdkEventKey* button);
#endif
    void deleteRows(const std::vector<Gtk::TreeModel::Path>& rows);
    void onTreeViewRowChanged(const Gtk::TreeModel::Path& path,
                              const Gtk::TreeModel::iterator& iter);
    void onTreeViewRowValueChanged(const Gtk::TreeModel::Path& path,
                                   const Gtk::TreeModel::iterator& iter,
                                   const Glib::ustring value);
private:
    ::gig::Instrument* m_instrument;
    bool m_editing;
};

#endif // SCRIPT_PATCH_VARS_H
