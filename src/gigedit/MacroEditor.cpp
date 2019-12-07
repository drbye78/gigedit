/*
    Copyright (c) MMXVII - MMXIX Christian Schoenebeck

    This file is part of "gigedit" and released under the terms of the
    GNU General Public License version 2.
*/

#include "MacroEditor.h"
#include "global.h"
#include <assert.h>

MacroEditor::MacroEditor() :
    m_macroOriginal(NULL),
    m_statusLabel("",  Gtk::ALIGN_START),
    m_deleteButton(Glib::ustring(_("Delete")) + " " + UNICODE_PRIMARY_KEY_SYMBOL + UNICODE_ERASE_KEY_SYMBOL),
    m_inverseDeleteButton(Glib::ustring(_("Inverse Delete")) + " " + UNICODE_ALT_KEY_SYMBOL + UNICODE_ERASE_KEY_SYMBOL),
    m_applyButton(_("_Apply"), true),
    m_cancelButton(_("_Cancel"), true),
    m_altKeyDown(false),
    m_primaryKeyDown(false)
{
    add(m_vbox);

    if (!Settings::singleton()->autoRestoreWindowDimension) {
        set_default_size(800, 600);
        set_position(Gtk::WIN_POS_MOUSE);
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
          "triggered by the user. Only the parameters listed here will be "
          "applied to the instrument editor when this macro is triggered, all "
          "other ones remain untouched. So simply delete parameters here which "
          "you don't want to be modified by this macro. Double click on a "
          "value to change it.")
    );
    m_vbox.pack_start(m_labelIntro, Gtk::PACK_SHRINK);

    // create Macro treeview (including its data model)
    m_treeStoreMacro = MacroTreeStore::create(m_treeModelMacro);
    m_treeViewMacro.set_model(m_treeStoreMacro);
    m_treeViewMacro.get_selection()->set_mode(Gtk::SELECTION_MULTIPLE);
    //m_treeViewMacro.set_tooltip_text(_(""));
    m_treeViewMacro.append_column(_("Key"), m_treeModelMacro.m_col_name);
    m_treeViewMacro.append_column(_("Type"), m_treeModelMacro.m_col_type);
    //m_treeViewMacro.append_column_editable(_("Value"), m_treeModelMacro.m_col_value);
    //m_treeViewMacro.append_column(_("Value"), m_valueCellRenderer);
    Gtk::TreeViewColumn* valueColumn = new Gtk::TreeViewColumn(_("Value"));
    valueColumn->pack_start(m_valueCellRenderer);
    m_treeViewMacro.append_column(*valueColumn);
    // m_valueCellRenderer.property_model() = m_comboBoxModel;
    // m_valueCellRenderer.property_text_column() = 0;
    //m_valueCellRenderer.property_editable() = true;
    {
        Gtk::TreeView::Column* column = valueColumn;// m_treeViewMacro.get_column(2);
        //column->set_renderer(m_valueCellRenderer, m_treeModelMacro.m_col_value);
        column->add_attribute(m_valueCellRenderer.property_text(),
                              m_treeModelMacro.m_col_value);
        column->add_attribute(m_valueCellRenderer.property_has_entry(),
                              m_treeModelMacro.m_col_allowTextEntry);
        column->add_attribute(m_valueCellRenderer.property_editable(),
                              m_treeModelMacro.m_col_editable);
        column->add_attribute(m_valueCellRenderer.property_model(),
                              m_treeModelMacro.m_col_options);
    }
    m_valueCellRenderer.property_text_column() = 0;
    m_valueCellRenderer.signal_edited().connect(
        sigc::mem_fun(*this, &MacroEditor::onValueCellEdited)
    );

    {
        Gtk::TreeViewColumn* column = m_treeViewMacro.get_column(1);
        Gtk::CellRendererText* cellrenderer =
            dynamic_cast<Gtk::CellRendererText*>(column->get_first_cell());
        cellrenderer->property_foreground().set_value("#bababa");
    }
    m_treeViewMacro.set_headers_visible(true);
    m_treeViewMacro.get_selection()->signal_changed().connect(
        sigc::mem_fun(*this, &MacroEditor::onTreeViewSelectionChanged)
    );
#if GTKMM_MAJOR_VERSION > 3 || (GTKMM_MAJOR_VERSION == 3 && (GTKMM_MINOR_VERSION > 91 || (GTKMM_MINOR_VERSION == 91 && GTKMM_MICRO_VERSION >= 2))) // GTKMM >= 3.91.2
    m_treeViewMacro.signal_key_release_event().connect(
#else
    m_treeViewMacro.signal_key_release_event().connect_notify(
#endif
        sigc::mem_fun(*this, &MacroEditor::onMacroTreeViewKeyRelease)
    );
    m_treeStoreMacro->signal_row_changed().connect(
        sigc::mem_fun(*this, &MacroEditor::onMacroTreeViewRowValueChanged)
    );
    m_ignoreTreeViewValueChange = false;

    m_scrolledWindow.add(m_treeViewMacro);
    m_scrolledWindow.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
    m_vbox.pack_start(m_scrolledWindow);

    m_buttonBoxL.set_layout(Gtk::BUTTONBOX_START);
    m_buttonBoxL.pack_start(m_deleteButton);
    m_buttonBoxL.pack_start(m_inverseDeleteButton);
    m_deleteButton.set_sensitive(false);
    m_inverseDeleteButton.set_sensitive(false);

    m_buttonBox.set_layout(Gtk::BUTTONBOX_END);
    m_buttonBox.pack_start(m_applyButton);
    m_buttonBox.pack_start(m_cancelButton);
    m_applyButton.set_can_default();
    m_applyButton.set_sensitive(false);
    m_applyButton.grab_focus();

#if GTKMM_MAJOR_VERSION > 3 || (GTKMM_MAJOR_VERSION == 3 && GTKMM_MINOR_VERSION >= 12)
    m_labelIntro.set_margin_start(6);
    m_labelIntro.set_margin_end(6);
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

    m_applyButton.signal_clicked().connect(
        sigc::mem_fun(*this, &MacroEditor::onButtonApply)
    );

    m_cancelButton.signal_clicked().connect(
        sigc::mem_fun(*this, &MacroEditor::onButtonCancel)
    );

    m_deleteButton.signal_clicked().connect(
        sigc::mem_fun(*this, &MacroEditor::deleteSelectedRows)
    );

    m_inverseDeleteButton.signal_clicked().connect(
        sigc::mem_fun(*this, &MacroEditor::inverseDeleteSelectedRows)
    );

    signal_hide().connect(
        sigc::mem_fun(*this, &MacroEditor::onWindowHide)
    );

    signal_delete_event().connect(
#if GTKMM_MAJOR_VERSION > 3 || (GTKMM_MAJOR_VERSION == 3 && (GTKMM_MINOR_VERSION > 91 || (GTKMM_MINOR_VERSION == 91 && GTKMM_MICRO_VERSION >= 2))) // GTKMM >= 3.91.2
        sigc::mem_fun(*this, &MacroEditor::onWindowDelete)
#else
        sigc::mem_fun(*this, &MacroEditor::onWindowDeleteP)
#endif
    );

    signal_key_press_event().connect(
        sigc::mem_fun(*this, &MacroEditor::onKeyPressed)
    );
    signal_key_release_event().connect(
        sigc::mem_fun(*this, &MacroEditor::onKeyReleased)
    );

    m_deleteButton.set_tooltip_text(_("Delete the selected parameters from this macro."));
    m_inverseDeleteButton.set_tooltip_text(_("Delete all parameters from this macro except the selected ones."));

#if HAS_GTKMM_SHOW_ALL_CHILDREN
    show_all_children();
#endif
    updateStatus();
}

MacroEditor::~MacroEditor() {
    printf("MacroEditor destruct\n");
}

void MacroEditor::setMacro(Serialization::Archive* macro, bool isClipboard) {
    m_macroOriginal = macro;
    if (!macro) {
        set_title(_("No Macro"));
        return;
    }

    if (isClipboard)
        set_title(std::string(_("Macro Editor:")) + " " + _("Clipboard Content"));
    else {
        if (macro->name().empty())
            set_title(std::string(_("Macro Editor:")) + " " + _("Unnamed Macro"));
        else
            set_title(std::string(_("Macro Editor:")) + " \"" + macro->name() + "\"");
    }

    // copy for non-destructive editing
    m_macro = *macro;

    reloadTreeView();
}

sigc::signal<void>& MacroEditor::signal_changes_applied() {
    return m_changes_applied;
}

Glib::RefPtr<Gtk::ListStore> MacroEditor::createComboOptions(const char** options) {
    Glib::RefPtr<Gtk::ListStore> refOptions = Gtk::ListStore::create(m_comboOptionsModel);
    for (size_t i = 0; options[i]; ++i)
        (*refOptions->append())[m_comboOptionsModel.m_col_choice] = options[i];
    return refOptions;
}

inline static Serialization::String _boolToStr(bool b) {
    // 'NO' intentional all uper case in contrast to 'Yes', simply because I
    // find them easier distinguishable that way on quick readings
    return b ? "Yes" : "NO";
}

static const char* _boolOptions[] = { "Yes", "NO", NULL };

void MacroEditor::buildTreeView(const Gtk::TreeModel::Row& parentRow, const Serialization::Object& parentObject) {
    for (int iMember = 0; iMember < parentObject.members().size(); ++iMember) {
        const Serialization::Member& member = parentObject.members()[iMember];
        const Serialization::Object& object = m_macro.objectByUID(member.uid());
        
#if GTKMM_MAJOR_VERSION > 3 || (GTKMM_MAJOR_VERSION == 3 && GTKMM_MINOR_VERSION > 24)
        //HACK: on GTKMM 3.9x append() below requires TreeNodeChildren, parentRow.children() returns TreeNodeConstChildren though, probably going to be fixed before final GTKMM4 release though.
        const Gtk::TreeNodeConstChildren& children = parentRow.children();
        Gtk::TreeNodeChildren* const pChildren = (Gtk::TreeNodeChildren* const) &children;
        Gtk::TreeModel::iterator iterRow = m_treeStoreMacro->append(*pChildren);
#else
        Gtk::TreeModel::iterator iterRow = m_treeStoreMacro->append(parentRow.children());
#endif
        Gtk::TreeModel::Row row = *iterRow;
        row[m_treeModelMacro.m_col_name] = gig_to_utf8(member.name());
        row[m_treeModelMacro.m_col_type] = gig_to_utf8(member.type().asLongDescr());
        row[m_treeModelMacro.m_col_uid]  = object.uid();
        row[m_treeModelMacro.m_col_allowTextEntry] = true;

        if (object.type().isClass()) {
            row[m_treeModelMacro.m_col_value] = "(class)";
            row[m_treeModelMacro.m_col_editable] = false;
            buildTreeView(row, object);
        } else if (object.type().isEnum()) {
            const char* key = gig::enumKey(
                object.type().customTypeName(), m_macro.valueAsInt(object)
            );
            row[m_treeModelMacro.m_col_value] = key ? key : m_macro.valueAsString(object);
            row[m_treeModelMacro.m_col_editable] = true;
            const char** allKeys = gig::enumKeys(object.type().customTypeName());
            if (allKeys) {
                Glib::RefPtr<Gtk::ListStore> refOptions = createComboOptions(allKeys);
                row[m_treeModelMacro.m_col_options] = refOptions;
            }
        } else if (object.type().isBool()) {
            row[m_treeModelMacro.m_col_value] =  _boolToStr( m_macro.valueAsBool(object) );
            row[m_treeModelMacro.m_col_editable] = true;
            Glib::RefPtr<Gtk::ListStore> refOptions = createComboOptions(_boolOptions);
            row[m_treeModelMacro.m_col_options] = refOptions;
            row[m_treeModelMacro.m_col_allowTextEntry] = false;
        } else {
            row[m_treeModelMacro.m_col_value] = m_macro.valueAsString(object);
            row[m_treeModelMacro.m_col_editable] = true;
        }
    }
}

void MacroEditor::reloadTreeView() {
    m_ignoreTreeViewValueChange = true;

    m_treeStoreMacro->clear();

    const Serialization::Object& rootObject = m_macro.rootObject();

    Gtk::TreeModel::iterator iterRoot = m_treeStoreMacro->append();
    Gtk::TreeModel::Row rowRoot = *iterRoot;
    rowRoot[m_treeModelMacro.m_col_name]  = "(Root)";
    rowRoot[m_treeModelMacro.m_col_type]  = gig_to_utf8(rootObject.type().asLongDescr());
    rowRoot[m_treeModelMacro.m_col_value] = "";
    rowRoot[m_treeModelMacro.m_col_uid]   = rootObject.uid();
    rowRoot[m_treeModelMacro.m_col_allowTextEntry] = false;
    rowRoot[m_treeModelMacro.m_col_editable] = false;

    buildTreeView(rowRoot, rootObject);

    m_treeViewMacro.expand_all();

    updateStatus();

    m_ignoreTreeViewValueChange = false;
}

void MacroEditor::onTreeViewSelectionChanged() {
    std::vector<Gtk::TreeModel::Path> v = m_treeViewMacro.get_selection()->get_selected_rows();
    const bool bValidSelection = !v.empty();
    m_deleteButton.set_sensitive(bValidSelection);
    m_inverseDeleteButton.set_sensitive(bValidSelection);
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
bool MacroEditor::onKeyPressed(Gdk::EventKey& _key) {
    GdkEventKey* key = _key.gobj();
#else
bool MacroEditor::onKeyPressed(GdkEventKey* key) {
#endif
    //printf("key down 0x%x\n", key->keyval);
    if (key->keyval == GDK_KEY_Alt_L || key->keyval == GDK_KEY_Alt_R)
        m_altKeyDown = true;
    if (key->keyval == primaryKeyL || key->keyval == primaryKeyR)
        m_primaryKeyDown = true;
    return false;
}

#if GTKMM_MAJOR_VERSION > 3 || (GTKMM_MAJOR_VERSION == 3 && (GTKMM_MINOR_VERSION > 91 || (GTKMM_MINOR_VERSION == 91 && GTKMM_MICRO_VERSION >= 2))) // GTKMM >= 3.91.2
bool MacroEditor::onKeyReleased(Gdk::EventKey& _key) {
    GdkEventKey* key = _key.gobj();
#else
bool MacroEditor::onKeyReleased(GdkEventKey* key) {
#endif
    //printf("key up 0x%x\n", key->keyval);
    if (key->keyval == GDK_KEY_Alt_L || key->keyval == GDK_KEY_Alt_R)
        m_altKeyDown = false;
    if (key->keyval == primaryKeyL || key->keyval == primaryKeyR)
        m_primaryKeyDown = false;
    return false;
}

#if GTKMM_MAJOR_VERSION > 3 || (GTKMM_MAJOR_VERSION == 3 && (GTKMM_MINOR_VERSION > 91 || (GTKMM_MINOR_VERSION == 91 && GTKMM_MICRO_VERSION >= 2))) // GTKMM >= 3.91.2
bool MacroEditor::onMacroTreeViewKeyRelease(Gdk::EventKey& _key) {
    GdkEventKey* key = _key.gobj();
#else
void MacroEditor::onMacroTreeViewKeyRelease(GdkEventKey* key) {
#endif
    if (key->keyval == GDK_KEY_BackSpace || key->keyval == GDK_KEY_Delete) {
        if (m_altKeyDown)
            inverseDeleteSelectedRows();
        else if (m_primaryKeyDown)
            deleteSelectedRows();
    }
#if GTKMM_MAJOR_VERSION > 3 || (GTKMM_MAJOR_VERSION == 3 && (GTKMM_MINOR_VERSION > 91 || (GTKMM_MINOR_VERSION == 91 && GTKMM_MICRO_VERSION >= 2))) // GTKMM >= 3.91.2
    return true;
#endif
}

void MacroEditor::onValueCellEdited(const Glib::ustring& sPath, const Glib::ustring& text) {
    Gtk::TreePath path(sPath);
    Gtk::TreeModel::iterator iter = m_treeStoreMacro->get_iter(path);
    onMacroTreeViewRowValueChangedImpl(path, iter, text);
}

void MacroEditor::onMacroTreeViewRowValueChanged(const Gtk::TreeModel::Path& path,
                                                 const Gtk::TreeModel::iterator& iter)
{
    if (!iter) return;
    Gtk::TreeModel::Row row = *iter;
    Glib::ustring value = row[m_treeModelMacro.m_col_value];
    onMacroTreeViewRowValueChangedImpl(path, iter, value);
}

void MacroEditor::onMacroTreeViewRowValueChangedImpl(const Gtk::TreeModel::Path& path,
                                                     const Gtk::TreeModel::iterator& iter,
                                                     const Glib::ustring& value)
{
    if (m_ignoreTreeViewValueChange) return;
    if (!iter) return;
    Gtk::TreeModel::Row row = *iter;
    Serialization::UID uid = row[m_treeModelMacro.m_col_uid];
    Serialization::String gigvalue(gig_from_utf8(value));
    Serialization::Object& object = m_macro.objectByUID(uid);
    std::string errorText;
    try {
        if (object.type().isEnum() &&
            gig::enumKey(object.type().customTypeName(), gigvalue))
        {
            size_t iValue = gig::enumValue(gigvalue);
            m_macro.setAutoValue(object, ToString(iValue));
            // no auto correct here yet (due to numeric vs. textual values)
            if (row[m_treeModelMacro.m_col_value] != value)
                row[m_treeModelMacro.m_col_value] = value;
        } else if (object.type().isBool()) {
            m_macro.setAutoValue(object, gigvalue);
            Serialization::String sBoolean = _boolToStr( m_macro.valueAsBool(object) );
            // potentially auto correct (i.e. when type is bool, user entered '5' -> yields 'Yes')
            if (row[m_treeModelMacro.m_col_value] != sBoolean)
                row[m_treeModelMacro.m_col_value] = sBoolean;
        } else {
            m_macro.setAutoValue(object, gigvalue);
            // potentially auto correct (i.e. when type is bool, user entered 5 -> yields 1)
            if (row[m_treeModelMacro.m_col_value] != m_macro.valueAsString(object))
                row[m_treeModelMacro.m_col_value] = m_macro.valueAsString(object);
        }
        updateStatus();
    } catch (Serialization::Exception e) {
        errorText = e.Message;
    } catch (...) {
        errorText = _("Unknown exception during object value change");
    }
    if (!errorText.empty()) {
        Glib::ustring txt = _("Couldn't change value:\n") + errorText;
        Gtk::MessageDialog msg(*this, txt, false, Gtk::MESSAGE_ERROR);
        msg.run();
    }
}

void MacroEditor::deleteSelectedRows() {
    Glib::RefPtr<Gtk::TreeSelection> sel = m_treeViewMacro.get_selection();
    std::vector<Gtk::TreeModel::Path> rows = sel->get_selected_rows();
    deleteRows(rows);
}

void MacroEditor::deleteRows(const std::vector<Gtk::TreeModel::Path>& rows) {
    for (int r = rows.size() - 1; r >= 0; --r) {
        Gtk::TreeModel::iterator it = m_treeStoreMacro->get_iter(rows[r]);
        if (!it) continue;
        Gtk::TreeModel::Row row = *it;
        Serialization::UID uid = row[m_treeModelMacro.m_col_uid];
        if (uid == m_macro.rootObject().uid()) continue; // prohibit deleting root object
        Gtk::TreeModel::iterator itParent = row.parent();
        if (!itParent) continue;
        Gtk::TreeModel::Row rowParent = *itParent;
        Serialization::UID uidParent = rowParent[m_treeModelMacro.m_col_uid];
        //Serialization::Object& object = m_macro.objectByUID(uid);
        Serialization::Object& parentObject = m_macro.objectByUID(uidParent);
        const Serialization::Member& member = parentObject.memberByUID(uid);
        m_macro.removeMember(parentObject, member);
        //m_macro.remove(object);
    }
    reloadTreeView();
}

static bool _onEachTreeRow(const Gtk::TreeModel::Path& input, std::vector<Gtk::TreeModel::Path>* output) {
    output->push_back(input);
    return false; // continue walking the tree
}

void MacroEditor::inverseDeleteSelectedRows() {
    // get all rows of tree view
    std::vector<Gtk::TreeModel::Path> rows;
    m_treeViewMacro.get_model()->foreach_path(
        sigc::bind(
            sigc::ptr_fun(&_onEachTreeRow),
            &rows
        )
    );

    // erase all entries from "rows" which are currently selected
    std::vector<Gtk::TreeModel::Path> vSelected = m_treeViewMacro.get_selection()->get_selected_rows();
    for (int i = rows.size() - 1; i >= 0; --i) {
        bool bIsSelected = std::find(vSelected.begin(), vSelected.end(),
                                     rows[i]) != vSelected.end();
        if (bIsSelected)
            rows.erase(rows.begin() + i);
    }

    // delete those 'inverse' selected rows
    deleteRows(rows);
}

void MacroEditor::updateStatus() {
    m_applyButton.set_sensitive(isModified());
    updateStatusBar();
}

void MacroEditor::updateStatusBar() {
    // update status text
    std::string txt;
    m_statusLabel.set_markup(txt);
}

#if GTKMM_MAJOR_VERSION > 3 || (GTKMM_MAJOR_VERSION == 3 && (GTKMM_MINOR_VERSION > 91 || (GTKMM_MINOR_VERSION == 91 && GTKMM_MICRO_VERSION >= 2))) // GTKMM >= 3.91.2
bool MacroEditor::onWindowDelete(Gdk::Event& e) {
    return onWindowDeleteP(NULL);
}
#endif

bool MacroEditor::onWindowDeleteP(GdkEventAny* /*e*/) {
    //printf("onWindowDelete\n");

    if (!isModified()) return false; // propagate event further (which will close this window)

    //gchar* msg = g_strdup_printf(_("Apply changes to macro \"%s\" before closing?"),
    //                             m_macroOriginal->Name.c_str());
    gchar* msg = g_strdup(_("Apply changes to macro before closing?"));
    Gtk::MessageDialog dialog(*this, msg, false, Gtk::MESSAGE_WARNING, Gtk::BUTTONS_NONE);
    g_free(msg);
    dialog.set_secondary_text(_("If you close without applying, your changes will be lost."));
    dialog.add_button(_("Close _Without Applying"), Gtk::RESPONSE_NO);
    dialog.add_button(_("_Cancel"), Gtk::RESPONSE_CANCEL);
    dialog.add_button(_("_Apply"), Gtk::RESPONSE_YES);
    dialog.set_default_response(Gtk::RESPONSE_YES);
    int response = dialog.run();
    dialog.hide();

    // user decided to close macro editor without saving
    if (response == Gtk::RESPONSE_NO)
        return false; // propagate event further (which will close this window)

    // user cancelled dialog, thus don't close macro editor
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

bool MacroEditor::isModified() const {
    return m_macro.isModified();
}

void MacroEditor::onButtonCancel() {
    bool dropEvent = onWindowDeleteP(NULL);
    if (dropEvent) return;
    hide();
}

void MacroEditor::onButtonApply() {
    std::string errorText;
    try {
        // enforce re-encoding the abstract object model and resetting the
        // 'modified' state
        m_macro.rawData();
        // replace actual effective Archive object which is effectively used
        // for macro apply operations
        *m_macroOriginal = m_macro;
    } catch (Serialization::Exception e) {
        errorText = e.Message;
    } catch (...) {
        errorText = _("Unknown exception while applying macro changes");
    }
    if (!errorText.empty()) {
        Glib::ustring txt = _("Couldn't apply macro changes:\n") + errorText;
        Gtk::MessageDialog msg(*this, txt, false, Gtk::MESSAGE_ERROR);
        msg.run();
    }
    updateStatus();
    m_changes_applied.emit();
}

void MacroEditor::onWindowHide() {
    delete this; // this is the end, my friend
}
