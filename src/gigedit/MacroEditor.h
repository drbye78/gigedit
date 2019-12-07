/*
    Copyright (c) MMXVII Christian Schoenebeck

    This file is part of "gigedit" and released under the terms of the
    GNU General Public License version 2.
*/

#ifndef GIGEDIT_MACROEDITOR_H
#define GIGEDIT_MACROEDITOR_H

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
#include "ManagedWindow.h"

/** @brief Editor for gigedit macros.
 *
 * Implements a window which allows to edit the abstract tree of one macro
 * that may be applied with gigedit.
 */
class MacroEditor : public ManagedWindow {
public:
    MacroEditor();
   ~MacroEditor();
    void setMacro(Serialization::Archive* macro, bool isClipboard);

    sigc::signal<void>& signal_changes_applied();

    // implementation for abstract methods of interface class "ManagedWindow"
    virtual Settings::Property<int>* windowSettingX() { return &Settings::singleton()->macroEditorWindowX; }
    virtual Settings::Property<int>* windowSettingY() { return &Settings::singleton()->macroEditorWindowY; }
    virtual Settings::Property<int>* windowSettingWidth() { return &Settings::singleton()->macroEditorWindowW; }
    virtual Settings::Property<int>* windowSettingHeight() { return &Settings::singleton()->macroEditorWindowH; }

protected:
    Serialization::Archive* m_macroOriginal;
    Serialization::Archive  m_macro;

    sigc::signal<void> m_changes_applied;

    VBox m_vbox;
    HBox m_footerHBox;
    HBox m_statusHBox;
    HButtonBox m_buttonBoxL;
    HButtonBox m_buttonBox;
    Gtk::ScrolledWindow m_scrolledWindow;
#if GTKMM_MAJOR_VERSION < 3
    view::WrapLabel m_labelIntro;
#else
    Gtk::Label m_labelIntro;
#endif

    class ComboOptionsModel : public Gtk::TreeModel::ColumnRecord {
    public:
        ComboOptionsModel() {
            add(m_col_choice);
        }

        Gtk::TreeModelColumn<Glib::ustring> m_col_choice;
    } m_comboOptionsModel;

    class MacroModel : public Gtk::TreeModel::ColumnRecord {
    public:
        MacroModel() {
            add(m_col_name);
            add(m_col_type);
            add(m_col_value);
            add(m_col_uid);
            add(m_col_allowTextEntry);
            add(m_col_editable);
            add(m_col_options);
        }

        Gtk::TreeModelColumn<Glib::ustring> m_col_name;
        Gtk::TreeModelColumn<Glib::ustring> m_col_type;
        Gtk::TreeModelColumn<Glib::ustring> m_col_value;
        Gtk::TreeModelColumn<Serialization::UID> m_col_uid;
        Gtk::TreeModelColumn<bool> m_col_allowTextEntry;
        Gtk::TreeModelColumn<bool> m_col_editable;
        Gtk::TreeModelColumn<Glib::RefPtr<Gtk::ListStore> > m_col_options;
    } m_treeModelMacro;

    class MacroTreeStore : public Gtk::TreeStore {
    public:
        static Glib::RefPtr<MacroTreeStore> create(const MacroModel& columns) {
            return Glib::RefPtr<MacroTreeStore>( new MacroTreeStore(columns) );
        }
    protected:
        MacroTreeStore(const MacroModel& columns) : Gtk::TreeStore(columns) {}
    };

    Gtk::TreeView m_treeViewMacro;
    Glib::RefPtr<MacroTreeStore> m_treeStoreMacro;
    Gtk::CellRendererCombo m_valueCellRenderer;
    Glib::RefPtr<Gtk::ListStore> m_comboBoxModel;
    bool m_ignoreTreeViewValueChange;

    Gtk::Label m_statusLabel;
    Gtk::Button m_deleteButton;
    Gtk::Button m_inverseDeleteButton;
    Gtk::Button m_applyButton;
    Gtk::Button m_cancelButton;

    bool m_altKeyDown;
    bool m_primaryKeyDown; // on Mac: Cmd key, on all other OSs: Ctrl key

    bool isModified() const;
    void onButtonCancel();
    void onButtonApply();
    void onWindowHide();
#if GTKMM_MAJOR_VERSION > 3 || (GTKMM_MAJOR_VERSION == 3 && (GTKMM_MINOR_VERSION > 91 || (GTKMM_MINOR_VERSION == 91 && GTKMM_MICRO_VERSION >= 2))) // GTKMM >= 3.91.2
    bool onWindowDelete(Gdk::Event& e);
#endif
    bool onWindowDeleteP(GdkEventAny* e);
    void updateStatus();
    void updateStatusBar();
    void reloadTreeView();
    void buildTreeView(const Gtk::TreeModel::Row& parentRow, const Serialization::Object& parentObject);
    void onTreeViewSelectionChanged();
    
#if GTKMM_MAJOR_VERSION > 3 || (GTKMM_MAJOR_VERSION == 3 && (GTKMM_MINOR_VERSION > 91 || (GTKMM_MINOR_VERSION == 91 && GTKMM_MICRO_VERSION >= 2))) // GTKMM >= 3.91.2
    bool onMacroTreeViewKeyRelease(Gdk::EventKey& key);
#else
    void onMacroTreeViewKeyRelease(GdkEventKey* button);
#endif
    void onMacroTreeViewRowValueChanged(const Gtk::TreeModel::Path& path,
                                        const Gtk::TreeModel::iterator& iter);
    void onMacroTreeViewRowValueChangedImpl(const Gtk::TreeModel::Path& path,
                                            const Gtk::TreeModel::iterator& iter,
                                            const Glib::ustring& value);
    void onValueCellEdited(const Glib::ustring& sPath, const Glib::ustring& text);
    void deleteSelectedRows();
    void inverseDeleteSelectedRows();
    void deleteRows(const std::vector<Gtk::TreeModel::Path>& rows);
#if GTKMM_MAJOR_VERSION > 3 || (GTKMM_MAJOR_VERSION == 3 && (GTKMM_MINOR_VERSION > 91 || (GTKMM_MINOR_VERSION == 91 && GTKMM_MICRO_VERSION >= 2))) // GTKMM >= 3.91.2
    bool onKeyPressed(Gdk::EventKey& key);
    bool onKeyReleased(Gdk::EventKey& key);
#else
    bool onKeyPressed(GdkEventKey* key);
    bool onKeyReleased(GdkEventKey* key);
#endif
    Glib::RefPtr<Gtk::ListStore> createComboOptions(const char** options);
};

#endif // GIGEDIT_MACROEDITOR_H
