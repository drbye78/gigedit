/*                                                         -*- c++ -*-
 * Copyright (C) 2011-2019 Andreas Persson
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

#ifndef GIGEDIT_COMPAT_H
#define GIGEDIT_COMPAT_H

#ifdef GTK_HEADER_FILE
# include GTK_HEADER_FILE(gtk.h)
#else
# include <gtk/gtk.h>
#endif

#ifdef GLIBMM_HEADER_FILE
# include GLIBMM_HEADER_FILE(glibmmconfig.h)
#else
# include <glibmmconfig.h>
#endif

#ifdef PANGOMM_HEADER_FILE
# include PANGOMM_HEADER_FILE(pangommconfig.h)
#else
# include <pangommconfig.h>
#endif

#ifdef CAIROMM_HEADER_FILE
# include CAIROMM_HEADER_FILE(cairommconfig.h)
#elif HAVE_CAIROMMCONFIG_H
# include <cairommconfig.h>
#endif

#ifdef GTKMM_HEADER_FILE
# include GTKMM_HEADER_FILE(gtkmmconfig.h)
#else
# include <gtkmmconfig.h>
#endif

#ifndef HAS_PANGOMM_CPP11_ENUMS
   // new enums introduced in unstable pangomm 2.41.3, but not in stable 2.42
# if PANGOMM_MAJOR_VERSION > 2 || (PANGOMM_MAJOR_VERSION == 2 && ((PANGOMM_MINOR_VERSION == 41 && PANGOMM_MICRO_VERSION >= 3) || PANGOMM_MINOR_VERSION > 42))
#  define HAS_PANGOMM_CPP11_ENUMS 1
# else
#  define HAS_PANGOMM_CPP11_ENUMS 0
# endif
#endif

// Gtk::UIManager had been replaced by Gtk::Builder in GTKMM 3 and removed in GTKMM 4
// Gtk::ActionGroup had been replaced by Gio::ActionGroup in GTKMM 3 and removed in GTKMM 4
#if !defined(USE_GTKMM_BUILDER) || !defined(USE_GLIB_ACTION) // gtkmm > 3.22 :
# if GTKMM_MAJOR_VERSION > 3 || (GTKMM_MAJOR_VERSION == 3 && GTKMM_MINOR_VERSION > 24)
#  define USE_GTKMM_BUILDER 1
#  define USE_GLIB_ACTION 1
# else
#  define USE_GTKMM_BUILDER 0
#  define USE_GLIB_ACTION 0
# endif
#endif

#ifndef HAS_GDKMM_SEAT
# if GTKMM_MAJOR_VERSION > 2 || (GTKMM_MAJOR_VERSION == 2 && (GTKMM_MINOR_VERSION > 99 || (GTKMM_MINOR_VERSION == 99 && GTKMM_MICRO_VERSION >= 1))) // GTKM >= 2.99.1
#  define HAS_GDKMM_SEAT 1
# else
#  define HAS_GDKMM_SEAT 0
# endif
#endif

#ifndef HAS_GTKMM_CPP11_ENUMS // gtkmm > 3.22
# if GTKMM_MAJOR_VERSION > 3 || (GTKMM_MAJOR_VERSION == 3 && GTKMM_MINOR_VERSION > 24)
#  define HAS_GTKMM_CPP11_ENUMS 1
# else
#  define HAS_GTKMM_CPP11_ENUMS 0
# endif
#endif

// alignment.h had been removed in GTKMM 4
#ifndef HAS_GTKMM_ALIGNMENT // gtkmm > 3.22 :
# if GTKMM_MAJOR_VERSION > 3 || (GTKMM_MAJOR_VERSION == 3 && GTKMM_MINOR_VERSION > 24)
#  define HAS_GTKMM_ALIGNMENT 0
# else
#  define HAS_GTKMM_ALIGNMENT 1
# endif
#endif

// Gtk::Table had been replaced by Gtk::Grid in GTKMM 3 and removed in GTKMM 4
// stock.h had been removed in GTKMM 4
// Gtk::VBox, Gtk::HBox and Gtk::HButtonBox had been replaced by Gtk::Box in GTKMM 3 and removed in GTKMM 4
#if !defined(USE_GTKMM_GRID) || !defined(HAS_GTKMM_STOCK) || !defined(USE_GTKMM_BOX) || !defined(USE_GTKMM_PANED) // gtkmm > 3.22 :
# if GTKMM_MAJOR_VERSION > 3 || (GTKMM_MAJOR_VERSION == 3 && GTKMM_MINOR_VERSION > 24)
#  define USE_GTKMM_GRID 1
#  define HAS_GTKMM_STOCK 0
#  define USE_GTKMM_BOX 1
#  define USE_GTKMM_PANED 1
# else
#  define USE_GTKMM_GRID 0
#  define HAS_GTKMM_STOCK 1
#  define USE_GTKMM_BOX 0
#  define USE_GTKMM_PANED 0
# endif
#endif

// Gtk::Widget::show_all_children() had been removed in GTKMM 3.89.4
#ifndef HAS_GTKMM_SHOW_ALL_CHILDREN
# if GTKMM_MAJOR_VERSION > 3 || (GTKMM_MAJOR_VERSION == 3 && (GTKMM_MINOR_VERSION > 89 || (GTKMM_MINOR_VERSION == 89 && GTKMM_MICRO_VERSION >= 4)))
#  define HAS_GTKMM_SHOW_ALL_CHILDREN 0
# else
#  define HAS_GTKMM_SHOW_ALL_CHILDREN 1
# endif
#endif

// enums have been renamed in cairomm 1.15.4
#ifndef HAS_CAIROMM_CPP11_ENUMS // cairomm >= 1.15.4 :
# if CAIROMM_MAJOR_VERSION > 1 || (CAIROMM_MAJOR_VERSION == 1 && (CAIROMM_MINOR_VERSION > 15 || (CAIROMM_MINOR_VERSION == 15 && CAIROMM_MICRO_VERSION >= 4)))
#  define HAS_CAIROMM_CPP11_ENUMS 1
# else
#  define HAS_CAIROMM_CPP11_ENUMS 0
# endif
#endif

#include <gtkmm/buttonbox.h>
#include <gtkmm/scale.h>

#if USE_GTKMM_BOX
# include <gtkmm/box.h>
class HBox : public Gtk::Box {
public:
    HBox() : Gtk::Box(Gtk::Orientation::HORIZONTAL) {}
};

class VBox : public Gtk::Box {
public:
    VBox() : Gtk::Box(Gtk::Orientation::VERTICAL) {}
};

class HButtonBox : public Gtk::ButtonBox {
public:
    HButtonBox() : Gtk::ButtonBox(Gtk::Orientation::HORIZONTAL) {}
};

class HScale : public Gtk::Scale {
public:
    HScale() : Gtk::Scale(Gtk::Orientation::HORIZONTAL) {}
    HScale(const Glib::RefPtr<Gtk::Adjustment>& adjustment) : Gtk::Scale(adjustment, Gtk::Orientation::HORIZONTAL) {}
};
#else
# if GTKMM_MAJOR_VERSION > 3 || (GTKMM_MAJOR_VERSION == 3 && (GTKMM_MINOR_VERSION > 1 || (GTKMM_MINOR_VERSION == 1 && GTKMM_MICRO_VERSION >= 6))) // GTKMM >= 3.1.6
#  include <gtkmm/hvbox.h>
#endif
typedef Gtk::HBox HBox;
typedef Gtk::VBox VBox;
typedef Gtk::HButtonBox HButtonBox;
typedef Gtk::HScale HScale;
#endif // USE_GTKMM_BOX

#if USE_GTKMM_PANED
# include <gtkmm/paned.h>
namespace Gtk {
    class HPaned : public Paned {
    public:
        HPaned() : Paned(Orientation::HORIZONTAL) {}
    };
}
#endif // USE_GTKMM_PANED

#if USE_GLIB_ACTION
# include <giomm/simpleactiongroup.h>
# include <giomm/simpleaction.h>
typedef Gio::SimpleActionGroup ActionGroup;
typedef Gio::SimpleAction Action;
#else
# include <gtkmm/actiongroup.h>
# include <gtkmm/action.h>
typedef Gtk::ActionGroup ActionGroup;
typedef Gtk::Action Action;
#endif // USE_GLIB_ACTION

#if HAS_PANGOMM_CPP11_ENUMS
# include <pangomm/layout.h>
namespace Pango {
    const Alignment ALIGN_LEFT = Alignment::LEFT;
    const Alignment ALIGN_CENTER = Alignment::CENTER;
    const Alignment ALIGN_RIGHT = Alignment::RIGHT;
}
#endif // HAS_PANGOMM_CPP11_ENUMS

#if HAS_GTKMM_CPP11_ENUMS
# include <gdkmm/types.h>
# include <gdkmm/cursor.h>
# include <gdkmm/pixbuf.h>
# include <gtkmm/enums.h>
# include <gtkmm/box.h>
# include <gtkmm/icontheme.h>
# include <gtkmm/messagedialog.h>
# include <gtkmm/dialog.h>
# include <gtkmm/textview.h>
# include <gtkmm/filechooser.h>
namespace Gdk {
    const Cursor::Type SB_H_DOUBLE_ARROW = Cursor::Type::SB_H_DOUBLE_ARROW;
    const Cursor::Type FLEUR = Cursor::Type::FLEUR;
    const InterpType INTERP_NEAREST = InterpType::NEAREST;
    const InterpType INTERP_TILES = InterpType::TILES;
    const InterpType INTERP_BILINEAR = InterpType::BILINEAR;
    const InterpType INTERP_HYPER = InterpType::HYPER;
    const ModifierType SHIFT_MASK = ModifierType::SHIFT_MASK;
    const ModifierType LOCK_MASK = ModifierType::LOCK_MASK;
    const ModifierType CONTROL_MASK = ModifierType::CONTROL_MASK;
    const ModifierType MOD1_MASK = ModifierType::MOD1_MASK;
    const ModifierType MOD2_MASK = ModifierType::MOD2_MASK;
    const ModifierType MOD3_MASK = ModifierType::MOD3_MASK;
    const ModifierType MOD4_MASK = ModifierType::MOD4_MASK;
    const ModifierType MOD5_MASK = ModifierType::MOD5_MASK;
    const ModifierType BUTTON1_MASK = ModifierType::BUTTON1_MASK;
    const ModifierType BUTTON2_MASK = ModifierType::BUTTON2_MASK;
    const ModifierType BUTTON3_MASK = ModifierType::BUTTON3_MASK;
    const ModifierType BUTTON4_MASK = ModifierType::BUTTON4_MASK;
    const ModifierType BUTTON5_MASK = ModifierType::BUTTON5_MASK;
    const ModifierType SUPER_MASK = ModifierType::SUPER_MASK;
    const ModifierType HYPER_MASK = ModifierType::HYPER_MASK;
    const ModifierType META_MASK = ModifierType::META_MASK;
    const ModifierType RELEASE_MASK = ModifierType::RELEASE_MASK;
    const ModifierType MODIFIER_MASK = ModifierType::MODIFIER_MASK;
}
namespace Gtk {
    const PackOptions SHRINK = PackOptions::SHRINK;
    const PackOptions PACK_SHRINK = PackOptions::SHRINK;
    const PackOptions PACK_EXPAND_PADDING = PackOptions::EXPAND_PADDING;
    const PackOptions PACK_EXPAND_WIDGET = PackOptions::EXPAND_WIDGET;
    const ButtonBoxStyle BUTTONBOX_SPREAD = ButtonBoxStyle::SPREAD;
    const ButtonBoxStyle BUTTONBOX_EDGE = ButtonBoxStyle::EDGE;
    const ButtonBoxStyle BUTTONBOX_START = ButtonBoxStyle::START;
    const ButtonBoxStyle BUTTONBOX_END = ButtonBoxStyle::END;
    const ButtonBoxStyle BUTTONBOX_CENTER = ButtonBoxStyle::CENTER;
    const ButtonBoxStyle BUTTONBOX_EXPAND = ButtonBoxStyle::EXPAND;
    const MessageType MESSAGE_INFO = MessageType::INFO;
    const MessageType MESSAGE_WARNING = MessageType::WARNING;
    const MessageType MESSAGE_QUESTION = MessageType::QUESTION;
    const MessageType MESSAGE_ERROR = MessageType::ERROR;
    const MessageType MESSAGE_OTHER = MessageType::OTHER;
    const ButtonsType BUTTONS_NONE = ButtonsType::NONE;
    const ButtonsType BUTTONS_OK = ButtonsType::OK;
    const ButtonsType BUTTONS_CLOSE = ButtonsType::CLOSE;
    const ButtonsType BUTTONS_CANCEL = ButtonsType::CANCEL;
    const ButtonsType BUTTONS_YES_NO = ButtonsType::YES_NO;
    const ButtonsType BUTTONS_OK_CANCEL = ButtonsType::OK_CANCEL;
    const ResponseType RESPONSE_NONE = ResponseType::NONE;
    const ResponseType RESPONSE_REJECT = ResponseType::REJECT;
    const ResponseType RESPONSE_ACCEPT = ResponseType::ACCEPT;
    const ResponseType RESPONSE_DELETE_EVENT = ResponseType::DELETE_EVENT;
    const ResponseType RESPONSE_OK = ResponseType::OK;
    const ResponseType RESPONSE_CANCEL = ResponseType::CANCEL;
    const ResponseType RESPONSE_CLOSE = ResponseType::CLOSE;
    const ResponseType RESPONSE_YES = ResponseType::YES;
    const ResponseType RESPONSE_NO = ResponseType::NO;
    const ResponseType RESPONSE_APPLY = ResponseType::APPLY;
    const ResponseType RESPONSE_HELP = ResponseType::HELP;
    const ShadowType SHADOW_NONE = ShadowType::NONE;
    const ShadowType SHADOW_IN = ShadowType::IN;
    const ShadowType SHADOW_OUT = ShadowType::OUT;
    const ShadowType SHADOW_ETCHED_IN = ShadowType::ETCHED_IN;
    const ShadowType SHADOW_ETCHED_OUT = ShadowType::ETCHED_OUT;
    const Align ALIGN_FILL = Align::FILL;
    const Align ALIGN_START = Align::START;
    const Align ALIGN_END = Align::END;
    const Align ALIGN_CENTER = Align::CENTER;
    const Align ALIGN_BASELINE = Align::BASELINE;
    const WindowPosition WIN_POS_NONE = WindowPosition::NONE;
    const WindowPosition WIN_POS_CENTER = WindowPosition::CENTER;
    const WindowPosition WIN_POS_MOUSE = WindowPosition::MOUSE;
    const WindowPosition WIN_POS_CENTER_ALWAYS = WindowPosition::CENTER_ALWAYS;
    const WindowPosition WIN_POS_CENTER_ON_PARENT = WindowPosition::CENTER_ON_PARENT;
    const PolicyType POLICY_ALWAYS = PolicyType::ALWAYS;
    const PolicyType POLICY_AUTOMATIC = PolicyType::AUTOMATIC;
    const PolicyType POLICY_NEVER = PolicyType::NEVER;
    const PolicyType POLICY_EXTERNAL = PolicyType::EXTERNAL;
    const ToolbarStyle TOOLBAR_ICONS = ToolbarStyle::ICONS;
    const ToolbarStyle TOOLBAR_TEXT = ToolbarStyle::TEXT;
    const ToolbarStyle TOOLBAR_BOTH = ToolbarStyle::BOTH;
    const ToolbarStyle TOOLBAR_BOTH_HORIZ = ToolbarStyle::BOTH_HORIZ;
    const DirectionType DIR_TAB_FORWARD = DirectionType::TAB_FORWARD;
    const DirectionType DIR_TAB_BACKWARD = DirectionType::TAB_BACKWARD;
    const DirectionType DIR_UP = DirectionType::UP;
    const DirectionType DIR_DOWN = DirectionType::DOWN;
    const DirectionType DIR_LEFT = DirectionType::LEFT;
    const DirectionType DIR_RIGHT = DirectionType::RIGHT;
    const SelectionMode SELECTION_NONE = SelectionMode::NONE;
    const SelectionMode SELECTION_SINGLE = SelectionMode::SINGLE;
    const SelectionMode SELECTION_BROWSE = SelectionMode::BROWSE;
    const SelectionMode SELECTION_MULTIPLE = SelectionMode::MULTIPLE;
    const BuiltinIconSize ICON_SIZE_INVALID = BuiltinIconSize::INVALID;
    const BuiltinIconSize ICON_SIZE_MENU = BuiltinIconSize::MENU;
    const BuiltinIconSize ICON_SIZE_SMALL_TOOLBAR = BuiltinIconSize::SMALL_TOOLBAR;
    const BuiltinIconSize ICON_SIZE_LARGE_TOOLBAR = BuiltinIconSize::LARGE_TOOLBAR;
    const BuiltinIconSize ICON_SIZE_BUTTON = BuiltinIconSize::BUTTON;
    const BuiltinIconSize ICON_SIZE_DND = BuiltinIconSize::DND;
    const BuiltinIconSize ICON_SIZE_DIALOG = BuiltinIconSize::DIALOG;
    const IconLookupFlags ICON_LOOKUP_NO_SVG = IconLookupFlags::NO_SVG;
    const IconLookupFlags ICON_LOOKUP_FORCE_SVG = IconLookupFlags::FORCE_SVG;
    const IconLookupFlags ICON_LOOKUP_USE_BUILTIN = IconLookupFlags::USE_BUILTIN;
    const IconLookupFlags ICON_LOOKUP_GENERIC_FALLBACK = IconLookupFlags::GENERIC_FALLBACK;
    const IconLookupFlags ICON_LOOKUP_FORCE_SIZE = IconLookupFlags::FORCE_SIZE;
    const IconLookupFlags ICON_LOOKUP_FORCE_REGULAR = IconLookupFlags::FORCE_REGULAR;
    const IconLookupFlags ICON_LOOKUP_FORCE_SYMBOLIC = IconLookupFlags::FORCE_SYMBOLIC;
    const IconLookupFlags ICON_LOOKUP_DIR_LTR = IconLookupFlags::DIR_LTR;
    const IconLookupFlags ICON_LOOKUP_DIR_RTL = IconLookupFlags::DIR_RTL;
    const TextWindowType TEXT_WINDOW_PRIVATE = TextWindowType::PRIVATE;
    const TextWindowType TEXT_WINDOW_WIDGET = TextWindowType::WIDGET;
    const TextWindowType TEXT_WINDOW_TEXT = TextWindowType::TEXT;
    const TextWindowType TEXT_WINDOW_LEFT = TextWindowType::LEFT;
    const TextWindowType TEXT_WINDOW_RIGHT = TextWindowType::RIGHT;
    const TextWindowType TEXT_WINDOW_TOP = TextWindowType::TOP;
    const TextWindowType TEXT_WINDOW_BOTTOM = TextWindowType::BOTTOM;
    const FileChooser::Action FILE_CHOOSER_ACTION_OPEN = FileChooser::Action::OPEN;
    const FileChooser::Action FILE_CHOOSER_ACTION_SAVE = FileChooser::Action::SAVE;
    const FileChooser::Action FILE_CHOOSER_ACTION_SELECT_FOLDER = FileChooser::Action::SELECT_FOLDER;
    const FileChooser::Action FILE_CHOOSER_ACTION_CREATE_FOLDER = FileChooser::Action::CREATE_FOLDER;
}
#endif // HAS_GTKMM_CPP11_ENUMS

// 2.10

#if (GTKMM_MAJOR_VERSION == 2 && GTKMM_MINOR_VERSION < 10) || GTKMM_MAJOR_VERSION < 2
#define create_cairo_context()                                          \
    gobj() ? Cairo::RefPtr<Cairo::Context>(                             \
        new Cairo::Context(gdk_cairo_create(get_window()->gobj()))) :   \
    Cairo::RefPtr<Cairo::Context>()
#endif


// 2.12

#if (GTKMM_MAJOR_VERSION == 2 && GTKMM_MINOR_VERSION < 12) || GTKMM_MAJOR_VERSION < 2
#include <gtkmm/cellrenderertext.h>
namespace Gtk {
    // this is not a real spin cell renderer, instead it's just text,
    // extended with a property for storing an adjustment
    class CellRendererSpin : public CellRendererText {
    private:
        Adjustment* adj;
        struct Proxy {
            Adjustment*& adj;
            Proxy(Adjustment*& adj) : adj(adj) { }
            const Adjustment* get_value() const { return adj; }
            void operator=(Adjustment* x) { adj = x; }
        };
    public:
        Proxy property_adjustment() const {
            return const_cast<Adjustment*&>(adj);
        }
    };
}
#endif


// 2.18

#if (GTKMM_MAJOR_VERSION == 2 && GTKMM_MINOR_VERSION < 18) || GTKMM_MAJOR_VERSION < 2
#define set_can_focus() set_flags(Gtk::CAN_FOCUS)
#define set_can_default() set_flags(Gtk::CAN_DEFAULT)
#endif


// 2.21.9

#if (GTKMM_MAJOR_VERSION == 2 && GTKMM_MINOR_VERSION == 21 && GTKMM_MICRO_VERSION < 9) || \
    (GTKMM_MAJOR_VERSION == 2 && GTKMM_MINOR_VERSION < 21) || GTKMM_MAJOR_VERSION < 2
#define ALIGN_START ALIGN_LEFT
#define ALIGN_END ALIGN_RIGHT
#endif


// 2.24

#if (GTKMM_MAJOR_VERSION == 2 && GTKMM_MINOR_VERSION < 24) || GTKMM_MAJOR_VERSION < 2
#define get_first_cell() get_first_cell_renderer()
#endif


// 3.0

#if (GTKMM_MAJOR_VERSION == 2 && GTKMM_MINOR_VERSION < 90) || GTKMM_MAJOR_VERSION < 2
#include <cairomm/context.h>
#include <gdkmm/color.h>

namespace Gdk {
    typedef Color RGBA;

    namespace Cairo {
        void set_source_rgba(const ::Cairo::RefPtr< ::Cairo::Context >& context,
                             const Gdk::RGBA& color);
    }
}
#endif


// glibmm 2.31.2

#if (GLIBMM_MAJOR_VERSION == 2 && GLIBMM_MINOR_VERSION == 31 && GLIBMM_MICRO_VERSION < 2) || \
    (GLIBMM_MAJOR_VERSION == 2 && GLIBMM_MINOR_VERSION < 31) || GLIBMM_MAJOR_VERSION < 2

namespace Glib {
    namespace Threads {
        using Glib::Thread;
        using Glib::Mutex;
        using Glib::Cond;
    }
}

#define OLD_THREADS

#endif // glibmm 2.31.2


// glibmm 2.47.3

#if (GLIBMM_MAJOR_VERSION == 2 && GLIBMM_MINOR_VERSION == 47 && GLIBMM_MICRO_VERSION < 3) || \
    (GLIBMM_MAJOR_VERSION == 2 && GLIBMM_MINOR_VERSION < 47) || GLIBMM_MAJOR_VERSION < 2

#define GLIB_THREADS

#endif

#include <gtkmm/label.h>

/** @brief Auto wrapped (multi-line) text, auto shrank to min. dimensions.
 *
 * This widgets is intended for large text blocks which are expected to require
 * the text to be automatically wrapped (i.e. auto inserted line breaks) such
 * that it would not exceed the label's maximum allowed width according to its
 * parent container.
 *
 * This class overrides get_preferred_height_for_width_vfunc() to fix the
 * default behaviour of Gtk::Label, which by default calculates a height under
 * the assumption that EVERY word of the text had a line break, which would
 * cause the label to claim much more vertical space than actually required for
 * the text.
 */
class MultiLineLabel : public Gtk::Label {
public:
    MultiLineLabel() : Gtk::Label() {
        #if GTKMM_MAJOR_VERSION >= 3
        set_line_wrap();
        #endif
    }

    void set_markup(const Glib::ustring& s) {
        Gtk::Label::set_markup(s);
        m_markup = s;
    }

    void get_preferred_height_for_width_vfunc(int width, int& minimum_height, int& natural_height) const {
        Gtk::Label::get_preferred_height_for_width_vfunc(width, minimum_height, natural_height);
        //printf("super suggests minimum_height=%d natural_height=%d\n", minimum_height, natural_height);

        Pango::Layout* origLayout = const_cast<Pango::Layout*>(get_layout().operator->());
        Glib::RefPtr<Pango::Layout> layout = origLayout->copy();
        Glib::ustring s = (!m_markup.empty()) ? m_markup : get_text();
        layout->set_markup(s);
        int w, h;
        layout->get_pixel_size(w, h);
        h += get_margin_top() + get_margin_bottom();
        //printf("calculated w=%d h=%d\n", w, h);

        minimum_height = h;
        if (natural_height < h)
            natural_height = h;
    }

private:
    Glib::ustring m_markup;
};

#endif // GIGEDIT_COMPAT_H
