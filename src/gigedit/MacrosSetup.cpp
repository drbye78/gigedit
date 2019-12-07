/*
    Copyright (c) MMXVII - MMXIX Christian Schoenebeck

    This file is part of "gigedit" and released under the terms of the
    GNU General Public License version 2.
*/

#include "MacrosSetup.h"
#include "compat.h"
#include "global.h"
#include <assert.h>
#include <set>
#include <math.h>
#if HAS_GTKMM_STOCK
# include <gtkmm/stock.h>
#endif
#include "MacroEditor.h"

MacrosSetup::MacrosSetup() :
    m_modified(false),
    m_clipboardContent(NULL),
    m_addFromClipboardButton("  " + Glib::ustring(_("From Clipboard")) + "  " + UNICODE_PRIMARY_KEY_SYMBOL + "B"),
    m_addFromSelectionButton("  " + Glib::ustring(_("From Selection")) + "  " + UNICODE_PRIMARY_KEY_SYMBOL + "S"),
#if HAS_GTKMM_STOCK
    m_buttonUp(Gtk::Stock::GO_UP),
    m_buttonDown(Gtk::Stock::GO_DOWN),
    m_buttonEdit(Gtk::Stock::EDIT),
#else
    m_buttonUp(_("Up"), true),
    m_buttonDown(_("Down"), true),
    m_buttonEdit(_("Edit"), true),
#endif
    m_buttonDuplicate(_("Duplicate")),
    m_statusLabel("",  Gtk::ALIGN_START),
    m_labelComment(_("Comment"), Gtk::ALIGN_START),
    m_deleteButton("  " + Glib::ustring(_("Delete")) + "  " + UNICODE_PRIMARY_KEY_SYMBOL + UNICODE_ERASE_KEY_SYMBOL),
    m_inverseDeleteButton("  " + Glib::ustring(_("Inverse Delete")) + "  " + UNICODE_ALT_KEY_SYMBOL + UNICODE_ERASE_KEY_SYMBOL),
#if HAS_GTKMM_STOCK
    m_applyButton(Gtk::Stock::APPLY),
    m_cancelButton(Gtk::Stock::CANCEL),
#else
    m_applyButton(_("Apply"), true),
    m_cancelButton(_("_Cancel"), true),
#endif
    m_altKeyDown(false),
    m_primaryKeyDown(false)
{
    add(m_vbox);

    set_title(_("Setup Macros"));

#if !HAS_GTKMM_STOCK
    // see : https://specifications.freedesktop.org/icon-naming-spec/icon-naming-spec-latest.html
    m_buttonUp.set_icon_name("go-up");
    m_buttonDown.set_icon_name("go-down");
    m_buttonEdit.set_icon_name("insert-text");
#endif

    if (!Settings::singleton()->autoRestoreWindowDimension) {
        set_default_size(680, 500);
        set_position(Gtk::WIN_POS_CENTER);
    }

#if GTKMM_MAJOR_VERSION > 3 || (GTKMM_MAJOR_VERSION == 3 && GTKMM_MINOR_VERSION >= 12)
    m_labelIntro.set_margin_start(10);
    m_labelIntro.set_margin_end(10);
#else
    m_labelIntro.set_padding(10, 10);
#endif

#if GTKMM_MAJOR_VERSION >= 3
    m_labelIntro.set_line_wrap();
#endif
    m_labelIntro.set_text(
        _("A macro is a list of parameters and corresponding values which "
          "should be applied to the instrument editor when the macro is "
          "triggered by the user. A macro is triggered either by selecting "
          "the macro from the \"Macro\" menu, or by hitting the macro's "
          "respective keyboard accelerator (F1 to F12), or by applying a "
          "previously copied macro from the clipboard.")
    );
    m_vbox.pack_start(m_labelIntro, Gtk::PACK_SHRINK);

#if HAS_GTKMM_STOCK
    m_addFromClipboardButton.set_image(
        *new Gtk::Image(Gtk::Stock::ADD, Gtk::ICON_SIZE_BUTTON)
    );
    m_addFromSelectionButton.set_image(
        *new Gtk::Image(Gtk::Stock::ADD, Gtk::ICON_SIZE_BUTTON)
    );
    m_buttonDuplicate.set_image(
        *new Gtk::Image(Gtk::Stock::COPY, Gtk::ICON_SIZE_BUTTON)
    );
    m_deleteButton.set_image(
        *new Gtk::Image(Gtk::Stock::DELETE, Gtk::ICON_SIZE_BUTTON)
    );
    m_inverseDeleteButton.set_image(
        *new Gtk::Image(Gtk::Stock::DELETE, Gtk::ICON_SIZE_BUTTON)
    );
#else // since GTKMM 3.90 ...
    // see : https://specifications.freedesktop.org/icon-naming-spec/icon-naming-spec-latest.html
    m_addFromClipboardButton.set_icon_name("list-add");
    m_addFromSelectionButton.set_icon_name("list-add");
    m_buttonDuplicate.set_icon_name("edit-copy");
    m_deleteButton.set_icon_name("edit-delete");
    m_inverseDeleteButton.set_icon_name("edit-delete");
#endif
    m_addFromClipboardButton.set_tooltip_text(_("Create a new macro from the content currently available on the clipboard."));
    m_addFromSelectionButton.set_tooltip_text(_("Create a new macro from the currently selected dimension region's parameters currently shown on the main window."));
    m_buttonDuplicate.set_tooltip_text(_("Duplicate the selected macro(s). The new macro(s) will be appended to the end of the list."));
    m_buttonUp.set_tooltip_text(
        _("Move the selected macro up in the list, which changes its order of "
          "appearance in the main window's \"Macro\" menu and changes to which "
          "keyboard accelerator key (F1 to F12) the macro is assigned to."));
    m_buttonDown.set_tooltip_text(
        _("Move the selected macro down in the list, which changes its order of "
          "appearance in the main window's \"Macro\" menu and changes to which "
          "keyboard accelerator key (F1 to F12) the macro is assigned to."));
    m_buttonEdit.set_tooltip_text(_("Open the Macro Editor for the selected macro."));
    m_deleteButton.set_tooltip_text(_("Delete the selected macros."));
    m_inverseDeleteButton.set_tooltip_text(_("Delete all macros except the selected ones."));
    m_addHBox.pack_start(m_addFromClipboardButton, Gtk::PACK_EXPAND_WIDGET/*, 15*/);
    m_addHBox.pack_start(m_addFromSelectionButton, Gtk::PACK_EXPAND_WIDGET/*, 15*/);
    m_vbox.pack_start(m_addHBox, Gtk::PACK_SHRINK);

    m_vbox.pack_start(m_mainHBox);
    m_vbox.set_spacing(5);

    // create Macro list treeview (including its data model)
    m_treeStoreMacros = MacroListTreeStore::create(m_treeModelMacros);
    m_treeViewMacros.set_model(m_treeStoreMacros);
    m_treeViewMacros.get_selection()->set_mode(Gtk::SELECTION_MULTIPLE);
    //m_treeViewMacro.set_tooltip_text(_(""));
    m_treeViewMacros.append_column(_("Key"), m_treeModelMacros.m_col_key);
    m_treeViewMacros.append_column_editable(_("Macro Name"), m_treeModelMacros.m_col_name);
    m_treeViewMacros.append_column(_("Created"), m_treeModelMacros.m_col_created);
    m_treeViewMacros.append_column(_("Modified"), m_treeModelMacros.m_col_modified);
    m_treeViewMacros.set_tooltip_column(m_treeModelMacros.m_col_comment.index());
    // make all rows gray text, except of "Name" column
    for (int i = 0; i <= 3; ++i) {
        if (i == m_treeModelMacros.m_col_name.index())
            continue;
        Gtk::TreeViewColumn* column = m_treeViewMacros.get_column(i);
        Gtk::CellRendererText* cellrenderer =
            dynamic_cast<Gtk::CellRendererText*>(column->get_first_cell());
        cellrenderer->property_foreground().set_value("#bababa");
    }
    /*{
        Gtk::TreeViewColumn* column = m_treeViewMacro.get_column(0);
        Gtk::CellRendererText* cellrenderer =
            dynamic_cast<Gtk::CellRendererText*>(column->get_first_cell());
        column->add_attribute(
            cellrenderer->property_foreground(), m_SamplesModel.m_color
        );
    }*/
    /*{
        Gtk::TreeViewColumn* column = m_treeViewMacro.get_column(1);
        Gtk::CellRendererText* cellrenderer =
            dynamic_cast<Gtk::CellRendererText*>(column->get_first_cell());
        column->add_attribute(
            cellrenderer->property_foreground(), m_SamplesModel.m_color
        );
    }*/
    m_treeViewMacros.set_headers_visible(true);
    m_treeViewMacros.get_selection()->signal_changed().connect(
        sigc::mem_fun(*this, &MacrosSetup::onTreeViewSelectionChanged)
    );
#if GTKMM_MAJOR_VERSION > 3 || (GTKMM_MAJOR_VERSION == 3 && (GTKMM_MINOR_VERSION > 91 || (GTKMM_MINOR_VERSION == 91 && GTKMM_MICRO_VERSION >= 2))) // GTKMM >= 3.91.2
    m_treeViewMacros.signal_key_release_event().connect(
#else
    m_treeViewMacros.signal_key_release_event().connect_notify(
#endif
        sigc::mem_fun(*this, &MacrosSetup::onMacroTreeViewKeyRelease)
    );
    m_treeStoreMacros->signal_row_changed().connect(
        sigc::mem_fun(*this, &MacrosSetup::onMacroTreeViewRowValueChanged)
    );
    m_ignoreTreeViewValueChange = false;
    m_ignoreCommentTextViewChange = false;

    m_scrolledWindow.add(m_treeViewMacros);
    m_scrolledWindow.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
    m_mainHBox.pack_start(m_scrolledWindow);

    m_rvbox.set_spacing(5);

    m_mainHBox.pack_start(m_rvbox, Gtk::PACK_SHRINK);
    m_mainHBox.set_spacing(5),
    m_rvbox.set_spacing(5);
    m_rvbox.pack_start(m_buttonDuplicate, Gtk::PACK_SHRINK);
    m_rvbox.pack_start(m_detailsButtonBox, Gtk::PACK_SHRINK);

    //m_textViewComment.set_left_margin(3);
    //m_textViewComment.set_right_margin(3);
    m_textViewComment.set_indent(2);
    m_textViewComment.set_tooltip_text(
        _("Write arbitrary comments for the selected macro which help you to "
          "remember the purpose of your macro. The comment will be shown as "
          "tooltip in the main window's \"Macro\" menu for example.")
    );
    m_scrolledWindowComment.add(m_textViewComment);
    m_scrolledWindowComment.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
    m_labelComment.set_markup(
        "<b>" + m_labelComment.get_text() + "</b>"
    );
    m_rvbox.pack_start(m_labelComment, Gtk::PACK_SHRINK);
    m_rvbox.pack_start(m_scrolledWindowComment);

    m_detailsButtonBox.pack_start(m_buttonUp);
    m_detailsButtonBox.pack_start(m_buttonDown);
    m_detailsButtonBox.pack_start(m_buttonEdit);

    m_buttonBoxL.set_layout(Gtk::BUTTONBOX_START);
    m_buttonBoxL.pack_start(m_deleteButton);
    m_buttonBoxL.pack_start(m_inverseDeleteButton);
    m_deleteButton.set_sensitive(false);
    m_inverseDeleteButton.set_sensitive(false);
    m_buttonDuplicate.set_sensitive(false);
    m_buttonUp.set_sensitive(false);
    m_buttonDown.set_sensitive(false);

    m_buttonBox.set_layout(Gtk::BUTTONBOX_END);
    m_buttonBox.pack_start(m_applyButton);
    m_buttonBox.pack_start(m_cancelButton);
    m_applyButton.set_can_default();
    m_applyButton.set_sensitive(false);
    m_applyButton.grab_focus();

#if GTKMM_MAJOR_VERSION > 3 || (GTKMM_MAJOR_VERSION == 3 && GTKMM_MINOR_VERSION >= 12)
    m_statusLabel.set_margin_start(6);
    m_statusLabel.set_margin_end(6);
#elif GTKMM_MAJOR_VERSION >= 3
    m_statusLabel.set_margin_left(6);
    m_statusLabel.set_margin_right(6);
#else
    m_statusHBox.set_spacing(6);
#endif

    m_statusHBox.pack_start(m_statusLabel);
#if HAS_GTKMM_SHOW_ALL_CHILDREN
    m_statusHBox.show_all_children();
#endif

    m_footerHBox.pack_start(m_buttonBoxL, Gtk::PACK_SHRINK);
    m_footerHBox.pack_start(m_statusHBox);
    m_footerHBox.pack_start(m_buttonBox, Gtk::PACK_SHRINK);

    m_vbox.pack_start(m_footerHBox, Gtk::PACK_SHRINK);

    m_addFromClipboardButton.signal_clicked().connect(
        sigc::mem_fun(*this, &MacrosSetup::onButtonAddFromClipboard)
    );

    m_addFromSelectionButton.signal_clicked().connect(
        sigc::mem_fun(*this, &MacrosSetup::onButtonAddFromSelection)
    );

    m_buttonUp.signal_clicked().connect(
        sigc::mem_fun(*this, &MacrosSetup::onButtonUp)
    );

    m_buttonDown.signal_clicked().connect(
        sigc::mem_fun(*this, &MacrosSetup::onButtonDown)
    );

    m_buttonEdit.signal_clicked().connect(
        sigc::mem_fun(*this, &MacrosSetup::onButtonEdit)
    );

    m_buttonDuplicate.signal_clicked().connect(
        sigc::mem_fun(*this, &MacrosSetup::onButtonDuplicate)
    );

    m_textViewComment.get_buffer()->signal_changed().connect(
        sigc::mem_fun(*this, &MacrosSetup::onCommentTextViewChanged)
    );

    m_applyButton.signal_clicked().connect(
        sigc::mem_fun(*this, &MacrosSetup::onButtonApply)
    );

    m_cancelButton.signal_clicked().connect(
        sigc::mem_fun(*this, &MacrosSetup::onButtonCancel)
    );

    m_deleteButton.signal_clicked().connect(
        sigc::mem_fun(*this, &MacrosSetup::deleteSelectedRows)
    );

    m_inverseDeleteButton.signal_clicked().connect(
        sigc::mem_fun(*this, &MacrosSetup::inverseDeleteSelectedRows)
    );

    signal_hide().connect(
        sigc::mem_fun(*this, &MacrosSetup::onWindowHide)
    );

    signal_delete_event().connect(
#if GTKMM_MAJOR_VERSION > 3 || (GTKMM_MAJOR_VERSION == 3 && (GTKMM_MINOR_VERSION > 91 || (GTKMM_MINOR_VERSION == 91 && GTKMM_MICRO_VERSION >= 2))) // GTKMM >= 3.91.2
        sigc::mem_fun(*this, &MacrosSetup::onWindowDelete)
#else
        sigc::mem_fun(*this, &MacrosSetup::onWindowDeleteP)
#endif
    );

    signal_key_press_event().connect(
        sigc::mem_fun(*this, &MacrosSetup::onKeyPressed)
    );
    signal_key_release_event().connect(
        sigc::mem_fun(*this, &MacrosSetup::onKeyReleased)
    );

#if HAS_GTKMM_SHOW_ALL_CHILDREN
    show_all_children();
#endif
    updateStatus();
}

MacrosSetup::~MacrosSetup() {
    printf("MacrosSetup destruct\n");
}

void MacrosSetup::setMacros(const std::vector<Serialization::Archive>& macros,
                            Serialization::Archive* pClipboardContent,
                            gig::DimensionRegion* pSelectedDimRgn)
{
    // copy for non-destructive editing
    m_macros = macros;

    m_clipboardContent = pClipboardContent;
    m_selectedDimRgn = pSelectedDimRgn;

    reloadTreeView();
}

void MacrosSetup::onButtonAddFromClipboard() {
    printf("+fromClipboard\n");
    if (!m_clipboardContent) return;
    if (!m_clipboardContent->rootObject()) return;
    m_macros.push_back(*m_clipboardContent);
    m_modified = true;
    reloadTreeView();
}

void MacrosSetup::onButtonAddFromSelection() {
    printf("+fromSelection\n");
    if (!m_selectedDimRgn) return;
    std::string errorText;
    try {
        Serialization::Archive archive;
        archive.serialize(m_selectedDimRgn);
        //archive.setName("Unnamed Macro");
        m_macros.push_back(archive);
        m_modified = true;
        reloadTreeView();
    } catch (Serialization::Exception e) {
        errorText = e.Message;
    } catch (...) {
        errorText = _("Unknown exception while creating macro");
    }
    if (!errorText.empty()) {
        Glib::ustring txt = _("Couldn't create macro:\n") + errorText;
        Gtk::MessageDialog msg(*this, txt, false, Gtk::MESSAGE_ERROR);
        msg.run();
    }
}

void MacrosSetup::moveByDir(int d) {
    if (d < -1 || d > 1) return;
    int index = getSelectedMacroIndex();
    if (index < 0) return;
    if (d == -1 && index == 0) return;
    if (d == +1 && index >= m_macros.size() - 1) return;

    // swap macros
    std::swap(m_macros[index + d], m_macros[index]);

    // swap tree view rows
    Gtk::TreePath p1(ToString(index + d));
    Gtk::TreePath p2(ToString(index));
    Gtk::TreeModel::iterator it1 = m_treeStoreMacros->get_iter(p1);
    Gtk::TreeModel::iterator it2 = m_treeStoreMacros->get_iter(p2);
    m_treeStoreMacros->iter_swap(it1, it2);
    int idx1 = (*it1)[m_treeModelMacros.m_col_index];
    int idx2 = (*it2)[m_treeModelMacros.m_col_index];
    (*it1)[m_treeModelMacros.m_col_index] = idx2;
    (*it2)[m_treeModelMacros.m_col_index] = idx1;
    Glib::ustring s1 = (*it1)[m_treeModelMacros.m_col_key];
    Glib::ustring s2 = (*it2)[m_treeModelMacros.m_col_key];
    (*it1)[m_treeModelMacros.m_col_key] = s2;
    (*it2)[m_treeModelMacros.m_col_key] = s1;

    m_modified = true;
}

void MacrosSetup::onButtonUp() {
    moveByDir(-1);
}

void MacrosSetup::onButtonDown() {
    moveByDir(+1);
}

void MacrosSetup::onButtonDuplicate() {
    Glib::RefPtr<Gtk::TreeSelection> sel = m_treeViewMacros.get_selection();
    std::vector<Gtk::TreeModel::Path> rows = sel->get_selected_rows();
    duplicateRows(rows);
}

void MacrosSetup::onButtonEdit() {
    Serialization::Archive* macro = getSelectedMacro();
    if (!macro) return;

    m_modifiedBeforeMacroEditor = isModified();

    MacroEditor* editor = new MacroEditor();
    editor->setMacro(macro, false);
    editor->signal_changes_applied().connect(
        sigc::mem_fun(*this, &MacrosSetup::onMacroEditorAppliedChanges)
    );
    editor->show();
}

void MacrosSetup::onMacroEditorAppliedChanges() {
    // so that the user does not need to click on a Apply buttons twice
    if (!m_modifiedBeforeMacroEditor)
        onButtonApply();
    updateStatus();
}

void MacrosSetup::onCommentTextViewChanged() {
    if (m_ignoreCommentTextViewChange) return;
    //printf("textChanged\n");
    Serialization::Archive* macro = getSelectedMacro();
    if (!macro) return;
    macro->setComment(
        m_textViewComment.get_buffer()->get_text()
    );
    updateStatus();
}

int MacrosSetup::getSelectedMacroIndex() const {
    std::vector<Gtk::TreeModel::Path> v = m_treeViewMacros.get_selection()->get_selected_rows();
    if (v.empty()) return -1;
    Gtk::TreeModel::iterator it = m_treeStoreMacros->get_iter(v[0]);
    if (!it) return -1;
    const Gtk::TreeModel::Row& row = *it;
    int index = row[m_treeModelMacros.m_col_index];
    if (index < 0 || index >= m_macros.size()) return -1;
    return index;
}

Serialization::Archive* MacrosSetup::getSelectedMacro() {
    int index = getSelectedMacroIndex();
    if (index < 0) return NULL;
    return &m_macros[index];
}

static Glib::ustring indexToAccKey(uint index) {
    if (index >= 19) return "";
    return "F" + ToString(index+1);
}

static int daysAgo(const tm& t) {
    time_t now;
    time(&now);
    tm* pNow = localtime(&now);
    if (!pNow) return 0;
    if (pNow->tm_year == t.tm_year &&
        pNow->tm_mon  == t.tm_mon &&
        pNow->tm_mday == t.tm_mday) return 0;
    time_t past = mktime((tm*)&t);
    return ceil(difftime(now, past) / 60.0 / 60.0 / 24.0);
}

static Glib::ustring humanShortStr(const tm& t) {
    int iDaysAgo = daysAgo(t);
    char buf[70];
    if (iDaysAgo == 0) {
        // C-Time specification for a time somewhere today (see 'man strftime()').
        if (strftime(buf, sizeof buf, _("%R"), &t))
            return buf;
    } else if (iDaysAgo == 1) {
        // C-Time specification for a time somewhere yesterday (see 'man strftime()').
        if (strftime(buf, sizeof buf, _("Yesterday %R"), &t))
            return buf;
    } else if (iDaysAgo == 2) {
        // C-Time specification for a time somewhere 2 days ago (see 'man strftime()').
        if (strftime(buf, sizeof buf, _("2 days ago %R"), &t))
            return buf;
    } else {
        // C-Time specification for a time far more than 2 days ago (see 'man strftime()').
        if (strftime(buf, sizeof buf, "%d %b %Y", &t))
            return buf;
    }
    return "";
}

void MacrosSetup::reloadTreeView() {
    m_ignoreTreeViewValueChange = true;

    m_treeStoreMacros->clear();

    for (int iMacro = 0; iMacro < m_macros.size(); ++iMacro) {
        const Serialization::Archive& macro = m_macros[iMacro];

        Gtk::TreeModel::iterator iter = m_treeStoreMacros->append();
        Gtk::TreeModel::Row row = *iter;
        row[m_treeModelMacros.m_col_key] = indexToAccKey(iMacro);
        row[m_treeModelMacros.m_col_name] = macro.name().empty() ? _("Unnamed Macro") : gig_to_utf8(macro.name());
        row[m_treeModelMacros.m_col_comment] = macro.comment().empty() ? _("No comment assigned to this macro yet.") : gig_to_utf8(macro.comment());
        row[m_treeModelMacros.m_col_created] = humanShortStr(macro.dateTimeCreated());
        row[m_treeModelMacros.m_col_modified] = humanShortStr(macro.dateTimeModified());
        row[m_treeModelMacros.m_col_index] = iMacro;
    }

    m_treeViewMacros.expand_all();

    updateStatus();

    m_ignoreTreeViewValueChange = false;
}

void MacrosSetup::onTreeViewSelectionChanged() {
    std::vector<Gtk::TreeModel::Path> v = m_treeViewMacros.get_selection()->get_selected_rows();
    const bool bValidSelection = !v.empty();
    m_deleteButton.set_sensitive(bValidSelection);
    m_inverseDeleteButton.set_sensitive(bValidSelection);
    m_buttonEdit.set_sensitive(bValidSelection);
    m_buttonDuplicate.set_sensitive(bValidSelection);
    m_buttonUp.set_sensitive(bValidSelection);
    m_buttonDown.set_sensitive(bValidSelection);

    // update comment text view
    std::string sComment;
    Serialization::Archive* macro = getSelectedMacro();
    if (macro)
        sComment = macro->comment();
    m_ignoreCommentTextViewChange = true;
    m_textViewComment.get_buffer()->set_text(sComment);
    m_ignoreCommentTextViewChange = false;
    m_textViewComment.set_sensitive(bValidSelection);
}

// Cmd key on Mac, Ctrl key on all other OSs
static const guint primaryKeyL =
    #if defined(__APPLE__)
    GDK_KEY_Meta_L;
    #else
    GDK_KEY_Control_L;
    #endif

static const guint primaryKeyR =
    #if defined(__APPLE__)
    GDK_KEY_Meta_R;
    #else
    GDK_KEY_Control_R;
    #endif

#if GTKMM_MAJOR_VERSION > 3 || (GTKMM_MAJOR_VERSION == 3 && (GTKMM_MINOR_VERSION > 91 || (GTKMM_MINOR_VERSION == 91 && GTKMM_MICRO_VERSION >= 2))) // GTKMM >= 3.91.2
bool MacrosSetup::onKeyPressed(Gdk::EventKey& _key) {
    GdkEventKey* key = _key.gobj();
#else
bool MacrosSetup::onKeyPressed(GdkEventKey* key) {
#endif
    //printf("key down 0x%x\n", key->keyval);
    if (key->keyval == GDK_KEY_Alt_L || key->keyval == GDK_KEY_Alt_R)
        m_altKeyDown = true;
    if (key->keyval == primaryKeyL || key->keyval == primaryKeyR)
        m_primaryKeyDown = true;
    return false;
}

#if GTKMM_MAJOR_VERSION > 3 || (GTKMM_MAJOR_VERSION == 3 && (GTKMM_MINOR_VERSION > 91 || (GTKMM_MINOR_VERSION == 91 && GTKMM_MICRO_VERSION >= 2))) // GTKMM >= 3.91.2
bool MacrosSetup::onKeyReleased(Gdk::EventKey& _key) {
    GdkEventKey* key = _key.gobj();
#else
bool MacrosSetup::onKeyReleased(GdkEventKey* key) {
#endif
    //printf("key up 0x%x\n", key->keyval);
    if (key->keyval == GDK_KEY_Alt_L || key->keyval == GDK_KEY_Alt_R)
        m_altKeyDown = false;
    if (key->keyval == primaryKeyL || key->keyval == primaryKeyR)
        m_primaryKeyDown = false;
    if (m_primaryKeyDown && key->keyval == GDK_KEY_b)
        onButtonAddFromClipboard();
    if (m_primaryKeyDown && key->keyval == GDK_KEY_s)
        onButtonAddFromSelection();
    return false;
}

#if GTKMM_MAJOR_VERSION > 3 || (GTKMM_MAJOR_VERSION == 3 && (GTKMM_MINOR_VERSION > 91 || (GTKMM_MINOR_VERSION == 91 && GTKMM_MICRO_VERSION >= 2))) // GTKMM >= 3.91.2
bool MacrosSetup::onMacroTreeViewKeyRelease(Gdk::EventKey& _key) {
    GdkEventKey* key = _key.gobj();
#else
void MacrosSetup::onMacroTreeViewKeyRelease(GdkEventKey* key) {
#endif
    if (key->keyval == GDK_KEY_BackSpace || key->keyval == GDK_KEY_Delete) {
        if (m_altKeyDown)
            inverseDeleteSelectedRows();
        else if (m_primaryKeyDown)
            deleteSelectedRows();
    }
#if GTKMM_MAJOR_VERSION > 3 || (GTKMM_MAJOR_VERSION == 3 && (GTKMM_MINOR_VERSION > 91 || (GTKMM_MINOR_VERSION == 91 && GTKMM_MICRO_VERSION >= 2))) // GTKMM >= 3.91.2
    return false;
#endif
}

void MacrosSetup::onMacroTreeViewRowValueChanged(const Gtk::TreeModel::Path& path,
                                                 const Gtk::TreeModel::iterator& iter)
{
    if (m_ignoreTreeViewValueChange) return;
    if (!iter) return;
    Gtk::TreeModel::Row row = *iter;
    Glib::ustring name = row[m_treeModelMacros.m_col_name];
    int index = row[m_treeModelMacros.m_col_index];
    m_macros[index].setName(name);
    //reloadTreeView();
    m_modified = true;
    updateStatus();
}

void MacrosSetup::deleteSelectedRows() {
    Glib::RefPtr<Gtk::TreeSelection> sel = m_treeViewMacros.get_selection();
    std::vector<Gtk::TreeModel::Path> rows = sel->get_selected_rows();
    deleteRows(rows);
}

void MacrosSetup::duplicateRows(const std::vector<Gtk::TreeModel::Path>& rows) {
    if (!rows.empty()) m_modified = true;
    bool bError = false;
    for (int r = 0; r < rows.size(); ++r) {
        Gtk::TreeModel::iterator it = m_treeStoreMacros->get_iter(rows[r]);
        if (!it) continue;
        Gtk::TreeModel::Row row = *it;
        int index = row[m_treeModelMacros.m_col_index];
        if (index < 0 || index >= m_macros.size()) continue;

        Serialization::Archive clone = m_macros[index];
        if (!endsWith(clone.name(), "COPY", true)) {
            clone.setName(
                (clone.name().empty()) ? "Unnamed COPY" : (clone.name() + " COPY")
            );
        }
        try {
            // enforce re-encoding the abstract object model and resetting the
            // 'modified' state
            clone.rawData();
        } catch (Serialization::Exception e) {
            bError = true;
            e.PrintMessage();
            continue;
        } catch (...) {
            bError = true;
            std::cerr << "Unknown exception while cloning macro." << std::endl;
            continue;
        }
        // finally add new cloned macro
        m_macros.push_back(clone);
    }
    reloadTreeView();
    if (bError) {
        Glib::ustring txt = _("At least one of the macros could not be cloned due to an error (check console output).");
        Gtk::MessageDialog msg(*this, txt, false, Gtk::MESSAGE_ERROR);
        msg.run();
    }
}

void MacrosSetup::deleteRows(const std::vector<Gtk::TreeModel::Path>& rows) {
    if (!rows.empty()) m_modified = true;
    std::set<int> macros;
    for (int r = rows.size() - 1; r >= 0; --r) {
        Gtk::TreeModel::iterator it = m_treeStoreMacros->get_iter(rows[r]);
        if (!it) continue;
        Gtk::TreeModel::Row row = *it;
        macros.insert(
            row[m_treeModelMacros.m_col_index]
        );
    }
    for (std::set<int>::const_reverse_iterator it = macros.rbegin();
         it != macros.rend(); ++it)
    {
        m_macros.erase(m_macros.begin() + *it);
    }
    reloadTreeView();
}

static bool _onEachTreeRow(const Gtk::TreeModel::Path& input, std::vector<Gtk::TreeModel::Path>* output) {
    output->push_back(input);
    return false; // continue walking the tree
}

void MacrosSetup::inverseDeleteSelectedRows() {
    // get all rows of tree view
    std::vector<Gtk::TreeModel::Path> rows;
    m_treeViewMacros.get_model()->foreach_path(
        sigc::bind(
            sigc::ptr_fun(&_onEachTreeRow),
            &rows
        )
    );

    // erase all entries from "rows" which are currently selected
    std::vector<Gtk::TreeModel::Path> vSelected = m_treeViewMacros.get_selection()->get_selected_rows();
    for (int i = rows.size() - 1; i >= 0; --i) {
        bool bIsSelected = std::find(vSelected.begin(), vSelected.end(),
                                     rows[i]) != vSelected.end();
        if (bIsSelected)
            rows.erase(rows.begin() + i);
    }

    // delete those 'inverse' selected rows
    deleteRows(rows);
}

void MacrosSetup::updateStatus() {
    bool bValidSelection = !m_treeViewMacros.get_selection()->get_selected_rows().empty();
    m_addFromClipboardButton.set_sensitive(
        m_clipboardContent && m_clipboardContent->rootObject()
    );
    m_addFromSelectionButton.set_sensitive(m_selectedDimRgn);
    m_buttonEdit.set_sensitive(bValidSelection);
    m_buttonDuplicate.set_sensitive(bValidSelection);
    m_buttonUp.set_sensitive(bValidSelection);
    m_buttonDown.set_sensitive(bValidSelection);
    m_applyButton.set_sensitive(isModified());
    m_textViewComment.set_sensitive(bValidSelection);
    updateStatusBar();
}

void MacrosSetup::updateStatusBar() {
    // update status text
    std::string txt;
    m_statusLabel.set_markup(txt);
}

sigc::signal<void, const std::vector<Serialization::Archive>& >& MacrosSetup::signal_macros_changed()
{
    return m_macros_changed;
}
    
#if GTKMM_MAJOR_VERSION > 3 || (GTKMM_MAJOR_VERSION == 3 && (GTKMM_MINOR_VERSION > 91 || (GTKMM_MINOR_VERSION == 91 && GTKMM_MICRO_VERSION >= 2))) // GTKMM >= 3.91.2
bool MacrosSetup::onWindowDelete(Gdk::Event& e) {
    return onWindowDeleteP(NULL);
}
#endif

bool MacrosSetup::onWindowDeleteP(GdkEventAny* /*e*/) {
    //printf("onWindowDelete\n");

    if (!isModified()) return false; // propagate event further (which will close this window)

    //gchar* msg = g_strdup_printf(_("Apply changes to macro \"%s\" before closing?"),
    //                             m_macroOriginal->Name.c_str());
    gchar* msg = g_strdup(_("Apply changes to macro list before closing?"));
    Gtk::MessageDialog dialog(*this, msg, false, Gtk::MESSAGE_WARNING, Gtk::BUTTONS_NONE);
    g_free(msg);
    dialog.set_secondary_text(_("If you close without applying, your changes will be lost."));
    dialog.add_button(_("Close _Without Applying"), Gtk::RESPONSE_NO);
    dialog.add_button(_("_Cancel"), Gtk::RESPONSE_CANCEL);
    dialog.add_button(_("_Apply"), Gtk::RESPONSE_YES);
    dialog.set_default_response(Gtk::RESPONSE_YES);
    int response = dialog.run();
    dialog.hide();

    // user decided to close this window without saving
    if (response == Gtk::RESPONSE_NO)
        return false; // propagate event further (which will close this window)

    // user cancelled dialog, thus don't close this window
    if (response == Gtk::RESPONSE_CANCEL) {
        show();
        return true; // drop event (prevents closing this window)
    }

    // user wants to apply the changes, afterwards close window
    if (response == Gtk::RESPONSE_YES) {
        onButtonApply();
        return false; // propagate event further (which will close this window)
    }

    // should never ever make it to this point actually
    return false;
}

bool MacrosSetup::isModified() const {
    if (m_modified) return true;
    bool bModified = false;
    for (int i = 0; i < m_macros.size(); ++i) {
        if (m_macros[i].isModified()) {
            bModified = true;
            break;
        }
    }
    return bModified;
}

void MacrosSetup::onButtonCancel() {
    bool dropEvent = onWindowDeleteP(NULL);
    if (dropEvent) return;
    hide();
}

void MacrosSetup::onButtonApply() {
    std::string errorText;
    try {
        for (int i = 0; i < m_macros.size(); ++i) {
            if (!m_macros[i].isModified()) continue;
            // enforce re-encoding the abstract object model and resetting the
            // 'modified' state
            m_macros[i].rawData();
        }
        m_modified = false;
    } catch (Serialization::Exception e) {
        errorText = e.Message;
    } catch (...) {
        errorText = _("Unknown exception while applying macro changes");
    }
    if (!errorText.empty()) {
        Glib::ustring txt = _("Couldn't apply macro changes:\n") + errorText;
        Gtk::MessageDialog msg(*this, txt, false, Gtk::MESSAGE_ERROR);
        msg.run();
    } else {
        // update MainWindow with edited list of macros
        m_macros_changed.emit(m_macros);
    }
    updateStatus();
}

void MacrosSetup::onWindowHide() {
    delete this; // this is the end, my friend
}
