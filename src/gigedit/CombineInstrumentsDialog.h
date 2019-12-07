/*
    Copyright (c) 2014-2018 Christian Schoenebeck
    
    This file is part of "gigedit" and released under the terms of the
    GNU General Public License version 2.
*/

#ifndef GIGEDIT_COMBINEINSTRUMENTSDIALOG
#define GIGEDIT_COMBINEINSTRUMENTSDIALOG

#ifdef LIBGIG_HEADER_FILE
# include LIBGIG_HEADER_FILE(gig.h)
#else
# include <gig.h>
#endif

#include "compat.h"

#include <gtkmm/buttonbox.h>
#include <gtkmm/window.h>
#include <gtkmm/dialog.h>
#include <gtkmm/treeview.h>
#include <gtkmm/liststore.h>
#include <gtkmm/iconview.h>
#if USE_GTKMM_GRID
# include <gtkmm/grid.h>
#else
# include <gtkmm/table.h>
#endif
#include <gtkmm/comboboxtext.h>
#include <gtkmm/scrolledwindow.h>

#include "wrapLabel.hh"
#include "ManagedWindow.h"

#include <set>

/**
 * @brief Modal dialog which allows to merge instruments.
 *
 * This dialog shows a list of all instruments of the currently open .gig file
 * in gigedit and allows the user to shift select a set of instruments to be
 * combined.
 *
 * If the user successfully combined instruments in this dialog, then
 * newCombinedInstrument() will return a pointer to that new instrument.
 */
class CombineInstrumentsDialog : public ManagedDialog {
public:
    CombineInstrumentsDialog(Gtk::Window& parent, gig::File* gig);
    bool fileWasChanged() const;
    gig::Instrument* newCombinedInstrument() const;
    void setSelectedInstruments(const std::set<int>& instrumentIndeces);

    // implementation for abstract methods of interface class "ManagedDialog"
    virtual Settings::Property<int>* windowSettingX() { return &Settings::singleton()->combineInstrumentsWindowX; }
    virtual Settings::Property<int>* windowSettingY() { return &Settings::singleton()->combineInstrumentsWindowY; }
    virtual Settings::Property<int>* windowSettingWidth() { return &Settings::singleton()->combineInstrumentsWindowW; }
    virtual Settings::Property<int>* windowSettingHeight() { return &Settings::singleton()->combineInstrumentsWindowH; }

protected:
    gig::File* m_gig;
    bool m_fileWasChanged;
    gig::Instrument* m_newCombinedInstrument;

    HButtonBox m_buttonBox;
    Gtk::ScrolledWindow m_scrolledWindow;
    Gtk::TreeView   m_treeView;
    Gtk::IconView   m_iconView;
    Gtk::Button     m_cancelButton;
    Gtk::Button     m_OKButton;
#if GTKMM_MAJOR_VERSION < 3
    view::WrapLabel m_descriptionLabel;
#else
    Gtk::Label      m_descriptionLabel;
#endif
#if USE_GTKMM_GRID
    Gtk::Grid       m_tableDimCombo;
#else
    Gtk::Table      m_tableDimCombo;
#endif
    Gtk::ComboBox   m_comboDimType;
    Gtk::Label      m_labelDimType;
    Gtk::Label      m_labelOrder;

    class ComboDimsModel : public Gtk::TreeModel::ColumnRecord {
    public:
        ComboDimsModel() {
            add(m_type_id);
            add(m_type_name);
        }

        Gtk::TreeModelColumn<int> m_type_id;
        Gtk::TreeModelColumn<Glib::ustring> m_type_name;
    } m_comboDimsModel;

    class ListModel : public Gtk::TreeModel::ColumnRecord {
    public:
        ListModel() {
            add(m_col_index);
            add(m_col_name);
            add(m_col_instr);
        }

        Gtk::TreeModelColumn<int> m_col_index;
        Gtk::TreeModelColumn<Glib::ustring>    m_col_name;
        Gtk::TreeModelColumn<gig::Instrument*> m_col_instr;
    } m_columns;

    class OrderListModel : public Gtk::TreeModel::ColumnRecord {
    public:
        OrderListModel() {
            add(m_col_name);
            add(m_col_markup);
            add(m_col_instr);
        }

        Gtk::TreeModelColumn<Glib::ustring>    m_col_name;
        Gtk::TreeModelColumn<Glib::ustring>    m_col_markup;
        Gtk::TreeModelColumn<gig::Instrument*> m_col_instr;
    } m_orderColumns;

    Glib::RefPtr<Gtk::ListStore> m_refTreeModel;
    Glib::RefPtr<Gtk::ListStore> m_refOrderModel;
    bool first_call_to_drag_data_get;

    void combineSelectedInstruments();
    void onSelectionChanged();
    void on_order_drag_begin(const Glib::RefPtr<Gdk::DragContext>& context);
    void on_order_drag_data_get(const Glib::RefPtr<Gdk::DragContext>&,
                                Gtk::SelectionData& selection_data, guint, guint);
    void on_order_drop_drag_data_received(
        const Glib::RefPtr<Gdk::DragContext>& context, int x, int y,
        const Gtk::SelectionData& selection_data, guint, guint time
    );
    void on_show_tooltips_changed();
};

#endif // GIGEDIT_COMBINEINSTRUMENTSDIALOG
