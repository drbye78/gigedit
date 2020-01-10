/*
 * Copyright (C) 2006-2020 Andreas Persson
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

#include <iostream>
#include <cstring>

#include "compat.h"

#include <glibmm/convert.h>
#include <glibmm/dispatcher.h>
#include <glibmm/miscutils.h>
#include <glibmm/stringutils.h>
#include <glibmm/regex.h>
#include <gtkmm/aboutdialog.h>
#include <gtkmm/filechooserdialog.h>
#include <gtkmm/messagedialog.h>
#if HAS_GTKMM_STOCK
# include <gtkmm/stock.h>
#endif
#include <gtkmm/targetentry.h>
#include <gtkmm/main.h>
#if GTKMM_MAJOR_VERSION < 3 || (GTKMM_MAJOR_VERSION == 3 && GTKMM_MINOR_VERSION < 89)
# include <gtkmm/toggleaction.h>
#endif
#include <gtkmm/accelmap.h>
#if GTKMM_MAJOR_VERSION < 3
#include "wrapLabel.hh"
#endif

#include "global.h"
#include "compat.h"

#include <stdio.h>
#ifdef LIBSNDFILE_HEADER_FILE
# include LIBSNDFILE_HEADER_FILE(sndfile.h)
#else
# include <sndfile.h>
#endif
#include <assert.h>

#include "mainwindow.h"
#include "Settings.h"
#include "CombineInstrumentsDialog.h"
#include "scripteditor.h"
#include "scriptslots.h"
#include "ReferencesView.h"
#include "../../gfx/status_attached.xpm"
#include "../../gfx/status_detached.xpm"
#include "gfx/builtinpix.h"
#include "MacroEditor.h"
#include "MacrosSetup.h"
#if defined(__APPLE__)
# include "MacHelper.h"
#endif

static const Gdk::ModifierType primaryModifierKey =
    #if defined(__APPLE__)
    Gdk::META_MASK; // Cmd key on Mac
    #else
    Gdk::CONTROL_MASK; // Ctrl key on all other OSs
    #endif

MainWindow::MainWindow() :
    m_DimRegionChooser(*this),
    dimreg_label(_("Changes apply to:")),
    dimreg_all_regions(_("all regions")),
    dimreg_all_dimregs(_("all dimension splits")),
    dimreg_stereo(_("both channels")),
    labelLegend(_("Legend:")),
    labelNoSample(_(" No Sample")),
    labelMissingSample(_(" Missing some Sample(s)")),
    labelLooped(_(" Looped")),
    labelSomeLoops(_(" Some Loop(s)"))
{
    loadBuiltInPix();

    this->file = NULL;

//    set_border_width(5);

    if (!Settings::singleton()->autoRestoreWindowDimension) {
#if GTKMM_MAJOR_VERSION >= 3
        set_default_size(960, 600);
#else
        set_default_size(865, 600);
#endif
        set_position(Gtk::WIN_POS_CENTER);
    }

    add(m_VBox);

    // Handle selection
    m_TreeView.get_selection()->signal_changed().connect(
        sigc::mem_fun(*this, &MainWindow::on_sel_change));

    // m_TreeView.set_reorderable();

#if GTKMM_MAJOR_VERSION > 3 || (GTKMM_MAJOR_VERSION == 3 && (GTKMM_MINOR_VERSION > 91 || (GTKMM_MINOR_VERSION == 91 && GTKMM_MICRO_VERSION >= 2))) // GTKMM >= 3.91.2
    m_TreeView.signal_button_press_event().connect(
        sigc::mem_fun(*this, &MainWindow::on_button_release));
#else
    m_TreeView.signal_button_press_event().connect_notify(
        sigc::mem_fun(*this, &MainWindow::on_button_release));
#endif

    // Add the TreeView tab, inside a ScrolledWindow, with the button underneath:
    m_ScrolledWindow.add(m_TreeView);
//    m_ScrolledWindow.set_size_request(200, 600);
    m_ScrolledWindow.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);

    m_ScrolledWindowSamples.add(m_TreeViewSamples);
    m_ScrolledWindowSamples.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);

    m_ScrolledWindowScripts.add(m_TreeViewScripts);
    m_ScrolledWindowScripts.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);

#if GTKMM_MAJOR_VERSION < 3
    m_TreeViewNotebook.set_size_request(300);
#endif

    m_searchLabel.set_text(Glib::ustring(" ") + _("Filter:"));
    m_searchField.pack_start(m_searchLabel, Gtk::PACK_SHRINK);
    m_searchField.pack_start(m_searchText);
    m_searchField.set_spacing(5);

    m_left_vbox.pack_start(m_TreeViewNotebook);
    m_left_vbox.pack_start(m_searchField, Gtk::PACK_SHRINK);

    m_HPaned.add1(m_left_vbox);

    dimreg_hbox.add(dimreg_label);
    dimreg_hbox.add(dimreg_all_regions);
    dimreg_hbox.add(dimreg_all_dimregs);
    dimreg_stereo.set_active();
    dimreg_hbox.add(dimreg_stereo);
    dimreg_vbox.add(dimreg_edit);
    dimreg_vbox.pack_start(dimreg_hbox, Gtk::PACK_SHRINK);
    {
        legend_hbox.add(labelLegend);

        imageNoSample.set(redDot);
#if HAS_GTKMM_ALIGNMENT
        imageNoSample.set_alignment(Gtk::ALIGN_END);
        labelNoSample.set_alignment(Gtk::ALIGN_START);
#else
        imageNoSample.set_halign(Gtk::ALIGN_END);
        labelNoSample.set_halign(Gtk::ALIGN_START);
#endif
        legend_hbox.add(imageNoSample);
        legend_hbox.add(labelNoSample);

        imageMissingSample.set(yellowDot);
#if HAS_GTKMM_ALIGNMENT
        imageMissingSample.set_alignment(Gtk::ALIGN_END);
        labelMissingSample.set_alignment(Gtk::ALIGN_START);
#else
        imageMissingSample.set_halign(Gtk::ALIGN_END);
        labelMissingSample.set_halign(Gtk::ALIGN_START);
#endif
        legend_hbox.add(imageMissingSample);
        legend_hbox.add(labelMissingSample);

        imageLooped.set(blackLoop);
#if HAS_GTKMM_ALIGNMENT
        imageLooped.set_alignment(Gtk::ALIGN_END);
        labelLooped.set_alignment(Gtk::ALIGN_START);
#else
        imageLooped.set_halign(Gtk::ALIGN_END);
        labelLooped.set_halign(Gtk::ALIGN_START);
#endif
        legend_hbox.add(imageLooped);
        legend_hbox.add(labelLooped);

        imageSomeLoops.set(grayLoop);
#if HAS_GTKMM_ALIGNMENT
        imageSomeLoops.set_alignment(Gtk::ALIGN_END);
        labelSomeLoops.set_alignment(Gtk::ALIGN_START);
#else
        imageSomeLoops.set_halign(Gtk::ALIGN_END);
        labelSomeLoops.set_halign(Gtk::ALIGN_START);
#endif
        legend_hbox.add(imageSomeLoops);
        legend_hbox.add(labelSomeLoops);

#if HAS_GTKMM_SHOW_ALL_CHILDREN
        legend_hbox.show_all_children();
#endif
    }
    dimreg_vbox.pack_start(legend_hbox, Gtk::PACK_SHRINK);
    m_HPaned.add2(dimreg_vbox);

    dimreg_label.set_tooltip_text(_("To automatically apply your changes above globally to the entire instrument, check all 3 check boxes on the right."));
    dimreg_all_regions.set_tooltip_text(_("If checked: all changes you perform above will automatically be applied to all regions of this instrument as well."));
    dimreg_all_dimregs.set_tooltip_text(_("If checked: all changes you perform above will automatically be applied as well to all dimension splits of the region selected below."));
    dimreg_stereo.set_tooltip_text(_("If checked: all changes you perform above will automatically be applied to both audio channel splits (only if a \"stereo\" dimension is defined below)."));

    m_TreeViewNotebook.append_page(m_ScrolledWindowSamples, _("Samples"));
    m_TreeViewNotebook.append_page(m_ScrolledWindow, _("Instruments"));
    m_TreeViewNotebook.append_page(m_ScrolledWindowScripts, _("Scripts"));

#if USE_GLIB_ACTION
    m_actionGroup = Gio::SimpleActionGroup::create();
    m_actionGroup->add_action(
        "New", sigc::mem_fun(*this, &MainWindow::on_action_file_new)
    );
    m_actionGroup->add_action(
        "Open", sigc::mem_fun(*this, &MainWindow::on_action_file_open)
    );
    m_actionGroup->add_action(
        "Save", sigc::mem_fun(*this, &MainWindow::on_action_file_save)
    );
    m_actionGroup->add_action(
        "SaveAs", sigc::mem_fun(*this, &MainWindow::on_action_file_save_as)
    );
    m_actionGroup->add_action(
        "Properties", sigc::mem_fun(*this, &MainWindow::on_action_file_properties)
    );
    m_actionGroup->add_action(
        "InstrProperties", sigc::mem_fun(*this, &MainWindow::show_instr_props)
    );
    m_actionMIDIRules = m_actionGroup->add_action(
        "MidiRules", sigc::mem_fun(*this, &MainWindow::show_midi_rules)
    );
    m_actionGroup->add_action(
        "ScriptSlots", sigc::mem_fun(*this, &MainWindow::show_script_slots)
    );
    m_actionGroup->add_action(
        "Quit", sigc::mem_fun(*this, &MainWindow::on_action_quit)
    );
    m_actionGroup->add_action(
        "MenuSample", sigc::mem_fun(*this, &MainWindow::show_samples_tab)
    );
    m_actionGroup->add_action(
        "MenuInstrument", sigc::mem_fun(*this, &MainWindow::show_intruments_tab)
    );
    m_actionGroup->add_action(
        "MenuScript", sigc::mem_fun(*this, &MainWindow::show_scripts_tab)
    );
#else
    actionGroup = Gtk::ActionGroup::create();

    actionGroup->add(Gtk::Action::create("MenuFile", _("_File")));
    actionGroup->add(Gtk::Action::create("New", Gtk::Stock::NEW),
                     sigc::mem_fun(
                         *this, &MainWindow::on_action_file_new));
    Glib::RefPtr<Gtk::Action> action =
        Gtk::Action::create("Open", Gtk::Stock::OPEN);
    action->property_label() = action->property_label() + "...";
    actionGroup->add(action,
                     sigc::mem_fun(
                         *this, &MainWindow::on_action_file_open));
    actionGroup->add(Gtk::Action::create("Save", Gtk::Stock::SAVE),
                     sigc::mem_fun(
                         *this, &MainWindow::on_action_file_save));
    action = Gtk::Action::create("SaveAs", Gtk::Stock::SAVE_AS);
    action->property_label() = action->property_label() + "...";
    actionGroup->add(action,
                     Gtk::AccelKey("<shift><control>s"),
                     sigc::mem_fun(
                         *this, &MainWindow::on_action_file_save_as));
    actionGroup->add(Gtk::Action::create("Properties",
                                         Gtk::Stock::PROPERTIES),
                     sigc::mem_fun(
                         *this, &MainWindow::on_action_file_properties));
    actionGroup->add(Gtk::Action::create("InstrProperties",
                                         Gtk::Stock::PROPERTIES),
                     sigc::mem_fun(
                         *this, &MainWindow::show_instr_props));
    actionGroup->add(Gtk::Action::create("MidiRules",
                                         _("_Midi Rules...")),
                     sigc::mem_fun(
                         *this, &MainWindow::show_midi_rules));
    actionGroup->add(Gtk::Action::create("ScriptSlots",
                                         _("_Script Slots...")),
                     sigc::mem_fun(
                         *this, &MainWindow::show_script_slots));
    actionGroup->add(Gtk::Action::create("Quit", Gtk::Stock::QUIT),
                     sigc::mem_fun(
                         *this, &MainWindow::on_action_quit));
    actionGroup->add(
        Gtk::Action::create("MenuSample", _("_Sample")),
        sigc::mem_fun(*this, &MainWindow::show_samples_tab)
    );
    actionGroup->add(
        Gtk::Action::create("MenuInstrument", _("_Instrument")),
        sigc::mem_fun(*this, &MainWindow::show_intruments_tab)
    );
    actionGroup->add(
        Gtk::Action::create("MenuScript", _("Scr_ipt")),
        sigc::mem_fun(*this, &MainWindow::show_scripts_tab)
    );
    actionGroup->add(Gtk::Action::create("AllInstruments", _("_Select")));
    actionGroup->add(Gtk::Action::create("AssignScripts", _("Assign Script")));

    actionGroup->add(Gtk::Action::create("MenuEdit", _("_Edit")));
#endif

    const Gdk::ModifierType primaryModifierKey =
#if defined(__APPLE__)
    Gdk::META_MASK; // Cmd key on Mac
#else
    Gdk::CONTROL_MASK; // Ctrl key on all other OSs
#endif

#if USE_GLIB_ACTION
    m_actionCopyDimRgn = m_actionGroup->add_action(
        "CopyDimRgn", sigc::mem_fun(*this, &MainWindow::copy_selected_dimrgn)
    );
    m_actionPasteDimRgn = m_actionGroup->add_action(
        "PasteDimRgn", sigc::mem_fun(*this, &MainWindow::paste_copied_dimrgn)
    );
    m_actionAdjustClipboard = m_actionGroup->add_action(
        "AdjustClipboard", sigc::mem_fun(*this, &MainWindow::adjust_clipboard_content)
    );
    m_actionGroup->add_action(
        "SelectPrevInstr", sigc::mem_fun(*this, &MainWindow::select_prev_instrument)
    );
    m_actionGroup->add_action(
        "SelectNextInstr", sigc::mem_fun(*this, &MainWindow::select_next_instrument)
    );
    m_actionGroup->add_action(
        "SelectPrevRegion", sigc::mem_fun(*this, &MainWindow::select_prev_region)
    );
    m_actionGroup->add_action(
        "SelectNextRegion", sigc::mem_fun(*this, &MainWindow::select_next_region)
    );
    m_actionGroup->add_action(
        "SelectPrevDimRgnZone", sigc::mem_fun(*this, &MainWindow::select_prev_dim_rgn_zone)
    );
    m_actionGroup->add_action(
        "SelectNextDimRgnZone", sigc::mem_fun(*this, &MainWindow::select_next_dim_rgn_zone)
    );
    m_actionGroup->add_action(
        "SelectPrevDimension", sigc::mem_fun(*this, &MainWindow::select_prev_dimension)
    );
    m_actionGroup->add_action(
        "SelectNextDimension", sigc::mem_fun(*this, &MainWindow::select_next_dimension)
    );
    m_actionGroup->add_action(
        "SelectAddPrevDimRgnZone", sigc::mem_fun(*this, &MainWindow::select_add_prev_dim_rgn_zone)
    );
    m_actionGroup->add_action(
        "SelectAddNextDimRgnZone", sigc::mem_fun(*this, &MainWindow::select_add_next_dim_rgn_zone)
    );
#else
    actionGroup->add(Gtk::Action::create("CopyDimRgn",
                                         _("Copy selected dimension region")),
                     Gtk::AccelKey(GDK_KEY_c, Gdk::MOD1_MASK),
                     sigc::mem_fun(*this, &MainWindow::copy_selected_dimrgn));

    actionGroup->add(Gtk::Action::create("PasteDimRgn",
                                         _("Paste dimension region")),
                     Gtk::AccelKey(GDK_KEY_v, Gdk::MOD1_MASK),
                     sigc::mem_fun(*this, &MainWindow::paste_copied_dimrgn));

    actionGroup->add(Gtk::Action::create("AdjustClipboard",
                                         _("Adjust Clipboard Content")),
                     Gtk::AccelKey(GDK_KEY_x, Gdk::MOD1_MASK),
                     sigc::mem_fun(*this, &MainWindow::adjust_clipboard_content));

    actionGroup->add(Gtk::Action::create("SelectPrevInstr",
                                         _("Select Previous Instrument")),
                     Gtk::AccelKey(GDK_KEY_Up, primaryModifierKey),
                     sigc::mem_fun(*this, &MainWindow::select_prev_instrument));

    actionGroup->add(Gtk::Action::create("SelectNextInstr",
                                         _("Select Next Instrument")),
                     Gtk::AccelKey(GDK_KEY_Down, primaryModifierKey),
                     sigc::mem_fun(*this, &MainWindow::select_next_instrument));

    actionGroup->add(Gtk::Action::create("SelectPrevRegion",
                                         _("Select Previous Region")),
                     Gtk::AccelKey(GDK_KEY_Left, primaryModifierKey),
                     sigc::mem_fun(*this, &MainWindow::select_prev_region));

    actionGroup->add(Gtk::Action::create("SelectNextRegion",
                                         _("Select Next Region")),
                     Gtk::AccelKey(GDK_KEY_Right, primaryModifierKey),
                     sigc::mem_fun(*this, &MainWindow::select_next_region));

    actionGroup->add(Gtk::Action::create("SelectPrevDimRgnZone",
                                         _("Select Previous Dimension Region Zone")),
                     Gtk::AccelKey(GDK_KEY_Left, Gdk::MOD1_MASK),
                     sigc::mem_fun(*this, &MainWindow::select_prev_dim_rgn_zone));

    actionGroup->add(Gtk::Action::create("SelectNextDimRgnZone",
                                         _("Select Next Dimension Region Zone")),
                     Gtk::AccelKey(GDK_KEY_Right, Gdk::MOD1_MASK),
                     sigc::mem_fun(*this, &MainWindow::select_next_dim_rgn_zone));

    actionGroup->add(Gtk::Action::create("SelectPrevDimension",
                                         _("Select Previous Dimension")),
                     Gtk::AccelKey(GDK_KEY_Up, Gdk::MOD1_MASK),
                     sigc::mem_fun(*this, &MainWindow::select_prev_dimension));

    actionGroup->add(Gtk::Action::create("SelectNextDimension",
                                         _("Select Next Dimension")),
                     Gtk::AccelKey(GDK_KEY_Down, Gdk::MOD1_MASK),
                     sigc::mem_fun(*this, &MainWindow::select_next_dimension));

    actionGroup->add(Gtk::Action::create("SelectAddPrevDimRgnZone",
                                         _("Add Previous Dimension Region Zone to Selection")),
                     Gtk::AccelKey(GDK_KEY_Left, Gdk::MOD1_MASK | Gdk::SHIFT_MASK),
                     sigc::mem_fun(*this, &MainWindow::select_add_prev_dim_rgn_zone));

    actionGroup->add(Gtk::Action::create("SelectAddNextDimRgnZone",
                                         _("Add Next Dimension Region Zone to Selection")),
                     Gtk::AccelKey(GDK_KEY_Right, Gdk::MOD1_MASK | Gdk::SHIFT_MASK),
                     sigc::mem_fun(*this, &MainWindow::select_add_next_dim_rgn_zone));
#endif

#if USE_GLIB_ACTION
    m_actionToggleCopySampleUnity = m_actionGroup->add_action_bool("CopySampleUnity", true);
    m_actionToggleCopySampleTune  = m_actionGroup->add_action_bool("CopySampleTune", true);
    m_actionToggleCopySampleLoop  = m_actionGroup->add_action_bool("CopySampleLoop", true);
#else
    Glib::RefPtr<Gtk::ToggleAction> toggle_action =
        Gtk::ToggleAction::create("CopySampleUnity", _("Copy Sample's _Unity Note"));
    toggle_action->set_active(true);
    actionGroup->add(toggle_action);

    toggle_action =
        Gtk::ToggleAction::create("CopySampleTune", _("Copy Sample's _Fine Tune"));
    toggle_action->set_active(true);
    actionGroup->add(toggle_action);

    toggle_action =
        Gtk::ToggleAction::create("CopySampleLoop", _("Copy Sample's _Loop Points"));
    toggle_action->set_active(true);
    actionGroup->add(toggle_action);
#endif

#if USE_GLIB_ACTION
    m_actionToggleStatusBar =
        m_actionGroup->add_action_bool("Statusbar", sigc::mem_fun(*this, &MainWindow::on_action_view_status_bar), true);
    m_actionToggleRestoreWinDim =
        m_actionGroup->add_action_bool("AutoRestoreWinDim", sigc::mem_fun(*this, &MainWindow::on_auto_restore_win_dim), Settings::singleton()->autoRestoreWindowDimension);
    m_actionInstrDoubleClickOpensProps =
        m_actionGroup->add_action_bool(
            "OpenInstrPropsByDoubleClick",
            sigc::mem_fun(*this, &MainWindow::on_instr_double_click_opens_props),
            Settings::singleton()->instrumentDoubleClickOpensProps
        );
    m_actionToggleShowTooltips = m_actionGroup->add_action_bool(
        "ShowTooltips", sigc::mem_fun(*this, &MainWindow::on_action_show_tooltips),
        Settings::singleton()->showTooltips
    );
    m_actionToggleSaveWithTempFile =
        m_actionGroup->add_action_bool("SaveWithTemporaryFile", sigc::mem_fun(*this, &MainWindow::on_save_with_temporary_file), Settings::singleton()->saveWithTemporaryFile);
    m_actionGroup->add_action("RefreshAll", sigc::mem_fun(*this, &MainWindow::on_action_refresh_all));
#else
    actionGroup->add(Gtk::Action::create("MenuMacro", _("_Macro")));


    actionGroup->add(Gtk::Action::create("MenuView", _("Vie_w")));
    toggle_action =
        Gtk::ToggleAction::create("Statusbar", _("_Statusbar"));
    toggle_action->set_active(true);
    actionGroup->add(toggle_action,
                     sigc::mem_fun(
                         *this, &MainWindow::on_action_view_status_bar));

    toggle_action =
        Gtk::ToggleAction::create("AutoRestoreWinDim", _("_Auto Restore Window Dimension"));
    toggle_action->set_active(Settings::singleton()->autoRestoreWindowDimension);
    actionGroup->add(toggle_action,
                     sigc::mem_fun(
                         *this, &MainWindow::on_auto_restore_win_dim));

    toggle_action =
        Gtk::ToggleAction::create("OpenInstrPropsByDoubleClick", _("Instrument Properties by Double Click"));
    toggle_action->set_active(Settings::singleton()->instrumentDoubleClickOpensProps);
    actionGroup->add(toggle_action,
                     sigc::mem_fun(
                         *this, &MainWindow::on_instr_double_click_opens_props));

    toggle_action =
        Gtk::ToggleAction::create("ShowTooltips", _("Tooltips for Beginners"));
    toggle_action->set_active(Settings::singleton()->showTooltips);
    actionGroup->add(
        toggle_action,
        sigc::mem_fun(*this, &MainWindow::on_action_show_tooltips)
    );

    toggle_action =
        Gtk::ToggleAction::create("SaveWithTemporaryFile", _("Save with _temporary file"));
    toggle_action->set_active(Settings::singleton()->saveWithTemporaryFile);
    actionGroup->add(toggle_action,
                     sigc::mem_fun(
                         *this, &MainWindow::on_save_with_temporary_file));

    actionGroup->add(
        Gtk::Action::create("RefreshAll", _("_Refresh All")),
        sigc::mem_fun(*this, &MainWindow::on_action_refresh_all)
    );                 
#endif

#if USE_GLIB_ACTION
    m_actionGroup->add_action(
        "About", sigc::mem_fun(*this, &MainWindow::on_action_help_about)
    );
    m_actionGroup->add_action(
        "AddInstrument", sigc::mem_fun(*this, &MainWindow::on_action_add_instrument)
    );
    m_actionGroup->add_action(
        "DupInstrument", sigc::mem_fun(*this, &MainWindow::on_action_duplicate_instrument)
    );
    m_actionGroup->add_action(
        "MoveInstrument", sigc::mem_fun(*this, &MainWindow::on_action_move_instr)
    );
    m_actionGroup->add_action(
        "CombInstruments", sigc::mem_fun(*this, &MainWindow::on_action_combine_instruments)
    );
    m_actionGroup->add_action(
        "RemoveInstrument", sigc::mem_fun(*this, &MainWindow::on_action_remove_instrument)
    );
#else
    action = Gtk::Action::create("MenuHelp", Gtk::Stock::HELP);
    actionGroup->add(Gtk::Action::create("MenuHelp",
                                         action->property_label()));
    actionGroup->add(Gtk::Action::create("About", Gtk::Stock::ABOUT),
                     sigc::mem_fun(
                         *this, &MainWindow::on_action_help_about));
    actionGroup->add(
        Gtk::Action::create("AddInstrument", _("Add _Instrument")),
        sigc::mem_fun(*this, &MainWindow::on_action_add_instrument)
    );
    actionGroup->add(
        Gtk::Action::create("DupInstrument", _("_Duplicate Instrument")),
        sigc::mem_fun(*this, &MainWindow::on_action_duplicate_instrument)
    );
    actionGroup->add(
        Gtk::Action::create("MoveInstrument", _("Move _Instrument To ...")),
        Gtk::AccelKey(GDK_KEY_i, primaryModifierKey),
        sigc::mem_fun(*this, &MainWindow::on_action_move_instr)
    );
    actionGroup->add(
        Gtk::Action::create("CombInstruments", _("_Combine Instruments ...")),
        Gtk::AccelKey(GDK_KEY_j, primaryModifierKey),
        sigc::mem_fun(*this, &MainWindow::on_action_combine_instruments)
    );
    actionGroup->add(
        Gtk::Action::create("RemoveInstrument", Gtk::Stock::REMOVE),
        sigc::mem_fun(*this, &MainWindow::on_action_remove_instrument)
    );
#endif

#if USE_GLIB_ACTION
    m_actionToggleWarnOnExtensions = m_actionGroup->add_action_bool(
        "WarnUserOnExtensions", sigc::mem_fun(*this, &MainWindow::on_action_warn_user_on_extensions),
        Settings::singleton()->warnUserOnExtensions
    );
    m_actionToggleSyncSamplerSelection = m_actionGroup->add_action_bool(
        "SyncSamplerInstrumentSelection", sigc::mem_fun(*this, &MainWindow::on_action_sync_sampler_instrument_selection),
        Settings::singleton()->syncSamplerInstrumentSelection
    );
    m_actionToggleMoveRootNoteWithRegion = m_actionGroup->add_action_bool(
        "MoveRootNoteWithRegionMoved", sigc::mem_fun(*this, &MainWindow::on_action_move_root_note_with_region_moved),
        Settings::singleton()->moveRootNoteWithRegionMoved
    );
#else
    actionGroup->add(Gtk::Action::create("MenuSettings", _("_Settings")));
    
    toggle_action =
        Gtk::ToggleAction::create("WarnUserOnExtensions", _("Show warning on format _extensions"));
    toggle_action->set_active(Settings::singleton()->warnUserOnExtensions);
    actionGroup->add(
        toggle_action,
        sigc::mem_fun(*this, &MainWindow::on_action_warn_user_on_extensions)
    );

    toggle_action =
        Gtk::ToggleAction::create("SyncSamplerInstrumentSelection", _("Synchronize sampler's instrument selection"));
    toggle_action->set_active(Settings::singleton()->syncSamplerInstrumentSelection);
    actionGroup->add(
        toggle_action,
        sigc::mem_fun(*this, &MainWindow::on_action_sync_sampler_instrument_selection)
    );

    toggle_action =
        Gtk::ToggleAction::create("MoveRootNoteWithRegionMoved", _("Move root note with region moved"));
    toggle_action->set_active(Settings::singleton()->moveRootNoteWithRegionMoved);
    actionGroup->add(
        toggle_action,
        sigc::mem_fun(*this, &MainWindow::on_action_move_root_note_with_region_moved)
    );
#endif

#if USE_GLIB_ACTION
    m_actionGroup->add_action(
        "CombineInstruments", sigc::mem_fun(*this, &MainWindow::on_action_combine_instruments)
    );
    m_actionGroup->add_action(
        "MergeFiles", sigc::mem_fun(*this, &MainWindow::on_action_merge_files)
    );
#else
    actionGroup->add(Gtk::Action::create("MenuTools", _("_Tools")));

    actionGroup->add(
        Gtk::Action::create("CombineInstruments", _("_Combine Instruments...")),
        sigc::mem_fun(*this, &MainWindow::on_action_combine_instruments)
    );

    actionGroup->add(
        Gtk::Action::create("MergeFiles", _("_Merge Files...")),
        sigc::mem_fun(*this, &MainWindow::on_action_merge_files)
    );
#endif

    // sample right-click popup actions
#if USE_GLIB_ACTION
    m_actionSampleProperties = m_actionGroup->add_action(
        "SampleProperties", sigc::mem_fun(*this, &MainWindow::on_action_sample_properties)
    );
    m_actionAddSampleGroup = m_actionGroup->add_action(
        "AddGroup", sigc::mem_fun(*this, &MainWindow::on_action_add_group)
    );
    m_actionAddSample = m_actionGroup->add_action(
        "AddSample", sigc::mem_fun(*this, &MainWindow::on_action_add_sample)
    );
    m_actionRemoveSample = m_actionGroup->add_action(
        "RemoveSample", sigc::mem_fun(*this, &MainWindow::on_action_remove_sample)
    );
    m_actionGroup->add_action(
        "RemoveUnusedSamples", sigc::mem_fun(*this, &MainWindow::on_action_remove_unused_samples)
    );
    m_actionViewSampleRefs = m_actionGroup->add_action(
        "ShowSampleRefs", sigc::mem_fun(*this, &MainWindow::on_action_view_references)
    );
    m_actionReplaceSample = m_actionGroup->add_action(
        "ReplaceSample", sigc::mem_fun(*this, &MainWindow::on_action_replace_sample)
    );
    m_actionGroup->add_action(
        "ReplaceAllSamplesInAllGroups", sigc::mem_fun(*this, &MainWindow::on_action_replace_all_samples_in_all_groups)
    );
#else
    actionGroup->add(
        Gtk::Action::create("SampleProperties", Gtk::Stock::PROPERTIES),
        sigc::mem_fun(*this, &MainWindow::on_action_sample_properties)
    );
    actionGroup->add(
        Gtk::Action::create("AddGroup", _("Add _Group")),
        sigc::mem_fun(*this, &MainWindow::on_action_add_group)
    );
    actionGroup->add(
        Gtk::Action::create("AddSample", _("Add _Sample(s)...")),
        sigc::mem_fun(*this, &MainWindow::on_action_add_sample)
    );
    actionGroup->add(
        Gtk::Action::create("RemoveSample", Gtk::Stock::REMOVE),
        sigc::mem_fun(*this, &MainWindow::on_action_remove_sample)
    );
    actionGroup->add(
        Gtk::Action::create("RemoveUnusedSamples", _("Remove _Unused Samples")),
        sigc::mem_fun(*this, &MainWindow::on_action_remove_unused_samples)
    );
    actionGroup->add(
        Gtk::Action::create("ShowSampleRefs", _("Show References...")),
        sigc::mem_fun(*this, &MainWindow::on_action_view_references)
    );
    actionGroup->add(
        Gtk::Action::create("ReplaceSample",
                            _("Replace Sample...")),
        sigc::mem_fun(*this, &MainWindow::on_action_replace_sample)
    );
    actionGroup->add(
        Gtk::Action::create("ReplaceAllSamplesInAllGroups",
                            _("Replace All Samples in All Groups...")),
        sigc::mem_fun(*this, &MainWindow::on_action_replace_all_samples_in_all_groups)
    );
#endif
    
    // script right-click popup actions
#if USE_GLIB_ACTION
    m_actionAddScriptGroup = m_actionGroup->add_action(
        "AddScriptGroup", sigc::mem_fun(*this, &MainWindow::on_action_add_script_group)
    );
    m_actionAddScript = m_actionGroup->add_action(
        "AddScript", sigc::mem_fun(*this, &MainWindow::on_action_add_script)
    );
    m_actionEditScript = m_actionGroup->add_action(
        "EditScript", sigc::mem_fun(*this, &MainWindow::on_action_edit_script)
    );
    m_actionRemoveScript = m_actionGroup->add_action(
        "RemoveScript", sigc::mem_fun(*this, &MainWindow::on_action_remove_script)
    );
#else
    actionGroup->add(
        Gtk::Action::create("AddScriptGroup", _("Add _Group")),
        sigc::mem_fun(*this, &MainWindow::on_action_add_script_group)
    );
    actionGroup->add(
        Gtk::Action::create("AddScript", _("Add _Script")),
        sigc::mem_fun(*this, &MainWindow::on_action_add_script)
    );
    actionGroup->add(
        Gtk::Action::create("EditScript", _("_Edit Script...")),
        sigc::mem_fun(*this, &MainWindow::on_action_edit_script)
    );
    actionGroup->add(
        Gtk::Action::create("RemoveScript", Gtk::Stock::REMOVE),
        sigc::mem_fun(*this, &MainWindow::on_action_remove_script)
    );
#endif

#if USE_GTKMM_BUILDER
    insert_action_group("AppMenu", m_actionGroup);
    
    m_uiManager = Gtk::Builder::create();
    Glib::ustring ui_info =
        "<interface>"
        "  <menubar id='MenuBar'>"
        "    <menu id='MenuFile'>"
        "      <attribute name='label' translatable='yes'>_File</attribute>"
        "      <section>"
        "        <item id='New'>"
        "          <attribute name='label' translatable='yes'>New</attribute>"
        "          <attribute name='action'>AppMenu.New</attribute>"
        "        </item>"
        "        <item id='Open'>"
        "          <attribute name='label' translatable='yes'>Open</attribute>"
        "          <attribute name='action'>AppMenu.Open</attribute>"
        "        </item>"
        "      </section>"
        "      <section>"
        "        <item id='Save'>"
        "          <attribute name='label' translatable='yes'>Save</attribute>"
        "          <attribute name='action'>AppMenu.Save</attribute>"
        "        </item>"
        "        <item id='SaveAs'>"
        "          <attribute name='label' translatable='yes'>Save As</attribute>"
        "          <attribute name='action'>AppMenu.SaveAs</attribute>"
        "        </item>"
        "      </section>"
        "      <section>"
        "        <item id='Properties'>"
        "          <attribute name='label' translatable='yes'>Properties</attribute>"
        "          <attribute name='action'>AppMenu.Properties</attribute>"
        "        </item>"
        "      </section>"
        "      <section>"
        "        <item id='Quit'>"
        "          <attribute name='label' translatable='yes'>Quit</attribute>"
        "          <attribute name='action'>AppMenu.Quit</attribute>"
        "        </item>"
        "      </section>"
        "    </menu>"
        "    <menu id='MenuEdit'>"
        "      <attribute name='label' translatable='yes'>Edit</attribute>"
        "      <section>"
        "        <item id='CopyDimRgn'>"
        "          <attribute name='label' translatable='yes'>Copy Dimension Region</attribute>"
        "          <attribute name='action'>AppMenu.CopyDimRgn</attribute>"
        "        </item>"
        "        <item id='AdjustClipboard'>"
        "          <attribute name='label' translatable='yes'>Adjust Clipboard</attribute>"
        "          <attribute name='action'>AppMenu.AdjustClipboard</attribute>"
        "        </item>"
        "        <item id='PasteDimRgn'>"
        "          <attribute name='label' translatable='yes'>Paste Dimension Region</attribute>"
        "          <attribute name='action'>AppMenu.PasteDimRgn</attribute>"
        "        </item>"
        "      </section>"
        "        <item id='SelectPrevInstr'>"
        "          <attribute name='label' translatable='yes'>Previous Instrument</attribute>"
        "          <attribute name='action'>AppMenu.SelectPrevInstr</attribute>"
        "        </item>"
        "        <item id='SelectNextInstr'>"
        "          <attribute name='label' translatable='yes'>Next Instrument</attribute>"
        "          <attribute name='action'>AppMenu.SelectNextInstr</attribute>"
        "        </item>"
        "      <section>"
        "        <item id='SelectPrevRegion'>"
        "          <attribute name='label' translatable='yes'>Previous Region</attribute>"
        "          <attribute name='action'>AppMenu.SelectPrevRegion</attribute>"
        "        </item>"
        "        <item id='SelectNextRegion'>"
        "          <attribute name='label' translatable='yes'>Next Region</attribute>"
        "          <attribute name='action'>AppMenu.SelectNextRegion</attribute>"
        "        </item>"
        "      </section>"
        "        <item id='SelectPrevDimension'>"
        "          <attribute name='label' translatable='yes'>Previous Dimension</attribute>"
        "          <attribute name='action'>AppMenu.SelectPrevDimension</attribute>"
        "        </item>"
        "        <item id='SelectNextDimension'>"
        "          <attribute name='label' translatable='yes'>Next Dimension</attribute>"
        "          <attribute name='action'>AppMenu.SelectNextDimension</attribute>"
        "        </item>"
        "        <item id='SelectPrevDimRgnZone'>"
        "          <attribute name='label' translatable='yes'>Previous Dimension Region Zone</attribute>"
        "          <attribute name='action'>AppMenu.SelectPrevDimRgnZone</attribute>"
        "        </item>"
        "        <item id='SelectNextDimRgnZone'>"
        "          <attribute name='label' translatable='yes'>Next Dimension Region Zone</attribute>"
        "          <attribute name='action'>AppMenu.SelectNextDimRgnZone</attribute>"
        "        </item>"
        "        <item id='SelectAddPrevDimRgnZone'>"
        "          <attribute name='label' translatable='yes'>Add Previous Dimension Region Zone</attribute>"
        "          <attribute name='action'>AppMenu.SelectAddPrevDimRgnZone</attribute>"
        "        </item>"
        "        <item id='SelectAddNextDimRgnZone'>"
        "          <attribute name='label' translatable='yes'>Add Next Dimension Region Zone</attribute>"
        "          <attribute name='action'>AppMenu.SelectAddNextDimRgnZone</attribute>"
        "        </item>"
        "      <section>"
        "        <item id='CopySampleUnity'>"
        "          <attribute name='label' translatable='yes'>Copy Sample Unity</attribute>"
        "          <attribute name='action'>AppMenu.CopySampleUnity</attribute>"
        "        </item>"
        "        <item id='CopySampleTune'>"
        "          <attribute name='label' translatable='yes'>Copy Sample Tune</attribute>"
        "          <attribute name='action'>AppMenu.CopySampleTune</attribute>"
        "        </item>"
        "        <item id='CopySampleLoop'>"
        "          <attribute name='label' translatable='yes'>Copy Sample Loop</attribute>"
        "          <attribute name='action'>AppMenu.CopySampleLoop</attribute>"
        "        </item>"
        "      </section>"
        "    </menu>"
        "    <menu id='MenuMacro'>"
        "      <attribute name='label' translatable='yes'>Macro</attribute>"
        "      <section>"
        "      </section>"
        "    </menu>"
        "    <menu id='MenuSample'>"
        "      <attribute name='label' translatable='yes'>Sample</attribute>"
        "      <section>"
        "        <item id='SampleProperties'>"
        "          <attribute name='label' translatable='yes'>Properties</attribute>"
        "          <attribute name='action'>AppMenu.SampleProperties</attribute>"
        "        </item>"
        "        <item id='AddGroup'>"
        "          <attribute name='label' translatable='yes'>Add Group</attribute>"
        "          <attribute name='action'>AppMenu.AddGroup</attribute>"
        "        </item>"
        "        <item id='AddSample'>"
        "          <attribute name='label' translatable='yes'>Add Sample</attribute>"
        "          <attribute name='action'>AppMenu.AddSample</attribute>"
        "        </item>"
        "        <item id='ShowSampleRefs'>"
        "          <attribute name='label' translatable='yes'>Show Sample References</attribute>"
        "          <attribute name='action'>AppMenu.ShowSampleRefs</attribute>"
        "        </item>"
        "        <item id='ReplaceSample'>"
        "          <attribute name='label' translatable='yes'>Replace Sample</attribute>"
        "          <attribute name='action'>AppMenu.ReplaceSample</attribute>"
        "        </item>"
        "        <item id='ReplaceAllSamplesInAllGroups'>"
        "          <attribute name='label' translatable='yes'>Replace all Samples in all Groups</attribute>"
        "          <attribute name='action'>AppMenu.ReplaceAllSamplesInAllGroups</attribute>"
        "        </item>"
        "      </section>"
        "      <section>"
        "        <item id='RemoveSample'>"
        "          <attribute name='label' translatable='yes'>Remove Sample</attribute>"
        "          <attribute name='action'>AppMenu.RemoveSample</attribute>"
        "        </item>"
        "        <item id='RemoveUnusedSamples'>"
        "          <attribute name='label' translatable='yes'>Remove unused Samples</attribute>"
        "          <attribute name='action'>AppMenu.RemoveUnusedSamples</attribute>"
        "        </item>"
        "      </section>"
        "    </menu>"
        "    <menu id='MenuInstrument'>"
        "      <attribute name='label' translatable='yes'>Instrument</attribute>"
        "      <section>"
        "        <item id='InstrProperties'>"
        "          <attribute name='label' translatable='yes'>Properties</attribute>"
        "          <attribute name='action'>AppMenu.InstrProperties</attribute>"
        "        </item>"
        "        <item id='MidiRules'>"
        "          <attribute name='label' translatable='yes'>MIDI Rules</attribute>"
        "          <attribute name='action'>AppMenu.MidiRules</attribute>"
        "        </item>"
        "        <item id='ScriptSlots'>"
        "          <attribute name='label' translatable='yes'>Script Slots</attribute>"
        "          <attribute name='action'>AppMenu.ScriptSlots</attribute>"
        "        </item>"
        "      </section>"
        "      <submenu id='AssignScripts'>"
        "        <attribute name='label' translatable='yes'>Assign Scripts</attribute>"
        "      </submenu>"
        "      <section>"
        "        <item id='AddInstrument'>"
        "          <attribute name='label' translatable='yes'>Add Instrument</attribute>"
        "          <attribute name='action'>AppMenu.AddInstrument</attribute>"
        "        </item>"
        "        <item id='DupInstrument'>"
        "          <attribute name='label' translatable='yes'>Duplicate Instrument</attribute>"
        "          <attribute name='action'>AppMenu.DupInstrument</attribute>"
        "        </item>"
        "        <item id='MoveInstrument'>"
        "          <attribute name='label' translatable='yes'>Move Instrument To ...</attribute>"
        "          <attribute name='action'>AppMenu.MoveInstrument</attribute>"
        "        </item>"
        "        <item id='CombInstruments'>"
        "          <attribute name='label' translatable='yes'>Combine Instrument</attribute>"
        "          <attribute name='action'>AppMenu.CombInstruments</attribute>"
        "        </item>"
        "      </section>"
        "      <section>"
        "        <item id='RemoveInstrument'>"
        "          <attribute name='label' translatable='yes'>Remove Instrument</attribute>"
        "          <attribute name='action'>AppMenu.RemoveInstrument</attribute>"
        "        </item>"
        "      </section>"
        "    </menu>"
        "    <menu id='MenuScript'>"
        "      <attribute name='label' translatable='yes'>Script</attribute>"
        "      <section>"
        "        <item id='AddScriptGroup'>"
        "          <attribute name='label' translatable='yes'>Add Script Group</attribute>"
        "          <attribute name='action'>AppMenu.AddScriptGroup</attribute>"
        "        </item>"
        "        <item id='AddScript'>"
        "          <attribute name='label' translatable='yes'>Add Script</attribute>"
        "          <attribute name='action'>AppMenu.AddScript</attribute>"
        "        </item>"
        "        <item id='EditScript'>"
        "          <attribute name='label' translatable='yes'>Edit Script</attribute>"
        "          <attribute name='action'>AppMenu.EditScript</attribute>"
        "        </item>"
        "      </section>"
        "      <section>"
        "        <item id='RemoveScript'>"
        "          <attribute name='label' translatable='yes'>Remove Script</attribute>"
        "          <attribute name='action'>AppMenu.RemoveScript</attribute>"
        "        </item>"
        "      </section>"
        "    </menu>"
        "    <menu id='MenuView'>"
        "      <attribute name='label' translatable='yes'>View</attribute>"
        "      <section>"
        "        <item id='Statusbar'>"
        "          <attribute name='label' translatable='yes'>Statusbar</attribute>"
        "          <attribute name='action'>AppMenu.Statusbar</attribute>"
        "        </item>"
        "        <item id='ShowTooltips'>"
        "          <attribute name='label' translatable='yes'>Tooltips for Beginners</attribute>"
        "          <attribute name='action'>AppMenu.ShowTooltips</attribute>"
        "        </item>"
        "        <item id='AutoRestoreWinDim'>"
        "          <attribute name='label' translatable='yes'>Auto restore Window Dimensions</attribute>"
        "          <attribute name='action'>AppMenu.AutoRestoreWinDim</attribute>"
        "        </item>"
        "        <item id='OpenInstrPropsByDoubleClick'>"
        "          <attribute name='label' translatable='yes'>Instrument Properties by Double Click</attribute>"
        "          <attribute name='action'>AppMenu.OpenInstrPropsByDoubleClick</attribute>"
        "        </item>"
        "      </section>"
        "      <section>"
        "        <item id='RefreshAll'>"
        "          <attribute name='label' translatable='yes'>Refresh All</attribute>"
        "          <attribute name='action'>AppMenu.RefreshAll</attribute>"
        "        </item>"
        "      </section>"
        "    </menu>"
        "    <menu id='MenuTools'>"
        "      <attribute name='label' translatable='yes'>Tools</attribute>"
        "      <section>"
        "        <item id='CombineInstruments'>"
        "          <attribute name='label' translatable='yes'>Combine Instruments ...</attribute>"
        "          <attribute name='action'>AppMenu.CombineInstruments</attribute>"
        "        </item>"
        "        <item id='MergeFiles'>"
        "          <attribute name='label' translatable='yes'>Merge Files ...</attribute>"
        "          <attribute name='action'>AppMenu.MergeFiles</attribute>"
        "        </item>"
        "      </section>"
        "    </menu>"
        "    <menu id='MenuSettings'>"
        "      <attribute name='label' translatable='yes'>Settings</attribute>"
        "      <section>"
        "        <item id='WarnUserOnExtensions'>"
        "          <attribute name='label' translatable='yes'>Warning on Format Extensions</attribute>"
        "          <attribute name='action'>AppMenu.WarnUserOnExtensions</attribute>"
        "        </item>"
        "        <item id='SyncSamplerInstrumentSelection'>"
        "          <attribute name='label' translatable='yes'>Synchronize Sampler Selection</attribute>"
        "          <attribute name='action'>AppMenu.SyncSamplerInstrumentSelection</attribute>"
        "        </item>"
        "        <item id='MoveRootNoteWithRegionMoved'>"
        "          <attribute name='label' translatable='yes'>Move Root Note with Region moved</attribute>"
        "          <attribute name='action'>AppMenu.MoveRootNoteWithRegionMoved</attribute>"
        "        </item>"
        "        <item id='SaveWithTemporaryFile'>"
        "          <attribute name='label' translatable='yes'>Save with temporary file</attribute>"
        "          <attribute name='action'>AppMenu.SaveWithTemporaryFile</attribute>"
        "        </item>"
        "      </section>"
        "    </menu>"
        "    <menu id='MenuHelp'>"
        "      <attribute name='label' translatable='yes'>Help</attribute>"
        "      <section>"
        "        <item id='About'>"
        "          <attribute name='label' translatable='yes'>About ...</attribute>"
        "          <attribute name='action'>AppMenu.About</attribute>"
        "        </item>"
        "      </section>"
        "    </menu>"
        "  </menubar>"
        // popups
        "  <menu id='PopupMenu'>"
        "    <section>"
        "      <item id='InstrProperties'>"
        "        <attribute name='label' translatable='yes'>Instrument Properties</attribute>"
        "        <attribute name='action'>AppMenu.InstrProperties</attribute>"
        "      </item>"
        "      <item id='MidiRules'>"
        "        <attribute name='label' translatable='yes'>MIDI Rules</attribute>"
        "        <attribute name='action'>AppMenu.MidiRules</attribute>"
        "      </item>"
        "      <item id='ScriptSlots'>"
        "        <attribute name='label' translatable='yes'>Script Slots</attribute>"
        "        <attribute name='action'>AppMenu.ScriptSlots</attribute>"
        "      </item>"
        "      <item id='AddInstrument'>"
        "        <attribute name='label' translatable='yes'>Add Instrument</attribute>"
        "        <attribute name='action'>AppMenu.AddInstrument</attribute>"
        "      </item>"
        "      <item id='DupInstrument'>"
        "        <attribute name='label' translatable='yes'>Duplicate Instrument</attribute>"
        "        <attribute name='action'>AppMenu.DupInstrument</attribute>"
        "      </item>"
        "      <item id='MoveInstrument'>"
        "        <attribute name='label' translatable='yes'>Move Instrument To ...</attribute>"
        "        <attribute name='action'>AppMenu.MoveInstrument</attribute>"
        "      </item>"
        "      <item id='CombInstruments'>"
        "        <attribute name='label' translatable='yes'>Combine Instruments</attribute>"
        "        <attribute name='action'>AppMenu.CombInstruments</attribute>"
        "      </item>"
        "    </section>"
        "    <section>"
        "      <item id='RemoveInstrument'>"
        "        <attribute name='label' translatable='yes'>Remove Instruments</attribute>"
        "        <attribute name='action'>AppMenu.RemoveInstrument</attribute>"
        "      </item>"
        "    </section>"
        "  </menu>"
        "  <menu id='SamplePopupMenu'>"
        "    <section>"
        "      <item id='SampleProperties'>"
        "        <attribute name='label' translatable='yes'>Sample Properties</attribute>"
        "        <attribute name='action'>AppMenu.SampleProperties</attribute>"
        "      </item>"
        "      <item id='AddGroup'>"
        "        <attribute name='label' translatable='yes'>Add Sample Group</attribute>"
        "        <attribute name='action'>AppMenu.AddGroup</attribute>"
        "      </item>"
        "      <item id='AddSample'>"
        "        <attribute name='label' translatable='yes'>Add Sample</attribute>"
        "        <attribute name='action'>AppMenu.AddSample</attribute>"
        "      </item>"
        "      <item id='ShowSampleRefs'>"
        "        <attribute name='label' translatable='yes'>Show Sample References ...</attribute>"
        "        <attribute name='action'>AppMenu.ShowSampleRefs</attribute>"
        "      </item>"
        "      <item id='ReplaceSample'>"
        "        <attribute name='label' translatable='yes'>Replace Sample</attribute>"
        "        <attribute name='action'>AppMenu.ReplaceSample</attribute>"
        "      </item>"
        "      <item id='ReplaceAllSamplesInAllGroups'>"
        "        <attribute name='label' translatable='yes'>Replace all Samples ...</attribute>"
        "        <attribute name='action'>AppMenu.ReplaceAllSamplesInAllGroups</attribute>"
        "      </item>"
        "    </section>"
        "    <section>"
        "      <item id='RemoveSample'>"
        "        <attribute name='label' translatable='yes'>Remove Sample</attribute>"
        "        <attribute name='action'>AppMenu.RemoveSample</attribute>"
        "      </item>"
        "      <item id='RemoveUnusedSamples'>"
        "        <attribute name='label' translatable='yes'>Remove unused Samples</attribute>"
        "        <attribute name='action'>AppMenu.RemoveUnusedSamples</attribute>"
        "      </item>"
        "    </section>"
        "  </menu>"
        "  <menu id='ScriptPopupMenu'>"
        "    <section>"
        "      <item id='AddScriptGroup'>"
        "        <attribute name='label' translatable='yes'>Add Script Group</attribute>"
        "        <attribute name='action'>AppMenu.AddScriptGroup</attribute>"
        "      </item>"
        "      <item id='AddScript'>"
        "        <attribute name='label' translatable='yes'>Add Script</attribute>"
        "        <attribute name='action'>AppMenu.AddScript</attribute>"
        "      </item>"
        "      <item id='EditScript'>"
        "        <attribute name='label' translatable='yes'>Edit Script</attribute>"
        "        <attribute name='action'>AppMenu.EditScript</attribute>"
        "      </item>"
        "    </section>"
        "    <section>"
        "      <item id='RemoveScript'>"
        "        <attribute name='label' translatable='yes'>Remove Script</attribute>"
        "        <attribute name='action'>AppMenu.RemoveScript</attribute>"
        "      </item>"
        "    </section>"
        "  </menu>"
        "</interface>";
    m_uiManager->add_from_string(ui_info);
#else
    uiManager = Gtk::UIManager::create();
    uiManager->insert_action_group(actionGroup);
    add_accel_group(uiManager->get_accel_group());

    Glib::ustring ui_info =
        "<ui>"
        "  <menubar name='MenuBar'>"
        "    <menu action='MenuFile'>"
        "      <menuitem action='New'/>"
        "      <menuitem action='Open'/>"
        "      <separator/>"
        "      <menuitem action='Save'/>"
        "      <menuitem action='SaveAs'/>"
        "      <separator/>"
        "      <menuitem action='Properties'/>"
        "      <separator/>"
        "      <menuitem action='Quit'/>"
        "    </menu>"
        "    <menu action='MenuEdit'>"
        "      <menuitem action='CopyDimRgn'/>"
        "      <menuitem action='AdjustClipboard'/>"
        "      <menuitem action='PasteDimRgn'/>"
        "      <separator/>"
        "      <menuitem action='SelectPrevInstr'/>"
        "      <menuitem action='SelectNextInstr'/>"
        "      <separator/>"
        "      <menuitem action='SelectPrevRegion'/>"
        "      <menuitem action='SelectNextRegion'/>"
        "      <separator/>"
        "      <menuitem action='SelectPrevDimension'/>"
        "      <menuitem action='SelectNextDimension'/>"
        "      <menuitem action='SelectPrevDimRgnZone'/>"
        "      <menuitem action='SelectNextDimRgnZone'/>"
        "      <menuitem action='SelectAddPrevDimRgnZone'/>"
        "      <menuitem action='SelectAddNextDimRgnZone'/>"
        "      <separator/>"
        "      <menuitem action='CopySampleUnity'/>"
        "      <menuitem action='CopySampleTune'/>"
        "      <menuitem action='CopySampleLoop'/>"
        "    </menu>"
        "    <menu action='MenuMacro'>"
        "    </menu>"
        "    <menu action='MenuSample'>"
        "      <menuitem action='SampleProperties'/>"
        "      <menuitem action='AddGroup'/>"
        "      <menuitem action='AddSample'/>"
        "      <menuitem action='ShowSampleRefs'/>"
        "      <menuitem action='ReplaceSample' />"
        "      <menuitem action='ReplaceAllSamplesInAllGroups' />"
        "      <separator/>"
        "      <menuitem action='RemoveSample'/>"
        "      <menuitem action='RemoveUnusedSamples'/>"
        "    </menu>"
        "    <menu action='MenuInstrument'>"
        "      <menu action='AllInstruments'>"
        "      </menu>"
        "      <separator/>"
        "      <menuitem action='InstrProperties'/>"
        "      <menuitem action='MidiRules'/>"
        "      <menuitem action='ScriptSlots'/>"
        "      <menu action='AssignScripts'/>"
        "      <menuitem action='AddInstrument'/>"
        "      <menuitem action='DupInstrument'/>"
        "      <menuitem action='MoveInstrument'/>"
        "      <menuitem action='CombInstruments'/>"
        "      <separator/>"
        "      <menuitem action='RemoveInstrument'/>"
        "    </menu>"
        "    <menu action='MenuScript'>"
        "      <menuitem action='AddScriptGroup'/>"
        "      <menuitem action='AddScript'/>"
        "      <menuitem action='EditScript'/>"
        "      <separator/>"
        "      <menuitem action='RemoveScript'/>"
        "    </menu>"
        "    <menu action='MenuView'>"
        "      <menuitem action='Statusbar'/>"
        "      <menuitem action='ShowTooltips'/>"
        "      <menuitem action='AutoRestoreWinDim'/>"
        "      <menuitem action='OpenInstrPropsByDoubleClick'/>"
        "      <separator/>"
        "      <menuitem action='RefreshAll'/>"
        "    </menu>"
        "    <menu action='MenuTools'>"
        "      <menuitem action='CombineInstruments'/>"
        "      <menuitem action='MergeFiles'/>"
        "    </menu>"
        "    <menu action='MenuSettings'>"
        "      <menuitem action='WarnUserOnExtensions'/>"
        "      <menuitem action='SyncSamplerInstrumentSelection'/>"
        "      <menuitem action='MoveRootNoteWithRegionMoved'/>"
        "      <menuitem action='SaveWithTemporaryFile'/>"
        "    </menu>"
        "    <menu action='MenuHelp'>"
        "      <menuitem action='About'/>"
        "    </menu>"
        "  </menubar>"
        "  <popup name='PopupMenu'>"
        "    <menuitem action='InstrProperties'/>"
        "    <menuitem action='MidiRules'/>"
        "    <menuitem action='ScriptSlots'/>"
        "    <menuitem action='AddInstrument'/>"
        "    <menuitem action='DupInstrument'/>"
        "    <menuitem action='MoveInstrument'/>"
        "    <menuitem action='CombInstruments'/>"
        "    <separator/>"
        "    <menuitem action='RemoveInstrument'/>"
        "  </popup>"
        "  <popup name='SamplePopupMenu'>"
        "    <menuitem action='SampleProperties'/>"
        "    <menuitem action='AddGroup'/>"
        "    <menuitem action='AddSample'/>"
        "    <menuitem action='ShowSampleRefs'/>"
        "    <menuitem action='ReplaceSample' />"
        "    <menuitem action='ReplaceAllSamplesInAllGroups' />"
        "    <separator/>"
        "    <menuitem action='RemoveSample'/>"
        "    <menuitem action='RemoveUnusedSamples'/>"
        "  </popup>"
        "  <popup name='ScriptPopupMenu'>"
        "    <menuitem action='AddScriptGroup'/>"
        "    <menuitem action='AddScript'/>"
        "    <menuitem action='EditScript'/>"
        "    <separator/>"
        "    <menuitem action='RemoveScript'/>"
        "  </popup>"
        "</ui>";
    uiManager->add_ui_from_string(ui_info);
#endif

#if USE_GTKMM_BUILDER
    popup_menu = new Gtk::Menu(
        Glib::RefPtr<Gio::Menu>::cast_dynamic(
            m_uiManager->get_object("PopupMenu")
        )
    );
    sample_popup = new Gtk::Menu(
        Glib::RefPtr<Gio::Menu>::cast_dynamic(
            m_uiManager->get_object("SamplePopupMenu")
        )
    );
    script_popup = new Gtk::Menu(
        Glib::RefPtr<Gio::Menu>::cast_dynamic(
            m_uiManager->get_object("ScriptPopupMenu")
        )
    );
#else
    popup_menu = dynamic_cast<Gtk::Menu*>(uiManager->get_widget("/PopupMenu"));
    
    // Set tooltips for menu items (for some reason, setting a tooltip on the
    // respective Gtk::Action objects above will simply be ignored, no matter
    // if using Gtk::Action::set_tooltip() or passing the tooltip string on
    // Gtk::Action::create()).
    {
        Gtk::MenuItem* item = dynamic_cast<Gtk::MenuItem*>(
            uiManager->get_widget("/MenuBar/MenuEdit/CopySampleUnity"));
        item->set_tooltip_text(_("Used when dragging a sample to a region's sample reference field. You may disable this for example if you want to replace an existing sample in a region with a new sample, but don't want that the region's current unity note setting will be altered by this action."));
    }
    {
        Gtk::MenuItem* item = dynamic_cast<Gtk::MenuItem*>(
            uiManager->get_widget("/MenuBar/MenuEdit/CopySampleTune"));
        item->set_tooltip_text(_("Used when dragging a sample to a region's sample reference field. You may disable this for example if you want to replace an existing sample in a region with a new sample, but don't want that the region's current sample playback tuning will be altered by this action."));
    }
    {
        Gtk::MenuItem* item = dynamic_cast<Gtk::MenuItem*>(
            uiManager->get_widget("/MenuBar/MenuEdit/CopySampleLoop"));
        item->set_tooltip_text(_("Used when dragging a sample to a region's sample reference field. You may disable this for example if you want to replace an existing sample in a region with a new sample, but don't want that the region's current loop information to be altered by this action."));
    }
    {
        Gtk::MenuItem* item = dynamic_cast<Gtk::MenuItem*>(
            uiManager->get_widget("/MenuBar/MenuSettings/WarnUserOnExtensions"));
        item->set_tooltip_text(_("If checked, a warning will be shown whenever you try to use a feature which is based on a LinuxSampler extension ontop of the original gig format, which would not work with the Gigasampler/GigaStudio application."));
    }
    {
        Gtk::MenuItem* item = dynamic_cast<Gtk::MenuItem*>(
            uiManager->get_widget("/MenuBar/MenuSettings/SyncSamplerInstrumentSelection"));
        item->set_tooltip_text(_("If checked, the sampler's current instrument will automatically be switched whenever another instrument was selected in gigedit (only available in live-mode)."));
    }
    {
        Gtk::MenuItem* item = dynamic_cast<Gtk::MenuItem*>(
            uiManager->get_widget("/MenuBar/MenuSettings/MoveRootNoteWithRegionMoved"));
        item->set_tooltip_text(_("If checked, and when a region is moved by dragging it around on the virtual keyboard, the keyboard position dependent pitch will move exactly with the amount of semi tones the region was moved around."));
    }
    {
        Gtk::MenuItem* item = dynamic_cast<Gtk::MenuItem*>(
            uiManager->get_widget("/MenuBar/MenuSample/RemoveUnusedSamples"));
        item->set_tooltip_text(_("Removes all samples that are not referenced by any instrument (i.e. red ones)."));
        // copy tooltip to popup menu
        Gtk::MenuItem* item2 = dynamic_cast<Gtk::MenuItem*>(
            uiManager->get_widget("/SamplePopupMenu/RemoveUnusedSamples"));
        item2->set_tooltip_text(item->get_tooltip_text());
    }
    {
        Gtk::MenuItem* item = dynamic_cast<Gtk::MenuItem*>(
            uiManager->get_widget("/MenuBar/MenuView/RefreshAll"));
        item->set_tooltip_text(_("Reloads the currently open gig file and updates the entire graphical user interface."));
    }
    {
        Gtk::MenuItem* item = dynamic_cast<Gtk::MenuItem*>(
            uiManager->get_widget("/MenuBar/MenuView/AutoRestoreWinDim"));
        item->set_tooltip_text(_("If checked, size and position of all windows will be saved and automatically restored next time."));
    }
    {
        Gtk::MenuItem* item = dynamic_cast<Gtk::MenuItem*>(
            uiManager->get_widget("/MenuBar/MenuView/OpenInstrPropsByDoubleClick"));
        item->set_tooltip_text(_("If checked, double clicking an instrument opens its properties dialog."));
    }
    {
        Gtk::MenuItem* item = dynamic_cast<Gtk::MenuItem*>(
            uiManager->get_widget("/MenuBar/MenuTools/CombineInstruments"));
        item->set_tooltip_text(_("Create combi sounds out of individual sounds of this .gig file."));
    }
    {
        Gtk::MenuItem* item = dynamic_cast<Gtk::MenuItem*>(
            uiManager->get_widget("/MenuBar/MenuTools/MergeFiles"));
        item->set_tooltip_text(_("Add instruments and samples of other .gig files to this .gig file."));
    }
#endif

#if USE_GTKMM_BUILDER
    assign_scripts_menu = new Gtk::Menu(
        Glib::RefPtr<Gio::Menu>::cast_dynamic(
            m_uiManager->get_object("AssignScripts")
        )
    );
#else
    instrument_menu = static_cast<Gtk::MenuItem*>(
        uiManager->get_widget("/MenuBar/MenuInstrument/AllInstruments"))->get_submenu();

    assign_scripts_menu = static_cast<Gtk::MenuItem*>(
        uiManager->get_widget("/MenuBar/MenuInstrument/AssignScripts"))->get_submenu();
#endif

#if USE_GTKMM_BUILDER
    Gtk::Widget* menuBar = NULL;
    m_uiManager->get_widget("MenuBar", menuBar);
#else
    Gtk::Widget* menuBar = uiManager->get_widget("/MenuBar");
#endif

    m_VBox.pack_start(*menuBar, Gtk::PACK_SHRINK);
    m_VBox.pack_start(m_HPaned);
    m_VBox.pack_start(m_RegionChooser, Gtk::PACK_SHRINK);
    m_VBox.pack_start(m_RegionChooser.m_VirtKeybPropsBox, Gtk::PACK_SHRINK);
    m_VBox.pack_start(m_DimRegionChooser, Gtk::PACK_SHRINK);
    m_VBox.pack_start(m_StatusBar, Gtk::PACK_SHRINK);

    set_file_is_shared(false);

    // Status Bar:
#if USE_GTKMM_BOX
# warning No status bar layout for GTKMM 4 yet
#else
    m_StatusBar.pack_start(m_AttachedStateLabel, Gtk::PACK_SHRINK);
    m_StatusBar.pack_start(m_AttachedStateImage, Gtk::PACK_SHRINK);
#endif
    m_StatusBar.show();

    m_RegionChooser.signal_region_selected().connect(
        sigc::mem_fun(*this, &MainWindow::region_changed) );
    m_DimRegionChooser.signal_dimregion_selected().connect(
        sigc::mem_fun(*this, &MainWindow::dimreg_changed) );


    // Create the Tree model:
    m_refTreeModel = Gtk::ListStore::create(m_Columns);
    m_refTreeModelFilter = Gtk::TreeModelFilter::create(m_refTreeModel);
    m_refTreeModelFilter->set_visible_func(
        sigc::mem_fun(*this, &MainWindow::instrument_row_visible)
    );
    m_TreeView.set_model(m_refTreeModelFilter);

    m_TreeView.get_selection()->set_mode(Gtk::SELECTION_MULTIPLE);
    m_TreeView.set_has_tooltip(true);
    m_TreeView.signal_query_tooltip().connect(
        sigc::mem_fun(*this, &MainWindow::onQueryTreeViewTooltip)
    );
    instrument_name_connection = m_refTreeModel->signal_row_changed().connect(
        sigc::mem_fun(*this, &MainWindow::instrument_name_changed)
    );

    // Add the TreeView's view columns:
    m_TreeView.append_column(_("Nr"), m_Columns.m_col_nr);
    m_TreeView.append_column_editable(_("Instrument"), m_Columns.m_col_name);
    m_TreeView.append_column(_("Scripts"), m_Columns.m_col_scripts);
    m_TreeView.set_headers_visible(true);
    
    // establish drag&drop within the instrument tree view, allowing to reorder
    // the sequence of instruments within the gig file
    {
        std::vector<Gtk::TargetEntry> drag_target_instrument;
        drag_target_instrument.push_back(Gtk::TargetEntry("gig::Instrument"));
        m_TreeView.drag_source_set(drag_target_instrument);
        m_TreeView.drag_dest_set(drag_target_instrument);
        m_TreeView.signal_drag_begin().connect(
            sigc::mem_fun(*this, &MainWindow::on_instruments_treeview_drag_begin)
        );
        m_TreeView.signal_drag_data_get().connect(
            sigc::mem_fun(*this, &MainWindow::on_instruments_treeview_drag_data_get)
        );
        m_TreeView.signal_drag_data_received().connect(
            sigc::mem_fun(*this, &MainWindow::on_instruments_treeview_drop_drag_data_received)
        );
    }

    // create samples treeview (including its data model)
    m_refSamplesTreeModel = SamplesTreeStore::create(m_SamplesModel);
    m_TreeViewSamples.set_model(m_refSamplesTreeModel);
    m_TreeViewSamples.get_selection()->set_mode(Gtk::SELECTION_MULTIPLE);
    m_TreeViewSamples.set_tooltip_text(_("To actually use a sample, drag it from this list view to \"Sample\" -> \"Sample:\" on the region's settings pane on the right.\n\nRight click here for more actions on samples."));
    // m_TreeViewSamples.set_reorderable();
    m_TreeViewSamples.append_column_editable(_("Name"), m_SamplesModel.m_col_name);
    m_TreeViewSamples.append_column(_("Referenced"), m_SamplesModel.m_col_refcount);
    {
        Gtk::TreeViewColumn* column = m_TreeViewSamples.get_column(0);
        Gtk::CellRendererText* cellrenderer =
            dynamic_cast<Gtk::CellRendererText*>(column->get_first_cell());
        column->add_attribute(
            cellrenderer->property_foreground(), m_SamplesModel.m_color
        );
    }
    {
        Gtk::TreeViewColumn* column = m_TreeViewSamples.get_column(1);
        Gtk::CellRendererText* cellrenderer =
            dynamic_cast<Gtk::CellRendererText*>(column->get_first_cell());
        column->add_attribute(
            cellrenderer->property_foreground(), m_SamplesModel.m_color
        );
    }
    m_TreeViewSamples.set_headers_visible(true);
#if GTKMM_MAJOR_VERSION > 3 || (GTKMM_MAJOR_VERSION == 3 && (GTKMM_MINOR_VERSION > 91 || (GTKMM_MINOR_VERSION == 91 && GTKMM_MICRO_VERSION >= 2))) // GTKMM >= 3.91.2
    m_TreeViewSamples.signal_button_press_event().connect(
        sigc::mem_fun(*this, &MainWindow::on_sample_treeview_button_release)
    );
#else
    m_TreeViewSamples.signal_button_press_event().connect_notify(
        sigc::mem_fun(*this, &MainWindow::on_sample_treeview_button_release)
    );
#endif
    m_refSamplesTreeModel->signal_row_changed().connect(
        sigc::mem_fun(*this, &MainWindow::sample_name_changed)
    );

    // create scripts treeview (including its data model)
    m_refScriptsTreeModel = ScriptsTreeStore::create(m_ScriptsModel);
    m_TreeViewScripts.set_model(m_refScriptsTreeModel);
    m_TreeViewScripts.set_tooltip_text(_(
        "Use CTRL + double click for editing a script."
        "\n\n"
        "Note: instrument scripts are a LinuxSampler extension of the gig "
        "format. This feature will not work with the GigaStudio software!"
    ));
    // m_TreeViewScripts.set_reorderable();
    m_TreeViewScripts.append_column_editable("Samples", m_ScriptsModel.m_col_name);
    m_TreeViewScripts.set_headers_visible(false);
#if GTKMM_MAJOR_VERSION > 3 || (GTKMM_MAJOR_VERSION == 3 && (GTKMM_MINOR_VERSION > 91 || (GTKMM_MINOR_VERSION == 91 && GTKMM_MICRO_VERSION >= 2))) // GTKMM >= 3.91.2
    m_TreeViewScripts.signal_button_press_event().connect(
        sigc::mem_fun(*this, &MainWindow::on_script_treeview_button_release)
    );
#else
    m_TreeViewScripts.signal_button_press_event().connect_notify(
        sigc::mem_fun(*this, &MainWindow::on_script_treeview_button_release)
    );
#endif
    //FIXME: why the heck does this double click signal_row_activated() only fire while CTRL key is pressed ?
    m_TreeViewScripts.signal_row_activated().connect(
        sigc::mem_fun(*this, &MainWindow::script_double_clicked)
    );
    m_refScriptsTreeModel->signal_row_changed().connect(
        sigc::mem_fun(*this, &MainWindow::script_name_changed)
    );

    // establish drag&drop between scripts tree view and ScriptSlots window
    std::vector<Gtk::TargetEntry> drag_target_gig_script;
    drag_target_gig_script.push_back(Gtk::TargetEntry("gig::Script"));
    m_TreeViewScripts.drag_source_set(drag_target_gig_script);
    m_TreeViewScripts.signal_drag_begin().connect(
        sigc::mem_fun(*this, &MainWindow::on_scripts_treeview_drag_begin)
    );
    m_TreeViewScripts.signal_drag_data_get().connect(
        sigc::mem_fun(*this, &MainWindow::on_scripts_treeview_drag_data_get)
    );

    // establish drag&drop between samples tree view and dimension region 'Sample' text entry
    std::vector<Gtk::TargetEntry> drag_target_gig_sample;
    drag_target_gig_sample.push_back(Gtk::TargetEntry("gig::Sample"));
    m_TreeViewSamples.drag_source_set(drag_target_gig_sample);
    m_TreeViewSamples.signal_drag_begin().connect(
        sigc::mem_fun(*this, &MainWindow::on_sample_treeview_drag_begin)
    );
    m_TreeViewSamples.signal_drag_data_get().connect(
        sigc::mem_fun(*this, &MainWindow::on_sample_treeview_drag_data_get)
    );
    dimreg_edit.wSample->drag_dest_set(drag_target_gig_sample);
    dimreg_edit.wSample->signal_drag_data_received().connect(
        sigc::mem_fun(*this, &MainWindow::on_sample_label_drop_drag_data_received)
    );
    dimreg_edit.signal_dimreg_changed().connect(
        sigc::hide(sigc::mem_fun(*this, &MainWindow::file_changed)));
    m_RegionChooser.signal_instrument_changed().connect(
        sigc::mem_fun(*this, &MainWindow::file_changed));
    m_RegionChooser.signal_instrument_changed().connect(
        sigc::mem_fun(*this, &MainWindow::region_changed));
    m_DimRegionChooser.signal_region_changed().connect(
        sigc::mem_fun(*this, &MainWindow::file_changed));
    instrumentProps.signal_changed().connect(
        sigc::mem_fun(*this, &MainWindow::file_changed));
    sampleProps.signal_changed().connect(
        sigc::mem_fun(*this, &MainWindow::file_changed));
    fileProps.signal_changed().connect(
        sigc::mem_fun(*this, &MainWindow::file_changed));
    midiRules.signal_changed().connect(
        sigc::mem_fun(*this, &MainWindow::file_changed));

    dimreg_edit.signal_dimreg_to_be_changed().connect(
        dimreg_to_be_changed_signal.make_slot());
    dimreg_edit.signal_dimreg_changed().connect(
        dimreg_changed_signal.make_slot());
    dimreg_edit.signal_sample_ref_changed().connect(
        sample_ref_changed_signal.make_slot());
    sample_ref_changed_signal.connect(
        sigc::mem_fun(*this, &MainWindow::on_sample_ref_changed)
    );
    samples_to_be_removed_signal.connect(
        sigc::mem_fun(*this, &MainWindow::on_samples_to_be_removed)
    );

    dimreg_edit.signal_select_sample().connect(
        sigc::mem_fun(*this, &MainWindow::select_sample)
    );

    m_RegionChooser.signal_instrument_struct_to_be_changed().connect(
        sigc::hide(
            sigc::bind(
                file_structure_to_be_changed_signal.make_slot(),
#if SIGCXX_MAJOR_VERSION > 2 || (SIGCXX_MAJOR_VERSION == 2 && SIGCXX_MINOR_VERSION >= 8)
                std::ref(this->file)
#else
                sigc::ref(this->file)
#endif
            )
        )
    );
    m_RegionChooser.signal_instrument_struct_changed().connect(
        sigc::hide(
            sigc::bind(
                file_structure_changed_signal.make_slot(),
#if SIGCXX_MAJOR_VERSION > 2 || (SIGCXX_MAJOR_VERSION == 2 && SIGCXX_MINOR_VERSION >= 8)
                std::ref(this->file)
#else
                sigc::ref(this->file)
#endif
            )
        )
    );
    m_RegionChooser.signal_region_to_be_changed().connect(
        region_to_be_changed_signal.make_slot());
    m_RegionChooser.signal_region_changed_signal().connect(
        region_changed_signal.make_slot());

    note_on_signal.connect(
        sigc::mem_fun(m_RegionChooser, &RegionChooser::on_note_on_event));
    note_off_signal.connect(
        sigc::mem_fun(m_RegionChooser, &RegionChooser::on_note_off_event));

    dimreg_all_regions.signal_toggled().connect(
        sigc::mem_fun(*this, &MainWindow::update_dimregs));
    dimreg_all_dimregs.signal_toggled().connect(
        sigc::mem_fun(*this, &MainWindow::dimreg_all_dimregs_toggled));
    dimreg_stereo.signal_toggled().connect(
        sigc::mem_fun(*this, &MainWindow::update_dimregs));

    m_searchText.signal_changed().connect(
        sigc::mem_fun(*m_refTreeModelFilter.operator->(), &Gtk::TreeModelFilter::refilter)
    );

    file = 0;
    file_is_changed = false;

#if HAS_GTKMM_SHOW_ALL_CHILDREN
    show_all_children();
#endif

    // start with a new gig file by default
    on_action_file_new();

    m_TreeViewNotebook.signal_switch_page().connect(
        sigc::mem_fun(*this, &MainWindow::on_notebook_tab_switched)
    );

    // select 'Instruments' tab by default
    // (gtk allows this only if the tab childs are visible, thats why it's here)
    m_TreeViewNotebook.set_current_page(1);

    Gtk::Clipboard::get()->signal_owner_change().connect(
        sigc::mem_fun(*this, &MainWindow::on_clipboard_owner_change)
    );
    updateClipboardPasteAvailable();
    updateClipboardCopyAvailable();

    // setup macros and their keyboard accelerators
    {
#if USE_GTKMM_BUILDER
        menuMacro = new Gtk::Menu(
            Glib::RefPtr<Gio::Menu>::cast_dynamic(
                m_uiManager->get_object("MenuMacro")
            )
        );
#else
        Gtk::Menu* menuMacro = dynamic_cast<Gtk::MenuItem*>(
            uiManager->get_widget("/MenuBar/MenuMacro")
        )->get_submenu();
#endif

        const Gdk::ModifierType noModifier = (Gdk::ModifierType)0;
        Gtk::AccelMap::add_entry("<Macros>/macro_0", GDK_KEY_F1, noModifier);
        Gtk::AccelMap::add_entry("<Macros>/macro_1", GDK_KEY_F2, noModifier);
        Gtk::AccelMap::add_entry("<Macros>/macro_2", GDK_KEY_F3, noModifier);
        Gtk::AccelMap::add_entry("<Macros>/macro_3", GDK_KEY_F4, noModifier);
        Gtk::AccelMap::add_entry("<Macros>/macro_4", GDK_KEY_F5, noModifier);
        Gtk::AccelMap::add_entry("<Macros>/macro_5", GDK_KEY_F6, noModifier);
        Gtk::AccelMap::add_entry("<Macros>/macro_6", GDK_KEY_F7, noModifier);
        Gtk::AccelMap::add_entry("<Macros>/macro_7", GDK_KEY_F8, noModifier);
        Gtk::AccelMap::add_entry("<Macros>/macro_8", GDK_KEY_F9, noModifier);
        Gtk::AccelMap::add_entry("<Macros>/macro_9", GDK_KEY_F10, noModifier);
        Gtk::AccelMap::add_entry("<Macros>/macro_10", GDK_KEY_F11, noModifier);
        Gtk::AccelMap::add_entry("<Macros>/macro_11", GDK_KEY_F12, noModifier);
        Gtk::AccelMap::add_entry("<Macros>/macro_12", GDK_KEY_F13, noModifier);
        Gtk::AccelMap::add_entry("<Macros>/macro_13", GDK_KEY_F14, noModifier);
        Gtk::AccelMap::add_entry("<Macros>/macro_14", GDK_KEY_F15, noModifier);
        Gtk::AccelMap::add_entry("<Macros>/macro_15", GDK_KEY_F16, noModifier);
        Gtk::AccelMap::add_entry("<Macros>/macro_16", GDK_KEY_F17, noModifier);
        Gtk::AccelMap::add_entry("<Macros>/macro_17", GDK_KEY_F18, noModifier);
        Gtk::AccelMap::add_entry("<Macros>/macro_18", GDK_KEY_F19, noModifier);
        Gtk::AccelMap::add_entry("<Macros>/SetupMacros", 'm', primaryModifierKey);

        Glib::RefPtr<Gtk::AccelGroup> accelGroup = this->get_accel_group();
        menuMacro->set_accel_group(accelGroup);

        updateMacroMenu();
    }

    // setup "Assign Scripts" keyboard accelerators
    {
        Gtk::AccelMap::add_entry("<Scripts>/script_0", GDK_KEY_F1, Gdk::SHIFT_MASK);
        Gtk::AccelMap::add_entry("<Scripts>/script_1", GDK_KEY_F2, Gdk::SHIFT_MASK);
        Gtk::AccelMap::add_entry("<Scripts>/script_2", GDK_KEY_F3, Gdk::SHIFT_MASK);
        Gtk::AccelMap::add_entry("<Scripts>/script_3", GDK_KEY_F4, Gdk::SHIFT_MASK);
        Gtk::AccelMap::add_entry("<Scripts>/script_4", GDK_KEY_F5, Gdk::SHIFT_MASK);
        Gtk::AccelMap::add_entry("<Scripts>/script_5", GDK_KEY_F6, Gdk::SHIFT_MASK);
        Gtk::AccelMap::add_entry("<Scripts>/script_6", GDK_KEY_F7, Gdk::SHIFT_MASK);
        Gtk::AccelMap::add_entry("<Scripts>/script_7", GDK_KEY_F8, Gdk::SHIFT_MASK);
        Gtk::AccelMap::add_entry("<Scripts>/script_8", GDK_KEY_F9, Gdk::SHIFT_MASK);
        Gtk::AccelMap::add_entry("<Scripts>/script_9", GDK_KEY_F10, Gdk::SHIFT_MASK);
        Gtk::AccelMap::add_entry("<Scripts>/script_10", GDK_KEY_F11, Gdk::SHIFT_MASK);
        Gtk::AccelMap::add_entry("<Scripts>/script_11", GDK_KEY_F12, Gdk::SHIFT_MASK);
        Gtk::AccelMap::add_entry("<Scripts>/script_12", GDK_KEY_F13, Gdk::SHIFT_MASK);
        Gtk::AccelMap::add_entry("<Scripts>/script_13", GDK_KEY_F14, Gdk::SHIFT_MASK);
        Gtk::AccelMap::add_entry("<Scripts>/script_14", GDK_KEY_F15, Gdk::SHIFT_MASK);
        Gtk::AccelMap::add_entry("<Scripts>/script_15", GDK_KEY_F16, Gdk::SHIFT_MASK);
        Gtk::AccelMap::add_entry("<Scripts>/script_16", GDK_KEY_F17, Gdk::SHIFT_MASK);
        Gtk::AccelMap::add_entry("<Scripts>/script_17", GDK_KEY_F18, Gdk::SHIFT_MASK);
        Gtk::AccelMap::add_entry("<Scripts>/script_18", GDK_KEY_F19, Gdk::SHIFT_MASK);
        Gtk::AccelMap::add_entry("<Scripts>/DropAllScriptSlots", GDK_KEY_BackSpace, Gdk::SHIFT_MASK);

        Glib::RefPtr<Gtk::AccelGroup> accelGroup = this->get_accel_group();
        assign_scripts_menu->set_accel_group(accelGroup);
    }

    on_show_tooltips_changed();

    Glib::signal_idle().connect_once(
        sigc::mem_fun(*this, &MainWindow::bringToFront),
        200
    );
}

MainWindow::~MainWindow()
{
}

void MainWindow::bringToFront() {
    #if defined(__APPLE__)
    macRaiseAppWindow();
    #endif
    raise();
    present();
}

void MainWindow::updateMacroMenu() {
#if !USE_GTKMM_BUILDER
    Gtk::Menu* menuMacro = dynamic_cast<Gtk::MenuItem*>(
        uiManager->get_widget("/MenuBar/MenuMacro")
    )->get_submenu();
#endif

    // remove all entries from "Macro" menu
    {
        const std::vector<Gtk::Widget*> children = menuMacro->get_children();
        for (int i = 0; i < children.size(); ++i) {
            Gtk::Widget* child = children[i];
            menuMacro->remove(*child);
            delete child;
        }
    }

    // (re)load all macros from config file
    try {
        Settings::singleton()->loadMacros(m_macros);
    } catch (Serialization::Exception e) {
        std::cerr << "Exception while loading macros: " << e.Message << std::endl;
    } catch (...) {
        std::cerr << "Unknown exception while loading macros!" << std::endl;
    }

    // add all configured macros as menu items to the "Macro" menu
    for (int iMacro = 0; iMacro < m_macros.size(); ++iMacro) {
        const Serialization::Archive& macro = m_macros[iMacro];
        std::string name =
            macro.name().empty() ?
                (std::string(_("Unnamed Macro")) + " " + ToString(iMacro+1)) : macro.name();
        Gtk::MenuItem* item = new Gtk::MenuItem(name);
        item->signal_activate().connect(
            sigc::bind(
                sigc::mem_fun(*this, &MainWindow::onMacroSelected), iMacro
            )
        );
        menuMacro->append(*item);
        item->set_accel_path("<Macros>/macro_" + ToString(iMacro));
        Glib::ustring comment = macro.comment();
        if (!comment.empty())
            item->set_tooltip_text(comment);
    }
    // if there are no macros configured at all, then show a dummy entry instead
    if (m_macros.empty()) {
        Gtk::MenuItem* item = new Gtk::MenuItem(_("No Macros"));
        item->set_sensitive(false);
        menuMacro->append(*item);
    }

    // add separator line to menu
    menuMacro->append(*new Gtk::SeparatorMenuItem);

    {
        Gtk::MenuItem* item = new Gtk::MenuItem(_("Setup Macros ..."));
        item->signal_activate().connect(
            sigc::mem_fun(*this, &MainWindow::setupMacros)
        );
        menuMacro->append(*item);
        item->set_accel_path("<Macros>/SetupMacros");
    }

#if HAS_GTKMM_SHOW_ALL_CHILDREN
    menuMacro->show_all_children();
#endif
}

void MainWindow::onMacroSelected(int iMacro) {
    printf("onMacroSelected(%d)\n", iMacro);
    if (iMacro < 0 || iMacro >= m_macros.size()) return;
    Glib::ustring errorText;
    try {
        applyMacro(m_macros[iMacro]);
    } catch (Serialization::Exception e) {
        errorText = e.Message;
    } catch (...) {
        errorText = _("Unknown exception while applying macro");
    }
    if (!errorText.empty()) {
        Glib::ustring txt = _("Applying macro failed:\n") + errorText;
        Gtk::MessageDialog msg(*this, txt, false, Gtk::MESSAGE_ERROR);
        msg.run();
    }
}

void MainWindow::setupMacros() {
    MacrosSetup* setup = new MacrosSetup();
    gig::DimensionRegion* pDimRgn = m_DimRegionChooser.get_main_dimregion();
    setup->setMacros(m_macros, &m_serializationArchive, pDimRgn);
    setup->signal_macros_changed().connect(
        sigc::mem_fun(*this, &MainWindow::onMacrosSetupChanged)
    );
    setup->show();
}

void MainWindow::onMacrosSetupChanged(const std::vector<Serialization::Archive>& macros) {
    m_macros = macros;
    Settings::singleton()->saveMacros(m_macros);
    updateMacroMenu();
}

//NOTE: the actual signal's first argument for argument 'page' is on some gtkmm version GtkNotebookPage* and on some Gtk::Widget*. Since we don't need that argument, it is simply void* here for now.
void MainWindow::on_notebook_tab_switched(void* page, guint page_num) {
    bool isInstrumentsPage = (page_num == 1);
    // so far we only support filtering for the instruments list, so hide the
    // filter text entry field if another tab is selected
    m_searchField.set_visible(isInstrumentsPage);
}

bool MainWindow::on_delete_event(GdkEventAny* event)
{
    return !file_is_shared && file_is_changed && !close_confirmation_dialog();
}

void MainWindow::on_action_quit()
{
    if (!file_is_shared && file_is_changed && !close_confirmation_dialog()) return;
    hide();
}

void MainWindow::region_changed()
{
    m_DimRegionChooser.set_region(m_RegionChooser.get_region());
}

gig::Instrument* MainWindow::get_instrument()
{
    gig::Instrument* instrument = 0;
    std::vector<Gtk::TreeModel::Path> rows = m_TreeView.get_selection()->get_selected_rows();
    if (rows.empty()) return NULL;
    //NOTE: was const_iterator before, which did not compile with GTKMM4 development branch, probably going to be fixed before final GTKMM4 release though.
    Gtk::TreeModel::iterator it = m_refTreeModel->get_iter(rows[0]);
    if (it) {
        Gtk::TreeModel::Row row = *it;
        instrument = row[m_Columns.m_col_instr];
    }
    return instrument;
}

void MainWindow::add_region_to_dimregs(gig::Region* region, bool stereo, bool all_dimregs)
{
    if (all_dimregs) {
        for (int i = 0 ; i < region->DimensionRegions ; i++) {
            if (region->pDimensionRegions[i]) {
                dimreg_edit.dimregs.insert(region->pDimensionRegions[i]);
            }
        }
    } else {
        m_DimRegionChooser.get_dimregions(region, stereo, dimreg_edit.dimregs);
    }
}

void MainWindow::update_dimregs()
{
    dimreg_edit.dimregs.clear();
    bool all_regions = dimreg_all_regions.get_active();
    bool stereo = dimreg_stereo.get_active();
    bool all_dimregs = dimreg_all_dimregs.get_active();

    if (all_regions) {
        gig::Instrument* instrument = get_instrument();
        if (instrument) {
            for (gig::Region* region = instrument->GetFirstRegion() ;
                 region ;
                 region = instrument->GetNextRegion()) {
                add_region_to_dimregs(region, stereo, all_dimregs);
            }
        }
    } else {
        gig::Region* region = m_RegionChooser.get_region();
        if (region) {
            add_region_to_dimregs(region, stereo, all_dimregs);
        }
    }

    m_RegionChooser.setModifyAllRegions(all_regions);
    m_DimRegionChooser.setModifyAllRegions(all_regions);
    m_DimRegionChooser.setModifyAllDimensionRegions(all_dimregs);
    m_DimRegionChooser.setModifyBothChannels(stereo);

    updateClipboardCopyAvailable();
}

void MainWindow::dimreg_all_dimregs_toggled()
{
    dimreg_stereo.set_sensitive(!dimreg_all_dimregs.get_active());
    update_dimregs();
}

void MainWindow::dimreg_changed()
{
    update_dimregs();
    dimreg_edit.set_dim_region(m_DimRegionChooser.get_main_dimregion());
}

void MainWindow::on_sel_change()
{
#if !USE_GTKMM_BUILDER
    // select item in instrument menu
    std::vector<Gtk::TreeModel::Path> rows = m_TreeView.get_selection()->get_selected_rows();
    if (!rows.empty()) {
        Gtk::TreeModel::iterator it = m_refTreeModel->get_iter(rows[0]);
        if (it) {
            Gtk::TreePath path(it);
            int index = path[0];
            const std::vector<Gtk::Widget*> children =
                instrument_menu->get_children();
            static_cast<Gtk::RadioMenuItem*>(children[index])->set_active();
        }
    }
#endif

    updateScriptListOfMenu();

    m_RegionChooser.set_instrument(get_instrument());

    if (Settings::singleton()->syncSamplerInstrumentSelection) {
        switch_sampler_instrument_signal.emit(get_instrument());
    }
}


LoaderSaverBase::LoaderSaverBase(const Glib::ustring filename, gig::File* gig) :
    filename(filename), gig(gig),
#ifdef GLIB_THREADS
    thread(0),
#endif
    progress(0.f)
{
}

void loader_progress_callback(gig::progress_t* progress)
{
    LoaderSaverBase* loader = static_cast<LoaderSaverBase*>(progress->custom);
    loader->progress_callback(progress->factor);
}

void LoaderSaverBase::progress_callback(float fraction)
{
    {
#ifdef GLIB_THREADS
        Glib::Threads::Mutex::Lock lock(progressMutex);
#else
        std::lock_guard<std::mutex> lock(progressMutex);
#endif
        progress = fraction;
    }
    progress_dispatcher();
}

#if defined(WIN32) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 2))
// make sure stack is 16-byte aligned for SSE instructions
__attribute__((force_align_arg_pointer))
#endif
void LoaderSaverBase::thread_function()
{
#ifdef GLIB_THREADS
    printf("thread_function self=%p\n",
           static_cast<void*>(Glib::Threads::Thread::self()));
#else
    std::cout << "thread_function self=" << std::this_thread::get_id() << "\n";
#endif
    printf("Start %s\n", filename.c_str());
    try {
        gig::progress_t progress;
        progress.callback = loader_progress_callback;
        progress.custom = this;

        thread_function_sub(progress);
        printf("End\n");
        finished_dispatcher();
    } catch (RIFF::Exception e) {
        error_message = e.Message;
        error_dispatcher.emit();
    } catch (...) {
        error_message = _("Unknown exception occurred");
        error_dispatcher.emit();
    }
}

void LoaderSaverBase::launch()
{
#ifdef GLIB_THREADS
#ifdef OLD_THREADS
    thread = Glib::Thread::create(sigc::mem_fun(*this, &LoaderSaverBase::thread_function), true);
#else
    thread = Glib::Threads::Thread::create(sigc::mem_fun(*this, &LoaderSaverBase::thread_function));
#endif
    printf("launch thread=%p\n", static_cast<void*>(thread));
#else
    thread = std::thread([this](){ thread_function(); });
    std::cout << "launch thread=" << thread.get_id() << "\n";
#endif
}

float LoaderSaverBase::get_progress()
{
#ifdef GLIB_THREADS
    Glib::Threads::Mutex::Lock lock(progressMutex);
#else
    std::lock_guard<std::mutex> lock(progressMutex);
#endif
    return progress;
}

Glib::Dispatcher& LoaderSaverBase::signal_progress()
{
    return progress_dispatcher;
}

Glib::Dispatcher& LoaderSaverBase::signal_finished()
{
    return finished_dispatcher;
}

Glib::Dispatcher& LoaderSaverBase::signal_error()
{
    return error_dispatcher;
}

void LoaderSaverBase::join() {
#ifdef GLIB_THREADS
    thread->join();
#else
    thread.join();
#endif
}


Loader::Loader(const char* filename) :
    LoaderSaverBase(filename, 0)
{
}

void Loader::thread_function_sub(gig::progress_t& progress)
{
    RIFF::File* riff = new RIFF::File(filename);
    gig = new gig::File(riff);

    gig->GetInstrument(0, &progress);
}


Saver::Saver(gig::File* file, Glib::ustring filename) :
    LoaderSaverBase(filename, file)
{
}

void Saver::thread_function_sub(gig::progress_t& progress)
{
    // if no filename was provided, that means "save", if filename was provided means "save as"
    if (filename.empty()) {
        if (!Settings::singleton()->saveWithTemporaryFile) {
            // save directly over the existing .gig file
            // (requires less disk space than solution below
            // but may be slower)
            gig->Save(&progress);
        } else {
            // save the file as separate temporary file first,
            // then move the saved file over the old file
            // (may result in performance speedup during save)
            gig::String tmpname = filename + ".TMP";
            gig->Save(tmpname, &progress);
#if defined(WIN32)
            if (!DeleteFile(filename.c_str())) {
                throw RIFF::Exception("Could not replace original file with temporary file (unable to remove original file).");
            }
#else // POSIX ...
            if (unlink(filename.c_str())) {
                throw RIFF::Exception("Could not replace original file with temporary file (unable to remove original file): " + gig::String(strerror(errno)));
            }
#endif
            if (rename(tmpname.c_str(), filename.c_str())) {
#if defined(WIN32)
                throw RIFF::Exception("Could not replace original file with temporary file (unable to rename temp file).");
#else
                throw RIFF::Exception("Could not replace original file with temporary file (unable to rename temp file): " + gig::String(strerror(errno)));
#endif
            }
        }
    } else {
        gig->Save(filename, &progress);
    }
}


ProgressDialog::ProgressDialog(const Glib::ustring& title, Gtk::Window& parent)
    : Gtk::Dialog(title, parent, true)
{
#if USE_GTKMM_BOX
    get_content_area()->pack_start(progressBar);
#else
    get_vbox()->pack_start(progressBar);
#endif
#if HAS_GTKMM_SHOW_ALL_CHILDREN
    show_all_children();
#endif
    resize(600,50);
}

// Clear all GUI elements / controls. This method is typically called
// before a new .gig file is to be created or to be loaded.
void MainWindow::__clear() {
    // forget all samples that ought to be imported
    m_SampleImportQueue.clear();
    // clear the samples and instruments tree views
    m_refTreeModel->clear();
    m_refSamplesTreeModel->clear();
    m_refScriptsTreeModel->clear();
#if !USE_GTKMM_BUILDER
    // remove all entries from "Instrument" menu
    while (!instrument_menu->get_children().empty()) {
        remove_instrument_from_menu(0);
    }
#endif
    // free libgig's gig::File instance
    if (file && !file_is_shared) delete file;
    file = NULL;
    set_file_is_shared(false);
}

void MainWindow::__refreshEntireGUI() {
    // clear the samples and instruments tree views
    m_refTreeModel->clear();
    m_refSamplesTreeModel->clear();
    m_refScriptsTreeModel->clear();
#if !USE_GTKMM_BUILDER
    // remove all entries from "Instrument" menu
    while (!instrument_menu->get_children().empty()) {
        remove_instrument_from_menu(0);
    }
#endif

    if (!this->file) return;

    load_gig(
        this->file, this->file->pInfo->Name.c_str(), this->file_is_shared
    );
}

void MainWindow::on_action_file_new()
{
    if (!file_is_shared && file_is_changed && !close_confirmation_dialog()) return;

    if (file_is_shared && !leaving_shared_mode_dialog()) return;

    // clear all GUI elements
    __clear();
    // create a new .gig file (virtually yet)
    gig::File* pFile = new gig::File;
    // already add one new instrument by default
    gig::Instrument* pInstrument = pFile->AddInstrument();
    pInstrument->pInfo->Name = gig_from_utf8(_("Unnamed Instrument"));
    // update GUI with that new gig::File
    load_gig(pFile, 0 /*no file name yet*/);
}

bool MainWindow::close_confirmation_dialog()
{
    gchar* msg = g_strdup_printf(_("Save changes to \"%s\" before closing?"),
                                 Glib::filename_display_basename(filename).c_str());
    Gtk::MessageDialog dialog(*this, msg, false, Gtk::MESSAGE_WARNING, Gtk::BUTTONS_NONE);
    g_free(msg);
    dialog.set_secondary_text(_("If you close without saving, your changes will be lost."));
    dialog.add_button(_("Close _Without Saving"), Gtk::RESPONSE_NO);
#if HAS_GTKMM_STOCK
    dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
    dialog.add_button(file_has_name ? Gtk::Stock::SAVE : Gtk::Stock::SAVE_AS, Gtk::RESPONSE_YES);
#else
    dialog.add_button(_("_OK"), Gtk::RESPONSE_OK);
    dialog.add_button(_("_Cancel"), Gtk::RESPONSE_CANCEL);
#endif
    dialog.set_default_response(Gtk::RESPONSE_YES);
    int response = dialog.run();
    dialog.hide();

    // user decided to exit app without saving
    if (response == Gtk::RESPONSE_NO) return true;

    // user cancelled dialog, thus don't close app
    if (response == Gtk::RESPONSE_CANCEL) return false;

    // TODO: the following return valid is disabled and hard coded instead for
    // now, due to the fact that saving with progress bar is now implemented
    // asynchronously, as a result the app does not close automatically anymore
    // after saving the file has completed
    //
    //   if (response == Gtk::RESPONSE_YES) return file_save();
    //   return response != Gtk::RESPONSE_CANCEL;
    //
    if (response == Gtk::RESPONSE_YES) file_save();
    return false; // always prevent closing the app for now (see comment above)
}

bool MainWindow::leaving_shared_mode_dialog() {
    Glib::ustring msg = _("Detach from sampler and proceed working stand-alone?");
    Gtk::MessageDialog dialog(*this, msg, false, Gtk::MESSAGE_WARNING, Gtk::BUTTONS_NONE);
    dialog.set_secondary_text(
        _("If you proceed to work on another instrument file, it won't be "
          "used by the sampler until you tell the sampler explicitly to "
          "load it."));
    dialog.add_button(_("_Yes, Detach"), Gtk::RESPONSE_YES);
#if HAS_GTKMM_STOCK
    dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
#else
    dialog.add_button(_("_Cancel"), Gtk::RESPONSE_CANCEL);
#endif
    dialog.set_default_response(Gtk::RESPONSE_CANCEL);
    int response = dialog.run();
    dialog.hide();
    return response == Gtk::RESPONSE_YES;
}

void MainWindow::on_action_file_open()
{
    if (!file_is_shared && file_is_changed && !close_confirmation_dialog()) return;

    if (file_is_shared && !leaving_shared_mode_dialog()) return;

    Gtk::FileChooserDialog dialog(*this, _("Open file"));
#if HAS_GTKMM_STOCK
    dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
    dialog.add_button(Gtk::Stock::OPEN, Gtk::RESPONSE_OK);
#else
    dialog.add_button(_("_Cancel"), Gtk::RESPONSE_CANCEL);
    dialog.add_button(_("_Open"), Gtk::RESPONSE_OK);
#endif
    dialog.set_default_response(Gtk::RESPONSE_OK);
#if (GTKMM_MAJOR_VERSION == 2 && GTKMM_MINOR_VERSION < 90) || GTKMM_MAJOR_VERSION < 2
    Gtk::FileFilter filter;
    filter.add_pattern("*.gig");
#else
    Glib::RefPtr<Gtk::FileFilter> filter = Gtk::FileFilter::create();
    filter->add_pattern("*.gig");
#endif
    dialog.set_filter(filter);
    if (current_gig_dir != "") {
        dialog.set_current_folder(current_gig_dir);
    }
    if (dialog.run() == Gtk::RESPONSE_OK) {
        dialog.hide();
        std::string filename = dialog.get_filename();
        printf("filename=%s\n", filename.c_str());
#ifdef GLIB_THREADS
        printf("on_action_file_open self=%p\n",
               static_cast<void*>(Glib::Threads::Thread::self()));
#else
        std::cout << "on_action_file_open self=" <<
            std::this_thread::get_id() << "\n";
#endif
        load_file(filename.c_str());
        current_gig_dir = Glib::path_get_dirname(filename);
    }
}

void MainWindow::load_file(const char* name)
{
    __clear();

    progress_dialog = new ProgressDialog( //FIXME: memory leak!
        _("Loading") +  Glib::ustring(" '") +
        Glib::filename_display_basename(name) + "' ...",
        *this
    );
#if HAS_GTKMM_SHOW_ALL_CHILDREN
    progress_dialog->show_all();
#endif
    loader = new Loader(name); //FIXME: memory leak!
    loader->signal_progress().connect(
        sigc::mem_fun(*this, &MainWindow::on_loader_progress));
    loader->signal_finished().connect(
        sigc::mem_fun(*this, &MainWindow::on_loader_finished));
    loader->signal_error().connect(
        sigc::mem_fun(*this, &MainWindow::on_loader_error));
    loader->launch();
}

void MainWindow::load_instrument(gig::Instrument* instr) {
    if (!instr) {
        Glib::ustring txt = "Provided instrument is NULL!\n";
        Gtk::MessageDialog msg(*this, txt, false, Gtk::MESSAGE_ERROR);
        msg.run();
        Gtk::Main::quit();
    }
    // clear all GUI elements
    __clear();
    // load the instrument
    gig::File* pFile = (gig::File*) instr->GetParent();
    load_gig(pFile, 0 /*file name*/, true /*shared instrument*/);
    // automatically select the given instrument
    int i = 0;
    for (gig::Instrument* instrument = pFile->GetFirstInstrument(); instrument;
         instrument = pFile->GetNextInstrument(), ++i)
    {
        if (instrument == instr) {
            // select item in "instruments" tree view
            m_TreeView.get_selection()->select(Gtk::TreePath(ToString(i)));
            // make sure the selected item in the "instruments" tree view is
            // visible (scroll to it)
            m_TreeView.scroll_to_row(Gtk::TreePath(ToString(i)));
#if !USE_GTKMM_BUILDER
            // select item in instrument menu
            {
                const std::vector<Gtk::Widget*> children =
                    instrument_menu->get_children();
                static_cast<Gtk::RadioMenuItem*>(children[i])->set_active();
            }
#endif
            // update region chooser and dimension region chooser
            m_RegionChooser.set_instrument(instr);
            break;
        }
    }
}

void MainWindow::on_loader_progress()
{
    progress_dialog->set_fraction(loader->get_progress());
}

void MainWindow::on_loader_finished()
{
    loader->join();
    printf("Loader finished!\n");
#ifdef GLIB_THREADS
    printf("on_loader_finished self=%p\n",
           static_cast<void*>(Glib::Threads::Thread::self()));
#else
    std::cout << "on_loader_finished self=" <<
        std::this_thread::get_id() << "\n";
#endif
    load_gig(loader->gig, loader->filename.c_str());
    progress_dialog->hide();
}

void MainWindow::on_loader_error()
{
    loader->join();
    Glib::ustring txt = _("Could not load file: ") + loader->error_message;
    Gtk::MessageDialog msg(*this, txt, false, Gtk::MESSAGE_ERROR);
    msg.run();
    progress_dialog->hide();
}

void MainWindow::on_action_file_save()
{
    file_save();
}

bool MainWindow::check_if_savable()
{
    if (!file) return false;

    if (!file->GetFirstSample()) {
        Gtk::MessageDialog(*this, _("The file could not be saved "
                                    "because it contains no samples"),
                           false, Gtk::MESSAGE_ERROR).run();
        return false;
    }

    for (gig::Instrument* instrument = file->GetFirstInstrument() ; instrument ;
         instrument = file->GetNextInstrument()) {
        if (!instrument->GetFirstRegion()) {
            Gtk::MessageDialog(*this, _("The file could not be saved "
                                        "because there are instruments "
                                        "that have no regions"),
                               false, Gtk::MESSAGE_ERROR).run();
            return false;
        }
    }
    return true;
}

bool MainWindow::file_save()
{
    if (!check_if_savable()) return false;
    if (!file_is_shared && !file_has_name) return file_save_as();

    std::cout << "Saving file\n" << std::flush;
    file_structure_to_be_changed_signal.emit(this->file);

    progress_dialog = new ProgressDialog( //FIXME: memory leak!
        _("Saving") +  Glib::ustring(" '") +
        Glib::filename_display_basename(this->filename) + "' ...",
        *this
    );
#if HAS_GTKMM_SHOW_ALL_CHILDREN
    progress_dialog->show_all();
#endif
    saver = new Saver(this->file); //FIXME: memory leak!
    saver->signal_progress().connect(
        sigc::mem_fun(*this, &MainWindow::on_saver_progress));
    saver->signal_finished().connect(
        sigc::mem_fun(*this, &MainWindow::on_saver_finished));
    saver->signal_error().connect(
        sigc::mem_fun(*this, &MainWindow::on_saver_error));
    saver->launch();

    return true;
}

void MainWindow::on_saver_progress()
{
    progress_dialog->set_fraction(saver->get_progress());
}

void MainWindow::on_saver_error()
{
    saver->join();
    file_structure_changed_signal.emit(this->file);
    Glib::ustring txt = _("Could not save file: ") + saver->error_message;
    Gtk::MessageDialog msg(*this, txt, false, Gtk::MESSAGE_ERROR);
    msg.run();
}

void MainWindow::on_saver_finished()
{
    saver->join();
    this->file = saver->gig;
    this->filename = saver->filename;
    current_gig_dir = Glib::path_get_dirname(filename);
    set_title(Glib::filename_display_basename(filename));
    file_has_name = true;
    file_is_changed = false;
    std::cout << "Saving file done. Importing queued samples now ...\n" << std::flush;
    __import_queued_samples();
    std::cout << "Importing queued samples done.\n" << std::flush;

    file_structure_changed_signal.emit(this->file);

    __refreshEntireGUI();
    progress_dialog->hide();
}

void MainWindow::on_action_file_save_as()
{
    if (!check_if_savable()) return;
    file_save_as();
}

bool MainWindow::file_save_as()
{
    Gtk::FileChooserDialog dialog(*this, _("Save as"), Gtk::FILE_CHOOSER_ACTION_SAVE);
#if HAS_GTKMM_STOCK
    dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
    dialog.add_button(Gtk::Stock::SAVE, Gtk::RESPONSE_OK);
#else
    dialog.add_button(_("_Cancel"), Gtk::RESPONSE_CANCEL);
    dialog.add_button(_("_Save"), Gtk::RESPONSE_OK);
#endif
    dialog.set_default_response(Gtk::RESPONSE_OK);
    dialog.set_do_overwrite_confirmation();

#if (GTKMM_MAJOR_VERSION == 2 && GTKMM_MINOR_VERSION < 90) || GTKMM_MAJOR_VERSION < 2
    Gtk::FileFilter filter;
    filter.add_pattern("*.gig");
#else
    Glib::RefPtr<Gtk::FileFilter> filter = Gtk::FileFilter::create();
    filter->add_pattern("*.gig");
#endif
    dialog.set_filter(filter);

    // set initial dir and filename of the Save As dialog
    // and prepare that initial filename as a copy of the gig
    {
        std::string basename = Glib::path_get_basename(filename);
        std::string dir = Glib::path_get_dirname(filename);
        basename = std::string(_("copy_of_")) + basename;
        Glib::ustring copyFileName = Glib::build_filename(dir, basename);
        if (Glib::path_is_absolute(filename)) {
            dialog.set_filename(copyFileName);
        } else {
            if (current_gig_dir != "") dialog.set_current_folder(current_gig_dir);
        }
        dialog.set_current_name(Glib::filename_display_basename(copyFileName));
    }

    // show warning in the dialog
    HBox descriptionArea;
    descriptionArea.set_spacing(15);
    Gtk::Image warningIcon;
    warningIcon.set_from_icon_name("dialog-warning",
                                   Gtk::IconSize(Gtk::ICON_SIZE_DIALOG));
    descriptionArea.pack_start(warningIcon, Gtk::PACK_SHRINK);
#if GTKMM_MAJOR_VERSION < 3
    view::WrapLabel description;
#else
    Gtk::Label description;
    description.set_line_wrap();
#endif
    description.set_markup(
        _("\n<b>CAUTION:</b> You <b>MUST</b> use the "
          "<span style=\"italic\">\"Save\"</span> dialog instead of "
          "<span style=\"italic\">\"Save As...\"</span> if you want to save "
          "to the same .gig file. Using "
          "<span style=\"italic\">\"Save As...\"</span> for writing to the "
          "same .gig file will end up in corrupted sample wave data!\n")
    );
    descriptionArea.pack_start(description);
#if USE_GTKMM_BOX
    dialog.get_content_area()->pack_start(descriptionArea, Gtk::PACK_SHRINK);
#else
    dialog.get_vbox()->pack_start(descriptionArea, Gtk::PACK_SHRINK);
#endif
#if HAS_GTKMM_SHOW_ALL_CHILDREN
    descriptionArea.show_all();
#endif

    if (dialog.run() == Gtk::RESPONSE_OK) {
        dialog.hide();
        std::string filename = dialog.get_filename();
        if (!Glib::str_has_suffix(filename, ".gig")) {
            filename += ".gig";
        }
        printf("filename=%s\n", filename.c_str());

        progress_dialog = new ProgressDialog( //FIXME: memory leak!
            _("Saving") +  Glib::ustring(" '") +
            Glib::filename_display_basename(filename) + "' ...",
            *this
        );
#if HAS_GTKMM_SHOW_ALL_CHILDREN
        progress_dialog->show_all();
#endif

        saver = new Saver(file, filename); //FIXME: memory leak!
        saver->signal_progress().connect(
            sigc::mem_fun(*this, &MainWindow::on_saver_progress));
        saver->signal_finished().connect(
            sigc::mem_fun(*this, &MainWindow::on_saver_finished));
        saver->signal_error().connect(
            sigc::mem_fun(*this, &MainWindow::on_saver_error));
        saver->launch();

        return true;
    }
    return false;
}

// actually write the sample(s)' data to the gig file
void MainWindow::__import_queued_samples() {
    std::cout << "Starting sample import\n" << std::flush;
    Glib::ustring error_files;
    printf("Samples to import: %d\n", int(m_SampleImportQueue.size()));
    for (std::map<gig::Sample*, SampleImportItem>::iterator iter = m_SampleImportQueue.begin();
         iter != m_SampleImportQueue.end(); ) {
        printf("Importing sample %s\n",iter->second.sample_path.c_str());
        SF_INFO info;
        info.format = 0;
        SNDFILE* hFile = sf_open(iter->second.sample_path.c_str(), SFM_READ, &info);
        sf_command(hFile, SFC_SET_SCALE_FLOAT_INT_READ, 0, SF_TRUE);
        try {
            if (!hFile) throw std::string(_("could not open file"));
            // determine sample's bit depth
            int bitdepth;
            switch (info.format & 0xff) {
                case SF_FORMAT_PCM_S8:
                case SF_FORMAT_PCM_16:
                case SF_FORMAT_PCM_U8:
                    bitdepth = 16;
                    break;
                case SF_FORMAT_PCM_24:
                case SF_FORMAT_PCM_32:
                case SF_FORMAT_FLOAT:
                case SF_FORMAT_DOUBLE:
                    bitdepth = 24;
                    break;
                default:
                    sf_close(hFile); // close sound file
                    throw std::string(_("format not supported")); // unsupported subformat (yet?)
            }

            // reset write position for sample
            iter->first->SetPos(0);

            const int bufsize = 10000;
            switch (bitdepth) {
                case 16: {
                    short* buffer = new short[bufsize * info.channels];
                    sf_count_t cnt = info.frames;
                    while (cnt) {
                        // libsndfile does the conversion for us (if needed)
                        int n = sf_readf_short(hFile, buffer, bufsize);
                        // write from buffer directly (physically) into .gig file
                        iter->first->Write(buffer, n);
                        cnt -= n;
                    }
                    delete[] buffer;
                    break;
                }
                case 24: {
                    int* srcbuf = new int[bufsize * info.channels];
                    uint8_t* dstbuf = new uint8_t[bufsize * 3 * info.channels];
                    sf_count_t cnt = info.frames;
                    while (cnt) {
                        // libsndfile returns 32 bits, convert to 24
                        int n = sf_readf_int(hFile, srcbuf, bufsize);
                        int j = 0;
                        for (int i = 0 ; i < n * info.channels ; i++) {
                            dstbuf[j++] = srcbuf[i] >> 8;
                            dstbuf[j++] = srcbuf[i] >> 16;
                            dstbuf[j++] = srcbuf[i] >> 24;
                        }
                        // write from buffer directly (physically) into .gig file
                        iter->first->Write(dstbuf, n);
                        cnt -= n;
                    }
                    delete[] srcbuf;
                    delete[] dstbuf;
                    break;
                }
            }
            // cleanup
            sf_close(hFile);
            // let the sampler re-cache the sample if needed
            sample_changed_signal.emit(iter->first);
            // on success we remove the sample from the import queue,
            // otherwise keep it, maybe it works the next time ?
            std::map<gig::Sample*, SampleImportItem>::iterator cur = iter;
            ++iter;
            m_SampleImportQueue.erase(cur);
        } catch (std::string what) {
            // remember the files that made trouble (and their cause)
            if (!error_files.empty()) error_files += "\n";
            error_files += iter->second.sample_path += " (" + what + ")";
            ++iter;
        }
    }
    // show error message box when some sample(s) could not be imported
    if (!error_files.empty()) {
        Glib::ustring txt = _("Could not import the following sample(s):\n") + error_files;
        Gtk::MessageDialog msg(*this, txt, false, Gtk::MESSAGE_ERROR);
        msg.run();
    }
}

void MainWindow::on_action_file_properties()
{
    fileProps.show();
    fileProps.deiconify();
}

void MainWindow::on_action_warn_user_on_extensions() {
    Settings::singleton()->warnUserOnExtensions =
        !Settings::singleton()->warnUserOnExtensions;
}

void MainWindow::on_action_show_tooltips() {
    Settings::singleton()->showTooltips =
        !Settings::singleton()->showTooltips;

    on_show_tooltips_changed();
}

void MainWindow::on_show_tooltips_changed() {
    const bool b = Settings::singleton()->showTooltips;

    dimreg_label.set_has_tooltip(b);
    dimreg_all_regions.set_has_tooltip(b);
    dimreg_all_dimregs.set_has_tooltip(b);
    dimreg_stereo.set_has_tooltip(b);

    // Not doing this here, we let onQueryTreeViewTooltip() handle this per cell
    //m_TreeView.set_has_tooltip(b);

    m_TreeViewSamples.set_has_tooltip(b);
    m_TreeViewScripts.set_has_tooltip(b);

    set_has_tooltip(b);
}

void MainWindow::on_action_sync_sampler_instrument_selection() {
    Settings::singleton()->syncSamplerInstrumentSelection =
        !Settings::singleton()->syncSamplerInstrumentSelection;
}

void MainWindow::on_action_move_root_note_with_region_moved() {
    Settings::singleton()->moveRootNoteWithRegionMoved =
        !Settings::singleton()->moveRootNoteWithRegionMoved;
}

void MainWindow::on_action_help_about()
{
    Gtk::AboutDialog dialog;
#if (GTKMM_MAJOR_VERSION == 2 && GTKMM_MINOR_VERSION >= 12) || GTKMM_MAJOR_VERSION > 2
    dialog.set_program_name("Gigedit");
#else
    dialog.set_name("Gigedit");
#endif
    dialog.set_version(VERSION);
    dialog.set_copyright("Copyright (C) 2006-2019 Andreas Persson");
    const std::string sComment =
        _("Built " __DATE__ "\nUsing ") +
        ::gig::libraryName() + " " + ::gig::libraryVersion() + "\n\n" +
        _(
            "Gigedit is released under the GNU General Public License.\n"
            "\n"
            "This program is distributed WITHOUT ANY WARRANTY; So better "
            "backup your Gigasampler/GigaStudio files before editing them with "
            "this application.\n"
            "\n"
            "Please report bugs to: https://bugs.linuxsampler.org"
        );
    dialog.set_comments(sComment.c_str());
    dialog.set_website("https://www.linuxsampler.org");
    dialog.set_website_label("https://www.linuxsampler.org");
    dialog.set_position(Gtk::WIN_POS_CENTER);
    dialog.run();
}

FilePropDialog::FilePropDialog()
    : eFileFormat(_("File Format")),
      eName(_("Name")),
      eCreationDate(_("Creation date")),
      eComments(_("Comments")),
      eProduct(_("Product")),
      eCopyright(_("Copyright")),
      eArtists(_("Artists")),
      eGenre(_("Genre")),
      eKeywords(_("Keywords")),
      eEngineer(_("Engineer")),
      eTechnician(_("Technician")),
      eSoftware(_("Software")),
      eMedium(_("Medium")),
      eSource(_("Source")),
      eSourceForm(_("Source form")),
      eCommissioned(_("Commissioned")),
      eSubject(_("Subject")),
#if HAS_GTKMM_STOCK
      quitButton(Gtk::Stock::CLOSE),
#else
      quitButton(_("_Close")),
#endif
      table(2, 1),
      m_file(NULL)
{
    if (!Settings::singleton()->autoRestoreWindowDimension) {
        set_default_size(470, 390);
        set_position(Gtk::WIN_POS_MOUSE);
    }

    set_title(_("File Properties"));
    eName.set_width_chars(50);

    connect(eFileFormat, &FilePropDialog::set_FileFormat);
    connect(eName, &DLS::Info::Name);
    connect(eCreationDate, &DLS::Info::CreationDate);
    connect(eComments, &DLS::Info::Comments);
    connect(eProduct, &DLS::Info::Product);
    connect(eCopyright, &DLS::Info::Copyright);
    connect(eArtists, &DLS::Info::Artists);
    connect(eGenre, &DLS::Info::Genre);
    connect(eKeywords, &DLS::Info::Keywords);
    connect(eEngineer, &DLS::Info::Engineer);
    connect(eTechnician, &DLS::Info::Technician);
    connect(eSoftware, &DLS::Info::Software);
    connect(eMedium, &DLS::Info::Medium);
    connect(eSource, &DLS::Info::Source);
    connect(eSourceForm, &DLS::Info::SourceForm);
    connect(eCommissioned, &DLS::Info::Commissioned);
    connect(eSubject, &DLS::Info::Subject);

    table.add(eFileFormat);
    table.add(eName);
    table.add(eCreationDate);
    table.add(eComments);
    table.add(eProduct);
    table.add(eCopyright);
    table.add(eArtists);
    table.add(eGenre);
    table.add(eKeywords);
    table.add(eEngineer);
    table.add(eTechnician);
    table.add(eSoftware);
    table.add(eMedium);
    table.add(eSource);
    table.add(eSourceForm);
    table.add(eCommissioned);
    table.add(eSubject);

#if USE_GTKMM_GRID
    table.set_column_spacing(5);
#else
    table.set_col_spacings(5);
#endif

    add(vbox);
#if GTKMM_MAJOR_VERSION > 3 || (GTKMM_MAJOR_VERSION == 3 && GTKMM_MINOR_VERSION > 24)
    table.set_margin(5);
#else
    table.set_border_width(5);
#endif
    vbox.add(table);
    vbox.pack_start(buttonBox, Gtk::PACK_SHRINK);
    buttonBox.set_layout(Gtk::BUTTONBOX_END);
#if GTKMM_MAJOR_VERSION > 3 || (GTKMM_MAJOR_VERSION == 3 && GTKMM_MINOR_VERSION > 24)
    buttonBox.set_margin(5);
#else
    buttonBox.set_border_width(5);
#endif
    buttonBox.show();
    buttonBox.pack_start(quitButton);
    quitButton.set_can_default();
    quitButton.grab_focus();
    quitButton.signal_clicked().connect(
        sigc::mem_fun(*this, &FilePropDialog::hide));

    quitButton.show();
    vbox.show();
#if HAS_GTKMM_SHOW_ALL_CHILDREN
    show_all_children();
#endif
}

void FilePropDialog::set_file(gig::File* file)
{
    m_file = file;
    update(file->pInfo);

    // update file format version combo box
    const std::string sGiga = "Gigasampler/GigaStudio v";
    const int major = file->pVersion->major;
    std::vector<std::string> txts;
    std::vector<int> values;
    txts.push_back(sGiga + "2"); values.push_back(2);
    txts.push_back(sGiga + "3"); values.push_back(3);
    txts.push_back(sGiga + "4"); values.push_back(4);
    if (major < 2 || major > 4) {
        txts.push_back(sGiga + ToString(major)); values.push_back(major);
    }
    std::vector<const char*> texts;
    for (int i = 0; i < txts.size(); ++i) texts.push_back(txts[i].c_str());
    texts.push_back(NULL); values.push_back(0);

    update_model++;
    eFileFormat.set_choices(&texts[0], &values[0]);
    eFileFormat.set_value(major);
    update_model--;
}

void FilePropDialog::set_FileFormat(int value)
{
    m_file->pVersion->major = value;
}


void InstrumentProps::set_Name(const gig::String& name)
{
    m->pInfo->Name = name;
}

void InstrumentProps::update_name()
{
    update_model++;
    eName.set_value(m->pInfo->Name);
    update_model--;
}

void InstrumentProps::set_IsDrum(bool value)
{
    m->IsDrum = value;
}

void InstrumentProps::set_MIDIBank(uint16_t value)
{
    m->MIDIBank = value;
}

void InstrumentProps::set_MIDIProgram(uint32_t value)
{
    m->MIDIProgram = value;
}

InstrumentProps::InstrumentProps() :
#if HAS_GTKMM_STOCK
    quitButton(Gtk::Stock::CLOSE),
#else
    quitButton(_("_Close")),
#endif
    table(2,1),
    eName(_("Name")),
    eIsDrum(_("Is drum")),
    eMIDIBank(_("MIDI bank"), 0, 16383),
    eMIDIProgram(_("MIDI program")),
    eAttenuation(_("Attenuation (dB)"), -96, +96, 0, 1),
    eEffectSend(_("Effect send"), 0, 65535),
    eFineTune(_("Fine tune"), -8400, 8400),
    ePitchbendRange(_("Pitchbend range (halftones)"), 0, 48),
    ePianoReleaseMode(_("Piano release mode")),
    eDimensionKeyRangeLow(_("Keyswitching range low")),
    eDimensionKeyRangeHigh(_("Keyswitching range high")),
    table2(2,1),
    eName2(_("Name")),
    eCreationDate(_("Creation date")),
    eComments(_("Comments")),
    eProduct(_("Product")),
    eCopyright(_("Copyright")),
    eArtists(_("Artists")),
    eGenre(_("Genre")),
    eKeywords(_("Keywords")),
    eEngineer(_("Engineer")),
    eTechnician(_("Technician")),
    eSoftware(_("Software")),
    eMedium(_("Medium")),
    eSource(_("Source")),
    eSourceForm(_("Source form")),
    eCommissioned(_("Commissioned")),
    eSubject(_("Subject"))
{
    if (!Settings::singleton()->autoRestoreWindowDimension) {
        //set_default_size(470, 390);
        set_position(Gtk::WIN_POS_MOUSE);
    }

    set_title(_("Instrument Properties"));

    tabs.append_page(vbox[1], _("Settings"));
    tabs.append_page(vbox[2], _("Info"));

    eDimensionKeyRangeLow.set_tip(
        _("start of the keyboard area which should switch the "
          "\"keyswitching\" dimension")
    );
    eDimensionKeyRangeHigh.set_tip(
        _("end of the keyboard area which should switch the "
          "\"keyswitching\" dimension")
    );

    connect(eName, &InstrumentProps::set_Name);
    connect(eIsDrum, &InstrumentProps::set_IsDrum);
    connect(eMIDIBank, &InstrumentProps::set_MIDIBank);
    connect(eMIDIProgram, &InstrumentProps::set_MIDIProgram);
    connect(eAttenuation, &gig::Instrument::Attenuation);
    connect(eEffectSend, &gig::Instrument::EffectSend);
    connect(eFineTune, &gig::Instrument::FineTune);
    connect(ePitchbendRange, &gig::Instrument::PitchbendRange);
    connect(ePianoReleaseMode, &gig::Instrument::PianoReleaseMode);
    connect(eDimensionKeyRangeLow, eDimensionKeyRangeHigh,
            &gig::Instrument::DimensionKeyRange);

    eName.signal_value_changed().connect(sig_name_changed.make_slot());

    connect(eName2, &InstrumentProps::set_Name);
    connectLambda(eCreationDate, [this](gig::String s) {
        m->pInfo->CreationDate = s;
    });
    connectLambda(eComments, [this](gig::String s) {
        m->pInfo->Comments = s;
    });
    connectLambda(eProduct, [this](gig::String s) {
        m->pInfo->Product = s;
    });
    connectLambda(eCopyright, [this](gig::String s) {
        m->pInfo->Copyright = s;
    });
    connectLambda(eArtists, [this](gig::String s) {
        m->pInfo->Artists = s;
    });
    connectLambda(eGenre, [this](gig::String s) {
        m->pInfo->Genre = s;
    });
    connectLambda(eKeywords, [this](gig::String s) {
        m->pInfo->Keywords = s;
    });
    connectLambda(eEngineer, [this](gig::String s) {
        m->pInfo->Engineer = s;
    });
    connectLambda(eTechnician, [this](gig::String s) {
        m->pInfo->Technician = s;
    });
    connectLambda(eSoftware, [this](gig::String s) {
        m->pInfo->Software = s;
    });
    connectLambda(eMedium, [this](gig::String s) {
        m->pInfo->Medium = s;
    });
    connectLambda(eSource, [this](gig::String s) {
        m->pInfo->Source = s;
    });
    connectLambda(eSourceForm, [this](gig::String s) {
        m->pInfo->SourceForm = s;
    });
    connectLambda(eCommissioned, [this](gig::String s) {
        m->pInfo->Commissioned = s;
    });
    connectLambda(eSubject, [this](gig::String s) {
        m->pInfo->Subject = s;
    });

    // tab 1
#if USE_GTKMM_GRID
    table.set_column_spacing(5);
#else
    table.set_col_spacings(5);
#endif
    table.add(eName);
    table.add(eIsDrum);
    table.add(eMIDIBank);
    table.add(eMIDIProgram);
    table.add(eAttenuation);
    table.add(eEffectSend);
    table.add(eFineTune);
    table.add(ePitchbendRange);
    table.add(ePianoReleaseMode);
    table.add(eDimensionKeyRangeLow);
    table.add(eDimensionKeyRangeHigh);

    // tab 2
#if USE_GTKMM_GRID
    table2.set_column_spacing(5);
#else
    table2.set_col_spacings(5);
#endif
    table2.add(eName2);
    table2.add(eCreationDate);
    table2.add(eComments);
    table2.add(eProduct);
    table2.add(eCopyright);
    table2.add(eArtists);
    table2.add(eGenre);
    table2.add(eKeywords);
    table2.add(eEngineer);
    table2.add(eTechnician);
    table2.add(eSoftware);
    table2.add(eMedium);
    table2.add(eSource);
    table2.add(eSourceForm);
    table2.add(eCommissioned);
    table2.add(eSubject);

    add(vbox[0]);
#if GTKMM_MAJOR_VERSION > 3 || (GTKMM_MAJOR_VERSION == 3 && GTKMM_MINOR_VERSION > 24)
    table.set_margin(5);
#else
    table.set_border_width(5);
#endif
    vbox[1].pack_start(table);
    vbox[2].pack_start(table2);
    table.show();
    table2.show();
    vbox[0].pack_start(tabs);
    vbox[0].pack_start(buttonBox, Gtk::PACK_SHRINK);
    buttonBox.set_layout(Gtk::BUTTONBOX_END);
#if GTKMM_MAJOR_VERSION > 3 || (GTKMM_MAJOR_VERSION == 3 && GTKMM_MINOR_VERSION > 24)
    buttonBox.set_margin(5);
#else
    buttonBox.set_border_width(5);
#endif
    buttonBox.show();
    buttonBox.pack_start(quitButton);
    quitButton.set_can_default();
    quitButton.grab_focus();

    quitButton.signal_clicked().connect(
        sigc::mem_fun(*this, &InstrumentProps::hide));

    quitButton.show();
    vbox[0].show();
#if HAS_GTKMM_SHOW_ALL_CHILDREN
    show_all_children();
#endif
}

void InstrumentProps::set_instrument(gig::Instrument* instrument)
{
    update(instrument);

    update_model++;

    // tab 1
    eName.set_value(instrument->pInfo->Name);
    eIsDrum.set_value(instrument->IsDrum);
    eMIDIBank.set_value(instrument->MIDIBank);
    eMIDIProgram.set_value(instrument->MIDIProgram);
    // tab 2
    eName2.set_value(instrument->pInfo->Name);
    eCreationDate.set_value(instrument->pInfo->CreationDate);
    eComments.set_value(instrument->pInfo->Comments);
    eProduct.set_value(instrument->pInfo->Product);
    eCopyright.set_value(instrument->pInfo->Copyright);
    eArtists.set_value(instrument->pInfo->Artists);
    eGenre.set_value(instrument->pInfo->Genre);
    eKeywords.set_value(instrument->pInfo->Keywords);
    eEngineer.set_value(instrument->pInfo->Engineer);
    eTechnician.set_value(instrument->pInfo->Technician);
    eSoftware.set_value(instrument->pInfo->Software);
    eMedium.set_value(instrument->pInfo->Medium);
    eSource.set_value(instrument->pInfo->Source);
    eSourceForm.set_value(instrument->pInfo->SourceForm);
    eCommissioned.set_value(instrument->pInfo->Commissioned);
    eSubject.set_value(instrument->pInfo->Subject);

    update_model--;
}


SampleProps::SampleProps() :
#if HAS_GTKMM_STOCK
    quitButton(Gtk::Stock::CLOSE),
#else
    quitButton(_("_Close")),
#endif
    table(2,1),
    eName(_("Name")),
    eUnityNote(_("Unity Note")),
    eSampleGroup(_("Sample Group")),
    eSampleFormatInfo(_("Sample Format")),
    eSampleID("Sample ID"),
    eChecksum("Wave Data CRC-32"),
    eLoopsCount(_("Loops"), 0, 1), // we might support more than 1 loop in future
    eLoopStart(_("Loop start position"), 0, 9999999),
    eLoopLength(_("Loop size"), 0, 9999999),
    eLoopType(_("Loop type")),
    eLoopPlayCount(_("Playback count")),
    table2(2,1),
    eName2(_("Name")),
    eCreationDate(_("Creation date")),
    eComments(_("Comments")),
    eProduct(_("Product")),
    eCopyright(_("Copyright")),
    eArtists(_("Artists")),
    eGenre(_("Genre")),
    eKeywords(_("Keywords")),
    eEngineer(_("Engineer")),
    eTechnician(_("Technician")),
    eSoftware(_("Software")),
    eMedium(_("Medium")),
    eSource(_("Source")),
    eSourceForm(_("Source form")),
    eCommissioned(_("Commissioned")),
    eSubject(_("Subject"))
{
    if (!Settings::singleton()->autoRestoreWindowDimension) {
        //set_default_size(470, 390);
        set_position(Gtk::WIN_POS_MOUSE);
    }

    set_title(_("Sample Properties"));

    tabs.append_page(vbox[1], _("Settings"));
    tabs.append_page(vbox[2], _("Info"));

    connect(eName, &SampleProps::set_Name);
    connect(eUnityNote, &gig::Sample::MIDIUnityNote);
    connect(eLoopsCount, &gig::Sample::Loops);
    connectLambda(eLoopStart, [this](uint32_t start){
        m->LoopStart = start;
        m->LoopEnd = start + m->LoopSize;
    });
    connectLambda(eLoopLength, [this](uint32_t length){
        m->LoopSize = length;
        m->LoopEnd = m->LoopStart + length;
    });
    {
        const char* choices[] = { _("normal"), _("bidirectional"), _("backward"), 0 };
        static const gig::loop_type_t values[] = {
            gig::loop_type_normal,
            gig::loop_type_bidirectional,
            gig::loop_type_backward
        };
        eLoopType.set_choices(choices, values);
    }
    connect(eLoopType, &gig::Sample::LoopType);
    connect(eLoopPlayCount, &gig::Sample::LoopPlayCount);

    eName.signal_value_changed().connect(sig_name_changed.make_slot());

    connect(eName2, &SampleProps::set_Name);
    connectLambda(eCreationDate, [this](gig::String s) {
        m->pInfo->CreationDate = s;
    });
    connectLambda(eComments, [this](gig::String s) {
        m->pInfo->Comments = s;
    });
    connectLambda(eProduct, [this](gig::String s) {
        m->pInfo->Product = s;
    });
    connectLambda(eCopyright, [this](gig::String s) {
        m->pInfo->Copyright = s;
    });
    connectLambda(eArtists, [this](gig::String s) {
        m->pInfo->Artists = s;
    });
    connectLambda(eGenre, [this](gig::String s) {
        m->pInfo->Genre = s;
    });
    connectLambda(eKeywords, [this](gig::String s) {
        m->pInfo->Keywords = s;
    });
    connectLambda(eEngineer, [this](gig::String s) {
        m->pInfo->Engineer = s;
    });
    connectLambda(eTechnician, [this](gig::String s) {
        m->pInfo->Technician = s;
    });
    connectLambda(eSoftware, [this](gig::String s) {
        m->pInfo->Software = s;
    });
    connectLambda(eMedium, [this](gig::String s) {
        m->pInfo->Medium = s;
    });
    connectLambda(eSource, [this](gig::String s) {
        m->pInfo->Source = s;
    });
    connectLambda(eSourceForm, [this](gig::String s) {
        m->pInfo->SourceForm = s;
    });
    connectLambda(eCommissioned, [this](gig::String s) {
        m->pInfo->Commissioned = s;
    });
    connectLambda(eSubject, [this](gig::String s) {
        m->pInfo->Subject = s;
    });

    // tab 1
#if USE_GTKMM_GRID
    table.set_column_spacing(5);
#else
    table.set_col_spacings(5);
#endif
    table.add(eName);
    table.add(eUnityNote);
    table.add(eSampleGroup);
    table.add(eSampleFormatInfo);
    table.add(eSampleID);
    table.add(eChecksum);
    table.add(eLoopsCount);
    table.add(eLoopStart);
    table.add(eLoopLength);
    table.add(eLoopType);
    table.add(eLoopPlayCount);

    // tab 2
#if USE_GTKMM_GRID
    table2.set_column_spacing(5);
#else
    table2.set_col_spacings(5);
#endif
    table2.add(eName2);
    table2.add(eCreationDate);
    table2.add(eComments);
    table2.add(eProduct);
    table2.add(eCopyright);
    table2.add(eArtists);
    table2.add(eGenre);
    table2.add(eKeywords);
    table2.add(eEngineer);
    table2.add(eTechnician);
    table2.add(eSoftware);
    table2.add(eMedium);
    table2.add(eSource);
    table2.add(eSourceForm);
    table2.add(eCommissioned);
    table2.add(eSubject);

    add(vbox[0]);
#if GTKMM_MAJOR_VERSION > 3 || (GTKMM_MAJOR_VERSION == 3 && GTKMM_MINOR_VERSION > 24)
    table.set_margin(5);
#else
    table.set_border_width(5);
#endif
    vbox[1].pack_start(table);
    vbox[2].pack_start(table2);
    table.show();
    table2.show();
    vbox[0].pack_start(tabs);
    vbox[0].pack_start(buttonBox, Gtk::PACK_SHRINK);
    buttonBox.set_layout(Gtk::BUTTONBOX_END);
#if GTKMM_MAJOR_VERSION > 3 || (GTKMM_MAJOR_VERSION == 3 && GTKMM_MINOR_VERSION > 24)
    buttonBox.set_margin(5);
#else
    buttonBox.set_border_width(5);
#endif
    buttonBox.show();
    buttonBox.pack_start(quitButton);
    quitButton.set_can_default();
    quitButton.grab_focus();

    quitButton.signal_clicked().connect(
        sigc::mem_fun(*this, &SampleProps::hide));

    quitButton.show();
    vbox[0].show();
#if HAS_GTKMM_SHOW_ALL_CHILDREN
    show_all_children();
#endif
}

void SampleProps::set_sample(gig::Sample* sample)
{
    update(sample);

    update_model++;

    // tab 1
    eName.set_value(sample->pInfo->Name);
    eUnityNote.set_value(sample->MIDIUnityNote);
    // show sample group name
    {
        Glib::ustring s = "---";
        if (sample && sample->GetGroup())
            s = sample->GetGroup()->Name;
        eSampleGroup.text.set_text(s);
    }
    // assemble sample format info string
    {
        Glib::ustring s;
        if (sample) {
            switch (sample->Channels) {
                case 1: s = _("Mono"); break;
                case 2: s = _("Stereo"); break;
                default:
                    s = ToString(sample->Channels) + _(" audio channels");
                    break;
            }
            s += " " + ToString(sample->BitDepth) + " Bits";
            s += " " + ToString(sample->SamplesPerSecond/1000) + "."
                     + ToString((sample->SamplesPerSecond%1000)/100) + " kHz";
        } else {
            s = _("No sample assigned to this dimension region.");
        }
        eSampleFormatInfo.text.set_text(s);
    }
    // generate sample's memory address pointer string
    {
        Glib::ustring s;
        if (sample) {
            char buf[64] = {};
            snprintf(buf, sizeof(buf), "%p", sample);
            s = buf;
        } else {
            s = "---";
        }
        eSampleID.text.set_text(s);
    }
    // generate raw wave form data CRC-32 checksum string
    {
        Glib::ustring s = "---";
        if (sample) {
            char buf[64] = {};
            snprintf(buf, sizeof(buf), "%x", sample->GetWaveDataCRC32Checksum());
            s = buf;
        }
        eChecksum.text.set_text(s);
    }
    eLoopsCount.set_value(sample->Loops);
    eLoopStart.set_value(sample->LoopStart);
    eLoopLength.set_value(sample->LoopSize);
    eLoopType.set_value(sample->LoopType);
    eLoopPlayCount.set_value(sample->LoopPlayCount);
    // tab 2
    eName2.set_value(sample->pInfo->Name);
    eCreationDate.set_value(sample->pInfo->CreationDate);
    eComments.set_value(sample->pInfo->Comments);
    eProduct.set_value(sample->pInfo->Product);
    eCopyright.set_value(sample->pInfo->Copyright);
    eArtists.set_value(sample->pInfo->Artists);
    eGenre.set_value(sample->pInfo->Genre);
    eKeywords.set_value(sample->pInfo->Keywords);
    eEngineer.set_value(sample->pInfo->Engineer);
    eTechnician.set_value(sample->pInfo->Technician);
    eSoftware.set_value(sample->pInfo->Software);
    eMedium.set_value(sample->pInfo->Medium);
    eSource.set_value(sample->pInfo->Source);
    eSourceForm.set_value(sample->pInfo->SourceForm);
    eCommissioned.set_value(sample->pInfo->Commissioned);
    eSubject.set_value(sample->pInfo->Subject);

    update_model--;
}

void SampleProps::set_Name(const gig::String& name)
{
    m->pInfo->Name = name;
}

void SampleProps::update_name()
{
    update_model++;
    eName.set_value(m->pInfo->Name);
    update_model--;
}


void MainWindow::file_changed()
{
    if (file && !file_is_changed) {
        set_title("*" + get_title());
        file_is_changed = true;
    }
}

void MainWindow::updateSampleRefCountMap(gig::File* gig) {
    sample_ref_count.clear();
    
    if (!gig) return;

    for (gig::Instrument* instrument = gig->GetFirstInstrument(); instrument;
         instrument = gig->GetNextInstrument())
    {
        for (gig::Region* rgn = instrument->GetFirstRegion(); rgn;
             rgn = instrument->GetNextRegion())
        {
            for (int i = 0; i < 256; ++i) {
                if (!rgn->pDimensionRegions[i]) continue;
                if (rgn->pDimensionRegions[i]->pSample) {
                    sample_ref_count[rgn->pDimensionRegions[i]->pSample]++;
                }
            }
        }
    }
}

bool MainWindow::onQueryTreeViewTooltip(int x, int y, bool keyboardTip, const Glib::RefPtr<Gtk::Tooltip>& tooltip) {
    Gtk::TreeModel::iterator iter;
    if (!m_TreeView.get_tooltip_context_iter(x, y, keyboardTip, iter)) {
        return false;
    }
    Gtk::TreeModel::Path path(iter);
    Gtk::TreeModel::Row row = *iter;
    Gtk::TreeViewColumn* pointedColumn = NULL;
    // resolve the precise table column the mouse points to
    {
        Gtk::TreeModel::Path path; // unused
        int cellX, cellY; // unused
        m_TreeView.get_path_at_pos(x, y, path, pointedColumn, cellX, cellY);
    }
    Gtk::TreeViewColumn* scriptsColumn = m_TreeView.get_column(2);
    if (pointedColumn == scriptsColumn) { // mouse hovers scripts column ...
        // show the script(s) assigned to the hovered instrument as tooltip
        tooltip->set_markup( row[m_Columns.m_col_tooltip] );
        m_TreeView.set_tooltip_cell(tooltip, &path, scriptsColumn, NULL);
    } else {
        // if beginners' tooltips is disabled then don't show the following one
        if (!Settings::singleton()->showTooltips)
            return false;
        // yeah, a beginners tooltip
        tooltip->set_text(_(
            "Right click here for actions on instruments & MIDI Rules. "
            "Drag & drop to change the order of instruments."
        ));
        m_TreeView.set_tooltip_cell(tooltip, &path, pointedColumn, NULL);
    }
    return true;
}

static Glib::ustring scriptTooltipFor(gig::Instrument* instrument, int index) {
    Glib::ustring name(gig_to_utf8(instrument->pInfo->Name));
    const int iScriptSlots = instrument->ScriptSlotCount();
    Glib::ustring tooltip = "<u>(" + ToString(index) + ") “"  + name + "”</u>\n\n";
    if (!iScriptSlots)
        tooltip += "<span foreground='red'><i>No script assigned</i></span>";
    else {
        for (int i = 0; i < iScriptSlots; ++i) {
            tooltip += "• " + ToString(i+1) + ". Script: “<span foreground='#46DEFF'><b>" +
                       instrument->GetScriptOfSlot(i)->Name + "</b></span>”";
            if (i + 1 < iScriptSlots) tooltip += "\n\n";
        }
    }
    return tooltip;
}

void MainWindow::load_gig(gig::File* gig, const char* filename, bool isSharedInstrument)
{
    file = 0;
    set_file_is_shared(isSharedInstrument);

    this->filename =
        (filename && strlen(filename) > 0) ?
            filename : (!gig->GetFileName().empty()) ?
                gig->GetFileName() : _("Unsaved Gig File");
    set_title(Glib::filename_display_basename(this->filename));
    file_has_name = filename;
    file_is_changed = false;

    fileProps.set_file(gig);

    instrument_name_connection.block();
    int index = 0;
    for (gig::Instrument* instrument = gig->GetFirstInstrument() ; instrument ;
         instrument = gig->GetNextInstrument(), ++index) {
        Glib::ustring name(gig_to_utf8(instrument->pInfo->Name));
        const int iScriptSlots = instrument->ScriptSlotCount();

        Gtk::TreeModel::iterator iter = m_refTreeModel->append();
        Gtk::TreeModel::Row row = *iter;
        row[m_Columns.m_col_nr] = index;
        row[m_Columns.m_col_name] = name;
        row[m_Columns.m_col_instr] = instrument;
        row[m_Columns.m_col_scripts] = iScriptSlots ? ToString(iScriptSlots) : "";
        row[m_Columns.m_col_tooltip] = scriptTooltipFor(instrument, index);

#if !USE_GTKMM_BUILDER
        add_instrument_to_menu(name);
#endif
    }
    instrument_name_connection.unblock();
#if !USE_GTKMM_BUILDER
    uiManager->get_widget("/MenuBar/MenuInstrument/AllInstruments")->show();
#endif

    updateSampleRefCountMap(gig);

    for (gig::Group* group = gig->GetFirstGroup(); group; group = gig->GetNextGroup()) {
        if (group->Name != "") {
            Gtk::TreeModel::iterator iterGroup = m_refSamplesTreeModel->append();
            Gtk::TreeModel::Row rowGroup = *iterGroup;
            rowGroup[m_SamplesModel.m_col_name]   = gig_to_utf8(group->Name);
            rowGroup[m_SamplesModel.m_col_group]  = group;
            rowGroup[m_SamplesModel.m_col_sample] = NULL;
            for (gig::Sample* sample = group->GetFirstSample();
                 sample; sample = group->GetNextSample()) {
                Gtk::TreeModel::iterator iterSample =
                    m_refSamplesTreeModel->append(rowGroup.children());
                Gtk::TreeModel::Row rowSample = *iterSample;
                rowSample[m_SamplesModel.m_col_name] =
                    gig_to_utf8(sample->pInfo->Name);
                rowSample[m_SamplesModel.m_col_sample] = sample;
                rowSample[m_SamplesModel.m_col_group]  = NULL;
                int refcount = sample_ref_count.count(sample) ? sample_ref_count[sample] : 0;
                rowSample[m_SamplesModel.m_col_refcount] = ToString(refcount) + " " + _("Refs.");
                rowSample[m_SamplesModel.m_color] = refcount ? "black" : "red";
            }
        }
    }
    
    for (int i = 0; gig->GetScriptGroup(i); ++i) {
        gig::ScriptGroup* group = gig->GetScriptGroup(i);

        Gtk::TreeModel::iterator iterGroup = m_refScriptsTreeModel->append();
        Gtk::TreeModel::Row rowGroup = *iterGroup;
        rowGroup[m_ScriptsModel.m_col_name]   = gig_to_utf8(group->Name);
        rowGroup[m_ScriptsModel.m_col_group]  = group;
        rowGroup[m_ScriptsModel.m_col_script] = NULL;
        for (int s = 0; group->GetScript(s); ++s) {
            gig::Script* script = group->GetScript(s);

            Gtk::TreeModel::iterator iterScript =
                m_refScriptsTreeModel->append(rowGroup.children());
            Gtk::TreeModel::Row rowScript = *iterScript;
            rowScript[m_ScriptsModel.m_col_name] = gig_to_utf8(script->Name);
            rowScript[m_ScriptsModel.m_col_script] = script;
            rowScript[m_ScriptsModel.m_col_group]  = NULL;
        }
    }
    // unfold all sample groups & script groups by default
    m_TreeViewSamples.expand_all();
    m_TreeViewScripts.expand_all();

    file = gig;

    // select the first instrument
    m_TreeView.get_selection()->select(Gtk::TreePath("0"));

    instr_props_set_instrument();
    gig::Instrument* instrument = get_instrument();
    if (instrument) {
        midiRules.set_instrument(instrument);
    }
}

bool MainWindow::instr_props_set_instrument()
{
    instrumentProps.signal_name_changed().clear();

    std::vector<Gtk::TreeModel::Path> rows = m_TreeView.get_selection()->get_selected_rows();
    if (rows.empty()) {
        instrumentProps.hide();
        return false;
    }
    //NOTE: was const_iterator before, which did not compile with GTKMM4 development branch, probably going to be fixed before final GTKMM4 release though.
    Gtk::TreeModel::iterator it = m_refTreeModel->get_iter(rows[0]);
    if (it) {
        Gtk::TreeModel::Row row = *it;
        gig::Instrument* instrument = row[m_Columns.m_col_instr];

        instrumentProps.set_instrument(instrument);

        // make sure instrument tree is updated when user changes the
        // instrument name in instrument properties window
        instrumentProps.signal_name_changed().connect(
            sigc::bind(
                sigc::mem_fun(*this, 
                              &MainWindow::instr_name_changed_by_instr_props),
                it));
    } else {
        instrumentProps.hide();
    }
    //NOTE: explicit boolean cast required for GTKMM4 development branch here
    return it ? true : false;
}

void MainWindow::show_instr_props()
{
    if (instr_props_set_instrument()) {
        instrumentProps.show();
        instrumentProps.deiconify();
    }
}

void MainWindow::instr_name_changed_by_instr_props(Gtk::TreeModel::iterator& it)
{
    Gtk::TreeModel::Row row = *it;
    Glib::ustring name = row[m_Columns.m_col_name];

    gig::Instrument* instrument = row[m_Columns.m_col_instr];
    Glib::ustring gigname(gig_to_utf8(instrument->pInfo->Name));
    if (gigname != name) {
        Gtk::TreeModel::Path path(*it);
        const int index = path[0];
        row[m_Columns.m_col_name] = gigname;
        row[m_Columns.m_col_tooltip] = scriptTooltipFor(instrument, index);
    }
}

bool MainWindow::sample_props_set_sample()
{
    sampleProps.signal_name_changed().clear();

    std::vector<Gtk::TreeModel::Path> rows = m_TreeViewSamples.get_selection()->get_selected_rows();
    if (rows.empty()) {
        sampleProps.hide();
        return false;
    }
    //NOTE: was const_iterator before, which did not compile with GTKMM4 development branch, probably going to be fixed before final GTKMM4 release though.
    Gtk::TreeModel::iterator it = m_refSamplesTreeModel->get_iter(rows[0]);
    if (it) {
        Gtk::TreeModel::Row row = *it;
        gig::Sample* sample = row[m_SamplesModel.m_col_sample];

        sampleProps.set_sample(sample);

        // make sure sample tree is updated when user changes the
        // sample name in sample properties window
        sampleProps.signal_name_changed().connect(
            sigc::bind(
                sigc::mem_fun(*this,
                    &MainWindow::sample_name_changed_by_sample_props
                ), it
            )
        );
    } else {
        sampleProps.hide();
    }
    //NOTE: explicit boolean cast required for GTKMM4 development branch here
    return it ? true : false;
}

void MainWindow::show_sample_props()
{
    if (sample_props_set_sample()) {
        sampleProps.show();
        sampleProps.deiconify();
    }
}

void MainWindow::sample_name_changed_by_sample_props(Gtk::TreeModel::iterator& it)
{
    Gtk::TreeModel::Row row = *it;
    Glib::ustring name = row[m_SamplesModel.m_col_name];

    gig::Sample* sample = row[m_SamplesModel.m_col_sample];
    Glib::ustring gigname(gig_to_utf8(sample->pInfo->Name));
    if (gigname != name) {
        Gtk::TreeModel::Path path(*it);
        row[m_SamplesModel.m_col_name] = gigname;
    }
}

void MainWindow::show_midi_rules()
{
    if (gig::Instrument* instrument = get_instrument())
    {
        midiRules.set_instrument(instrument);
        midiRules.show();
        midiRules.deiconify();
    }
}

void MainWindow::show_script_slots() {
    if (!file) return;
    // get selected instrument
    std::vector<Gtk::TreeModel::Path> rows = m_TreeView.get_selection()->get_selected_rows();
    if (rows.empty()) return;
    Gtk::TreeModel::iterator it = m_refTreeModel->get_iter(rows[0]);
    if (!it) return;
    Gtk::TreeModel::Row row = *it;
    gig::Instrument* instrument = row[m_Columns.m_col_instr];
    if (!instrument) return;

    ScriptSlots* window = new ScriptSlots;
    window->setInstrument(instrument);
    window->signal_script_slots_changed().connect(
        sigc::mem_fun(*this, &MainWindow::onScriptSlotsModified)
    );
    //window->reparent(*this);
    window->show();
}

void MainWindow::onScriptSlotsModified(gig::Instrument* pInstrument) {
    if (!pInstrument) return;
    const int iScriptSlots = pInstrument->ScriptSlotCount();

    //NOTE: This is a big mess! Sometimes GTK requires m_TreeView.get_model(), here we need m_refTreeModelFilter->get_model(), otherwise accessing children below causes an error!
    //Glib::RefPtr<Gtk::TreeModel> model = m_TreeView.get_model();
    Glib::RefPtr<Gtk::TreeModel> model = m_refTreeModelFilter->get_model();

    for (int i = 0; i < model->children().size(); ++i) {
        Gtk::TreeModel::Row row = model->children()[i];
        if (row[m_Columns.m_col_instr] != pInstrument) continue;
        row[m_Columns.m_col_scripts] = iScriptSlots ? ToString(iScriptSlots) : "";
        row[m_Columns.m_col_tooltip] = scriptTooltipFor(pInstrument, i);
        break;
    }

    // causes the sampler to reload the instrument with the new script
    on_sel_change();
}

void MainWindow::assignScript(gig::Script* pScript) {
    if (!pScript) {
        printf("assignScript() : !script\n");
        return;
    }
    printf("assignScript('%s')\n", pScript->Name.c_str());

    gig::Instrument* pInstrument = get_instrument();
    if (!pInstrument) {
        printf("!instrument\n");
        return;
    }

    pInstrument->AddScriptSlot(pScript);

    onScriptSlotsModified(pInstrument);
}

void MainWindow::dropAllScriptSlots() {
    gig::Instrument* pInstrument = get_instrument();
    if (!pInstrument) {
        printf("!instrument\n");
        return;
    }

    const int iScriptSlots = pInstrument->ScriptSlotCount();
    for (int i = iScriptSlots - 1; i >= 0; --i)
        pInstrument->RemoveScriptSlot(i);

    onScriptSlotsModified(pInstrument);
}

void MainWindow::on_action_refresh_all() {
    __refreshEntireGUI();
}

void MainWindow::on_action_view_status_bar() {
#if USE_GLIB_ACTION
    bool active = false;
    m_actionToggleStatusBar->get_state(active);
    // for some reason toggle state does not change automatically
    active = !active;
    m_actionToggleStatusBar->change_state(active);
    if (active)
        m_StatusBar.show();
    else
        m_StatusBar.hide();
#else
    Gtk::CheckMenuItem* item =
        dynamic_cast<Gtk::CheckMenuItem*>(uiManager->get_widget("/MenuBar/MenuView/Statusbar"));
    if (!item) {
        std::cerr << "/MenuBar/MenuView/Statusbar == NULL\n";
        return;
    }
    if (item->get_active()) m_StatusBar.show();
    else                    m_StatusBar.hide();
#endif
}

void MainWindow::on_auto_restore_win_dim() {
#if USE_GLIB_ACTION
    bool active = false;
    m_actionToggleRestoreWinDim->get_state(active);
    // for some reason toggle state does not change automatically
    active = !active;
    m_actionToggleRestoreWinDim->change_state(active);
    Settings::singleton()->autoRestoreWindowDimension = active;
#else
    Gtk::CheckMenuItem* item =
        dynamic_cast<Gtk::CheckMenuItem*>(uiManager->get_widget("/MenuBar/MenuView/AutoRestoreWinDim"));
    if (!item) {
        std::cerr << "/MenuBar/MenuView/AutoRestoreWinDim == NULL\n";
        return;
    }
    Settings::singleton()->autoRestoreWindowDimension = item->get_active();
#endif
}

void MainWindow::on_instr_double_click_opens_props() {
#if USE_GLIB_ACTION
    bool active = false;
    m_actionInstrDoubleClickOpensProps->get_state(active);
    // for some reason toggle state does not change automatically
    active = !active;
    m_actionInstrDoubleClickOpensProps->change_state(active);
    Settings::singleton()->instrumentDoubleClickOpensProps = active;
#else
    Gtk::CheckMenuItem* item =
        dynamic_cast<Gtk::CheckMenuItem*>(uiManager->get_widget("/MenuBar/MenuView/OpenInstrPropsByDoubleClick"));
    if (!item) {
        std::cerr << "/MenuBar/MenuView/OpenInstrPropsByDoubleClick == NULL\n";
        return;
    }
    Settings::singleton()->instrumentDoubleClickOpensProps = item->get_active();
#endif
}

void MainWindow::on_save_with_temporary_file() {
#if USE_GLIB_ACTION
    bool active = false;
    m_actionToggleSaveWithTempFile->get_state(active);
    // for some reason toggle state does not change automatically
    active = !active;
    m_actionToggleSaveWithTempFile->change_state(active);
    Settings::singleton()->saveWithTemporaryFile = active;
#else
    Gtk::CheckMenuItem* item =
        dynamic_cast<Gtk::CheckMenuItem*>(uiManager->get_widget("/MenuBar/MenuSettings/SaveWithTemporaryFile"));
    if (!item) {
        std::cerr << "/MenuBar/MenuSettings/SaveWithTemporaryFile == NULL\n";
        return;
    }
    Settings::singleton()->saveWithTemporaryFile = item->get_active();
#endif
}

bool MainWindow::is_copy_samples_unity_note_enabled() const {
#if USE_GLIB_ACTION
    bool active = false;
    m_actionToggleCopySampleUnity->get_state(active);
    return active;
#else
    Gtk::CheckMenuItem* item =
        dynamic_cast<Gtk::CheckMenuItem*>(uiManager->get_widget("/MenuBar/MenuEdit/CopySampleUnity"));
    if (!item) {
        std::cerr << "/MenuBar/MenuEdit/CopySampleUnity == NULL\n";
        return true;
    }
    return item->get_active();
#endif
}

bool MainWindow::is_copy_samples_fine_tune_enabled() const {
#if USE_GLIB_ACTION
    bool active = false;
    m_actionToggleCopySampleTune->get_state(active);
    return active;
#else
    Gtk::CheckMenuItem* item =
        dynamic_cast<Gtk::CheckMenuItem*>(uiManager->get_widget("/MenuBar/MenuEdit/CopySampleTune"));
    if (!item) {
        std::cerr << "/MenuBar/MenuEdit/CopySampleTune == NULL\n";
        return true;
    }
    return item->get_active();
#endif
}

bool MainWindow::is_copy_samples_loop_enabled() const {
#if USE_GLIB_ACTION
    bool active = false;
    m_actionToggleCopySampleLoop->get_state(active);
    return active;
#else
    Gtk::CheckMenuItem* item =
        dynamic_cast<Gtk::CheckMenuItem*>(uiManager->get_widget("/MenuBar/MenuEdit/CopySampleLoop"));
    if (!item) {
        std::cerr << "/MenuBar/MenuEdit/CopySampleLoop == NULL\n";
        return true;
    }
    return item->get_active();
#endif
}

#if GTKMM_MAJOR_VERSION > 3 || (GTKMM_MAJOR_VERSION == 3 && (GTKMM_MINOR_VERSION > 91 || (GTKMM_MINOR_VERSION == 91 && GTKMM_MICRO_VERSION >= 2))) // GTKMM >= 3.91.2
bool MainWindow::on_button_release(Gdk::EventButton& _button) {
    GdkEventButton* button = _button.gobj();
#else
void MainWindow::on_button_release(GdkEventButton* button) {
#endif
    if (button->type == GDK_2BUTTON_PRESS) {
        if (Settings::singleton()->instrumentDoubleClickOpensProps)
            show_instr_props();
    } else if (button->type == GDK_BUTTON_PRESS && button->button == 3) {
        // gig v2 files have no midi rules
        const bool bEnabled = !(file->pVersion && file->pVersion->major == 2);
#if USE_GTKMM_BUILDER
        m_actionMIDIRules->property_enabled() = bEnabled;
#else
        static_cast<Gtk::MenuItem*>(
            uiManager->get_widget("/MenuBar/MenuInstrument/MidiRules"))->set_sensitive(
                bEnabled
            );
        static_cast<Gtk::MenuItem*>(
            uiManager->get_widget("/PopupMenu/MidiRules"))->set_sensitive(
                bEnabled
            );
#endif
        popup_menu->popup(button->button, button->time);
    }
#if GTKMM_MAJOR_VERSION > 3 || (GTKMM_MAJOR_VERSION == 3 && (GTKMM_MINOR_VERSION > 91 || (GTKMM_MINOR_VERSION == 91 && GTKMM_MICRO_VERSION >= 2))) // GTKMM >= 3.91.2
    return false;
#endif
}

#if !USE_GTKMM_BUILDER
void MainWindow::on_instrument_selection_change(Gtk::RadioMenuItem* item) {
    if (item->get_active()) {
        const std::vector<Gtk::Widget*> children =
            instrument_menu->get_children();
        std::vector<Gtk::Widget*>::const_iterator it =
            find(children.begin(), children.end(), item);
        if (it != children.end()) {
            int index = it - children.begin();
            m_TreeView.get_selection()->select(Gtk::TreePath(ToString(index)));

            m_RegionChooser.set_instrument(file->GetInstrument(index));
        }
    }
}
#endif

void MainWindow::on_action_move_instr() {
    gig::Instrument* instr = get_instrument();
    if (!instr) return;

    int currentIndex = getIndexOf(instr);

    Gtk::Dialog dialog(_("Move Instrument"), true /*modal*/);
#if (GTKMM_MAJOR_VERSION == 2 && GTKMM_MINOR_VERSION < 90) || GTKMM_MAJOR_VERSION < 2
    Gtk::Adjustment adjustment(
        currentIndex,
        0 /*min*/, file->CountInstruments() - 1 /*max*/
    );
    Gtk::SpinButton spinBox(adjustment);
#else
    Gtk::SpinButton spinBox(
        Gtk::Adjustment::create(
            currentIndex,
            0 /*min*/, file->CountInstruments() - 1 /*max*/
        )
    );
#endif
#if USE_GTKMM_BOX
    dialog.get_content_area()->pack_start(spinBox);
#else
    dialog.get_vbox()->pack_start(spinBox);
#endif
#if HAS_GTKMM_STOCK
    Gtk::Button* okButton = dialog.add_button(Gtk::Stock::OK, 0);
    dialog.add_button(Gtk::Stock::CANCEL, 1);
#else
    Gtk::Button* okButton = dialog.add_button(_("_OK"), 0);
    dialog.add_button(_("_Cancel"), 1);
#endif
    okButton->set_sensitive(false);
    // show the dialog at a reasonable screen position
    dialog.set_position(Gtk::WIN_POS_MOUSE);
    // only enable the 'OK' button if entered new index is not instrument's
    // current index already
    spinBox.signal_value_changed().connect([&]{
        okButton->set_sensitive( spinBox.get_value_as_int() != currentIndex );
    });
    // usability acceleration: if user hits enter key on the text entry field
    // then auto trigger the 'OK' button
    spinBox.signal_activate().connect([&]{
        if (okButton->get_sensitive())
            okButton->clicked();
    });
#if HAS_GTKMM_SHOW_ALL_CHILDREN
    dialog.show_all_children();
#endif
    if (!dialog.run()) { // 'OK' selected ...
        int newIndex = spinBox.get_value_as_int();
        printf("MOVE TO %d\n", newIndex);
        gig::Instrument* dst = file->GetInstrument(newIndex);
        instr->MoveTo(dst);
        __refreshEntireGUI();
        select_instrument(instr);
    }
}

void MainWindow::select_instrument(gig::Instrument* instrument) {
    if (!instrument) return;

    //NOTE: This is a big mess! Sometimes GTK requires m_refTreeModelFilter->get_model(), here we need m_TreeView.get_model(), otherwise treeview selection below causes an error!
    Glib::RefPtr<Gtk::TreeModel> model = m_TreeView.get_model();
    //Glib::RefPtr<Gtk::TreeModel> model = m_refTreeModelFilter->get_model();

    for (int i = 0; i < model->children().size(); ++i) {
        Gtk::TreeModel::Row row = model->children()[i];
        if (row[m_Columns.m_col_instr] == instrument) {
            // select and show the respective instrument in the list view
            show_intruments_tab();
            m_TreeView.get_selection()->unselect_all();
            
#if GTKMM_MAJOR_VERSION > 3 || (GTKMM_MAJOR_VERSION == 3 && GTKMM_MINOR_VERSION > 24)
            auto iterSel = model->children()[i].get_iter();
            m_TreeView.get_selection()->select(iterSel);
#else
            m_TreeView.get_selection()->select(model->children()[i]);
#endif
            std::vector<Gtk::TreeModel::Path> rows =
                m_TreeView.get_selection()->get_selected_rows();
            if (!rows.empty())
                m_TreeView.scroll_to_row(rows[0]);
            on_sel_change(); // the regular instrument selection change callback
        }
    }
}

/// Returns true if requested dimension region was successfully selected and scrolled to in the list view, false on error.
bool MainWindow::select_dimension_region(gig::DimensionRegion* dimRgn) {
    gig::Region* pRegion = (gig::Region*) dimRgn->GetParent();
    gig::Instrument* pInstrument = (gig::Instrument*) pRegion->GetParent();

    //NOTE: This is a big mess! Sometimes GTK requires m_refTreeModelFilter->get_model(), here we need m_TreeView.get_model(), otherwise treeview selection below causes an error!
    Glib::RefPtr<Gtk::TreeModel> model = m_TreeView.get_model();
    //Glib::RefPtr<Gtk::TreeModel> model = m_refTreeModelFilter->get_model();

    for (int i = 0; i < model->children().size(); ++i) {
        Gtk::TreeModel::Row row = model->children()[i];
        if (row[m_Columns.m_col_instr] == pInstrument) {
            // select and show the respective instrument in the list view
            show_intruments_tab();
            m_TreeView.get_selection()->unselect_all();
#if GTKMM_MAJOR_VERSION > 3 || (GTKMM_MAJOR_VERSION == 3 && GTKMM_MINOR_VERSION > 24)
            auto iterSel = model->children()[i].get_iter();
            m_TreeView.get_selection()->select(iterSel);
#else
            m_TreeView.get_selection()->select(model->children()[i]);
#endif
            std::vector<Gtk::TreeModel::Path> rows =
                m_TreeView.get_selection()->get_selected_rows();
            if (!rows.empty())
                m_TreeView.scroll_to_row(rows[0]);
            on_sel_change(); // the regular instrument selection change callback

            // select respective region in the region selector
            m_RegionChooser.set_region(pRegion);

            // select and show the respective dimension region in the editor
            //update_dimregs();
            if (!m_DimRegionChooser.select_dimregion(dimRgn)) return false;
            //dimreg_edit.set_dim_region(dimRgn);

            return true;
        }
    }

    return false;
}

void MainWindow::select_sample(gig::Sample* sample) {
    Glib::RefPtr<Gtk::TreeModel> model = m_TreeViewSamples.get_model();
    for (int g = 0; g < model->children().size(); ++g) {
        Gtk::TreeModel::Row rowGroup = model->children()[g];
        for (int s = 0; s < rowGroup.children().size(); ++s) {
            Gtk::TreeModel::Row rowSample = rowGroup.children()[s];
            if (rowSample[m_SamplesModel.m_col_sample] == sample) {
                show_samples_tab();
                m_TreeViewSamples.get_selection()->unselect_all();
#if GTKMM_MAJOR_VERSION > 3 || (GTKMM_MAJOR_VERSION == 3 && GTKMM_MINOR_VERSION > 24)
                auto iterSel = rowGroup.children()[s].get_iter();
                m_TreeViewSamples.get_selection()->select(iterSel);
#else
                m_TreeViewSamples.get_selection()->select(rowGroup.children()[s]);
#endif
                std::vector<Gtk::TreeModel::Path> rows =
                    m_TreeViewSamples.get_selection()->get_selected_rows();
                if (rows.empty()) return;
                m_TreeViewSamples.scroll_to_row(rows[0]);
                return;
            }
        }
    }
}

#if GTKMM_MAJOR_VERSION > 3 || (GTKMM_MAJOR_VERSION == 3 && (GTKMM_MINOR_VERSION > 91 || (GTKMM_MINOR_VERSION == 91 && GTKMM_MICRO_VERSION >= 2))) // GTKMM >= 3.91.2
bool MainWindow::on_sample_treeview_button_release(Gdk::EventButton& _button) {
    GdkEventButton* button = _button.gobj();
#else
void MainWindow::on_sample_treeview_button_release(GdkEventButton* button) {
#endif
    if (button->type == GDK_BUTTON_PRESS && button->button == 3) {
        // by default if Ctrl keys is pressed down, then a mouse right-click
        // does not select the respective row, so we must assure this
        // programmatically ...
        /*{
            Gtk::TreeModel::Path path;
            Gtk::TreeViewColumn* pColumn = NULL;
            int cellX, cellY;
            bool bSuccess = m_TreeViewSamples.get_path_at_pos(
                (int)button->x, (int)button->y,
                path, pColumn, cellX, cellY
            );
            if (bSuccess) {
                if (m_TreeViewSamples.get_selection()->count_selected_rows() <= 0) {
                    printf("not selected !!!\n");
                    m_TreeViewSamples.get_selection()->select(path);
                }
            }
        }*/

#if !USE_GTKMM_BUILDER
        Gtk::Menu* sample_popup =
            dynamic_cast<Gtk::Menu*>(uiManager->get_widget("/SamplePopupMenu"));
#endif

        // update enabled/disabled state of sample popup items
        Glib::RefPtr<Gtk::TreeSelection> sel = m_TreeViewSamples.get_selection();
        std::vector<Gtk::TreeModel::Path> rows = sel->get_selected_rows();
        const int n = rows.size();
        int nGroups  = 0;
        int nSamples = 0;
        for (int r = 0; r < n; ++r) {
            Gtk::TreeModel::iterator it = m_refSamplesTreeModel->get_iter(rows[r]);
            if (!it) continue;
            Gtk::TreeModel::Row row = *it;
            if (row[m_SamplesModel.m_col_group]) nGroups++;
            if (row[m_SamplesModel.m_col_sample]) nSamples++;
        }

#if USE_GTKMM_BUILDER
        m_actionSampleProperties->property_enabled() = (n == 1);
        m_actionAddSample->property_enabled() = (n);
        m_actionAddSampleGroup->property_enabled() = (file);
        m_actionViewSampleRefs->property_enabled() = (nSamples == 1);
        m_actionRemoveSample->property_enabled() = (n);
        m_actionReplaceSample->property_enabled() = (nSamples == 1);
#else
        dynamic_cast<Gtk::MenuItem*>(uiManager->get_widget("/SamplePopupMenu/SampleProperties"))->
            set_sensitive(n == 1);
        dynamic_cast<Gtk::MenuItem*>(uiManager->get_widget("/SamplePopupMenu/AddSample"))->
            set_sensitive(n);
        dynamic_cast<Gtk::MenuItem*>(uiManager->get_widget("/SamplePopupMenu/AddGroup"))->
            set_sensitive(file);
        dynamic_cast<Gtk::MenuItem*>(uiManager->get_widget("/SamplePopupMenu/ShowSampleRefs"))->
            set_sensitive(nSamples == 1);
        dynamic_cast<Gtk::MenuItem*>(uiManager->get_widget("/SamplePopupMenu/RemoveSample"))->
            set_sensitive(n);
#endif
        // show sample popup
        sample_popup->popup(button->button, button->time);

#if !USE_GTKMM_BUILDER
        dynamic_cast<Gtk::MenuItem*>(uiManager->get_widget("/MenuBar/MenuSample/SampleProperties"))->
            set_sensitive(n == 1);
        dynamic_cast<Gtk::MenuItem*>(uiManager->get_widget("/MenuBar/MenuSample/AddSample"))->
            set_sensitive(n);
        dynamic_cast<Gtk::MenuItem*>(uiManager->get_widget("/MenuBar/MenuSample/AddGroup"))->
            set_sensitive(file);
        dynamic_cast<Gtk::MenuItem*>(uiManager->get_widget("/MenuBar/MenuSample/ShowSampleRefs"))->
            set_sensitive(nSamples == 1);
        dynamic_cast<Gtk::MenuItem*>(uiManager->get_widget("/MenuBar/MenuSample/RemoveSample"))->
            set_sensitive(n);
#endif
    }
    
#if GTKMM_MAJOR_VERSION > 3 || (GTKMM_MAJOR_VERSION == 3 && (GTKMM_MINOR_VERSION > 91 || (GTKMM_MINOR_VERSION == 91 && GTKMM_MICRO_VERSION >= 2))) // GTKMM >= 3.91.2
    return false;
#endif
}

#if GTKMM_MAJOR_VERSION > 3 || (GTKMM_MAJOR_VERSION == 3 && (GTKMM_MINOR_VERSION > 91 || (GTKMM_MINOR_VERSION == 91 && GTKMM_MICRO_VERSION >= 2))) // GTKMM >= 3.91.2
bool MainWindow::on_script_treeview_button_release(Gdk::EventButton& _button) {
    GdkEventButton* button = _button.gobj();
#else
void MainWindow::on_script_treeview_button_release(GdkEventButton* button) {
#endif
    if (button->type == GDK_BUTTON_PRESS && button->button == 3) {
#if !USE_GTKMM_BUILDER
        Gtk::Menu* script_popup =
            dynamic_cast<Gtk::Menu*>(uiManager->get_widget("/ScriptPopupMenu"));
#endif
        // update enabled/disabled state of sample popup items
        Glib::RefPtr<Gtk::TreeSelection> sel = m_TreeViewScripts.get_selection();
        Gtk::TreeModel::iterator it = sel->get_selected();
        bool group_selected  = false;
        bool script_selected = false;
        if (it) {
            Gtk::TreeModel::Row row = *it;
            group_selected  = row[m_ScriptsModel.m_col_group];
            script_selected = row[m_ScriptsModel.m_col_script];
        }
#if USE_GTKMM_BUILDER
        m_actionAddScript->property_enabled() = (group_selected || script_selected);
        m_actionAddScriptGroup->property_enabled() = (file);
        m_actionEditScript->property_enabled() = (script_selected);
        m_actionRemoveScript->property_enabled() = (group_selected || script_selected);
#else
        dynamic_cast<Gtk::MenuItem*>(uiManager->get_widget("/ScriptPopupMenu/AddScript"))->
            set_sensitive(group_selected || script_selected);
        dynamic_cast<Gtk::MenuItem*>(uiManager->get_widget("/ScriptPopupMenu/AddScriptGroup"))->
            set_sensitive(file);
        dynamic_cast<Gtk::MenuItem*>(uiManager->get_widget("/ScriptPopupMenu/EditScript"))->
            set_sensitive(script_selected);    
        dynamic_cast<Gtk::MenuItem*>(uiManager->get_widget("/ScriptPopupMenu/RemoveScript"))->
            set_sensitive(group_selected || script_selected);
#endif
        // show sample popup
        script_popup->popup(button->button, button->time);

#if !USE_GTKMM_BUILDER
        dynamic_cast<Gtk::MenuItem*>(uiManager->get_widget("/MenuBar/MenuScript/AddScript"))->
            set_sensitive(group_selected || script_selected);
        dynamic_cast<Gtk::MenuItem*>(uiManager->get_widget("/MenuBar/MenuScript/AddScriptGroup"))->
            set_sensitive(file);
        dynamic_cast<Gtk::MenuItem*>(uiManager->get_widget("/MenuBar/MenuScript/EditScript"))->
            set_sensitive(script_selected);    
        dynamic_cast<Gtk::MenuItem*>(uiManager->get_widget("/MenuBar/MenuScript/RemoveScript"))->
            set_sensitive(group_selected || script_selected);
#endif
    }
#if GTKMM_MAJOR_VERSION > 3 || (GTKMM_MAJOR_VERSION == 3 && (GTKMM_MINOR_VERSION > 91 || (GTKMM_MINOR_VERSION == 91 && GTKMM_MICRO_VERSION >= 2))) // GTKMM >= 3.91.2
    return false;
#endif
}

void MainWindow::updateScriptListOfMenu() {
    // remove all entries from "Assign Script" menu
    {
        const std::vector<Gtk::Widget*> children = assign_scripts_menu->get_children();
        for (int i = 0; i < children.size(); ++i) {
            Gtk::Widget* child = children[i];
            assign_scripts_menu->remove(*child);
            delete child;
        }
    }

    int iTotalScripts = 0;

    if (!file) goto noScripts;

    // add all configured macros as menu items to the "Macro" menu
    for (int iGroup = 0; file->GetScriptGroup(iGroup); ++iGroup) {
        gig::ScriptGroup* pGroup = file->GetScriptGroup(iGroup);
        for (int iScript = 0; pGroup->GetScript(iScript); ++iScript, ++iTotalScripts) {
            gig::Script* pScript = pGroup->GetScript(iScript);
            std::string name = pScript->Name;

            Gtk::MenuItem* item = new Gtk::MenuItem(name);
            item->signal_activate().connect(
                sigc::bind(
                    sigc::mem_fun(*this, &MainWindow::assignScript), pScript
                )
            );
            assign_scripts_menu->append(*item);
            item->set_accel_path("<Scripts>/script_" + ToString(iTotalScripts));
            //item->set_tooltip_text(comment);
        }
    }

    noScripts:

    // if there are no macros configured at all, then show a dummy entry instead
    if (!iTotalScripts) {
        Gtk::MenuItem* item = new Gtk::MenuItem(_("No Scripts"));
        item->set_sensitive(false);
        assign_scripts_menu->append(*item);
    }

    // add separator line to menu
    assign_scripts_menu->append(*new Gtk::SeparatorMenuItem);

    {
        Gtk::MenuItem* item = new Gtk::MenuItem(_("Unassign All Scripts"));
        item->signal_activate().connect(
            sigc::mem_fun(*this, &MainWindow::dropAllScriptSlots)
        );
        assign_scripts_menu->append(*item);
        item->set_accel_path("<Scripts>/DropAllScriptSlots");
    }

#if HAS_GTKMM_SHOW_ALL_CHILDREN
    assign_scripts_menu->show_all_children();
#endif
}

#if !USE_GTKMM_BUILDER
Gtk::RadioMenuItem* MainWindow::add_instrument_to_menu(
    const Glib::ustring& name, int position) {

    Gtk::RadioMenuItem::Group instrument_group;
    const std::vector<Gtk::Widget*> children = instrument_menu->get_children();
    if (!children.empty()) {
        instrument_group =
            static_cast<Gtk::RadioMenuItem*>(children[0])->get_group();
    }
    Gtk::RadioMenuItem* item =
        new Gtk::RadioMenuItem(instrument_group, name);
    if (position < 0) {
        instrument_menu->append(*item);
    } else {
        instrument_menu->insert(*item, position);
    }
    item->show();
    item->signal_activate().connect(
        sigc::bind(
            sigc::mem_fun(*this, &MainWindow::on_instrument_selection_change),
            item));
    return item;
}
#endif

#if !USE_GTKMM_BUILDER
void MainWindow::remove_instrument_from_menu(int index) {
    const std::vector<Gtk::Widget*> children =
        instrument_menu->get_children();
    Gtk::Widget* child = children[index];
    instrument_menu->remove(*child);
    delete child;
}
#endif

void MainWindow::add_instrument(gig::Instrument* instrument) {
    const Glib::ustring name(gig_to_utf8(instrument->pInfo->Name));

    // update instrument tree view
    instrument_name_connection.block();
    Gtk::TreeModel::iterator iterInstr = m_refTreeModel->append();
    Gtk::TreeModel::Row rowInstr = *iterInstr;
    const int index = m_refTreeModel->children().size() - 1;
    rowInstr[m_Columns.m_col_nr] = index;
    rowInstr[m_Columns.m_col_name] = name;
    rowInstr[m_Columns.m_col_instr] = instrument;
    rowInstr[m_Columns.m_col_scripts] = "";
    rowInstr[m_Columns.m_col_tooltip] = scriptTooltipFor(instrument, index);
    instrument_name_connection.unblock();

#if !USE_GTKMM_BUILDER
    add_instrument_to_menu(name);
#endif
    select_instrument(instrument);
    file_changed();
}

void MainWindow::on_action_add_instrument() {
    static int __instrument_indexer = 0;
    if (!file) return;
    gig::Instrument* instrument = file->AddInstrument();
    __instrument_indexer++;
    instrument->pInfo->Name = gig_from_utf8(_("Unnamed Instrument ") +
                                            ToString(__instrument_indexer));

    add_instrument(instrument);
}

void MainWindow::on_action_duplicate_instrument() {
    if (!file) return;

    // retrieve the currently selected instrument
    // (being the original instrument to be duplicated)
    Glib::RefPtr<Gtk::TreeSelection> sel = m_TreeView.get_selection();
    std::vector<Gtk::TreeModel::Path> rows = sel->get_selected_rows();
    for (int r = 0; r < rows.size(); ++r) {
        Gtk::TreeModel::iterator it = m_refTreeModel->get_iter(rows[r]);
        if (it) {
            Gtk::TreeModel::Row row = *it;
            gig::Instrument* instrOrig = row[m_Columns.m_col_instr];
            if (instrOrig) {
                // duplicate the orginal instrument
                gig::Instrument* instrNew = file->AddDuplicateInstrument(instrOrig);
                instrNew->pInfo->Name =
                    instrOrig->pInfo->Name +
                    gig_from_utf8(Glib::ustring(" (") + _("Copy") + ")");

                add_instrument(instrNew);
            }
        }
    }
}

void MainWindow::on_action_remove_instrument() {
    if (!file) return;
    if (file_is_shared) {
        Gtk::MessageDialog msg(
            *this,
             _("You cannot delete an instrument from this file, since it's "
               "currently used by the sampler."),
             false, Gtk::MESSAGE_INFO
        );
        msg.run();
        return;
    }

    Glib::RefPtr<Gtk::TreeSelection> sel = m_TreeView.get_selection();
    std::vector<Gtk::TreeModel::Path> rows = sel->get_selected_rows();
    for (int r = rows.size() - 1; r >= 0; --r) {
        Gtk::TreeModel::iterator it = m_refTreeModel->get_iter(rows[r]);
        if (!it) continue;
        Gtk::TreeModel::Row row = *it;
        gig::Instrument* instr = row[m_Columns.m_col_instr];
        try {
            Gtk::TreePath path(it);
            int index = path[0];

            // remove instrument from the gig file
            if (instr) file->DeleteInstrument(instr);
            file_changed();

#if !USE_GTKMM_BUILDER
            remove_instrument_from_menu(index);
#endif

            // remove row from instruments tree view
            m_refTreeModel->erase(it);
            // update "Nr" column of all instrument rows
            {
                int index = 0;
                for (Gtk::TreeModel::iterator it = m_refTreeModel->children().begin();
                     it != m_refTreeModel->children().end(); ++it, ++index)
                {
                    Gtk::TreeModel::Row row = *it;
                    gig::Instrument* instrument = row[m_Columns.m_col_instr];
                    row[m_Columns.m_col_nr] = index;
                    row[m_Columns.m_col_tooltip] = scriptTooltipFor(instrument, index);
                }
            }

#if GTKMM_MAJOR_VERSION < 3
            // select another instrument (in gtk3 this is done
            // automatically)
            if (!m_refTreeModel->children().empty()) {
                if (index == m_refTreeModel->children().size()) {
                    index--;
                }
                m_TreeView.get_selection()->select(
                    Gtk::TreePath(ToString(index)));
            }
#endif
            instr_props_set_instrument();
            instr = get_instrument();
            if (instr) {
                midiRules.set_instrument(instr);
            } else {
                midiRules.hide();
            }
        } catch (RIFF::Exception e) {
            Gtk::MessageDialog msg(*this, e.Message.c_str(), false, Gtk::MESSAGE_ERROR);
            msg.run();
        }
    }
}

void MainWindow::on_action_sample_properties() {
    show_sample_props();
}

void MainWindow::on_action_add_script_group() {
    static int __script_indexer = 0;
    if (!file) return;
    gig::ScriptGroup* group = file->AddScriptGroup();
    group->Name = gig_from_utf8(_("Unnamed Group"));
    if (__script_indexer) group->Name += " " + ToString(__script_indexer);
    __script_indexer++;
    // update sample tree view
    Gtk::TreeModel::iterator iterGroup = m_refScriptsTreeModel->append();
    Gtk::TreeModel::Row rowGroup = *iterGroup;
    rowGroup[m_ScriptsModel.m_col_name] = gig_to_utf8(group->Name);
    rowGroup[m_ScriptsModel.m_col_script] = NULL;
    rowGroup[m_ScriptsModel.m_col_group] = group;
    file_changed();
}

void MainWindow::on_action_add_script() {
    if (!file) return;
    // get selected group
    Glib::RefPtr<Gtk::TreeSelection> sel = m_TreeViewScripts.get_selection();
    Gtk::TreeModel::iterator it = sel->get_selected();
    if (!it) return;
    Gtk::TreeModel::Row row = *it;
    gig::ScriptGroup* group = row[m_ScriptsModel.m_col_group];
    if (!group) { // not a group, but a script is selected (probably)
        gig::Script* script = row[m_ScriptsModel.m_col_script];
        if (!script) return;
        it = row.parent(); // resolve parent (that is the script's group)
        if (!it) return;
        row = *it;
        group = row[m_ScriptsModel.m_col_group];
        if (!group) return;
    }

    // add a new script to the .gig file
    gig::Script* script = group->AddScript();    
    Glib::ustring name = _("Unnamed Script");
    script->Name = gig_from_utf8(name);

    // add script to the tree view
    Gtk::TreeModel::iterator iterScript =
        m_refScriptsTreeModel->append(row.children());
    Gtk::TreeModel::Row rowScript = *iterScript;
    rowScript[m_ScriptsModel.m_col_name] = name;
    rowScript[m_ScriptsModel.m_col_script] = script;
    rowScript[m_ScriptsModel.m_col_group]  = NULL;

    // unfold group of new script item in treeview
    Gtk::TreeModel::Path path(iterScript);
    m_TreeViewScripts.expand_to_path(path);
}

void MainWindow::on_action_edit_script() {
    if (!file) return;
    // get selected script
    Glib::RefPtr<Gtk::TreeSelection> sel = m_TreeViewScripts.get_selection();
    Gtk::TreeModel::iterator it = sel->get_selected();
    if (!it) return;
    Gtk::TreeModel::Row row = *it;
    gig::Script* script = row[m_ScriptsModel.m_col_script];
    if (!script) return;

    ScriptEditor* editor = new ScriptEditor;
    editor->signal_script_to_be_changed.connect(
        signal_script_to_be_changed.make_slot()
    );
    editor->signal_script_changed.connect(
        signal_script_changed.make_slot()
    );
    editor->setScript(script);
    //editor->reparent(*this);
    editor->show();
}

void MainWindow::on_action_remove_script() {
    if (!file) return;
    Glib::RefPtr<Gtk::TreeSelection> sel = m_TreeViewScripts.get_selection();
    Gtk::TreeModel::iterator it = sel->get_selected();
    if (it) {
        Gtk::TreeModel::Row row = *it;
        gig::ScriptGroup* group = row[m_ScriptsModel.m_col_group];
        gig::Script* script     = row[m_ScriptsModel.m_col_script];
        Glib::ustring name      = row[m_ScriptsModel.m_col_name];
        try {
            // remove script group or script from the gig file
            if (group) {
                // notify everybody that we're going to remove these samples
//TODO:         scripts_to_be_removed_signal.emit(members);
                // delete the group in the .gig file including the
                // samples that belong to the group
                file->DeleteScriptGroup(group);
                // notify that we're done with removal
//TODO:         scripts_removed_signal.emit();
                file_changed();
            } else if (script) {
                // notify everybody that we're going to remove this sample
//TODO:         std::list<gig::Script*> lscripts;
//TODO:         lscripts.push_back(script);
//TODO:         scripts_to_be_removed_signal.emit(lscripts);
                // remove sample from the .gig file
                script->GetGroup()->DeleteScript(script);
                // notify that we're done with removal
//TODO:         scripts_removed_signal.emit();
                dimreg_changed();
                file_changed();
            }
            // remove respective row(s) from samples tree view
            m_refScriptsTreeModel->erase(it);
        } catch (RIFF::Exception e) {
            // pretend we're done with removal (i.e. to avoid dead locks)
//TODO:     scripts_removed_signal.emit();
            // show error message
            Gtk::MessageDialog msg(*this, e.Message.c_str(), false, Gtk::MESSAGE_ERROR);
            msg.run();
        }
    }
}

void MainWindow::on_action_add_group() {
    static int __sample_indexer = 0;
    if (!file) return;
    gig::Group* group = file->AddGroup();
    group->Name = gig_from_utf8(_("Unnamed Group"));
    if (__sample_indexer) group->Name += " " + ToString(__sample_indexer);
    __sample_indexer++;
    // update sample tree view
    Gtk::TreeModel::iterator iterGroup = m_refSamplesTreeModel->append();
    Gtk::TreeModel::Row rowGroup = *iterGroup;
    rowGroup[m_SamplesModel.m_col_name] = gig_to_utf8(group->Name);
    rowGroup[m_SamplesModel.m_col_sample] = NULL;
    rowGroup[m_SamplesModel.m_col_group] = group;
    file_changed();
}

void MainWindow::on_action_replace_sample() {
    add_or_replace_sample(true);
}

void MainWindow::on_action_add_sample() {
    add_or_replace_sample(false);
}

void MainWindow::add_or_replace_sample(bool replace) {
    if (!file) return;

    // get selected group (and probably selected sample)
    Glib::RefPtr<Gtk::TreeSelection> sel = m_TreeViewSamples.get_selection();
    std::vector<Gtk::TreeModel::Path> rows = sel->get_selected_rows();
    if (rows.empty()) return;
    Gtk::TreeModel::iterator it = m_refSamplesTreeModel->get_iter(rows[0]);
    if (!it) return;
    Gtk::TreeModel::Row row = *it;
    gig::Sample* sample = NULL;
    gig::Group* group = row[m_SamplesModel.m_col_group];
    if (!group) { // not a group, but a sample is selected (probably)
        if (replace) sample = row[m_SamplesModel.m_col_sample];
        if (!row[m_SamplesModel.m_col_sample]) return;
        it = row.parent(); // resolve parent (that is the sample's group)
        if (!it) return;
        if (!replace) row = *it;
        group = (*it)[m_SamplesModel.m_col_group];
        if (!group) return;
    }
    if (replace && !sample) return;

    // show 'browse for file' dialog
    Gtk::FileChooserDialog dialog(*this, replace ? _("Replace Sample with") : _("Add Sample(s)"));
#if HAS_GTKMM_STOCK
    dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
    dialog.add_button(Gtk::Stock::OPEN, Gtk::RESPONSE_OK);
#else
    dialog.add_button(_("_Cancel"), Gtk::RESPONSE_CANCEL);
    dialog.add_button(_("_Open"), Gtk::RESPONSE_OK);
#endif
    dialog.set_select_multiple(!replace); // allow multi audio file selection only when adding new samples, does not make sense when replacing a specific sample

    // matches all file types supported by libsndfile
#if (GTKMM_MAJOR_VERSION == 2 && GTKMM_MINOR_VERSION < 90) || GTKMM_MAJOR_VERSION < 2
    Gtk::FileFilter soundfilter;
#else
    Glib::RefPtr<Gtk::FileFilter> soundfilter = Gtk::FileFilter::create();
#endif
    const char* const supportedFileTypes[] = {
        "*.wav", "*.WAV", "*.aiff", "*.AIFF", "*.aifc", "*.AIFC", "*.snd",
        "*.SND", "*.au", "*.AU", "*.paf", "*.PAF", "*.iff", "*.IFF",
        "*.svx", "*.SVX", "*.sf", "*.SF", "*.voc", "*.VOC", "*.w64",
        "*.W64", "*.pvf", "*.PVF", "*.xi", "*.XI", "*.htk", "*.HTK",
        "*.caf", "*.CAF", NULL
    };
    const char* soundfiles = _("Sound Files");
    const char* allfiles = _("All Files");
#if (GTKMM_MAJOR_VERSION == 2 && GTKMM_MINOR_VERSION < 90) || GTKMM_MAJOR_VERSION < 2
    for (int i = 0; supportedFileTypes[i]; i++)
        soundfilter.add_pattern(supportedFileTypes[i]);
    soundfilter.set_name(soundfiles);

    // matches every file
    Gtk::FileFilter allpassfilter;
    allpassfilter.add_pattern("*.*");
    allpassfilter.set_name(allfiles);
#else
    for (int i = 0; supportedFileTypes[i]; i++)
        soundfilter->add_pattern(supportedFileTypes[i]);
    soundfilter->set_name(soundfiles);

    // matches every file
    Glib::RefPtr<Gtk::FileFilter> allpassfilter = Gtk::FileFilter::create();
    allpassfilter->add_pattern("*.*");
    allpassfilter->set_name(allfiles);
#endif
    dialog.add_filter(soundfilter);
    dialog.add_filter(allpassfilter);
    if (current_sample_dir != "") {
        dialog.set_current_folder(current_sample_dir);
    }
    if (dialog.run() == Gtk::RESPONSE_OK) {
        dialog.hide();
        current_sample_dir = dialog.get_current_folder();
        Glib::ustring error_files;
        std::vector<std::string> filenames = dialog.get_filenames();
        for (std::vector<std::string>::iterator iter = filenames.begin();
             iter != filenames.end(); ++iter) {
            printf("Adding sample %s\n",(*iter).c_str());
            // use libsndfile to retrieve file information
            SF_INFO info;
            info.format = 0;
            SNDFILE* hFile = sf_open((*iter).c_str(), SFM_READ, &info);
            try {
                if (!hFile) throw std::string(_("could not open file"));
                int bitdepth;
                switch (info.format & 0xff) {
                    case SF_FORMAT_PCM_S8:
                    case SF_FORMAT_PCM_16:
                    case SF_FORMAT_PCM_U8:
                        bitdepth = 16;
                        break;
                    case SF_FORMAT_PCM_24:
                    case SF_FORMAT_PCM_32:
                    case SF_FORMAT_FLOAT:
                    case SF_FORMAT_DOUBLE:
                        bitdepth = 24;
                        break;
                    default:
                        sf_close(hFile); // close sound file
                        throw std::string(_("format not supported")); // unsupported subformat (yet?)
                }
                // add a new sample to the .gig file (if adding is requested actually)
                if (!replace) sample = file->AddSample();
                // file name without path
                Glib::ustring filename = Glib::filename_display_basename(*iter);
                // remove file extension if there is one
                for (int i = 0; supportedFileTypes[i]; i++) {
                    if (Glib::str_has_suffix(filename, supportedFileTypes[i] + 1)) {
                        filename.erase(filename.length() - strlen(supportedFileTypes[i] + 1));
                        break;
                    }
                }
                sample->pInfo->Name = gig_from_utf8(filename);
                sample->Channels = info.channels;
                sample->BitDepth = bitdepth;
                sample->FrameSize = bitdepth / 8/*1 byte are 8 bits*/ * info.channels;
                sample->SamplesPerSecond = info.samplerate;
                sample->AverageBytesPerSecond = sample->FrameSize * sample->SamplesPerSecond;
                sample->BlockAlign = sample->FrameSize;
                sample->SamplesTotal = info.frames;

                SF_INSTRUMENT instrument;
                if (sf_command(hFile, SFC_GET_INSTRUMENT,
                               &instrument, sizeof(instrument)) != SF_FALSE)
                {
                    sample->MIDIUnityNote = instrument.basenote;
                    sample->FineTune      = instrument.detune;

                    if (instrument.loop_count && instrument.loops[0].mode != SF_LOOP_NONE) {
                        sample->Loops = 1;

                        switch (instrument.loops[0].mode) {
                        case SF_LOOP_FORWARD:
                            sample->LoopType = gig::loop_type_normal;
                            break;
                        case SF_LOOP_BACKWARD:
                            sample->LoopType = gig::loop_type_backward;
                            break;
                        case SF_LOOP_ALTERNATING:
                            sample->LoopType = gig::loop_type_bidirectional;
                            break;
                        }
                        sample->LoopStart = instrument.loops[0].start;
                        sample->LoopEnd = instrument.loops[0].end;
                        sample->LoopPlayCount = instrument.loops[0].count;
                        sample->LoopSize = sample->LoopEnd - sample->LoopStart + 1;
                    }
                }

                // schedule resizing the sample (which will be done
                // physically when File::Save() is called)
                sample->Resize(info.frames);
                // make sure sample is part of the selected group
                if (!replace) group->AddSample(sample);
                // schedule that physical resize and sample import
                // (data copying), performed when "Save" is requested
                SampleImportItem sched_item;
                sched_item.gig_sample  = sample;
                sched_item.sample_path = *iter;
                m_SampleImportQueue[sample] = sched_item;
                // add sample to the tree view
                if (replace) {
                    row[m_SamplesModel.m_col_name] = gig_to_utf8(sample->pInfo->Name);
                } else {
                    Gtk::TreeModel::iterator iterSample =
                        m_refSamplesTreeModel->append(row.children());
                    Gtk::TreeModel::Row rowSample = *iterSample;
                    rowSample[m_SamplesModel.m_col_name] =
                        gig_to_utf8(sample->pInfo->Name);
                    rowSample[m_SamplesModel.m_col_sample] = sample;
                    rowSample[m_SamplesModel.m_col_group]  = NULL;
                }
                // close sound file
                sf_close(hFile);
                file_changed();
            } catch (std::string what) { // remember the files that made trouble (and their cause)
                if (!error_files.empty()) error_files += "\n";
                error_files += *iter += " (" + what + ")";
            }
        }
        // show error message box when some file(s) could not be opened / added
        if (!error_files.empty()) {
            Glib::ustring txt =
                (replace
                    ? _("Failed to replace sample with:\n")
                    : _("Could not add the following sample(s):\n"))
                + error_files;
            Gtk::MessageDialog msg(*this, txt, false, Gtk::MESSAGE_ERROR);
            msg.run();
        }
    }
}

void MainWindow::on_action_replace_all_samples_in_all_groups()
{
    if (!file) return;
    Gtk::FileChooserDialog dialog(*this, _("Select Folder"),
                                  Gtk::FILE_CHOOSER_ACTION_SELECT_FOLDER);
    const char* str =
        _("This is a very specific function. It tries to replace all samples "
          "in the current gig file by samples located in the chosen "
          "directory.\n\n"
          "It works like this: For each sample in the gig file, it tries to "
          "find a sample file in the selected directory with the same name as "
          "the sample in the gig file. Optionally, you can add a filename "
          "extension below, which will be added to the filename expected to be "
          "found. That is, assume you have a gig file with a sample called "
          "'Snare', if you enter '.wav' below (like it's done by default), it "
          "expects to find a sample file called 'Snare.wav' and will replace "
          "the sample in the gig file accordingly. If you don't need an "
          "extension, blank the field below. Any gig sample where no "
          "appropriate sample file could be found will be reported and left "
          "untouched.\n");
#if GTKMM_MAJOR_VERSION < 3
    view::WrapLabel description(str);
#else
    Gtk::Label description(str);
    description.set_line_wrap();
#endif
    HBox entryArea;
    Gtk::Label entryLabel( _("Add filename extension: "), Gtk::ALIGN_START);
    Gtk::Entry postfixEntryBox;
    postfixEntryBox.set_text(".wav");
    entryArea.pack_start(entryLabel);
    entryArea.pack_start(postfixEntryBox);
#if USE_GTKMM_BOX
    dialog.get_content_area()->pack_start(description, Gtk::PACK_SHRINK);
    dialog.get_content_area()->pack_start(entryArea, Gtk::PACK_SHRINK);
#else
    dialog.get_vbox()->pack_start(description, Gtk::PACK_SHRINK);
    dialog.get_vbox()->pack_start(entryArea, Gtk::PACK_SHRINK);
#endif
    description.show();

#if HAS_GTKMM_SHOW_ALL_CHILDREN
    entryArea.show_all();
#else
    entryArea.show();
#endif

#if HAS_GTKMM_STOCK
    dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
#else
    dialog.add_button(_("_Cancel"), Gtk::RESPONSE_CANCEL);
#endif
    dialog.add_button(_("Select"), Gtk::RESPONSE_OK);
    dialog.set_select_multiple(false);
    if (current_sample_dir != "") {
        dialog.set_current_folder(current_sample_dir);
    }
    if (dialog.run() == Gtk::RESPONSE_OK)
    {
        dialog.hide();
        current_sample_dir = dialog.get_current_folder();
        Glib::ustring error_files;
        std::string folder = dialog.get_filename();
        for (gig::Sample* sample = file->GetFirstSample();
             sample; sample = file->GetNextSample())
        {
            std::string filename =
                folder + G_DIR_SEPARATOR_S +
                Glib::filename_from_utf8(gig_to_utf8(sample->pInfo->Name) +
                                         postfixEntryBox.get_text());
            SF_INFO info;
            info.format = 0;
            SNDFILE* hFile = sf_open(filename.c_str(), SFM_READ, &info);
            try
            {
                if (!hFile) throw std::string(_("could not open file"));
                switch (info.format & 0xff) {
                    case SF_FORMAT_PCM_S8:
                    case SF_FORMAT_PCM_16:
                    case SF_FORMAT_PCM_U8:
                    case SF_FORMAT_PCM_24:
                    case SF_FORMAT_PCM_32:
                    case SF_FORMAT_FLOAT:
                    case SF_FORMAT_DOUBLE:
                        break;
                    default:
                        sf_close(hFile);
                        throw std::string(_("format not supported"));
                }
                SampleImportItem sched_item;
                sched_item.gig_sample  = sample;
                sched_item.sample_path = filename;
                m_SampleImportQueue[sample] = sched_item;
                sf_close(hFile);
                file_changed();
            }
            catch (std::string what)
            {
                if (!error_files.empty()) error_files += "\n";
                error_files += Glib::filename_to_utf8(filename) + 
                    " (" + what + ")";
            }
        }
        // show error message box when some file(s) could not be opened / added
        if (!error_files.empty()) {
            Glib::ustring txt =
                _("Could not replace the following sample(s):\n") + error_files;
            Gtk::MessageDialog msg(*this, txt, false, Gtk::MESSAGE_ERROR);
            msg.run();
        }
    }
}

void MainWindow::on_action_remove_sample() {
    if (!file) return;
    Glib::RefPtr<Gtk::TreeSelection> sel = m_TreeViewSamples.get_selection();
    std::vector<Gtk::TreeModel::Path> rows = sel->get_selected_rows();
    for (int r = rows.size() - 1; r >= 0; --r) {
        Gtk::TreeModel::iterator it = m_refSamplesTreeModel->get_iter(rows[r]);
        if (!it) continue;
        Gtk::TreeModel::Row row = *it;
        gig::Group* group   = row[m_SamplesModel.m_col_group];
        gig::Sample* sample = row[m_SamplesModel.m_col_sample];
        Glib::ustring name  = row[m_SamplesModel.m_col_name];
        try {
            // remove group or sample from the gig file
            if (group) {
                // temporarily remember the samples that belong to
                // that group (we need that to clean the queue)
                std::list<gig::Sample*> members;
                for (gig::Sample* pSample = group->GetFirstSample();
                     pSample; pSample = group->GetNextSample()) {
                    members.push_back(pSample);
                }
                // notify everybody that we're going to remove these samples
                samples_to_be_removed_signal.emit(members);
                // delete the group in the .gig file including the
                // samples that belong to the group
                file->DeleteGroup(group);
                // notify that we're done with removal
                samples_removed_signal.emit();
                // if sample(s) were just previously added, remove
                // them from the import queue
                for (std::list<gig::Sample*>::iterator member = members.begin();
                     member != members.end(); ++member)
                {
                    if (m_SampleImportQueue.count(*member)) {
                        printf("Removing previously added sample '%s' from group '%s'\n",
                               m_SampleImportQueue[sample].sample_path.c_str(), name.c_str());
                        m_SampleImportQueue.erase(*member);
                    }
                }
                file_changed();
            } else if (sample) {
                // notify everybody that we're going to remove this sample
                std::list<gig::Sample*> lsamples;
                lsamples.push_back(sample);
                samples_to_be_removed_signal.emit(lsamples);
                // remove sample from the .gig file
                file->DeleteSample(sample);
                // notify that we're done with removal
                samples_removed_signal.emit();
                // if sample was just previously added, remove it from
                // the import queue
                if (m_SampleImportQueue.count(sample)) {
                    printf("Removing previously added sample '%s'\n",
                           m_SampleImportQueue[sample].sample_path.c_str());
                    m_SampleImportQueue.erase(sample);
                }
                dimreg_changed();
                file_changed();
            }
            // remove respective row(s) from samples tree view
            m_refSamplesTreeModel->erase(it);
        } catch (RIFF::Exception e) {
            // pretend we're done with removal (i.e. to avoid dead locks)
            samples_removed_signal.emit();
            // show error message
            Gtk::MessageDialog msg(*this, e.Message.c_str(), false, Gtk::MESSAGE_ERROR);
            msg.run();
        }
    }
}

void MainWindow::on_action_remove_unused_samples() {
    if (!file) return;

    // collect all samples that are not referenced by any instrument
    std::list<gig::Sample*> lsamples;
    for (int iSample = 0; file->GetSample(iSample); ++iSample) {
        gig::Sample* sample = file->GetSample(iSample);
        bool isUsed = false;
        for (gig::Instrument* instrument = file->GetFirstInstrument(); instrument;
                              instrument = file->GetNextInstrument())
        {
            for (gig::Region* rgn = instrument->GetFirstRegion(); rgn;
                              rgn = instrument->GetNextRegion())
            {
                for (int i = 0; i < 256; ++i) {
                    if (!rgn->pDimensionRegions[i]) continue;
                    if (rgn->pDimensionRegions[i]->pSample != sample) continue;
                    isUsed = true;
                    goto endOfRefSearch;
                }
            }
        }
        endOfRefSearch:
        if (!isUsed) lsamples.push_back(sample);
    }

    if (lsamples.empty()) return;

    // notify everybody that we're going to remove these samples
    samples_to_be_removed_signal.emit(lsamples);

    // remove collected samples
    try {
        for (std::list<gig::Sample*>::iterator itSample = lsamples.begin();
             itSample != lsamples.end(); ++itSample)
        {
            gig::Sample* sample = *itSample;
            // remove sample from the .gig file
            file->DeleteSample(sample);
            // if sample was just previously added, remove it from the import queue
            if (m_SampleImportQueue.count(sample)) {
                printf("Removing previously added sample '%s'\n",
                       m_SampleImportQueue[sample].sample_path.c_str());
                m_SampleImportQueue.erase(sample);
            }
        }
    } catch (RIFF::Exception e) {
        // show error message
        Gtk::MessageDialog msg(*this, e.Message.c_str(), false, Gtk::MESSAGE_ERROR);
        msg.run();
    }

    // notify everybody that we're done with removal
    samples_removed_signal.emit();

    dimreg_changed();
    file_changed();
    __refreshEntireGUI();
}

// see comment on on_sample_treeview_drag_begin()
void MainWindow::on_scripts_treeview_drag_begin(const Glib::RefPtr<Gdk::DragContext>& context)
{
    first_call_to_drag_data_get = true;
}

void MainWindow::on_scripts_treeview_drag_data_get(const Glib::RefPtr<Gdk::DragContext>&,
                                                   Gtk::SelectionData& selection_data, guint, guint)
{
    if (!first_call_to_drag_data_get) return;
    first_call_to_drag_data_get = false;

    // get selected script
    gig::Script* script = NULL;
    Glib::RefPtr<Gtk::TreeSelection> sel = m_TreeViewScripts.get_selection();
    Gtk::TreeModel::iterator it = sel->get_selected();
    if (it) {
        Gtk::TreeModel::Row row = *it;
        script = row[m_ScriptsModel.m_col_script];
    }
    // pass the gig::Script as pointer
    selection_data.set(selection_data.get_target(), 0/*unused*/,
                       (const guchar*)&script,
                       sizeof(script)/*length of data in bytes*/);
}

// see comment on on_sample_treeview_drag_begin()
void MainWindow::on_instruments_treeview_drag_begin(const Glib::RefPtr<Gdk::DragContext>& context)
{
    first_call_to_drag_data_get = true;
}

void MainWindow::on_instruments_treeview_drag_data_get(const Glib::RefPtr<Gdk::DragContext>&,
                                                       Gtk::SelectionData& selection_data, guint, guint)
{
    if (!first_call_to_drag_data_get) return;
    first_call_to_drag_data_get = false;

    // get selected source instrument
    gig::Instrument* src = NULL;
    {
        Glib::RefPtr<Gtk::TreeSelection> sel = m_TreeView.get_selection();
        std::vector<Gtk::TreeModel::Path> rows = sel->get_selected_rows();
        if (!rows.empty()) {
            Gtk::TreeModel::iterator it = m_refTreeModel->get_iter(rows[0]);
            if (it) {
                Gtk::TreeModel::Row row = *it;
                src = row[m_Columns.m_col_instr];
            }
        }
    }
    if (!src) return;

    // pass the source gig::Instrument as pointer
    selection_data.set(selection_data.get_target(), 0/*unused*/, (const guchar*)&src,
                       sizeof(src)/*length of data in bytes*/);
}

void MainWindow::on_instruments_treeview_drop_drag_data_received(
    const Glib::RefPtr<Gdk::DragContext>& context, int x, int y,
    const Gtk::SelectionData& selection_data, guint, guint time)
{
    gig::Instrument* src = *((gig::Instrument**) selection_data.get_data());
    if (!src || selection_data.get_length() != sizeof(gig::Instrument*))
        return;

    gig::Instrument* dst = NULL;
    {
        Gtk::TreeModel::Path path;
        const bool found = m_TreeView.get_path_at_pos(x, y, path);
        if (!found) return;

        Gtk::TreeModel::iterator iter = m_refTreeModel->get_iter(path);
        if (!iter) return;
        Gtk::TreeModel::Row row = *iter;
        dst = row[m_Columns.m_col_instr];
    }
    if (!dst) return;

    //printf("dragdrop received src=%s dst=%s\n", src->pInfo->Name.c_str(), dst->pInfo->Name.c_str());
    src->MoveTo(dst);
    __refreshEntireGUI();
    select_instrument(src);
}

// For some reason drag_data_get gets called two times for each
// drag'n'drop (at least when target is an Entry). This work-around
// makes sure the code in drag_data_get and drop_drag_data_received is
// only executed once, as drag_begin only gets called once.
void MainWindow::on_sample_treeview_drag_begin(const Glib::RefPtr<Gdk::DragContext>& context)
{
    first_call_to_drag_data_get = true;
}

void MainWindow::on_sample_treeview_drag_data_get(const Glib::RefPtr<Gdk::DragContext>&,
                                                  Gtk::SelectionData& selection_data, guint, guint)
{
    if (!first_call_to_drag_data_get) return;
    first_call_to_drag_data_get = false;

    // get selected sample
    gig::Sample* sample = NULL;
    Glib::RefPtr<Gtk::TreeSelection> sel = m_TreeViewSamples.get_selection();
    std::vector<Gtk::TreeModel::Path> rows = sel->get_selected_rows();
    if (!rows.empty()) {
        Gtk::TreeModel::iterator it = m_refSamplesTreeModel->get_iter(rows[0]);
        if (it) {
            Gtk::TreeModel::Row row = *it;
            sample = row[m_SamplesModel.m_col_sample];
        }
    }
    // pass the gig::Sample as pointer
    selection_data.set(selection_data.get_target(), 0/*unused*/, (const guchar*)&sample,
                       sizeof(sample)/*length of data in bytes*/);
}

void MainWindow::on_sample_label_drop_drag_data_received(
    const Glib::RefPtr<Gdk::DragContext>& context, int, int,
    const Gtk::SelectionData& selection_data, guint, guint time)
{
    gig::Sample* sample = *((gig::Sample**) selection_data.get_data());

    if (sample && selection_data.get_length() == sizeof(gig::Sample*)) {
        std::cout << "Drop received sample \"" <<
            sample->pInfo->Name << "\"" << std::endl;
        // drop success
        context->drop_reply(true, time);

        //TODO: we should better move most of the following code to DimRegionEdit::set_sample()

        // notify everybody that we're going to alter the region
        gig::Region* region = m_RegionChooser.get_region();
        region_to_be_changed_signal.emit(region);

        // find the samplechannel dimension
        gig::dimension_def_t* stereo_dimension = 0;
        for (int i = 0 ; i < region->Dimensions ; i++) {
            if (region->pDimensionDefinitions[i].dimension ==
                gig::dimension_samplechannel) {
                stereo_dimension = &region->pDimensionDefinitions[i];
                break;
            }
        }
        bool channels_changed = false;
        if (sample->Channels == 1 && stereo_dimension) {
            // remove the samplechannel dimension
/* commented out, because it makes it impossible building up an instrument from scratch using two separate L/R samples
            region->DeleteDimension(stereo_dimension);
            channels_changed = true;
            region_changed();
*/
        }
        dimreg_edit.set_sample(
            sample,
            is_copy_samples_unity_note_enabled(),
            is_copy_samples_fine_tune_enabled(),
            is_copy_samples_loop_enabled()
        );

        if (sample->Channels == 2 && !stereo_dimension) {
            // add samplechannel dimension
            gig::dimension_def_t dim;
            dim.dimension = gig::dimension_samplechannel;
            dim.bits = 1;
            dim.zones = 2;
            region->AddDimension(&dim);
            channels_changed = true;
            region_changed();
        }
        if (channels_changed) {
            // unmap all samples with wrong number of channels
            // TODO: maybe there should be a warning dialog for this
            for (int i = 0 ; i < region->DimensionRegions ; i++) {
                gig::DimensionRegion* d = region->pDimensionRegions[i];
                if (d->pSample && d->pSample->Channels != sample->Channels) {
                    gig::Sample* oldref = d->pSample;
                    d->pSample = NULL;
                    sample_ref_changed_signal.emit(oldref, NULL);
                }
            }
        }

        // notify we're done with altering
        region_changed_signal.emit(region);

        file_changed();

        return;
    }
    // drop failed
    context->drop_reply(false, time);
}

void MainWindow::sample_name_changed(const Gtk::TreeModel::Path& path,
                                     const Gtk::TreeModel::iterator& iter) {
    if (!iter) return;
    Gtk::TreeModel::Row row = *iter;
    Glib::ustring name  = row[m_SamplesModel.m_col_name];
    gig::Group* group   = row[m_SamplesModel.m_col_group];
    gig::Sample* sample = row[m_SamplesModel.m_col_sample];
    gig::String gigname(gig_from_utf8(name));
    if (group) {
        if (group->Name != gigname) {
            group->Name = gigname;
            printf("group name changed\n");
            file_changed();
        }
    } else if (sample) {
        if (sample->pInfo->Name != gigname) {
            sample->pInfo->Name = gigname;
            printf("sample name changed\n");
            file_changed();
        }
    }
    // change name in the sample properties window
    if (sampleProps.get_sample() == sample && sample) {
        sampleProps.set_sample(sample);
    }
}

void MainWindow::script_name_changed(const Gtk::TreeModel::Path& path,
                                     const Gtk::TreeModel::iterator& iter) {
    if (!iter) return;
    Gtk::TreeModel::Row row = *iter;
    Glib::ustring name      = row[m_ScriptsModel.m_col_name];
    gig::ScriptGroup* group = row[m_ScriptsModel.m_col_group];
    gig::Script* script     = row[m_ScriptsModel.m_col_script];
    gig::String gigname(gig_from_utf8(name));
    if (group) {
        if (group->Name != gigname) {
            group->Name = gigname;
            printf("script group name changed\n");
            file_changed();
        }
    } else if (script) {
        if (script->Name != gigname) {
            script->Name = gigname;
            printf("script name changed\n");
            file_changed();
        }
    }
}

void MainWindow::script_double_clicked(const Gtk::TreeModel::Path& path,
                                       Gtk::TreeViewColumn* column)
{
    Gtk::TreeModel::iterator iter = m_refScriptsTreeModel->get_iter(path);
    if (!iter) return;
    Gtk::TreeModel::Row row = *iter;
    gig::Script* script = row[m_ScriptsModel.m_col_script];
    if (!script) return;

    ScriptEditor* editor = new ScriptEditor;
    editor->signal_script_to_be_changed.connect(
        signal_script_to_be_changed.make_slot()
    );
    editor->signal_script_changed.connect(
        signal_script_changed.make_slot()
    );
    editor->setScript(script);
    //editor->reparent(*this);
    editor->show();
}

void MainWindow::instrument_name_changed(const Gtk::TreeModel::Path& path,
                                         const Gtk::TreeModel::iterator& iter) {
    if (!iter) return;
    Gtk::TreeModel::Row row = *iter;
    Glib::ustring name = row[m_Columns.m_col_name];

#if !USE_GTKMM_BUILDER
    // change name in instrument menu
    int index = path[0];
    const std::vector<Gtk::Widget*> children = instrument_menu->get_children();
    if (index < children.size()) {
#if (GTKMM_MAJOR_VERSION == 2 && GTKMM_MINOR_VERSION >= 16) || GTKMM_MAJOR_VERSION > 2
        static_cast<Gtk::RadioMenuItem*>(children[index])->set_label(name);
#else
        remove_instrument_from_menu(index);
        Gtk::RadioMenuItem* item = add_instrument_to_menu(name, index);
        item->set_active();
#endif
    }
#endif

    // change name in gig
    gig::Instrument* instrument = row[m_Columns.m_col_instr];
    gig::String gigname(gig_from_utf8(name));
    if (instrument && instrument->pInfo->Name != gigname) {
        instrument->pInfo->Name = gigname;

        // change name in the instrument properties window
        if (instrumentProps.get_instrument() == instrument) {
            instrumentProps.update_name();
        }

        file_changed();
    }
}

bool MainWindow::instrument_row_visible(const Gtk::TreeModel::const_iterator& iter) {
    if (!iter)
        return true;

    Glib::ustring pattern = m_searchText.get_text().lowercase();
    trim(pattern);
    if (pattern.empty()) return true;

#if GTKMM_MAJOR_VERSION > 3 || (GTKMM_MAJOR_VERSION == 3 && GTKMM_MINOR_VERSION > 24)
    //HACK: on GTKMM4 development branch const_iterator cannot be easily converted to iterator, probably going to be fixed before final GTKMM4 release though.
    Gtk::TreeModel::Row row = **(Gtk::TreeModel::iterator*)(&iter);
#else
    Gtk::TreeModel::Row row = *iter;
#endif
    Glib::ustring name = row[m_Columns.m_col_name];
    name = name.lowercase();

    std::vector<Glib::ustring> tokens = Glib::Regex::split_simple(" ", pattern);
    for (int t = 0; t < tokens.size(); ++t)
        if (name.find(tokens[t]) == Glib::ustring::npos)
            return false;

    return true;
}

void MainWindow::on_action_combine_instruments() {
    CombineInstrumentsDialog* d = new CombineInstrumentsDialog(*this, file);

    // take over selection from instruments list view for the combine dialog's
    // list view as pre-selection
    std::set<int> indeces;
    {
        Glib::RefPtr<Gtk::TreeSelection> sel = m_TreeView.get_selection();
        std::vector<Gtk::TreeModel::Path> rows = sel->get_selected_rows();
        for (int r = 0; r < rows.size(); ++r) {
            Gtk::TreeModel::iterator it = m_refTreeModel->get_iter(rows[r]);
            if (it) {
                Gtk::TreeModel::Row row = *it;
                int index = row[m_Columns.m_col_nr];
                indeces.insert(index);
            }
        }
    }
    d->setSelectedInstruments(indeces);

#if HAS_GTKMM_SHOW_ALL_CHILDREN
    d->show_all();
#else
    d->show();
#endif
    d->run();
    if (d->fileWasChanged()) {
        // update GUI with new instrument just created
        add_instrument(d->newCombinedInstrument());
    }
    delete d;
}

void MainWindow::on_action_view_references() {
    Glib::RefPtr<Gtk::TreeSelection> sel = m_TreeViewSamples.get_selection();
    std::vector<Gtk::TreeModel::Path> rows = sel->get_selected_rows();
    if (rows.empty()) return;
    Gtk::TreeModel::iterator it = m_refSamplesTreeModel->get_iter(rows[0]);
    if (!it) return;
    Gtk::TreeModel::Row row = *it;
    gig::Sample* sample = row[m_SamplesModel.m_col_sample];
    if (!sample) return;

    ReferencesView* d = new ReferencesView(*this);
    d->setSample(sample);
    d->dimension_region_selected.connect(
        sigc::mem_fun(*this, &MainWindow::select_dimension_region)
    );
#if HAS_GTKMM_SHOW_ALL_CHILDREN
    d->show_all();
#else
    d->show();
#endif
    d->resize(500, 400);
    d->run();
    delete d;
}

void MainWindow::mergeFiles(const std::vector<std::string>& filenames) {
    struct _Source {
        std::vector<RIFF::File*> riffs;
        std::vector<gig::File*> gigs;
        
        ~_Source() {
            for (int k = 0; k < gigs.size(); ++k) delete gigs[k];
            for (int k = 0; k < riffs.size(); ++k) delete riffs[k];
            riffs.clear();
            gigs.clear();
        }
    } sources;

    if (filenames.empty())
        throw RIFF::Exception(_("No files selected, so nothing done."));

    // first open all input files (to avoid output file corruption)
    int i;
    try {
        for (i = 0; i < filenames.size(); ++i) {
            const std::string& filename = filenames[i];
            printf("opening file=%s\n", filename.c_str());

            RIFF::File* riff = new RIFF::File(filename);
            sources.riffs.push_back(riff);

            gig::File* gig = new gig::File(riff);
            sources.gigs.push_back(gig);
        }
    } catch (RIFF::Exception e) {
        throw RIFF::Exception(
            _("Error occurred while opening '") +
            filenames[i] +
            "': " +
            e.Message
        );
    } catch (...) {
        throw RIFF::Exception(
            _("Unknown exception occurred while opening '") + 
            filenames[i] + "'"
        );
    }

    // now merge the opened .gig files to the main .gig file currently being
    // open in gigedit
    try {
        for (i = 0; i < filenames.size(); ++i) {
            const std::string& filename = filenames[i];
            printf("merging file=%s\n", filename.c_str());
            assert(i < sources.gigs.size());

            this->file->AddContentOf(sources.gigs[i]);
        }
    } catch (RIFF::Exception e) {
        throw RIFF::Exception(
            _("Error occurred while merging '") +
            filenames[i] +
            "': " +
            e.Message
        );
    } catch (...) {
        throw RIFF::Exception(
            _("Unknown exception occurred while merging '") + 
            filenames[i] + "'"
        );
    }

    // Finally save gig file persistently to disk ...
    //NOTE: requires that this gig file already has a filename !
    {
        std::cout << "Saving file\n" << std::flush;
        file_structure_to_be_changed_signal.emit(this->file);

        progress_dialog = new ProgressDialog( //FIXME: memory leak!
            _("Saving") +  Glib::ustring(" '") +
            Glib::filename_display_basename(this->filename) + "' ...",
            *this
        );
#if HAS_GTKMM_SHOW_ALL_CHILDREN
        progress_dialog->show_all();
#else
        progress_dialog->show();
#endif
        saver = new Saver(this->file); //FIXME: memory leak!
        saver->signal_progress().connect(
            sigc::mem_fun(*this, &MainWindow::on_saver_progress));
        saver->signal_finished().connect(
            sigc::mem_fun(*this, &MainWindow::on_saver_finished));
        saver->signal_error().connect(
            sigc::mem_fun(*this, &MainWindow::on_saver_error));
        saver->launch();
    }
}

void MainWindow::on_action_merge_files() {
    if (this->file->GetFileName().empty()) {
        Glib::ustring txt = _(
            "You seem to have a new .gig file open that has not been saved "
            "yet. You must save it somewhere before starting to merge it with "
            "other .gig files though, because during the merge operation the "
            "other files' sample data must be written on file level to the "
            "target .gig file."
        );
        Gtk::MessageDialog msg(*this, txt, false, Gtk::MESSAGE_ERROR);
        msg.run();
        return;
    }

    Gtk::FileChooserDialog dialog(*this, _("Merge .gig files"));
#if HAS_GTKMM_STOCK
    dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
#else
    dialog.add_button(_("_Cancel"), Gtk::RESPONSE_CANCEL);
#endif
    dialog.add_button(_("Merge"), Gtk::RESPONSE_OK);
    dialog.set_default_response(Gtk::RESPONSE_CANCEL);
#if (GTKMM_MAJOR_VERSION == 2 && GTKMM_MINOR_VERSION < 90) || GTKMM_MAJOR_VERSION < 2
    Gtk::FileFilter filter;
    filter.add_pattern("*.gig");
#else
    Glib::RefPtr<Gtk::FileFilter> filter = Gtk::FileFilter::create();
    filter->add_pattern("*.gig");
#endif
    dialog.set_filter(filter);
    if (current_gig_dir != "") {
        dialog.set_current_folder(current_gig_dir);
    }
    dialog.set_select_multiple(true);

    // show warning in the file picker dialog
    HBox descriptionArea;
    descriptionArea.set_spacing(15);
    Gtk::Image warningIcon;
    warningIcon.set_from_icon_name("dialog-warning",
                                   Gtk::IconSize(Gtk::ICON_SIZE_DIALOG));
    descriptionArea.pack_start(warningIcon, Gtk::PACK_SHRINK);
#if GTKMM_MAJOR_VERSION < 3
    view::WrapLabel description;
#else
    Gtk::Label description;
    description.set_line_wrap();
#endif
    description.set_markup(_(
        "\nSelect at least one .gig file that shall be merged to the .gig file "
        "currently being open in gigedit.\n\n"
        "<b>Please Note:</b> Merging with other files will modify your "
        "currently open .gig file on file level! And be aware that the current "
        "merge algorithm does not detect duplicate samples yet. So if you are "
        "merging files which are using equivalent sample data, those "
        "equivalent samples will currently be treated as separate samples and "
        "will accordingly be stored separately in the target .gig file!"
    ));
    descriptionArea.pack_start(description);
#if USE_GTKMM_BOX
# warning No description area implemented for dialog on GTKMM 3
#else
    dialog.get_vbox()->pack_start(descriptionArea, Gtk::PACK_SHRINK);
#endif
#if HAS_GTKMM_SHOW_ALL_CHILDREN
    descriptionArea.show_all();
#else
    descriptionArea.show();
#endif

    if (dialog.run() == Gtk::RESPONSE_OK) {
        dialog.hide();
#ifdef GLIB_THREADS
        printf("on_action_merge_files self=%p\n",
               static_cast<void*>(Glib::Threads::Thread::self()));
#else
        std::cout << "on_action_merge_files self=" <<
            std::this_thread::get_id() << "\n";
#endif
        std::vector<std::string> filenames = dialog.get_filenames();

        // merge the selected files to the currently open .gig file
        try {
            mergeFiles(filenames);
        } catch (RIFF::Exception e) {
            Gtk::MessageDialog msg(*this, e.Message, false, Gtk::MESSAGE_ERROR);
            msg.run();
        }

        // update GUI
        __refreshEntireGUI();
    }
}

void MainWindow::set_file_is_shared(bool b) {
    this->file_is_shared = b;

    if (file_is_shared) {
        m_AttachedStateLabel.set_label(_("live-mode"));
        m_AttachedStateImage.set(
            Gdk::Pixbuf::create_from_xpm_data(status_attached_xpm)
        );
    } else {
        m_AttachedStateLabel.set_label(_("stand-alone"));
        m_AttachedStateImage.set(
            Gdk::Pixbuf::create_from_xpm_data(status_detached_xpm)
        );
    }

    {
#if USE_GTKMM_BUILDER
        m_actionToggleSyncSamplerSelection->property_enabled() = b;
#else
        Gtk::MenuItem* item = dynamic_cast<Gtk::MenuItem*>(
            uiManager->get_widget("/MenuBar/MenuSettings/SyncSamplerInstrumentSelection"));
        if (item) item->set_sensitive(b);
#endif
    }
}

void MainWindow::on_sample_ref_count_incremented(gig::Sample* sample, int offset) {
    if (!sample) return;
    sample_ref_count[sample] += offset;
    const int refcount = sample_ref_count[sample];

    Glib::RefPtr<Gtk::TreeModel> model = m_TreeViewSamples.get_model();
    for (int g = 0; g < model->children().size(); ++g) {
        Gtk::TreeModel::Row rowGroup = model->children()[g];
        for (int s = 0; s < rowGroup.children().size(); ++s) {
            Gtk::TreeModel::Row rowSample = rowGroup.children()[s];
            if (rowSample[m_SamplesModel.m_col_sample] != sample) continue;
            rowSample[m_SamplesModel.m_col_refcount] = ToString(refcount) + " " + _("Refs.");
            rowSample[m_SamplesModel.m_color] = refcount ? "black" : "red";
        }
    }
}

void MainWindow::on_sample_ref_changed(gig::Sample* oldSample, gig::Sample* newSample) {
    on_sample_ref_count_incremented(oldSample, -1);
    on_sample_ref_count_incremented(newSample, +1);
}

void MainWindow::on_samples_to_be_removed(std::list<gig::Sample*> samples) {
    // just in case a new sample is added later with exactly the same memory
    // address, which would lead to incorrect refcount if not deleted here
    for (std::list<gig::Sample*>::const_iterator it = samples.begin();
         it != samples.end(); ++it)
    {
        sample_ref_count.erase(*it);
    }
}

void MainWindow::show_samples_tab() {
    m_TreeViewNotebook.set_current_page(0);
}

void MainWindow::show_intruments_tab() {
    m_TreeViewNotebook.set_current_page(1);
}

void MainWindow::show_scripts_tab() {
    m_TreeViewNotebook.set_current_page(2);
}

void MainWindow::select_instrument_by_dir(int dir) {
    if (!file) return;
    gig::Instrument* pInstrument = get_instrument();
    if (!pInstrument) {
        select_instrument( file->GetInstrument(0) );
        return;
    }
    for (int i = 0; file->GetInstrument(i); ++i) {
        if (file->GetInstrument(i) == pInstrument) {
            select_instrument( file->GetInstrument(i + dir) );
            return;
        }
    }
}

void MainWindow::select_prev_instrument() {
    select_instrument_by_dir(-1);
}

void MainWindow::select_next_instrument() {
    select_instrument_by_dir(1);
}

void MainWindow::select_prev_region() {
    m_RegionChooser.select_prev_region();
}

void MainWindow::select_next_region() {
    m_RegionChooser.select_next_region();
}

void MainWindow::select_next_dim_rgn_zone() {
    if (m_DimRegionChooser.has_focus()) return; // avoid conflict with key stroke handler of DimenionRegionChooser
    m_DimRegionChooser.select_next_dimzone();
}

void MainWindow::select_prev_dim_rgn_zone() {
    if (m_DimRegionChooser.has_focus()) return; // avoid conflict with key stroke handler of DimenionRegionChooser
    m_DimRegionChooser.select_prev_dimzone();
}

void MainWindow::select_add_next_dim_rgn_zone() {
    m_DimRegionChooser.select_next_dimzone(true);
}

void MainWindow::select_add_prev_dim_rgn_zone() {
    m_DimRegionChooser.select_prev_dimzone(true);
}

void MainWindow::select_prev_dimension() {
    if (m_DimRegionChooser.has_focus()) return; // avoid conflict with key stroke handler of DimenionRegionChooser
    m_DimRegionChooser.select_prev_dimension();
}

void MainWindow::select_next_dimension() {
    if (m_DimRegionChooser.has_focus()) return; // avoid conflict with key stroke handler of DimenionRegionChooser
    m_DimRegionChooser.select_next_dimension();
}

#define CLIPBOARD_DIMENSIONREGION_TARGET \
    ("libgig.DimensionRegion." + m_serializationArchive.rawDataFormat())

void MainWindow::copy_selected_dimrgn() {
    gig::DimensionRegion* pDimRgn = m_DimRegionChooser.get_main_dimregion();
    if (!pDimRgn) {
        updateClipboardPasteAvailable();
        updateClipboardCopyAvailable();
        return;
    }

    std::vector<Gtk::TargetEntry> targets;
    targets.push_back( Gtk::TargetEntry(CLIPBOARD_DIMENSIONREGION_TARGET) );

    Glib::RefPtr<Gtk::Clipboard> clipboard = Gtk::Clipboard::get();
    clipboard->set(
        targets,
        sigc::mem_fun(*this, &MainWindow::on_clipboard_get),
        sigc::mem_fun(*this, &MainWindow::on_clipboard_clear)
    );

    m_serializationArchive.serialize(pDimRgn);

    updateClipboardPasteAvailable();
}

void MainWindow::paste_copied_dimrgn() {
    Glib::RefPtr<Gtk::Clipboard> clipboard = Gtk::Clipboard::get();
    clipboard->request_contents(
        CLIPBOARD_DIMENSIONREGION_TARGET,
        sigc::mem_fun(*this, &MainWindow::on_clipboard_received)
    );
    updateClipboardPasteAvailable();
}

void MainWindow::adjust_clipboard_content() {
    MacroEditor* editor = new MacroEditor();
    editor->setMacro(&m_serializationArchive, true);
    editor->show();
}

void MainWindow::updateClipboardPasteAvailable() {
    Glib::RefPtr<Gtk::Clipboard> clipboard = Gtk::Clipboard::get();
    clipboard->request_targets(
        sigc::mem_fun(*this, &MainWindow::on_clipboard_received_targets)
    );
}

void MainWindow::updateClipboardCopyAvailable() {
    bool bDimensionRegionCopyIsPossible = m_DimRegionChooser.get_main_dimregion();
#if USE_GTKMM_BUILDER
    m_actionCopyDimRgn->property_enabled() = bDimensionRegionCopyIsPossible;
#else
    static_cast<Gtk::MenuItem*>(
        uiManager->get_widget("/MenuBar/MenuEdit/CopyDimRgn")
    )->set_sensitive(bDimensionRegionCopyIsPossible);
#endif
}

#if GTKMM_MAJOR_VERSION > 3 || (GTKMM_MAJOR_VERSION == 3 && (GTKMM_MINOR_VERSION > 91 || (GTKMM_MINOR_VERSION == 91 && GTKMM_MICRO_VERSION >= 2))) // GTKMM >= 3.91.2
void MainWindow::on_clipboard_owner_change(Gdk::EventOwnerChange& event) {
#else
void MainWindow::on_clipboard_owner_change(GdkEventOwnerChange* event) {
#endif
    updateClipboardPasteAvailable();
}

void MainWindow::on_clipboard_get(Gtk::SelectionData& selection_data, guint /*info*/) {
    const std::string target = selection_data.get_target();
    if (target == CLIPBOARD_DIMENSIONREGION_TARGET) {
        selection_data.set(
            CLIPBOARD_DIMENSIONREGION_TARGET, 8 /* "format": probably unused*/,
            &m_serializationArchive.rawData()[0],
            m_serializationArchive.rawData().size()
        );
    } else {
        std::cerr << "Clipboard: content for unknown target '" << target << "' requested\n";
    }
}

void MainWindow::on_clipboard_clear() {
    m_serializationArchive.clear();
    updateClipboardPasteAvailable();
    updateClipboardCopyAvailable();
}

//NOTE: Might throw exception !!!
void MainWindow::applyMacro(Serialization::Archive& macro) {
    gig::DimensionRegion* pDimRgn = m_DimRegionChooser.get_main_dimregion();
    if (!pDimRgn) return;

    for (std::set<gig::DimensionRegion*>::iterator itDimReg = dimreg_edit.dimregs.begin();
         itDimReg != dimreg_edit.dimregs.end(); ++itDimReg)
    {
        gig::DimensionRegion* pDimRgn = *itDimReg;
        DimRegionChangeGuard(this, pDimRgn);
        macro.deserialize(pDimRgn);
    }
    //region_changed()
    file_changed();
    dimreg_changed();
}

void MainWindow::on_clipboard_received(const Gtk::SelectionData& selection_data) {
    const std::string target = selection_data.get_target();
    if (target == CLIPBOARD_DIMENSIONREGION_TARGET) {
        Glib::ustring errorText;
        try {
            m_serializationArchive.decode(
                selection_data.get_data(), selection_data.get_length()
            );
            applyMacro(m_serializationArchive);
        } catch (Serialization::Exception e) {
            errorText = e.Message;
        } catch (...) {
            errorText = _("Unknown exception while pasting DimensionRegion");
        }
        if (!errorText.empty()) {
            Glib::ustring txt = _("Pasting DimensionRegion failed:\n") + errorText;
            Gtk::MessageDialog msg(*this, txt, false, Gtk::MESSAGE_ERROR);
            msg.run();
        }
    }
}

void MainWindow::on_clipboard_received_targets(const std::vector<Glib::ustring>& targets) {
    const bool bDimensionRegionPasteIsPossible =
        std::find(targets.begin(), targets.end(),
                  CLIPBOARD_DIMENSIONREGION_TARGET) != targets.end();

#if USE_GTKMM_BUILDER
    m_actionPasteDimRgn->property_enabled() = bDimensionRegionPasteIsPossible;
    m_actionAdjustClipboard->property_enabled() = bDimensionRegionPasteIsPossible;
#else
    static_cast<Gtk::MenuItem*>(
        uiManager->get_widget("/MenuBar/MenuEdit/PasteDimRgn")
    )->set_sensitive(bDimensionRegionPasteIsPossible);

    static_cast<Gtk::MenuItem*>(
        uiManager->get_widget("/MenuBar/MenuEdit/AdjustClipboard")
    )->set_sensitive(bDimensionRegionPasteIsPossible);
#endif
}

sigc::signal<void, gig::File*>& MainWindow::signal_file_structure_to_be_changed() {
    return file_structure_to_be_changed_signal;
}

sigc::signal<void, gig::File*>& MainWindow::signal_file_structure_changed() {
    return file_structure_changed_signal;
}

sigc::signal<void, std::list<gig::Sample*> >& MainWindow::signal_samples_to_be_removed() {
    return samples_to_be_removed_signal;
}

sigc::signal<void>& MainWindow::signal_samples_removed() {
    return samples_removed_signal;
}

sigc::signal<void, gig::Region*>& MainWindow::signal_region_to_be_changed() {
    return region_to_be_changed_signal;
}

sigc::signal<void, gig::Region*>& MainWindow::signal_region_changed() {
    return region_changed_signal;
}

sigc::signal<void, gig::Sample*>& MainWindow::signal_sample_changed() {
    return sample_changed_signal;
}

sigc::signal<void, gig::Sample*/*old*/, gig::Sample*/*new*/>& MainWindow::signal_sample_ref_changed() {
    return sample_ref_changed_signal;
}

sigc::signal<void, gig::DimensionRegion*>& MainWindow::signal_dimreg_to_be_changed() {
    return dimreg_to_be_changed_signal;
}

sigc::signal<void, gig::DimensionRegion*>& MainWindow::signal_dimreg_changed() {
    return dimreg_changed_signal;
}

sigc::signal<void, int/*key*/, int/*velocity*/>& MainWindow::signal_note_on() {
    return note_on_signal;
}

sigc::signal<void, int/*key*/, int/*velocity*/>& MainWindow::signal_note_off() {
    return note_off_signal;
}

sigc::signal<void, int/*key*/, int/*velocity*/>& MainWindow::signal_keyboard_key_hit() {
    return m_RegionChooser.signal_keyboard_key_hit();
}

sigc::signal<void, int/*key*/, int/*velocity*/>& MainWindow::signal_keyboard_key_released() {
    return m_RegionChooser.signal_keyboard_key_released();
}

sigc::signal<void, gig::Instrument*>& MainWindow::signal_switch_sampler_instrument() {
    return switch_sampler_instrument_signal;
}
