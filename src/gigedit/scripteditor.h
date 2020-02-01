/*
    Copyright (c) 2014-2020 Christian Schoenebeck
    
    This file is part of "gigedit" and released under the terms of the
    GNU General Public License version 2.
*/

#ifndef GIGEDIT_SCRIPTEDITOR_H
#define GIGEDIT_SCRIPTEDITOR_H

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
#if HAVE_CONFIG_H
# include <config.h>
#endif
#include "compat.h"

#if USE_GTKMM_BUILDER
# include <gtkmm/builder.h>
#else
# include <gtkmm/uimanager.h> // deprecated in gtkmm >= 3.21.4
#endif

#if USE_GLIB_ACTION
# include <giomm/simpleactiongroup.h>
#else
# include <gtkmm/actiongroup.h>
#endif

#include "ManagedWindow.h"

// Should we use a very simple (and buggy) local NKSP syntax parser, or should
// we rather use the full featured NKSP syntax highlighting backend from
// liblinuxsampler for syntax highlighting of this text editor?
#if HAVE_LINUXSAMPLER
# define USE_LS_SCRIPTVM 1
#endif

#if USE_LS_SCRIPTVM
# ifdef LIBLINUXSAMPLER_HEADER_FILE
#  include LIBLINUXSAMPLER_HEADER_FILE(scriptvm/ScriptVM.h)
#  include LIBLINUXSAMPLER_HEADER_FILE(scriptvm/ScriptVMFactory.h)
# else
#  include <linuxsampler/scriptvm/ScriptVM.h>
#  include <linuxsampler/scriptvm/ScriptVMFactory.h>
# endif
#endif

class ScriptEditor : public ManagedWindow {
public:
    ScriptEditor();
   ~ScriptEditor();
    void setScript(gig::Script* script);

    sigc::signal<void, gig::Script*> signal_script_to_be_changed;
    sigc::signal<void, gig::Script*> signal_script_changed;

    // implementation for abstract methods of interface class "ManagedWindow"
    virtual Settings::Property<int>* windowSettingX() { return &Settings::singleton()->scriptEditorWindowX; }
    virtual Settings::Property<int>* windowSettingY() { return &Settings::singleton()->scriptEditorWindowY; }
    virtual Settings::Property<int>* windowSettingWidth() { return &Settings::singleton()->scriptEditorWindowW; }
    virtual Settings::Property<int>* windowSettingHeight() { return &Settings::singleton()->scriptEditorWindowH; }

protected:
    VBox m_vbox;
    HBox m_footerHBox;
    HBox m_statusHBox;
    HButtonBox m_buttonBox;
    Gtk::ScrolledWindow m_scrolledWindow;
    HBox m_textViewHBox;
    Glib::RefPtr<Gtk::TextBuffer> m_lineNrBuffer;
    Glib::RefPtr<Gtk::TextBuffer> m_textBuffer;
    Glib::RefPtr<Gtk::TextBuffer::TagTable> m_tagTable;
    Glib::RefPtr<Gtk::TextBuffer::Tag> m_keywordTag;
    Glib::RefPtr<Gtk::TextBuffer::Tag> m_eventTag;
    Glib::RefPtr<Gtk::TextBuffer::Tag> m_variableTag;
    Glib::RefPtr<Gtk::TextBuffer::Tag> m_functionTag;
    Glib::RefPtr<Gtk::TextBuffer::Tag> m_numberTag;
    Glib::RefPtr<Gtk::TextBuffer::Tag> m_stringTag;
    Glib::RefPtr<Gtk::TextBuffer::Tag> m_patchTag;
    Glib::RefPtr<Gtk::TextBuffer::Tag> m_commentTag;
    Glib::RefPtr<Gtk::TextBuffer::Tag> m_preprocTag;
    Glib::RefPtr<Gtk::TextBuffer::Tag> m_errorTag;
    Glib::RefPtr<Gtk::TextBuffer::Tag> m_warningTag;
    Glib::RefPtr<Gtk::TextBuffer::Tag> m_preprocCommentTag;
    Glib::RefPtr<Gtk::TextBuffer::Tag> m_lineNrTag;
    Glib::RefPtr<Gtk::TextBuffer::Tag> m_metricTag;
    Glib::RefPtr<Gtk::TextBuffer::Tag> m_stdUnitTag;
    Gtk::TextView m_lineNrView;
    Gtk::TextView m_lineNrTextViewSpacer;
    Gtk::TextView m_textView;
    Gtk::Image m_statusImage;
    Gtk::Label m_statusLabel;
    Gtk::Button m_applyButton;
    Gtk::Button m_cancelButton;

#if GTKMM_MAJOR_VERSION > 3 || (GTKMM_MAJOR_VERSION == 3 && GTKMM_MINOR_VERSION >= 20)
    Glib::RefPtr<Gtk::CssProvider> m_css;
#endif

    Glib::RefPtr<Gdk::Pixbuf> m_warningIcon;
    Glib::RefPtr<Gdk::Pixbuf> m_errorIcon;
    Glib::RefPtr<Gdk::Pixbuf> m_successIcon;

#if USE_GLIB_ACTION
    Glib::RefPtr<Gio::SimpleActionGroup> m_actionGroup;
#else
    Glib::RefPtr<Gtk::ActionGroup> m_actionGroup;
#endif
#if USE_GTKMM_BUILDER
    Glib::RefPtr<Gtk::Builder> m_uiManager;
#else
    Glib::RefPtr<Gtk::UIManager> m_uiManager;
#endif

    gig::Script* m_script;
#if USE_LS_SCRIPTVM
    LinuxSampler::ScriptVM* m_vm;
    std::vector<LinuxSampler::ParserIssue> m_issues;
    std::vector<LinuxSampler::ParserIssue> m_errors;
    std::vector<LinuxSampler::ParserIssue> m_warnings;
    std::vector<LinuxSampler::CodeBlock> m_preprocComments;
#endif

    bool isModified() const;
    void onButtonCancel();
    void onButtonApply();
    void onWindowHide();
    void onTextInserted(const Gtk::TextBuffer::iterator& it, const Glib::ustring& txt, int length);
    void onTextErased(const Gtk::TextBuffer::iterator& itStart, const Gtk::TextBuffer::iterator& itEnd);
    void onModifiedChanged();
#if USE_LS_SCRIPTVM
    void updateSyntaxHighlightingByVM();
    void updateParserIssuesByVM();
    LinuxSampler::ScriptVM* GetScriptVM();
    void updateIssueTooltip(GdkEventMotion* e);
    void updateStatusBar();
#endif
    bool on_motion_notify_event(GdkEventMotion* e);
#if GTKMM_MAJOR_VERSION > 3 || (GTKMM_MAJOR_VERSION == 3 && (GTKMM_MINOR_VERSION > 91 || (GTKMM_MINOR_VERSION == 91 && GTKMM_MICRO_VERSION >= 2))) // GTKMM >= 3.91.2
    bool onWindowDelete(Gdk::Event& e);
#endif
    bool onWindowDeleteP(GdkEventAny* e);
    void onMenuChangeFontSize();
    int  currentFontSize() const;
    void setFontSize(int size, bool save);
    void updateLineNumbers();
};

#endif // GIGEDIT_SCRIPTEDITOR_H
