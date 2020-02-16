/*
    Copyright (c) MMXX Christian Schoenebeck

    This file is part of "gigedit" and released under the terms of the
    GNU General Public License version 2.
*/

#include "ScriptPatchVars.h"

#define YELLOW "#c4950c"
#define MAGENTA "#790cc4"
#define LITE_GRAY "#bababa"

// Gtk treeview C callback if backspace key is hit while treeview has focus.
static gboolean onSelectCursorParent(GtkTreeView* treeview, gpointer userdata) {
    ScriptPatchVars* self = (ScriptPatchVars*) userdata;

    // This is a hack to prevent treeview from performing implied behaviour on
    // backspace key event. By default Gtk::TreeView would change current
    // selection whenever the user hits the backspace key (that is by navigating
    // to current node's parent). We don't want treeview doing that, since it
    // would prevent our custom backspace key handling (for reverting overridden
    // patch variables to their default value) from working correctly, because
    // our code relies on treeview's 'current selection' which would change
    // before our handler is triggered. Unfortunately there is currently no
    // official or cleaner way to prevent treeview from ignoring backspace key
    // events than the following hack.
    self->grab_focus();

    // Our key event handler would not be triggered automatically in this case,
    // so just call our handler directly instead.
    self->deleteSelectedRows();

    return false;
}

ScriptPatchVars::ScriptPatchVars() :
    m_ignoreTreeViewValueChange(false), m_instrument(NULL), m_editing(false)
{
    // create treeview (including its data model)
    m_treeStore = VarsTreeStore::create(m_treeModel);
    m_treeView.set_model(m_treeStore);
    m_treeView.get_selection()->set_mode(Gtk::SELECTION_MULTIPLE);
    {
        Gtk::CellRendererText* cellrenderer = Gtk::manage(new Gtk::CellRendererText());
        m_treeView.append_column(_("Name"), *cellrenderer);
        m_treeView.get_column(0)->add_attribute(cellrenderer->property_markup(), m_treeModel.m_col_name);
    }
    m_treeView.append_column(_("Type"), m_treeModel.m_col_type);
    Gtk::TreeViewColumn* valueColumn = new Gtk::TreeViewColumn(_("Value"));
    valueColumn->pack_start(m_valueCellRenderer);
    m_treeView.append_column(*valueColumn);
    {
        Gtk::TreeView::Column* column = valueColumn;
        column->add_attribute(m_valueCellRenderer.property_markup(),
                              m_treeModel.m_col_value);
        column->add_attribute(m_valueCellRenderer.property_has_entry(),
                              m_treeModel.m_col_allowTextEntry);
        column->add_attribute(m_valueCellRenderer.property_editable(),
                              m_treeModel.m_col_editable);
        column->add_attribute(m_valueCellRenderer.property_model(),
                              m_treeModel.m_col_options);
    }
    m_valueCellRenderer.property_text_column() = 0;
    m_valueCellRenderer.signal_edited().connect(
        sigc::mem_fun(*this, &ScriptPatchVars::onValueCellEdited)
    );
    {
        Gtk::TreeViewColumn* column = m_treeView.get_column(0);
        Gtk::CellRendererText* cellrenderer =
            dynamic_cast<Gtk::CellRendererText*>(column->get_first_cell());
        column->add_attribute(cellrenderer->property_foreground(), m_treeModel.m_col_name_color);
        column->add_attribute(cellrenderer->property_weight(), m_treeModel.m_col_name_weight);
    }
    {
        Gtk::TreeViewColumn* column = m_treeView.get_column(1);
        Gtk::CellRendererText* cellrenderer =
            dynamic_cast<Gtk::CellRendererText*>(column->get_first_cell());
        cellrenderer->property_foreground().set_value(LITE_GRAY);
    }
    {
        Gtk::TreeViewColumn* column = m_treeView.get_column(2);
        Gtk::CellRendererText* cellrenderer =
            dynamic_cast<Gtk::CellRendererText*>(column->get_first_cell());
        column->add_attribute(cellrenderer->property_foreground(), m_treeModel.m_col_value_color);
        column->add_attribute(cellrenderer->property_weight(), m_treeModel.m_col_value_weight);
        cellrenderer->signal_editing_started().connect(
            [this](Gtk::CellEditable*, const Glib::ustring&) {
                m_editing = true;
            }
        );
        cellrenderer->signal_editing_canceled().connect([this]{
            m_editing = false;
        });
    }
    m_treeView.set_headers_visible(true);
    m_treeView.get_selection()->signal_changed().connect(
        sigc::mem_fun(*this, &ScriptPatchVars::onTreeViewSelectionChanged)
    );
#if GTKMM_MAJOR_VERSION > 3 || (GTKMM_MAJOR_VERSION == 3 && (GTKMM_MINOR_VERSION > 91 || (GTKMM_MINOR_VERSION == 91 && GTKMM_MICRO_VERSION >= 2))) // GTKMM >= 3.91.2
    m_treeView.signal_key_release_event().connect
#else
    m_treeView.signal_key_release_event().connect_notify
#endif
    (
        sigc::mem_fun(*this, &ScriptPatchVars::onTreeViewKeyRelease)
    );
    // Gtk "row-activated" signal (Gtkmm's signal_row_activated) is only fired
    // if this single click property is set to true
    //m_treeView.set_activate_on_single_click(true);
    m_treeView.signal_row_activated().connect(
        [this](const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn* column){
            printf("row activated !!!!\n");
            //m_editing = true; // so we can detect while user is editing some value
        }
    );
    m_treeView.set_has_tooltip(true);
    m_treeView.signal_query_tooltip().connect(
        sigc::mem_fun(*this, &ScriptPatchVars::onQueryTreeViewTooltip)
    );
    // this signal does not exist in the gtkmm C++ TreeView class, so use C API
    g_signal_connect(G_OBJECT(m_treeView.gobj()), "select-cursor-parent", G_CALLBACK(onSelectCursorParent), this);
    m_treeStore->signal_row_changed().connect(
        sigc::mem_fun(*this, &ScriptPatchVars::onTreeViewRowChanged)
    );

/* FIXME: This was an attempt to prevent Gtk's treeview from reacting on
          backspace key event to prevent undesired navigation change by
          treeview. Didn't work out for some reason.
    {
        GtkWidget* widget = gtk_tree_view_new();
        GtkTreeViewClass* klass = GTK_TREE_VIEW_GET_CLASS(widget);
        GtkBindingSet* bindingSet = gtk_binding_set_by_class(klass);
        gtk_binding_entry_remove(bindingSet, GDK_KEY_BackSpace, (GdkModifierType) 0);
        gtk_binding_entry_remove(bindingSet, GDK_KEY_BackSpace, GDK_CONTROL_MASK);
        gtk_widget_destroy(widget);
    }
*/

    // scroll view setup
    add(m_treeView);
    set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);

#if HAS_GTKMM_SHOW_ALL_CHILDREN
    show_all_children();
#endif
}

static std::map<std::string,std::string> getDefaultValues(gig::Script* script) {
    std::map<std::string,std::string> vars;
    LinuxSampler::ScriptVM* vm = LinuxSampler::ScriptVMFactory::Create("gig");
    LinuxSampler::VMParserContext* ctx = vm->loadScript(
        script->GetScriptAsText(), std::map<String,String>(), &vars
    );
    if (ctx) delete ctx;
    if (vm) delete vm;
    return vars;
}

struct PatchVar {
    LinuxSampler::optional<std::string> defaultValue;
    LinuxSampler::optional<std::string> overrideValue;

    std::string value() const {
        if (overrideValue) return *overrideValue;
        if (defaultValue) return *defaultValue;
        return "";
    }

    bool isObsolete() const {
        return overrideValue && !defaultValue;
    }
};

static std::map<std::string,PatchVar> getPatchVars(gig::Instrument* instrument, int iScriptSlot) {
    std::map<std::string,PatchVar> vars;
    gig::Script* script = instrument->GetScriptOfSlot(iScriptSlot);
    if (!script) return vars;
    std::map<std::string,std::string> defaultValues = getDefaultValues(script);
    for (const auto& var : defaultValues) {
        vars[var.first].defaultValue = (std::string) trim(var.second);
    }
    std::map<std::string,std::string> overriddenValues =
        instrument->GetScriptPatchVariables(iScriptSlot);
    for (const auto& var : overriddenValues) {
        vars[var.first].overrideValue = var.second;
    }
    return vars;
}

static std::string varTypeStr(const std::string& varName) {
    if (varName.length() >= 1) {
        const char c = varName[0];
        switch (c) {
            case '$': return "Integer";
            case '%': return "Integer Array";
            case '~': return "Real";
            case '?': return "Real Array";
            case '@': return "String";
            case '!': return "String Array";
        }
    }
    return "";
}

void ScriptPatchVars::buildTreeViewVar(const Gtk::TreeModel::Row& parentRow,
                                       int iScriptSlot, gig::Script* script,
                                       const std::string name,
                                       const struct PatchVar* var)
{
#if GTKMM_MAJOR_VERSION > 3 || (GTKMM_MAJOR_VERSION == 3 && GTKMM_MINOR_VERSION > 24)
    //HACK: on GTKMM 3.9x append() below requires TreeNodeChildren, parentRow.children() returns TreeNodeConstChildren though, probably going to be fixed before final GTKMM4 release though.
    const Gtk::TreeNodeConstChildren& children = parentRow.children();
    Gtk::TreeNodeChildren* const pChildren = (Gtk::TreeNodeChildren* const) &children;
    Gtk::TreeModel::iterator iterRow = m_treeStore->append(*pChildren);
#else
    Gtk::TreeModel::iterator iterRow = m_treeStore->append(parentRow.children());
#endif
    Gtk::TreeModel::Row row = *iterRow;
    row[m_treeModel.m_col_name] = name;
    row[m_treeModel.m_col_name_color] = (var->isObsolete()) ? "red" : MAGENTA;
    if (var->overrideValue)
        row[m_treeModel.m_col_name_weight] = PANGO_WEIGHT_BOLD;
    row[m_treeModel.m_col_type] = varTypeStr(name) + " Variable";
    row[m_treeModel.m_col_slot] = iScriptSlot;
    row[m_treeModel.m_col_value] = var->value();
    row[m_treeModel.m_col_value_color] =
        (var->isObsolete()) ? "red" : (var->overrideValue) ? YELLOW : "gray";
    row[m_treeModel.m_col_value_tooltip] =
        (var->overrideValue && var->defaultValue) ?
            std::string("Default: ") + *var->defaultValue : "";
    if (var->overrideValue)
        row[m_treeModel.m_col_value_weight] = PANGO_WEIGHT_BOLD;
    row[m_treeModel.m_col_allowTextEntry] = true;
    row[m_treeModel.m_col_editable] = true;
    row[m_treeModel.m_col_script] = script;

    //TODO: syntax highlighting for values like e.g.:
    //row[m_treeModel.m_col_value] = "4<span foreground='#50BC00'>s</span>";
    //row[m_treeModel.m_col_value] = "0.3<span foreground='#50BC00'>s</span>";
    //row[m_treeModel.m_col_value] = "-6.3<span foreground='black'>d</span><span foreground='#50BC00'>B</span>";

    //TODO: in future we might add combo boxes for certain value selections
    // (would probably require some kind of NKSP language extension first)
    //const char** allKeys = gig::enumKeys(object.type().customTypeName());
    //if (allKeys) {
    //    Glib::RefPtr<Gtk::ListStore> refOptions = createComboOptions(allKeys);
    //    row[m_treeModelMacro.m_col_options] = refOptions;
    //}
    //Glib::RefPtr<Gtk::ListStore> refOptions = createComboOptions(_boolOptions);
    //row[m_treeModelMacro.m_col_options] = refOptions;
}

void ScriptPatchVars::buildTreeViewSlot(const Gtk::TreeModel::Row& parentRow, int iScriptSlot) {
#if GTKMM_MAJOR_VERSION > 3 || (GTKMM_MAJOR_VERSION == 3 && GTKMM_MINOR_VERSION > 24)
    //HACK: on GTKMM 3.9x append() below requires TreeNodeChildren, parentRow.children() returns TreeNodeConstChildren though, probably going to be fixed before final GTKMM4 release though.
    const Gtk::TreeNodeConstChildren& children = parentRow.children();
    Gtk::TreeNodeChildren* const pChildren = (Gtk::TreeNodeChildren* const) &children;
    Gtk::TreeModel::iterator iterRow = m_treeStore->append(*pChildren);
#else
    Gtk::TreeModel::iterator iterRow = m_treeStore->append(parentRow.children());
#endif
    gig::Script* pScript = m_instrument->GetScriptOfSlot(iScriptSlot);

    Gtk::TreeModel::Row row = *iterRow;
    row[m_treeModel.m_col_name] = pScript->Name + " <sup>Slot " + ToString(iScriptSlot+1) + "</sup>";
    row[m_treeModel.m_col_name_weight] = PANGO_WEIGHT_BOLD;
    row[m_treeModel.m_col_type] = "Script";
    row[m_treeModel.m_col_value] = "";
    row[m_treeModel.m_col_slot]  = -1;
    row[m_treeModel.m_col_allowTextEntry] = false;
    row[m_treeModel.m_col_editable] = false;
    row[m_treeModel.m_col_script] = NULL;
    row[m_treeModel.m_col_value_tooltip] = "";

    std::map<std::string,PatchVar> vars = getPatchVars(m_instrument, iScriptSlot);
    for (const auto& var : vars) {
        buildTreeViewVar(row, iScriptSlot, pScript, var.first, &var.second);
    }
}

void ScriptPatchVars::reloadTreeView() {
    m_ignoreTreeViewValueChange = true;

    m_treeStore->clear();
    if (!m_instrument) return;

    Gtk::TreeModel::iterator iterRoot = m_treeStore->append();
    Gtk::TreeModel::Row rowRoot = *iterRoot;
    rowRoot[m_treeModel.m_col_name]  = m_instrument->pInfo->Name;
    rowRoot[m_treeModel.m_col_name_weight] = PANGO_WEIGHT_BOLD;
    rowRoot[m_treeModel.m_col_type]  = "Instrument";
    rowRoot[m_treeModel.m_col_value] = "";
    rowRoot[m_treeModel.m_col_slot]  = -1;
    rowRoot[m_treeModel.m_col_allowTextEntry] = false;
    rowRoot[m_treeModel.m_col_editable] = false;
    rowRoot[m_treeModel.m_col_script] = NULL;
    rowRoot[m_treeModel.m_col_value_tooltip] = "";

    for (int i = 0; i < m_instrument->ScriptSlotCount(); ++i)
        buildTreeViewSlot(rowRoot, i);

    m_treeView.expand_all();

    m_ignoreTreeViewValueChange = false;
}

void ScriptPatchVars::onTreeViewSelectionChanged() {
}

bool ScriptPatchVars::onQueryTreeViewTooltip(int x, int y, bool keyboardTip,
                                             const Glib::RefPtr<Gtk::Tooltip>& tooltip)
{
    Gtk::TreeModel::iterator iter;
    if (!m_treeView.get_tooltip_context_iter(x, y, keyboardTip, iter)) {
        return false;
    }
    Gtk::TreeModel::Path path(iter);
    Gtk::TreeModel::Row row = *iter;
    Gtk::TreeViewColumn* pointedColumn = NULL;
    // resolve the precise table column the mouse points to
    {
        Gtk::TreeModel::Path path; // unused
        int cellX, cellY; // unused
        m_treeView.get_path_at_pos(x, y, path, pointedColumn, cellX, cellY);
    }
    Gtk::TreeViewColumn* valueColumn = m_treeView.get_column(2);
    if (pointedColumn == valueColumn) { // mouse hovers value column ...
        const Glib::ustring tip = row[m_treeModel.m_col_value_tooltip];
        if (tip.empty()) return false; // don't show tooltip
        // show model / cell's assigned tooltip
        tooltip->set_markup(tip);
        m_treeView.set_tooltip_cell(tooltip, &path, valueColumn, NULL);
        return true; // show tooltip
    }
    return false; // don't show tooltip
}

#if GTKMM_MAJOR_VERSION > 3 || (GTKMM_MAJOR_VERSION == 3 && (GTKMM_MINOR_VERSION > 91 || (GTKMM_MINOR_VERSION == 91 && GTKMM_MICRO_VERSION >= 2))) // GTKMM >= 3.91.2
bool ScriptPatchVars::onTreeViewKeyRelease(Gdk::EventKey& _key) {
    GdkEventKey* key = _key.gobj();
#else
void ScriptPatchVars::onTreeViewKeyRelease(GdkEventKey* key) {
#endif
    if (key->keyval == GDK_KEY_BackSpace || key->keyval == GDK_KEY_Delete) {
        printf("DELETE on script treeview row\n");
        deleteSelectedRows();
    }
out:
    ; // noop
#if GTKMM_MAJOR_VERSION > 3 || (GTKMM_MAJOR_VERSION == 3 && (GTKMM_MINOR_VERSION > 91 || (GTKMM_MINOR_VERSION == 91 && GTKMM_MICRO_VERSION >= 2))) // GTKMM >= 3.91.2
    return false;
#endif
}

void ScriptPatchVars::deleteSelectedRows() {
    Glib::RefPtr<Gtk::TreeSelection> sel = m_treeView.get_selection();
    std::vector<Gtk::TreeModel::Path> rows = sel->get_selected_rows();
    deleteRows(rows);
}

void ScriptPatchVars::deleteRows(const std::vector<Gtk::TreeModel::Path>& rows) {
    // ignore the backspace key here while user is editing some value
    if (m_ignoreTreeViewValueChange || m_editing) return;
    if (!m_instrument) return; // just to be sure

    m_ignoreTreeViewValueChange = true;

    signal_vars_to_be_changed.emit(m_instrument);

    for (ssize_t r = rows.size() - 1; r >= 0; --r) {
        Gtk::TreeModel::iterator it = m_treeStore->get_iter(rows[r]);
        if (!it) continue;
        Gtk::TreeModel::Row row = *it;
        gig::Script* script = row[m_treeModel.m_col_script];
        int slot = row[m_treeModel.m_col_slot];
        if (!script || slot == -1) continue; // prohibit deleting meta nodes
        std::string name = (Glib::ustring) row[m_treeModel.m_col_name];
        m_instrument->UnsetScriptPatchVariable(slot, name);
    }

    signal_vars_changed.emit(m_instrument);

    reloadTreeView();

    m_ignoreTreeViewValueChange = false;
}

void ScriptPatchVars::onValueCellEdited(const Glib::ustring& sPath, const Glib::ustring& text) {
    m_editing = false;
    Gtk::TreePath path(sPath);
    Gtk::TreeModel::iterator iter = m_treeStore->get_iter(path);
    onTreeViewRowValueChanged(path, iter, text);
}

void ScriptPatchVars::onTreeViewRowChanged(const Gtk::TreeModel::Path& path,
                                           const Gtk::TreeModel::iterator& iter)
{
    m_editing = false;
    if (!iter) return;
    Gtk::TreeModel::Row row = *iter;
    Glib::ustring value = row[m_treeModel.m_col_value];
    onTreeViewRowValueChanged(path, iter, value);
}

void ScriptPatchVars::onTreeViewRowValueChanged(const Gtk::TreeModel::Path& path,
                                                const Gtk::TreeModel::iterator& iter,
                                                const Glib::ustring value)
{
    m_editing = false;
    if (m_ignoreTreeViewValueChange || !m_instrument) return;

    Gtk::TreeModel::Row row = *iter;
    gig::Script* script = row[m_treeModel.m_col_script];
    int slot = row[m_treeModel.m_col_slot];
    if (!script || slot == -1) return; // prohibit altering meta nodes
    std::string name = (Glib::ustring) row[m_treeModel.m_col_name];

    signal_vars_to_be_changed.emit(m_instrument);

    m_instrument->SetScriptPatchVariable(slot, name, value);

    signal_vars_changed.emit(m_instrument);

    reloadTreeView();
}

void ScriptPatchVars::setInstrument(gig::Instrument* pInstrument, bool forceUpdate) {
    if (m_instrument == pInstrument && !forceUpdate)
        return;
    m_instrument = pInstrument;
    reloadTreeView();
}
