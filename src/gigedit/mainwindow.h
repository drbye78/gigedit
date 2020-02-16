/*                                                         -*- c++ -*-
 * Copyright (C) 2006 - 2020 Andreas Persson
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

#ifndef GIGEDIT_MAINWINDOW_H
#define GIGEDIT_MAINWINDOW_H

#ifdef LIBGIG_HEADER_FILE
# include LIBGIG_HEADER_FILE(gig.h)
# include LIBGIG_HEADER_FILE(Serialization.h)
#else
# include <gig.h>
# include <Serialization.h>
#endif

#include "compat.h"

#include <gtkmm/box.h>
#include <gtkmm/buttonbox.h>
#include <gtkmm/dialog.h>
#include <gtkmm/liststore.h>
#include <gtkmm/menu.h>
#include <gtkmm/paned.h>
#include <gtkmm/progressbar.h>
#include <gtkmm/radiomenuitem.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/treestore.h>
#include <gtkmm/treemodelfilter.h>
#include <gtkmm/window.h>
#include <gtkmm/statusbar.h>
#include <gtkmm/image.h>
#include <gtkmm/entry.h>
#include <gtkmm/notebook.h>

#if USE_GTKMM_BUILDER
# include <gtkmm/builder.h>
#else
# include <gtkmm/uimanager.h> // deprecated in gtkmm >= 3.21.4
#endif

#include <sstream>

#include "regionchooser.h"
#include "dimregionchooser.h"
#include "dimregionedit.h"
#include "midirules.h"
#ifdef GLIB_THREADS
#ifndef OLD_THREADS
#include <glibmm/threads.h>
#endif
#else
#include <thread>
#include <mutex>
#endif
#include "ManagedWindow.h"

class MainWindow;

class FilePropDialog : public ManagedWindow,
                       public PropEditor<DLS::Info> {
public:
    FilePropDialog();
    void set_file(gig::File* file);

    // implementation for abstract methods of interface class "ManagedWindow"
    virtual Settings::Property<int>* windowSettingX() { return &Settings::singleton()->filePropsWindowX; }
    virtual Settings::Property<int>* windowSettingY() { return &Settings::singleton()->filePropsWindowY; }
    virtual Settings::Property<int>* windowSettingWidth() { return &Settings::singleton()->filePropsWindowW; }
    virtual Settings::Property<int>* windowSettingHeight() { return &Settings::singleton()->filePropsWindowH; }

protected:
    ChoiceEntry<int> eFileFormat;
    StringEntry eName;
    StringEntry eCreationDate;
    StringEntryMultiLine eComments;
    StringEntry eProduct;
    StringEntry eCopyright;
    StringEntry eArtists;
    StringEntry eGenre;
    StringEntry eKeywords;
    StringEntry eEngineer;
    StringEntry eTechnician;
    StringEntry eSoftware;
    StringEntry eMedium;
    StringEntry eSource;
    StringEntry eSourceForm;
    StringEntry eCommissioned;
    StringEntry eSubject;
    VBox vbox;
    HButtonBox buttonBox;
    Gtk::Button quitButton;
    Table table;

    gig::File* m_file;

    void set_FileFormat(int value);
};

class InstrumentProps : public ManagedWindow,
                        public PropEditor<gig::Instrument> {
public:
    InstrumentProps();
    void set_instrument(gig::Instrument* instrument);
    gig::Instrument* get_instrument() { return m; }
    void update_name();
    sigc::signal<void>& signal_name_changed() {
        return sig_name_changed;
    }

    // implementation for abstract methods of interface class "ManagedWindow"
    virtual Settings::Property<int>* windowSettingX() { return &Settings::singleton()->instrPropsWindowX; }
    virtual Settings::Property<int>* windowSettingY() { return &Settings::singleton()->instrPropsWindowY; }
    virtual Settings::Property<int>* windowSettingWidth() { return &Settings::singleton()->instrPropsWindowW; }
    virtual Settings::Property<int>* windowSettingHeight() { return &Settings::singleton()->instrPropsWindowH; }

protected:
    void set_Name(const gig::String& name);
    void set_IsDrum(bool value);
    void set_MIDIBank(uint16_t value);
    void set_MIDIProgram(uint32_t value);

    sigc::signal<void> sig_name_changed;
    Gtk::Notebook tabs;
    VBox vbox[3];
    HButtonBox buttonBox;
    Gtk::Button quitButton;

    // tab 1
    Table table;
    StringEntry eName;
    BoolEntry eIsDrum;
    NumEntryTemp<uint16_t> eMIDIBank;
    NumEntryTemp<uint32_t> eMIDIProgram;
    NumEntryGain eAttenuation;
    NumEntryTemp<uint16_t> eEffectSend;
    NumEntryTemp<int16_t> eFineTune;
    NumEntryTemp<uint16_t> ePitchbendRange;
    BoolEntry ePianoReleaseMode;
    NoteEntry eDimensionKeyRangeLow;
    NoteEntry eDimensionKeyRangeHigh;

    // tab 2
    Table table2;
    StringEntry eName2;
    StringEntry eCreationDate;
    StringEntryMultiLine eComments;
    StringEntry eProduct;
    StringEntry eCopyright;
    StringEntry eArtists;
    StringEntry eGenre;
    StringEntry eKeywords;
    StringEntry eEngineer;
    StringEntry eTechnician;
    StringEntry eSoftware;
    StringEntry eMedium;
    StringEntry eSource;
    StringEntry eSourceForm;
    StringEntry eCommissioned;
    StringEntry eSubject;
};

class SampleProps : public ManagedWindow,
                    public PropEditor<gig::Sample> {
public:
    SampleProps();
    void set_sample(gig::Sample* sample);
    gig::Sample* get_sample() { return m; }
    void update_name();
    sigc::signal<void>& signal_name_changed() {
        return sig_name_changed;
    }

    // implementation for abstract methods of interface class "ManagedWindow"
    virtual Settings::Property<int>* windowSettingX() { return &Settings::singleton()->samplePropsWindowX; }
    virtual Settings::Property<int>* windowSettingY() { return &Settings::singleton()->samplePropsWindowY; }
    virtual Settings::Property<int>* windowSettingWidth() { return &Settings::singleton()->samplePropsWindowW; }
    virtual Settings::Property<int>* windowSettingHeight() { return &Settings::singleton()->samplePropsWindowH; }

protected:
    void set_Name(const gig::String& name);

    sigc::signal<void> sig_name_changed;
    Gtk::Notebook tabs;
    VBox vbox[3];
    HButtonBox buttonBox;
    Gtk::Button quitButton;

    // tab 1
    Table table;
    StringEntry eName;
    NoteEntry eUnityNote;
    ReadOnlyLabelWidget eSampleGroup;
    ReadOnlyLabelWidget eSampleFormatInfo;
    ReadOnlyLabelWidget eSampleID;
    ReadOnlyLabelWidget eChecksum;
    NumEntryTemp<uint32_t> eLoopsCount;
    NumEntryTemp<uint32_t> eLoopStart;
    NumEntryTemp<uint32_t> eLoopLength;
    ChoiceEntry<gig::loop_type_t> eLoopType;
    NumEntryTemp<uint32_t> eLoopPlayCount;
    // tab 2
    Table table2;
    StringEntry eName2;
    StringEntry eCreationDate;
    StringEntryMultiLine eComments;
    StringEntry eProduct;
    StringEntry eCopyright;
    StringEntry eArtists;
    StringEntry eGenre;
    StringEntry eKeywords;
    StringEntry eEngineer;
    StringEntry eTechnician;
    StringEntry eSoftware;
    StringEntry eMedium;
    StringEntry eSource;
    StringEntry eSourceForm;
    StringEntry eCommissioned;
    StringEntry eSubject;
};

class ProgressDialog : public Gtk::Dialog {
public:
    ProgressDialog(const Glib::ustring& title, Gtk::Window& parent);
    void set_fraction(float fraction) { progressBar.set_fraction(fraction); }
protected:
    Gtk::ProgressBar progressBar;
};

class LoaderSaverBase {
public:
    void launch();
    Glib::Dispatcher& signal_progress();
    Glib::Dispatcher& signal_finished(); ///< Finished successfully, without error.
    Glib::Dispatcher& signal_error();
    void progress_callback(float fraction);
    float get_progress();
    void join();
    const Glib::ustring filename;
    Glib::ustring error_message;
    gig::File* gig;

protected:
    LoaderSaverBase(const Glib::ustring filename, gig::File* gig);

private:
#ifdef GLIB_THREADS
    Glib::Threads::Thread* thread;
    Glib::Threads::Mutex progressMutex;
#else
    std::thread thread;
    std::mutex progressMutex;
#endif
    void thread_function();
    virtual void thread_function_sub(gig::progress_t& progress) = 0;
    Glib::Dispatcher finished_dispatcher;
    Glib::Dispatcher progress_dispatcher;
    Glib::Dispatcher error_dispatcher;
    float progress;
};

class Loader : public LoaderSaverBase {
public:
    Loader(const char* filename);

private:
    void thread_function_sub(gig::progress_t& progress);
};

class Saver : public LoaderSaverBase {
public:
    Saver(gig::File* file, Glib::ustring filename = ""); ///< one argument means "save", two arguments means "save as"

private:
    void thread_function_sub(gig::progress_t& progress);
};

class MainWindow : public ManagedWindow {
public:
    MainWindow();
    virtual ~MainWindow();
    void load_file(const char* name);
    void load_instrument(gig::Instrument* instr);
    void file_changed();
    sigc::signal<void, gig::File*>& signal_file_structure_to_be_changed();
    sigc::signal<void, gig::File*>& signal_file_structure_changed();
    sigc::signal<void, std::list<gig::Sample*> >& signal_samples_to_be_removed();
    sigc::signal<void>& signal_samples_removed();
    sigc::signal<void, gig::Region*>& signal_region_to_be_changed();
    sigc::signal<void, gig::Region*>& signal_region_changed();
    sigc::signal<void, gig::DimensionRegion*>& signal_dimreg_to_be_changed();
    sigc::signal<void, gig::DimensionRegion*>& signal_dimreg_changed();
    sigc::signal<void, gig::Sample*>& signal_sample_changed();
    sigc::signal<void, gig::Sample*/*old*/, gig::Sample*/*new*/>& signal_sample_ref_changed();

    sigc::signal<void, int/*key*/, int/*velocity*/>& signal_note_on();
    sigc::signal<void, int/*key*/, int/*velocity*/>& signal_note_off();

    sigc::signal<void, int/*key*/, int/*velocity*/>& signal_keyboard_key_hit();
    sigc::signal<void, int/*key*/, int/*velocity*/>& signal_keyboard_key_released();

    sigc::signal<void, gig::Instrument*>& signal_switch_sampler_instrument();

    sigc::signal<void, gig::Script*> signal_script_to_be_changed;
    sigc::signal<void, gig::Script*> signal_script_changed;

    // implementation for abstract methods of interface class "ManagedWindow"
    virtual Settings::Property<int>* windowSettingX() { return &Settings::singleton()->mainWindowX; }
    virtual Settings::Property<int>* windowSettingY() { return &Settings::singleton()->mainWindowY; }
    virtual Settings::Property<int>* windowSettingWidth() { return &Settings::singleton()->mainWindowW; }
    virtual Settings::Property<int>* windowSettingHeight() { return &Settings::singleton()->mainWindowH; }

protected:
#if USE_GTKMM_BUILDER
    Glib::RefPtr<Gio::SimpleActionGroup> m_actionGroup;
    Glib::RefPtr<Gtk::Builder> m_uiManager;
#else
    Glib::RefPtr<Gtk::ActionGroup> actionGroup;
    Glib::RefPtr<Gtk::UIManager> uiManager;
#endif

#if USE_GLIB_ACTION
    Glib::RefPtr<Gio::SimpleAction> m_actionMIDIRules;

    Glib::RefPtr<Gio::SimpleAction> m_actionCopyDimRgn;
    Glib::RefPtr<Gio::SimpleAction> m_actionPasteDimRgn;
    Glib::RefPtr<Gio::SimpleAction> m_actionAdjustClipboard;

    Glib::RefPtr<Gio::SimpleAction> m_actionSampleProperties;
    Glib::RefPtr<Gio::SimpleAction> m_actionAddSample;
    Glib::RefPtr<Gio::SimpleAction> m_actionRemoveSample;
    Glib::RefPtr<Gio::SimpleAction> m_actionViewSampleRefs;
    Glib::RefPtr<Gio::SimpleAction> m_actionReplaceSample;
    Glib::RefPtr<Gio::SimpleAction> m_actionAddSampleGroup;

    Glib::RefPtr<Gio::SimpleAction> m_actionAddScriptGroup;
    Glib::RefPtr<Gio::SimpleAction> m_actionAddScript;
    Glib::RefPtr<Gio::SimpleAction> m_actionEditScript;
    Glib::RefPtr<Gio::SimpleAction> m_actionRemoveScript;

    Glib::RefPtr<Gio::SimpleAction> m_actionInstrDoubleClickOpensProps;

    Glib::RefPtr<Gio::SimpleAction> m_actionToggleCopySampleUnity;
    Glib::RefPtr<Gio::SimpleAction> m_actionToggleCopySampleTune;
    Glib::RefPtr<Gio::SimpleAction> m_actionToggleCopySampleLoop;
    Glib::RefPtr<Gio::SimpleAction> m_actionToggleStatusBar;
    Glib::RefPtr<Gio::SimpleAction> m_actionToggleRestoreWinDim;
    Glib::RefPtr<Gio::SimpleAction> m_actionToggleSaveWithTempFile;
    Glib::RefPtr<Gio::SimpleAction> m_actionToggleWarnOnExtensions;
    Glib::RefPtr<Gio::SimpleAction> m_actionToggleShowTooltips;
    Glib::RefPtr<Gio::SimpleAction> m_actionToggleSyncSamplerSelection;
    Glib::RefPtr<Gio::SimpleAction> m_actionToggleMoveRootNoteWithRegion;
#endif

    Gtk::Statusbar m_StatusBar;
    Gtk::Label     m_AttachedStateLabel;
    Gtk::Image     m_AttachedStateImage;

    RegionChooser m_RegionChooser;
    DimRegionChooser m_DimRegionChooser;

    FilePropDialog fileProps;
    InstrumentProps instrumentProps;
    SampleProps sampleProps;
    MidiRules midiRules;

    /**
     * Ensures that the 2 signals MainWindow::dimreg_to_be_changed_signal and
     * MainWindowv::dimreg_changed_signal are always triggered correctly as a
     * pair. It behaves similar to a "mutex lock guard" design pattern.
     */
    class DimRegionChangeGuard : public SignalGuard<gig::DimensionRegion*> {
    public:
        DimRegionChangeGuard(MainWindow* w, gig::DimensionRegion* pDimReg) :
        SignalGuard<gig::DimensionRegion*>(w->dimreg_to_be_changed_signal, w->dimreg_changed_signal, pDimReg)
        {
        }
    };

    sigc::signal<void, gig::File*> file_structure_to_be_changed_signal;
    sigc::signal<void, gig::File*> file_structure_changed_signal;
    sigc::signal<void, std::list<gig::Sample*> > samples_to_be_removed_signal;
    sigc::signal<void> samples_removed_signal;
    sigc::signal<void, gig::Region*> region_to_be_changed_signal;
    sigc::signal<void, gig::Region*> region_changed_signal;
    sigc::signal<void, gig::DimensionRegion*> dimreg_to_be_changed_signal;
    sigc::signal<void, gig::DimensionRegion*> dimreg_changed_signal;
    sigc::signal<void, gig::Sample*> sample_changed_signal;
    sigc::signal<void, gig::Sample*/*old*/, gig::Sample*/*new*/> sample_ref_changed_signal;

    sigc::signal<void, int/*key*/, int/*velocity*/> note_on_signal;
    sigc::signal<void, int/*key*/, int/*velocity*/> note_off_signal;

    sigc::signal<void, gig::Instrument*> switch_sampler_instrument_signal;

#if !USE_GTKMM_BUILDER
    void on_instrument_selection_change(Gtk::RadioMenuItem* item);
#endif
    void on_action_move_instr();
    void on_sel_change();
    void region_changed();
    void dimreg_changed();
    void select_instrument(gig::Instrument* instrument);
    bool select_dimension_region(gig::DimensionRegion* dimRgn);
    void select_sample(gig::Sample* sample);
    void on_loader_progress();
    void on_loader_finished();
    void on_loader_error();
    void on_saver_progress();
    void on_saver_error();
    void on_saver_finished();
    void updateMacroMenu();
    void onMacroSelected(int iMacro);
    void setupMacros();
    void onMacrosSetupChanged(const std::vector<Serialization::Archive>& macros);
    void applyMacro(Serialization::Archive& macro);
    void onScriptSlotsModified(gig::Instrument* pInstrument);
    void bringToFront();

    void dimreg_all_dimregs_toggled();
    gig::Instrument* get_instrument();
    void add_region_to_dimregs(gig::Region* region, bool stereo, bool all_dimregs);
    void update_dimregs();

    class ModelColumns : public Gtk::TreeModel::ColumnRecord {
    public:
        ModelColumns() {
          add(m_col_nr);
          add(m_col_name);
          add(m_col_instr);
          add(m_col_scripts);
          add(m_col_tooltip);
        }

        Gtk::TreeModelColumn<int> m_col_nr;
        Gtk::TreeModelColumn<Glib::ustring> m_col_name;
        Gtk::TreeModelColumn<gig::Instrument*> m_col_instr;
        Gtk::TreeModelColumn<Glib::ustring> m_col_scripts;
        Gtk::TreeModelColumn<Glib::ustring> m_col_tooltip;
    } m_Columns;

    VBox m_VBox;
    Gtk::HPaned m_HPaned;

    Gtk::ScrolledWindow m_ScrolledWindow;

    Gtk::TreeView m_TreeView;
    Glib::RefPtr<Gtk::ListStore> m_refTreeModel;
    Glib::RefPtr<Gtk::TreeModelFilter> m_refTreeModelFilter; //FIXME: I really would love to get rid of TreeModelFilter, because it causes behavior conflicts with get_model() all over the place (see the respective comments regarding get_model()), however I found no other way to filter a treeview effectively.

#if USE_GTKMM_BUILDER
    Gtk::Menu* menuMacro;
#else
    Gtk::Menu* instrument_menu; // kept for GTKMM 2 version only, will be completely removed in future
#endif
    Gtk::Menu* assign_scripts_menu;

    std::map<gig::Sample*,int> sample_ref_count;

    class SamplesModel : public Gtk::TreeModel::ColumnRecord {
    public:
        SamplesModel() {
            add(m_col_name);
            add(m_col_sample);
            add(m_col_group);
            add(m_col_refcount);
            add(m_color);
        }

        Gtk::TreeModelColumn<Glib::ustring> m_col_name;
        Gtk::TreeModelColumn<gig::Sample*> m_col_sample;
        Gtk::TreeModelColumn<gig::Group*> m_col_group;
        Gtk::TreeModelColumn<Glib::ustring> m_col_refcount;
        Gtk::TreeModelColumn<Glib::ustring> m_color;
    } m_SamplesModel;

    class SamplesTreeStore : public Gtk::TreeStore {
    public:
        static Glib::RefPtr<SamplesTreeStore> create(const SamplesModel& columns) {
            return Glib::RefPtr<SamplesTreeStore>( new SamplesTreeStore(columns) );
        }
    protected:
        SamplesTreeStore(const SamplesModel& columns) : Gtk::TreeStore(columns) {}
    };

    Gtk::ScrolledWindow m_ScrolledWindowSamples;
    Gtk::TreeView m_TreeViewSamples;
    Glib::RefPtr<SamplesTreeStore> m_refSamplesTreeModel;

    class ScriptsModel : public Gtk::TreeModel::ColumnRecord {
    public:
        ScriptsModel() {
            add(m_col_name);
            add(m_col_script);
            add(m_col_group);
        }

        Gtk::TreeModelColumn<Glib::ustring> m_col_name;
        Gtk::TreeModelColumn<gig::Script*> m_col_script;
        Gtk::TreeModelColumn<gig::ScriptGroup*> m_col_group;
    } m_ScriptsModel;

    class ScriptsTreeStore : public Gtk::TreeStore {
    public:
        static Glib::RefPtr<ScriptsTreeStore> create(const ScriptsModel& columns) {
            return Glib::RefPtr<ScriptsTreeStore>( new ScriptsTreeStore(columns) );
        }
    protected:
        ScriptsTreeStore(const ScriptsModel& columns) : Gtk::TreeStore(columns) {}
    };

    Gtk::ScrolledWindow m_ScrolledWindowScripts;
    Gtk::TreeView m_TreeViewScripts;
    Glib::RefPtr<ScriptsTreeStore> m_refScriptsTreeModel;

    VBox dimreg_vbox;
    HBox dimreg_hbox;
    Gtk::Label dimreg_label;
    Gtk::CheckButton dimreg_all_regions;
    Gtk::CheckButton dimreg_all_dimregs;
    Gtk::CheckButton dimreg_stereo;

    HBox legend_hbox;
    Gtk::Label labelLegend;
    Gtk::Image imageNoSample;
    Gtk::Label labelNoSample;
    Gtk::Image imageMissingSample;
    Gtk::Label labelMissingSample;
    Gtk::Image imageLooped;
    Gtk::Label labelLooped;
    Gtk::Image imageSomeLoops;
    Gtk::Label labelSomeLoops;
    DimRegionEdit dimreg_edit;

    VBox m_left_vbox;
    Gtk::Notebook m_TreeViewNotebook;
    HBox m_searchField;
    Gtk::Label m_searchLabel;
    Gtk::Entry m_searchText;

    struct SampleImportItem {
        gig::Sample*  gig_sample;  // pointer to the gig::Sample to
                                   // which the sample data should be
                                   // imported to
        Glib::ustring sample_path; // file name of the sample to be
                                   // imported
    };
    std::map<gig::Sample*, SampleImportItem> m_SampleImportQueue;


    void on_action_file_new();
    void on_action_file_open();
    void on_action_file_save();
    void on_action_file_save_as();
    void on_action_file_properties();
    void on_action_quit();
    void show_instr_props();
    bool instr_props_set_instrument();
    void show_sample_props();
    bool sample_props_set_sample();
    void sample_name_changed_by_sample_props(Gtk::TreeModel::iterator& it);
    void show_midi_rules();
    void show_script_slots();
    void on_action_view_status_bar();
    void on_auto_restore_win_dim();
    void on_instr_double_click_opens_props();
    void on_save_with_temporary_file();
    void on_action_refresh_all();
    void on_action_warn_user_on_extensions();
    void on_action_show_tooltips();
    void on_show_tooltips_changed();
    void on_action_sync_sampler_instrument_selection();
    void on_action_move_root_note_with_region_moved();
    void on_action_help_about();

    void on_notebook_tab_switched(void* page, guint page_num);

    // sample right-click popup actions
#if GTKMM_MAJOR_VERSION > 3 || (GTKMM_MAJOR_VERSION == 3 && (GTKMM_MINOR_VERSION > 91 || (GTKMM_MINOR_VERSION == 91 && GTKMM_MICRO_VERSION >= 2))) // GTKMM >= 3.91.2
    bool on_sample_treeview_button_release(Gdk::EventButton& button);
#else
    void on_sample_treeview_button_release(GdkEventButton* button);
#endif
    void on_action_sample_properties();
    void on_action_add_group();
    void on_action_add_sample();
    void on_action_replace_sample();
    void on_action_replace_all_samples_in_all_groups();
    void on_action_remove_sample();
    void on_action_remove_unused_samples();

    // script right-click popup actions
#if GTKMM_MAJOR_VERSION > 3 || (GTKMM_MAJOR_VERSION == 3 && (GTKMM_MINOR_VERSION > 91 || (GTKMM_MINOR_VERSION == 91 && GTKMM_MICRO_VERSION >= 2))) // GTKMM >= 3.91.2
    bool on_script_treeview_button_release(Gdk::EventButton& button);
#else
    void on_script_treeview_button_release(GdkEventButton* button);
#endif
    void on_action_add_script_group();
    void on_action_add_script();
    void on_action_edit_script();
    void editScript(gig::Script* script);
    void on_action_remove_script();

    void on_action_add_instrument();
    void on_action_duplicate_instrument();
    void on_action_remove_instrument();

    void show_samples_tab();
    void show_intruments_tab();
    void show_scripts_tab();

    void select_prev_instrument();
    void select_next_instrument();
    void select_instrument_by_dir(int dir);

    void select_prev_region();
    void select_next_region();

    void select_next_dim_rgn_zone();
    void select_prev_dim_rgn_zone();
    void select_add_next_dim_rgn_zone();
    void select_add_prev_dim_rgn_zone();
    void select_prev_dimension();
    void select_next_dimension();

    Serialization::Archive m_serializationArchive; ///< Clipboard content.
    std::vector<Serialization::Archive> m_macros; ///< User configured list of macros.

    void copy_selected_dimrgn();
    void paste_copied_dimrgn();
    void adjust_clipboard_content();
    void updateClipboardCopyAvailable();
    void updateClipboardPasteAvailable();
#if GTKMM_MAJOR_VERSION > 3 || (GTKMM_MAJOR_VERSION == 3 && (GTKMM_MINOR_VERSION > 91 || (GTKMM_MINOR_VERSION == 91 && GTKMM_MICRO_VERSION >= 2))) // GTKMM >= 3.91.2
    void on_clipboard_owner_change(Gdk::EventOwnerChange& event);
#else
    void on_clipboard_owner_change(GdkEventOwnerChange* event);
#endif
    void on_clipboard_get(Gtk::SelectionData& selection_data, guint info);
    void on_clipboard_clear();
    void on_clipboard_received(const Gtk::SelectionData& selection_data);
    void on_clipboard_received_targets(const std::vector<Glib::ustring>& targets);

    void add_instrument(gig::Instrument* instrument);
#if !USE_GTKMM_BUILDER
    Gtk::RadioMenuItem* add_instrument_to_menu(const Glib::ustring& name,
                                               int position = -1);
    void remove_instrument_from_menu(int index);
#endif
    bool onQueryTreeViewTooltip(int x, int y, bool keyboardTip, const Glib::RefPtr<Gtk::Tooltip>& tooltip);

    ProgressDialog* progress_dialog;
    Loader* loader;
    Saver* saver;
    void load_gig(gig::File* gig, const char* filename, bool isSharedInstrument = false);
    void updateSampleRefCountMap(gig::File* gig);

    gig::File* file;
    bool file_is_shared;
    bool file_has_name;
    bool file_is_changed;
    std::string filename;
    std::string current_gig_dir;
    std::string current_sample_dir;

    void set_file_is_shared(bool);

    bool file_save();
    bool file_save_as();
    bool check_if_savable();

#if GTKMM_MAJOR_VERSION > 3 || (GTKMM_MAJOR_VERSION == 3 && (GTKMM_MINOR_VERSION > 91 || (GTKMM_MINOR_VERSION == 91 && GTKMM_MICRO_VERSION >= 2))) // GTKMM >= 3.91.2
    bool on_button_release(Gdk::EventButton& button);
#else
    void on_button_release(GdkEventButton* button);
#endif
    void on_instruments_treeview_drag_begin(const Glib::RefPtr<Gdk::DragContext>& context);
    void on_instruments_treeview_drag_data_get(const Glib::RefPtr<Gdk::DragContext>&,
                                               Gtk::SelectionData& selection_data, guint, guint);
    void on_instruments_treeview_drop_drag_data_received(
        const Glib::RefPtr<Gdk::DragContext>& context, int, int,
        const Gtk::SelectionData& selection_data, guint, guint time
    );
    void on_scripts_treeview_drag_begin(const Glib::RefPtr<Gdk::DragContext>& context);
    void on_scripts_treeview_drag_data_get(const Glib::RefPtr<Gdk::DragContext>&,
                                           Gtk::SelectionData& selection_data, guint, guint);
    void on_sample_treeview_drag_begin(const Glib::RefPtr<Gdk::DragContext>& context);
    void on_sample_treeview_drag_data_get(const Glib::RefPtr<Gdk::DragContext>&,
                                          Gtk::SelectionData& selection_data, guint, guint);
    void on_sample_label_drop_drag_data_received(const Glib::RefPtr<Gdk::DragContext>& context,
                                                 int, int,
                                                 const Gtk::SelectionData& selection_data,
                                                 guint, guint time);

    void script_name_changed(const Gtk::TreeModel::Path& path,
                             const Gtk::TreeModel::iterator& iter);
    void script_double_clicked(const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn* column);
    void sample_name_changed(const Gtk::TreeModel::Path& path,
                             const Gtk::TreeModel::iterator& iter);
    void instrument_name_changed(const Gtk::TreeModel::Path& path,
                                 const Gtk::TreeModel::iterator& iter);
    void instr_name_changed_by_instr_props(Gtk::TreeModel::iterator& it);
    bool instrument_row_visible(const Gtk::TreeModel::const_iterator& iter);
    sigc::connection instrument_name_connection;

    void on_action_combine_instruments();
    void on_action_view_references();
    void on_action_merge_files();
    void mergeFiles(const std::vector<std::string>& filenames);

    void on_sample_ref_changed(gig::Sample* oldSample, gig::Sample* newSample);
    void on_sample_ref_count_incremented(gig::Sample* sample, int offset);
    void on_samples_to_be_removed(std::list<gig::Sample*> samples);

    void add_or_replace_sample(bool replace);

    void __import_queued_samples();
    void __clear();
    void __refreshEntireGUI();
    void updateScriptListOfMenu();
    void assignScript(gig::Script* pScript);
    void dropAllScriptSlots();

    bool close_confirmation_dialog();
    bool leaving_shared_mode_dialog();

    Gtk::Menu* popup_menu;
#if USE_GTKMM_BUILDER
    Gtk::Menu* sample_popup;
    Gtk::Menu* script_popup;
#endif

    bool on_delete_event(GdkEventAny* event);

    bool first_call_to_drag_data_get;

    bool is_copy_samples_unity_note_enabled() const;
    bool is_copy_samples_fine_tune_enabled() const;
    bool is_copy_samples_loop_enabled() const;
};

#endif
