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

#include "global.h"
#include "compat.h"
#include "Settings.h"
#include <gtkmm/box.h>
#include "dimregionchooser.h"
#include <cairomm/context.h>
#include <cairomm/surface.h>
#include <gdkmm/cursor.h>
#include <gdkmm/general.h>
#if HAS_GDKMM_SEAT
# include <gdkmm/seat.h>
#endif
#include <glibmm/stringutils.h>
#if HAS_GTKMM_STOCK
# include <gtkmm/stock.h>
#endif
#include <glibmm/ustring.h>
#include <gtkmm/messagedialog.h>
#include <assert.h>

#include "gfx/builtinpix.h"

//TODO: this function and dimensionCaseOf() from global.h are duplicates, eliminate either one of them!
static DimensionCase caseOfDimRegion(gig::DimensionRegion* dr, bool* isValidZone) {
    DimensionCase dimCase;
    if (!dr) {
        *isValidZone = false;
        return dimCase;
    }

    gig::Region* rgn = (gig::Region*) dr->GetParent();

    // find the dimension region index of the passed dimension region
    int drIndex;
    for (drIndex = 0; drIndex < 256; ++drIndex)
        if (rgn->pDimensionRegions[drIndex] == dr)
            break;

    // not found in region, something's horribly wrong
    if (drIndex == 256) {
        fprintf(stderr, "DimRegionChooser: ERROR: index of dim region not found!\n");
        *isValidZone = false;
        return DimensionCase();
    }

    for (int d = 0, baseBits = 0; d < rgn->Dimensions; ++d) {
        const int bits = rgn->pDimensionDefinitions[d].bits;
        dimCase[rgn->pDimensionDefinitions[d].dimension] =
            (drIndex >> baseBits) & ((1 << bits) - 1);
        baseBits += bits;
        // there are also DimensionRegion objects of unused zones, skip them
        if (dimCase[rgn->pDimensionDefinitions[d].dimension] >= rgn->pDimensionDefinitions[d].zones) {
            *isValidZone = false;
            return DimensionCase();
        }
    }

    *isValidZone = true;
    return dimCase;
}

DimRegionChooser::DimRegionChooser(Gtk::Window& window) :
    red("#ff476e"),
    blue("#4796ff"),
    black("black"),
    white("white")
{
    // make sure blue hatched pattern pixmap is loaded
    loadBuiltInPix();

    // create blue hatched pattern 1
    {
        const int width = blueHatchedPattern->get_width();
        const int height = blueHatchedPattern->get_height();
        const int stride = blueHatchedPattern->get_rowstride();

        // manually convert from RGBA to ARGB
        this->blueHatchedPatternARGB = blueHatchedPattern->copy();
        const int pixelSize = stride / width;
        const int totalPixels = width * height;
        assert(pixelSize == 4);
        unsigned char* ptr = this->blueHatchedPatternARGB->get_pixels();
        for (int iPixel = 0; iPixel < totalPixels; ++iPixel, ptr += pixelSize) {
            const unsigned char r = ptr[0];
            const unsigned char g = ptr[1];
            const unsigned char b = ptr[2];
            const unsigned char a = ptr[3];
            ptr[0] = b;
            ptr[1] = g;
            ptr[2] = r;
            ptr[3] = a;
        }

        Cairo::RefPtr<Cairo::ImageSurface> imageSurface = Cairo::ImageSurface::create(
#if HAS_CAIROMM_CPP11_ENUMS
            this->blueHatchedPatternARGB->get_pixels(), Cairo::Surface::Format::ARGB32, width, height, stride
#else
            this->blueHatchedPatternARGB->get_pixels(), Cairo::FORMAT_ARGB32, width, height, stride
#endif
        );
        this->blueHatchedSurfacePattern = Cairo::SurfacePattern::create(imageSurface);
#if HAS_CAIROMM_CPP11_ENUMS
        this->blueHatchedSurfacePattern->set_extend(Cairo::Pattern::Extend::REPEAT);
#else
        this->blueHatchedSurfacePattern->set_extend(Cairo::EXTEND_REPEAT);
#endif
    }

    // create blue hatched pattern 2
    {
        const int width = blueHatchedPattern2->get_width();
        const int height = blueHatchedPattern2->get_height();
        const int stride = blueHatchedPattern2->get_rowstride();

        // manually convert from RGBA to ARGB
        this->blueHatchedPattern2ARGB = blueHatchedPattern2->copy();
        const int pixelSize = stride / width;
        const int totalPixels = width * height;
        assert(pixelSize == 4);
        unsigned char* ptr = this->blueHatchedPattern2ARGB->get_pixels();
        for (int iPixel = 0; iPixel < totalPixels; ++iPixel, ptr += pixelSize) {
            const unsigned char r = ptr[0];
            const unsigned char g = ptr[1];
            const unsigned char b = ptr[2];
            const unsigned char a = ptr[3];
            ptr[0] = b;
            ptr[1] = g;
            ptr[2] = r;
            ptr[3] = a;
        }

        Cairo::RefPtr<Cairo::ImageSurface> imageSurface = Cairo::ImageSurface::create(
#if HAS_CAIROMM_CPP11_ENUMS
            this->blueHatchedPattern2ARGB->get_pixels(), Cairo::Surface::Format::ARGB32, width, height, stride
#else
            this->blueHatchedPattern2ARGB->get_pixels(), Cairo::FORMAT_ARGB32, width, height, stride
#endif
        );
        this->blueHatchedSurfacePattern2 = Cairo::SurfacePattern::create(imageSurface);
#if HAS_CAIROMM_CPP11_ENUMS
        this->blueHatchedSurfacePattern2->set_extend(Cairo::Pattern::Extend::REPEAT);
#else
        this->blueHatchedSurfacePattern2->set_extend(Cairo::EXTEND_REPEAT);
#endif
    }

    // create gray blue hatched pattern
    {
        const int width = grayBlueHatchedPattern->get_width();
        const int height = grayBlueHatchedPattern->get_height();
        const int stride = grayBlueHatchedPattern->get_rowstride();

        // manually convert from RGBA to ARGB
        this->grayBlueHatchedPatternARGB = grayBlueHatchedPattern->copy();
        const int pixelSize = stride / width;
        const int totalPixels = width * height;
        assert(pixelSize == 4);
        unsigned char* ptr = this->grayBlueHatchedPatternARGB->get_pixels();
        for (int iPixel = 0; iPixel < totalPixels; ++iPixel, ptr += pixelSize) {
            const unsigned char r = ptr[0];
            const unsigned char g = ptr[1];
            const unsigned char b = ptr[2];
            const unsigned char a = ptr[3];
            ptr[0] = b;
            ptr[1] = g;
            ptr[2] = r;
            ptr[3] = a;
        }

        Cairo::RefPtr<Cairo::ImageSurface> imageSurface = Cairo::ImageSurface::create(
#if HAS_CAIROMM_CPP11_ENUMS
            this->grayBlueHatchedPatternARGB->get_pixels(), Cairo::Surface::Format::ARGB32, width, height, stride
#else
            this->grayBlueHatchedPatternARGB->get_pixels(), Cairo::FORMAT_ARGB32, width, height, stride
#endif
        );
        this->grayBlueHatchedSurfacePattern = Cairo::SurfacePattern::create(imageSurface);
#if HAS_CAIROMM_CPP11_ENUMS
        this->grayBlueHatchedSurfacePattern->set_extend(Cairo::Pattern::Extend::REPEAT);
#else
        this->grayBlueHatchedSurfacePattern->set_extend(Cairo::EXTEND_REPEAT);
#endif
    }

    instrument = 0;
    region = 0;
    maindimregno = -1;
    maindimtype = gig::dimension_none; // initialize with invalid dimension type
    focus_line = 0;
    resize.active = false;
    cursor_is_resize = false;
    h = 24;
    multiSelectKeyDown = false;
    primaryKeyDown = false;
    shiftKeyDown = false;
    modifybothchannels = modifyalldimregs = modifybothchannels = false;
    set_can_focus();

    const Glib::ustring txtUseCheckBoxAllRegions =
        _("Use checkbox 'all regions' to control whether this should apply to all regions.");

    actionGroup = ActionGroup::create();
#if USE_GLIB_ACTION
    actionSplitDimZone = actionGroup->add_action(
        "SplitDimZone", sigc::mem_fun(*this, &DimRegionChooser::split_dimension_zone)
    );
    actionDeleteDimZone = actionGroup->add_action(
         "DeleteDimZone", sigc::mem_fun(*this, &DimRegionChooser::delete_dimension_zone)
     );
    insert_action_group("PopupMenuInsideDimRegion", actionGroup);
#else
    actionSplitDimZone = Action::create("SplitDimZone", _("Split Dimensions Zone"), txtUseCheckBoxAllRegions);
    actionSplitDimZone->set_tooltip(txtUseCheckBoxAllRegions); //FIXME: doesn't work? why???
    actionGroup->add(
        actionSplitDimZone,
        sigc::mem_fun(*this, &DimRegionChooser::split_dimension_zone)
    );
    actionDeleteDimZone = Gtk::Action::create("DeleteDimZone", _("Delete Dimension Zone"), txtUseCheckBoxAllRegions);
    actionDeleteDimZone->set_tooltip(txtUseCheckBoxAllRegions); //FIXME: doesn't work? why???
    actionGroup->add(
        actionDeleteDimZone,
        sigc::mem_fun(*this, &DimRegionChooser::delete_dimension_zone)
    );
#endif

#if USE_GTKMM_BUILDER
    uiManager = Gtk::Builder::create();
    Glib::ustring ui_info =
        "<interface>"
        "  <menu id='menu-PopupMenuInsideDimRegion'>"
        "    <section>"
        "      <item id='item-split'>"
        "        <attribute name='label' translatable='yes'>Split Dimensions Zone</attribute>"
        "        <attribute name='action'>PopupMenuInsideDimRegion.SplitDimZone</attribute>"
        "      </item>"
        "      <item id='item-delete'>"
        "        <attribute name='label' translatable='yes'>Delete Dimension Zone</attribute>"
        "        <attribute name='action'>PopupMenuInsideDimRegion.DeleteDimZone</attribute>"
        "      </item>"
        "    </section>"
        "  </menu>"
        "</interface>";
    uiManager->add_from_string(ui_info);

    popup_menu_inside_dimregion = new Gtk::Menu(
        Glib::RefPtr<Gio::Menu>::cast_dynamic(
            uiManager->get_object("menu-PopupMenuInsideDimRegion")
        )
    );
#else
    uiManager = Gtk::UIManager::create();
    uiManager->insert_action_group(actionGroup);
    Glib::ustring ui_info =
        "<ui>"
        "  <popup name='PopupMenuInsideDimRegion'>"
        "    <menuitem action='SplitDimZone'/>"
        "    <menuitem action='DeleteDimZone'/>"
        "  </popup>"
//         "  <popup name='PopupMenuOutsideDimRegion'>"
//         "    <menuitem action='Add'/>"
//         "  </popup>"
        "</ui>";
    uiManager->add_ui_from_string(ui_info);

    popup_menu_inside_dimregion = dynamic_cast<Gtk::Menu*>(
        uiManager->get_widget("/PopupMenuInsideDimRegion"));
//     popup_menu_outside_dimregion = dynamic_cast<Gtk::Menu*>(
//         uiManager->get_widget("/PopupMenuOutsideDimRegion"));

#endif // USE_GTKMM_BUILDER


#if GTKMM_MAJOR_VERSION > 3 || (GTKMM_MAJOR_VERSION == 3 && GTKMM_MINOR_VERSION > 24)
# warning GTKMM4 event registration code missing for dimregionchooser!
    //add_events(Gdk::EventMask::BUTTON_PRESS_MASK);
#else
    add_events(Gdk::BUTTON_PRESS_MASK | Gdk::POINTER_MOTION_MASK |
               Gdk::POINTER_MOTION_HINT_MASK);
#endif

    labels_changed = true;

    set_tooltip_text(_(
        "Right click here for options on altering dimension zones. Press and "
        "hold CTRL key for selecting multiple dimension zones simultaniously."
    ));

    Settings::singleton()->showTooltips.get_proxy().signal_changed().connect(
        sigc::mem_fun(*this, &DimRegionChooser::on_show_tooltips_changed)
    );
    on_show_tooltips_changed();
    
    window.signal_key_press_event().connect(
        sigc::mem_fun(*this, &DimRegionChooser::onKeyPressed)
    );
    window.signal_key_release_event().connect(
        sigc::mem_fun(*this, &DimRegionChooser::onKeyReleased)
    );
}

DimRegionChooser::~DimRegionChooser()
{
}

void DimRegionChooser::on_show_tooltips_changed() {
    const bool b = Settings::singleton()->showTooltips;

    set_has_tooltip(b);
}

void DimRegionChooser::setModifyBothChannels(bool b) {
    modifybothchannels = b;
    // redraw required parts
    queue_draw();
}

void DimRegionChooser::setModifyAllDimensionRegions(bool b) {
    modifyalldimregs = b;
    // redraw required parts
    queue_draw();
}

void DimRegionChooser::setModifyAllRegions(bool b) {
    modifyallregions = b;

#if USE_GTKMM_BUILDER
    auto menuItemSplit = Glib::RefPtr<Gio::MenuItem>::cast_dynamic(
        uiManager->get_object("item-split")
    );
    auto menuItemDelete = Glib::RefPtr<Gio::MenuItem>::cast_dynamic(
        uiManager->get_object("item-delete")
    );
    menuItemDelete->set_label(b ? _("Delete Dimension Zone [ALL REGIONS]") : _("Delete Dimension Zone"));
    menuItemSplit->set_label(b ? _("Split Dimensions Zone [ALL REGIONS]") : _("Split Dimensions Zone"));
#else
    actionDeleteDimZone->set_label(b ? _("Delete Dimension Zone [ALL REGIONS]") : _("Delete Dimension Zone"));
    actionSplitDimZone->set_label(b ? _("Split Dimensions Zone [ALL REGIONS]") : _("Split Dimensions Zone"));
#endif

    // redraw required parts
    queue_draw();
}

void DimRegionChooser::drawIconsFor(
    gig::dimension_t dimension, uint zone,
    const Cairo::RefPtr<Cairo::Context>& cr,
    int x, int y, int w, int h)
{
    DimensionCase dimCase;
    dimCase[dimension] = zone;

    std::vector<gig::DimensionRegion*> dimregs =
        dimensionRegionsMatching(dimCase, region, true);

    if (dimregs.empty()) return;

    int iSampleRefs = 0;
    int iLoops = 0;

    for (uint i = 0; i < dimregs.size(); ++i) {
        if (dimregs[i]->pSample) iSampleRefs++;
        if (dimregs[i]->SampleLoops) iLoops++;
    }

    bool bShowLoopSymbol = (iLoops > 0);
    bool bShowSampleRefSymbol = (iSampleRefs < dimregs.size());

    if (bShowLoopSymbol || bShowSampleRefSymbol) {
        const int margin = 1;

        cr->save();
        cr->set_line_width(1);
        cr->rectangle(x, y + margin, w, h - 2*margin);
        cr->clip();
        if (bShowSampleRefSymbol) {
            const int wPic = 8;
            const int hPic = 8;
            Gdk::Cairo::set_source_pixbuf(
                cr, (iSampleRefs) ? yellowDot : redDot,
                x + (w-wPic)/2.f,
                y + (
                    (bShowLoopSymbol) ? margin : (h-hPic)/2.f
                )
            );
            cr->paint();
        }
        if (bShowLoopSymbol) {
            const int wPic = 12;
            const int hPic = 14;
            Gdk::Cairo::set_source_pixbuf(
                cr, (iLoops == dimregs.size()) ? blackLoop : grayLoop,
                x + (w-wPic)/2.f,
                y + (
                    (bShowSampleRefSymbol) ? h - hPic - margin : (h-hPic)/2.f
                )
            );
            cr->paint();
        }
        cr->restore();
    }
}

#if (GTKMM_MAJOR_VERSION == 2 && GTKMM_MINOR_VERSION < 90) || GTKMM_MAJOR_VERSION < 2
bool DimRegionChooser::on_expose_event(GdkEventExpose* e)
{
    double clipx1 = e->area.x;
    double clipx2 = e->area.x + e->area.width;
    double clipy1 = e->area.y;
    double clipy2 = e->area.y + e->area.height;

    const Cairo::RefPtr<Cairo::Context>& cr =
        get_window()->create_cairo_context();
#else
bool DimRegionChooser::on_draw(const Cairo::RefPtr<Cairo::Context>& cr)
{
    double clipx1, clipx2, clipy1, clipy2;
    cr->get_clip_extents(clipx1, clipy1, clipx2, clipy2);
#endif

    if (!region) return true;

    // This is where we draw on the window
    int w = get_width();
    Glib::RefPtr<Pango::Context> context = get_pango_context();

    Glib::RefPtr<Pango::Layout> layout = Pango::Layout::create(context);
    cr->set_line_width(1);

    int y = 0;
    if (labels_changed || label_width - 10 > clipx1) {
        // draw labels on the left (reflecting the dimension type)
        double maxwidth = 0;
        for (int i = 0 ; i < region->Dimensions ; i++) {
            int nbZones = region->pDimensionDefinitions[i].zones;
            if (nbZones) {
                const char* dstr;
                char dstrbuf[10];
                switch (region->pDimensionDefinitions[i].dimension) {
                case gig::dimension_none: dstr=_("none"); break;
                case gig::dimension_samplechannel: dstr=_("samplechannel");
                    break;
                case gig::dimension_layer: dstr=_("layer"); break;
                case gig::dimension_velocity: dstr=_("velocity"); break;
                case gig::dimension_channelaftertouch:
                    dstr=_("channelaftertouch"); break;
                case gig::dimension_releasetrigger:
                    dstr=_("releasetrigger"); break;
                case gig::dimension_keyboard: dstr=_("keyswitching"); break;
                case gig::dimension_roundrobin: dstr=_("roundrobin"); break;
                case gig::dimension_random: dstr=_("random"); break;
                case gig::dimension_smartmidi: dstr=_("smartmidi"); break;
                case gig::dimension_roundrobinkeyboard:
                    dstr=_("roundrobinkeyboard"); break;
                case gig::dimension_modwheel: dstr=_("modwheel"); break;
                case gig::dimension_breath: dstr=_("breath"); break;
                case gig::dimension_foot: dstr=_("foot"); break;
                case gig::dimension_portamentotime:
                    dstr=_("portamentotime"); break;
                case gig::dimension_effect1: dstr=_("effect1"); break;
                case gig::dimension_effect2: dstr=_("effect2"); break;
                case gig::dimension_genpurpose1: dstr=_("genpurpose1"); break;
                case gig::dimension_genpurpose2: dstr=_("genpurpose2"); break;
                case gig::dimension_genpurpose3: dstr=_("genpurpose3"); break;
                case gig::dimension_genpurpose4: dstr=_("genpurpose4"); break;
                case gig::dimension_sustainpedal:
                    dstr=_("sustainpedal"); break;
                case gig::dimension_portamento: dstr=_("portamento"); break;
                case gig::dimension_sostenutopedal:
                    dstr=_("sostenutopedal"); break;
                case gig::dimension_softpedal: dstr=_("softpedal"); break;
                case gig::dimension_genpurpose5: dstr=_("genpurpose5"); break;
                case gig::dimension_genpurpose6: dstr=_("genpurpose6"); break;
                case gig::dimension_genpurpose7: dstr=_("genpurpose7"); break;
                case gig::dimension_genpurpose8: dstr=_("genpurpose8"); break;
                case gig::dimension_effect1depth:
                    dstr=_("effect1depth"); break;
                case gig::dimension_effect2depth:
                    dstr=_("effect2depth"); break;
                case gig::dimension_effect3depth:
                    dstr=_("effect3depth"); break;
                case gig::dimension_effect4depth:
                    dstr=_("effect4depth"); break;
                case gig::dimension_effect5depth:
                    dstr=_("effect5depth"); break;
                default:
                    sprintf(dstrbuf, "%d",
                            region->pDimensionDefinitions[i].dimension);
                    dstr = dstrbuf;
                    break;
                }

                // Since bold font yields in larger label width, we first always
                // set the bold text variant, retrieve its dimensions (as worst
                // case dimensions of the label) ...
                layout->set_markup("<b>" + Glib::ustring(dstr) + "</b>");
                Pango::Rectangle rectangle = layout->get_logical_extents();
                // ... and then reset the label to regular font style in case
                // the line is not selected. Otherwise the right hand side
                // actual dimension zones would jump around on selection change.
                bool isSelectedLine = (focus_line == i);
                if (!isSelectedLine)
                    layout->set_markup(dstr);

                double text_w = double(rectangle.get_width()) / Pango::SCALE;
                if (text_w > maxwidth) maxwidth = text_w;

                if (y + h > clipy1 && y < clipy2 && text_w >= clipx1) {
                    double text_h = double(rectangle.get_height()) /
                        Pango::SCALE;
#if (GTKMM_MAJOR_VERSION == 2 && GTKMM_MINOR_VERSION < 90) || GTKMM_MAJOR_VERSION < 2
                    const Gdk::Color fg = get_style()->get_fg(get_state());
#else
                    const Gdk::RGBA fg =
# if GTKMM_MAJOR_VERSION >= 3
                        get_style_context()->get_color();
# else
                        get_style_context()->get_color(get_state_flags());
# endif
#endif
                    Gdk::Cairo::set_source_rgba(cr, fg);
                    cr->move_to(4, int(y + (h - text_h) / 2 + 0.5));
#if (GTKMM_MAJOR_VERSION == 2 && GTKMM_MINOR_VERSION < 16) || GTKMM_MAJOR_VERSION < 2
                    pango_cairo_show_layout(cr->cobj(), layout->gobj());
#else
                    layout->show_in_cairo_context(cr);
#endif
                }
            }
            y += h;
        }
        label_width = int(maxwidth + 10);
        labels_changed = false;
    }
    if (label_width >= clipx2) return true;

    // draw dimensions' zones areas
    y = 0;
    int bitpos = 0;
    for (int i = 0 ; i < region->Dimensions ; i++) {
        int nbZones = region->pDimensionDefinitions[i].zones;
        if (nbZones) {
            const gig::dimension_t dimension = region->pDimensionDefinitions[i].dimension;

            if (y >= clipy2) break;
            if (y + h > clipy1) {
                // draw focus rectangle around dimension's label and zones
                if (has_focus() && focus_line == i) {
#if (GTKMM_MAJOR_VERSION == 2 && GTKMM_MINOR_VERSION < 90) || GTKMM_MAJOR_VERSION < 2
                    Gdk::Rectangle farea(0, y, 150, h);
                    get_style()->paint_focus(get_window(), get_state(), farea,
                                             *this, "",
                                             0, y, label_width, h);
#else
                    get_style_context()->render_focus(cr,
                                                      0, y, label_width, h);
#endif
                }

                // draw top and bottom lines of dimension's zones
                Gdk::Cairo::set_source_rgba(cr, black);
                cr->move_to(label_width, y + 0.5);
                cr->line_to(w, y + 0.5);
                cr->move_to(w, y + h - 0.5);
                cr->line_to(label_width, y + h - 0.5);
                cr->stroke();

                // erase whole dimension's zones area
                Gdk::Cairo::set_source_rgba(cr, white);
                cr->rectangle(label_width + 1, y + 1,
                              (w - label_width - 2), h - 2);
                cr->fill();

                int c = 0;
                if (maindimregno >= 0) {
                    int mask =
                        ~(((1 << region->pDimensionDefinitions[i].bits) - 1) <<
                          bitpos);
                    c = maindimregno & mask; // mask away this dimension
                }
                bool customsplits =
                    ((region->pDimensionDefinitions[i].split_type ==
                      gig::split_type_normal &&
                      region->pDimensionRegions[c]->DimensionUpperLimits[i]) ||
                     (region->pDimensionDefinitions[i].dimension ==
                      gig::dimension_velocity &&
                      region->pDimensionRegions[c]->VelocityUpperLimit));

                // draw dimension zones
                Gdk::Cairo::set_source_rgba(cr, black);
                if (customsplits) {
                    cr->move_to(label_width + 0.5, y + 1);
                    cr->line_to(label_width + 0.5, y + h - 1);
                    int prevX = label_width;
                    int prevUpperLimit = -1;

                    for (int j = 0 ; j < nbZones ; j++) {
                        // draw dimension zone's borders for custom splits
                        gig::DimensionRegion* d =
                            region->pDimensionRegions[c + (j << bitpos)];
                        int upperLimit = d->DimensionUpperLimits[i];
                        if (!upperLimit) upperLimit = d->VelocityUpperLimit;
                        int v = upperLimit + 1;
                        int x = int((w - label_width - 1) * v / 128.0 + 0.5) +
                            label_width;
                        if (x >= clipx2) break;
                        if (x < clipx1) continue;
                        Gdk::Cairo::set_source_rgba(cr, black);
                        cr->move_to(x + 0.5, y + 1);
                        cr->line_to(x + 0.5, y + h - 1);
                        cr->stroke();

                        // draw fill for zone
                        bool isSelectedZone = this->dimzones[dimension].count(j);
                        bool isMainSelection =
                            this->maindimcase.find(dimension) != this->maindimcase.end() &&
                            this->maindimcase[dimension] == j;
                        bool isCheckBoxSelected =
                            modifyalldimregs ||
                            (modifybothchannels &&
                                dimension == gig::dimension_samplechannel);
                        if (isMainSelection)
                            Gdk::Cairo::set_source_rgba(cr, blue);
                        else if (isSelectedZone)
                            cr->set_source(blueHatchedSurfacePattern2);
                        else if (isCheckBoxSelected)
                            cr->set_source(blueHatchedSurfacePattern);
                        else
                            Gdk::Cairo::set_source_rgba(cr, white);

                        const int wZone = x - prevX - 1;

                        cr->rectangle(prevX + 1, y + 1, wZone, h - 1);
                        cr->fill();

                        // draw icons
                        drawIconsFor(dimension, j, cr, prevX, y, wZone, h);

                        // draw text showing the beginning of the dimension zone
                        // as numeric value to the user
                        {
                            Glib::RefPtr<Pango::Layout> layout = Pango::Layout::create(context);
                            layout->set_text(Glib::Ascii::dtostr(prevUpperLimit+1));
                            Gdk::Cairo::set_source_rgba(cr, black);
                            // get the text dimensions
                            int text_width, text_height;
                            layout->get_pixel_size(text_width, text_height);
                            // move text to the left end of the dimension zone
                            cr->move_to(prevX + 3, y + (h - text_height) / 2);
#if (GTKMM_MAJOR_VERSION == 2 && GTKMM_MINOR_VERSION < 16) || GTKMM_MAJOR_VERSION < 2
                            pango_cairo_show_layout(cr->cobj(), layout->gobj());
#else
                            layout->show_in_cairo_context(cr);
#endif
                        }
                        // draw text showing the end of the dimension zone
                        // as numeric value to the user
                        {
                            Glib::RefPtr<Pango::Layout> layout = Pango::Layout::create(context);
                            layout->set_text(Glib::Ascii::dtostr(upperLimit));
                            Gdk::Cairo::set_source_rgba(cr, black);
                            // get the text dimensions
                            int text_width, text_height;
                            layout->get_pixel_size(text_width, text_height);
                            // move text to the left end of the dimension zone
                            cr->move_to(x - 3 - text_width, y + (h - text_height) / 2);
#if (GTKMM_MAJOR_VERSION == 2 && GTKMM_MINOR_VERSION < 16) || GTKMM_MAJOR_VERSION < 2
                            pango_cairo_show_layout(cr->cobj(), layout->gobj());
#else
                            layout->show_in_cairo_context(cr);
#endif
                        }

                        prevX = x;
                        prevUpperLimit = upperLimit;
                    }
                } else {
                    int prevX = 0;
                    for (int j = 0 ; j <= nbZones ; j++) {
                        // draw dimension zone's borders for normal splits
                        int x = int((w - label_width - 1) * j /
                                    double(nbZones) + 0.5) + label_width;
                        if (x >= clipx2) break;
                        if (x < clipx1) continue;
                        Gdk::Cairo::set_source_rgba(cr, black);
                        cr->move_to(x + 0.5, y + 1);
                        cr->line_to(x + 0.5, y + h - 1);
                        cr->stroke();

                        if (j != 0) {
                            const int wZone = x - prevX - 1;

                            // draw fill for zone
                            bool isSelectedZone = this->dimzones[dimension].count(j-1);
                            bool isMainSelection =
                                this->maindimcase.find(dimension) != this->maindimcase.end() &&
                                this->maindimcase[dimension] == (j-1);
                            bool isCheckBoxSelected =
                                modifyalldimregs ||
                                (modifybothchannels &&
                                    dimension == gig::dimension_samplechannel);
                            if (isMainSelection)
                                Gdk::Cairo::set_source_rgba(cr, blue);
                            else if (isSelectedZone)
                                cr->set_source(blueHatchedSurfacePattern2);
                            else if (isCheckBoxSelected)
                                cr->set_source(blueHatchedSurfacePattern);
                            else
                                Gdk::Cairo::set_source_rgba(cr, white);
                            cr->rectangle(prevX + 1, y + 1, wZone, h - 1);
                            cr->fill();

                            // draw icons
                            drawIconsFor(dimension, j - 1, cr, prevX, y, wZone, h);

                            // draw text showing the beginning of the dimension zone
                            // as numeric value to the user
                            {
                                Glib::RefPtr<Pango::Layout> layout = Pango::Layout::create(context);
                                layout->set_text(Glib::Ascii::dtostr((j-1) * 128/nbZones));
                                Gdk::Cairo::set_source_rgba(cr, black);
                                // get the text dimensions
                                int text_width, text_height;
                                layout->get_pixel_size(text_width, text_height);
                                // move text to the left end of the dimension zone
                                cr->move_to(prevX + 3, y + (h - text_height) / 2);
#if (GTKMM_MAJOR_VERSION == 2 && GTKMM_MINOR_VERSION < 16) || GTKMM_MAJOR_VERSION < 2
                                pango_cairo_show_layout(cr->cobj(), layout->gobj());
#else
                                layout->show_in_cairo_context(cr);
#endif
                            }
                            // draw text showing the end of the dimension zone
                            // as numeric value to the user
                            {
                                Glib::RefPtr<Pango::Layout> layout = Pango::Layout::create(context);
                                layout->set_text(Glib::Ascii::dtostr(j * 128/nbZones - 1));
                                Gdk::Cairo::set_source_rgba(cr, black);
                                // get the text dimensions
                                int text_width, text_height;
                                layout->get_pixel_size(text_width, text_height);
                                // move text to the left end of the dimension zone
                                cr->move_to(x - 3 - text_width, y + (h - text_height) / 2);
#if (GTKMM_MAJOR_VERSION == 2 && GTKMM_MINOR_VERSION < 16) || GTKMM_MAJOR_VERSION < 2
                                pango_cairo_show_layout(cr->cobj(), layout->gobj());
#else
                                layout->show_in_cairo_context(cr);
#endif
                            }
                        }
                        prevX = x;
                    }       
                }
            }
            y += h;
        }
        bitpos += region->pDimensionDefinitions[i].bits;
    }

    return true;
}

void DimRegionChooser::set_region(gig::Region* region)
{
    this->region = region;
    maindimregno = 0;
    nbDimensions = 0;
    if (region) {
        int bitcount = 0;
        for (int dim = 0 ; dim < region->Dimensions ; dim++) {
            if (region->pDimensionDefinitions[dim].bits == 0) continue;
            nbDimensions++;

            int z = std::min(maindimcase[region->pDimensionDefinitions[dim].dimension],
                             region->pDimensionDefinitions[dim].zones - 1);
            maindimregno |= (z << bitcount);
            bitcount += region->pDimensionDefinitions[dim].bits;
        }
    }
    dimregion_selected();
    set_size_request(800, region ? nbDimensions * h : 0);

    labels_changed = true;
    queue_resize();
    queue_draw();
}

void DimRegionChooser::refresh_all() {
    set_region(region);
}

void DimRegionChooser::get_dimregions(const gig::Region* region, bool stereo,
                                      std::set<gig::DimensionRegion*>& dimregs) const
{
    for (int iDimRgn = 0; iDimRgn < 256; ++iDimRgn) {
        gig::DimensionRegion* dimRgn = region->pDimensionRegions[iDimRgn];
        if (!dimRgn) continue;
        bool isValidZone;
        std::map<gig::dimension_t,int> dimCase = caseOfDimRegion(dimRgn, &isValidZone);
        if (!isValidZone) continue;
        for (std::map<gig::dimension_t,int>::const_iterator it = dimCase.begin();
             it != dimCase.end(); ++it)
        {
            if (stereo && it->first == gig::dimension_samplechannel) continue; // is selected

            std::map<gig::dimension_t, std::set<int> >::const_iterator itSelectedDimension =
                this->dimzones.find(it->first);
            if (itSelectedDimension != this->dimzones.end()) {
                if (itSelectedDimension->second.count(it->second))
                    continue; // is selected
                // special case: no selection of dimzone yet; assume zone 0
                // being selected in this case
                //
                // (this is more or less a workaround for a bug, that is when
                // no explicit dimregion case had been selected [ever] by user
                // by clicking on some dimregionchooser zone yet, then the
                // individual dimension entries of this->dimzones are empty)
                if (itSelectedDimension->second.empty() && it->second == 0)
                    continue; // is selected
            }

            goto notSelected;
        }

        dimregs.insert(dimRgn);

        notSelected:
        ;
    }
}

void DimRegionChooser::update_after_resize()
{
    const uint8_t upperLimit = resize.pos - 1;
    gig::Instrument* instr = (gig::Instrument*)region->GetParent();

    int bitpos = 0;
    for (int j = 0 ; j < resize.dimension ; j++) {
        bitpos += region->pDimensionDefinitions[j].bits;
    }

    const int stereobitpos =
        (modifybothchannels) ? baseBits(gig::dimension_samplechannel, region) : -1;

    // the velocity dimension must be handled differently than all other
    // dimension types, because
    // 1. it is currently the only dimension type which allows different zone
    //    sizes for different cases
    // 2. for v2 format VelocityUpperLimit has to be set, DimensionUpperLimits for v3
    if (region->pDimensionDefinitions[resize.dimension].dimension == gig::dimension_velocity) {
        int mask =
            ~(((1 << region->pDimensionDefinitions[resize.dimension].bits) - 1) << bitpos);
        int c = maindimregno & mask; // mask away this dimension

        if (region->pDimensionRegions[c]->DimensionUpperLimits[resize.dimension] == 0) {
            // the velocity dimension didn't previously have
            // custom v3 splits, so we initialize all splits with
            // default values
            int nbZones = region->pDimensionDefinitions[resize.dimension].zones;
            for (int j = 0 ; j < nbZones ; j++) {
                gig::DimensionRegion* d = region->pDimensionRegions[c + (j << bitpos)];
                d->DimensionUpperLimits[resize.dimension] = int(128.0 * (j + 1) / nbZones - 1);
            }
        }
        if (region->pDimensionRegions[c]->VelocityUpperLimit == 0) {
            // the velocity dimension didn't previously have
            // custom v2 splits, so we initialize all splits with
            // default values
            int nbZones = region->pDimensionDefinitions[resize.dimension].zones;
            for (int j = 0 ; j < nbZones ; j++) {
                gig::DimensionRegion* d = region->pDimensionRegions[c + (j << bitpos)];
                d->VelocityUpperLimit = int(128.0 * (j + 1) / nbZones - 1);
            }
        }

        int index = c + (resize.zone << bitpos);
        gig::DimensionRegion* d = region->pDimensionRegions[index];
        // update both v2 and v3 values
        d->DimensionUpperLimits[resize.dimension] = upperLimit;
        d->VelocityUpperLimit = upperLimit;
        if (modifybothchannels && stereobitpos >= 0) { // do the same for the other audio channel's dimregion ...
            gig::DimensionRegion* d = region->pDimensionRegions[index ^ (1 << stereobitpos)];
            d->DimensionUpperLimits[resize.dimension] = upperLimit;
            d->VelocityUpperLimit = upperLimit;
        }

        if (modifyalldimregs) {
            gig::Region* rgn = NULL;
            for (int key = 0; key < 128; ++key) {
                if (!instr->GetRegion(key) || instr->GetRegion(key) == rgn) continue;
                rgn = instr->GetRegion(key);
                if (!modifyallregions && rgn != region) continue; // hack to reduce overall code amount a bit
                gig::dimension_def_t* dimdef = rgn->GetDimensionDefinition(resize.dimensionDef.dimension);
                if (!dimdef) continue;
                if (dimdef->zones != resize.dimensionDef.zones) continue;
                const int iDim = getDimensionIndex(resize.dimensionDef.dimension, rgn);
                assert(iDim >= 0 && iDim < rgn->Dimensions);

                // the dimension layout might be completely different in this
                // region, so we have to recalculate bitpos etc for this region
                const int bitpos = baseBits(resize.dimensionDef.dimension, rgn);
                const int stencil = ~(((1 << dimdef->bits) - 1) << bitpos);
                const int selection = resize.zone << bitpos;

                // primitive and inefficient loop implementation, however due to
                // this circumstance the loop code is much simpler, and its lack
                // of runtime efficiency should not be notable in practice
                for (int idr = 0; idr < 256; ++idr) {
                    const int index = (idr & stencil) | selection;
                    assert(index >= 0 && index < 256);
                    gig::DimensionRegion* dr = rgn->pDimensionRegions[index];
                    if (!dr) continue;
                    dr->DimensionUpperLimits[iDim] = upperLimit;
                    d->VelocityUpperLimit = upperLimit;
                }
            }
        } else if (modifyallregions) { // implies modifyalldimregs is false ...
            // resolve the precise case we need to modify for all other regions
            DimensionCase dimCase = dimensionCaseOf(d);
            // apply the velocity upper limit change to that resolved dim case
            // of all regions ...
            gig::Region* rgn = NULL;
            for (int key = 0; key < 128; ++key) {
                if (!instr->GetRegion(key) || instr->GetRegion(key) == rgn) continue;
                rgn = instr->GetRegion(key);
                gig::dimension_def_t* dimdef = rgn->GetDimensionDefinition(resize.dimensionDef.dimension);
                if (!dimdef) continue;
                if (dimdef->zones != resize.dimensionDef.zones) continue;
                const int iDim = getDimensionIndex(resize.dimensionDef.dimension, rgn);
                assert(iDim >= 0 && iDim < rgn->Dimensions);

                std::vector<gig::DimensionRegion*> dimrgns = dimensionRegionsMatching(dimCase, rgn);
                for (int i = 0; i < dimrgns.size(); ++i) {
                    gig::DimensionRegion* dr = dimrgns[i];
                    dr->DimensionUpperLimits[iDim] = upperLimit;
                    dr->VelocityUpperLimit = upperLimit;
                }
            }
        }
    } else {
        for (int i = 0 ; i < region->DimensionRegions ; ) {
            if (region->pDimensionRegions[i]->DimensionUpperLimits[resize.dimension] == 0) {
                // the dimension didn't previously have custom
                // limits, so we have to set default limits for
                // all the dimension regions
                int nbZones = region->pDimensionDefinitions[resize.dimension].zones;

                for (int j = 0 ; j < nbZones ; j++) {
                    gig::DimensionRegion* d = region->pDimensionRegions[i + (j << bitpos)];
                    d->DimensionUpperLimits[resize.dimension] = int(128.0 * (j + 1) / nbZones - 1);
                }
            }
            int index = i + (resize.zone << bitpos);
            gig::DimensionRegion* d = region->pDimensionRegions[index];
            d->DimensionUpperLimits[resize.dimension] = upperLimit;
#if 0       // the following is currently not necessary, because ATM the gig format uses for all dimension types except of the veleocity dimension the same zone sizes for all cases
            if (modifybothchannels && stereobitpos >= 0) { // do the same for the other audio channel's dimregion ...
                gig::DimensionRegion* d = region->pDimensionRegions[index ^ (1 << stereobitpos)];
                d->DimensionUpperLimits[resize.dimension] = upperLimit;
            }
#endif
            int bitpos = 0;
            int j;
            for (j = 0 ; j < region->Dimensions ; j++) {
                if (j != resize.dimension) {
                    int maxzones = 1 << region->pDimensionDefinitions[j].bits;
                    int dimj = (i >> bitpos) & (maxzones - 1);
                    if (dimj + 1 < region->pDimensionDefinitions[j].zones) break;
                }
                bitpos += region->pDimensionDefinitions[j].bits;
            }
            if (j == region->Dimensions) break;
            i = (i & ~((1 << bitpos) - 1)) + (1 << bitpos);
        }

        if (modifyallregions) { // TODO: this code block could be merged with the similar (and more generalized) code block of the velocity dimension above
            gig::Region* rgn = NULL;
            for (int key = 0; key < 128; ++key) {
                if (!instr->GetRegion(key) || instr->GetRegion(key) == rgn) continue;
                rgn = instr->GetRegion(key);
                gig::dimension_def_t* dimdef = rgn->GetDimensionDefinition(resize.dimensionDef.dimension);
                if (!dimdef) continue;
                if (dimdef->zones != resize.dimensionDef.zones) continue;
                const int iDim = getDimensionIndex(resize.dimensionDef.dimension, rgn);
                assert(iDim >= 0 && iDim < rgn->Dimensions);

                // the dimension layout might be completely different in this
                // region, so we have to recalculate bitpos etc for this region
                const int bitpos = baseBits(resize.dimensionDef.dimension, rgn);
                const int stencil = ~(((1 << dimdef->bits) - 1) << bitpos);
                const int selection = resize.zone << bitpos;

                // this loop implementation is less efficient than the above's
                // loop implementation (which skips unnecessary dimension regions)
                // however this code is much simpler, and its lack of runtime
                // efficiency should not be notable in practice
                for (int idr = 0; idr < 256; ++idr) {
                    const int index = (idr & stencil) | selection;
                    assert(index >= 0 && index < 256);
                    gig::DimensionRegion* dr = rgn->pDimensionRegions[index];
                    if (!dr) continue;
                    dr->DimensionUpperLimits[iDim] = upperLimit;
                }
            }
        }
    }
}

bool DimRegionChooser::on_button_release_event(GdkEventButton* event)
{
    if (resize.active) {
#if (GTKMM_MAJOR_VERSION == 2 && GTKMM_MINOR_VERSION < 90) || GTKMM_MAJOR_VERSION < 2
        get_window()->pointer_ungrab(event->time);
#else
# if GTKMM_MAJOR_VERSION < 3 || (GTKMM_MAJOR_VERSION == 3 && GTKMM_MINOR_VERSION < 20)
        Glib::wrap(event->device, true)->ungrab(event->time);
# else
        Glib::wrap(event->device, true)->get_seat()->ungrab();
# endif
#endif
        resize.active = false;

        region_changed();

        if (!is_in_resize_zone(event->x, event->y) && cursor_is_resize) {
            get_window()->set_cursor();
            cursor_is_resize = false;
        }
    }
    return true;
}

bool DimRegionChooser::on_button_press_event(GdkEventButton* event)
{
    int w = get_width();
    if (region && event->y < nbDimensions * h &&
        event->x >= label_width && event->x < w) {

        if (is_in_resize_zone(event->x, event->y)) {
#if (GTKMM_MAJOR_VERSION == 2 && GTKMM_MINOR_VERSION < 90) || GTKMM_MAJOR_VERSION < 2
            get_window()->pointer_grab(false,
                                       Gdk::BUTTON_RELEASE_MASK |
                                       Gdk::POINTER_MOTION_MASK |
                                       Gdk::POINTER_MOTION_HINT_MASK,
                                       Gdk::Cursor(Gdk::SB_H_DOUBLE_ARROW),
                                       event->time);
#else
# if GTKMM_MAJOR_VERSION < 3 || (GTKMM_MAJOR_VERSION == 3 && GTKMM_MINOR_VERSION < 20)
            Glib::wrap(event->device, true)->grab(get_window(),
                                                  Gdk::OWNERSHIP_NONE,
                                                  false,
                                                  Gdk::BUTTON_RELEASE_MASK |
                                                  Gdk::POINTER_MOTION_MASK |
                                                  Gdk::POINTER_MOTION_HINT_MASK,
                                                  Gdk::Cursor::create(Gdk::SB_H_DOUBLE_ARROW),
                                                  event->time);
# else
            Glib::wrap(event->device, true)->get_seat()->grab(
                get_window(),
                Gdk::SeatCapabilities::SEAT_CAPABILITY_ALL_POINTING,
                false,
                Gdk::Cursor::create(
                    Glib::wrap(event->device, true)->get_seat()->get_display(),
                    Gdk::SB_H_DOUBLE_ARROW
                ),
                reinterpret_cast<GdkEvent*>(event)
            );
# endif
#endif
            resize.active = true;
        } else {
            int ydim = int(event->y / h);
            int dim;
            for (dim = 0 ; dim < region->Dimensions ; dim++) {
                if (region->pDimensionDefinitions[dim].bits == 0) continue;
                if (ydim == 0) break;
                ydim--;
            }
            int nbZones = region->pDimensionDefinitions[dim].zones;

            int z = -1;
            int bitpos = 0;
            for (int i = 0 ; i < dim ; i++) {
                bitpos += region->pDimensionDefinitions[i].bits;
            }

            int i = dim;
            if (maindimregno < 0) maindimregno = 0;
            int mask = ~(((1 << region->pDimensionDefinitions[i].bits) - 1) << bitpos);
            int c = this->maindimregno & mask; // mask away this dimension

            bool customsplits =
                ((region->pDimensionDefinitions[i].split_type == gig::split_type_normal &&
                  region->pDimensionRegions[c]->DimensionUpperLimits[i]) ||
                 (region->pDimensionDefinitions[i].dimension == gig::dimension_velocity &&
                  region->pDimensionRegions[c]->VelocityUpperLimit));
            if (customsplits) {
                int val = int((event->x - label_width) * 128 / (w - label_width - 1));

                if (region->pDimensionRegions[c]->DimensionUpperLimits[i]) {
                    for (z = 0 ; z < nbZones ; z++) {
                        gig::DimensionRegion* d = region->pDimensionRegions[c + (z << bitpos)];
                        if (val <= d->DimensionUpperLimits[i]) break;
                    }
                } else {
                    for (z = 0 ; z < nbZones ; z++) {
                        gig::DimensionRegion* d = region->pDimensionRegions[c + (z << bitpos)];
                        if (val <= d->VelocityUpperLimit) break;
                    }
                }
            } else {
                z = int((event->x - label_width) * nbZones / (w - label_width - 1));
            }

            printf("dim=%d z=%d dimensionsource=%d split_type=%d zones=%d zone_size=%f\n", dim, z,
                   region->pDimensionDefinitions[dim].dimension,
                   region->pDimensionDefinitions[dim].split_type,
                   region->pDimensionDefinitions[dim].zones,
                   region->pDimensionDefinitions[dim].zone_size);
            this->maindimcase[region->pDimensionDefinitions[dim].dimension] = z;
            this->maindimregno = c | (z << bitpos);
            this->maindimtype = region->pDimensionDefinitions[dim].dimension;

            if (multiSelectKeyDown) {
                if (dimzones[this->maindimtype].count(z)) {
                    if (dimzones[this->maindimtype].size() > 1) {
                        dimzones[this->maindimtype].erase(z);
                    }
                } else {
                    dimzones[this->maindimtype].insert(z);
                }
            } else {
                this->dimzones.clear();
                for (std::map<gig::dimension_t,int>::const_iterator it = this->maindimcase.begin();
                     it != this->maindimcase.end(); ++it)
                {
                    this->dimzones[it->first].insert(it->second);
                }
            }

            focus_line = dim;
            if (has_focus()) queue_draw();
            else grab_focus();
            dimregion_selected();

            if (event->button == 3) {
                printf("dimregion right click\n");
                popup_menu_inside_dimregion->popup(event->button, event->time);
            }

            queue_draw();
        }
    }
    return true;
}

bool DimRegionChooser::on_motion_notify_event(GdkEventMotion* event)
{
    Glib::RefPtr<Gdk::Window> window = get_window();
    int x, y;
#if HAS_GDKMM_SEAT
    x = event->x;
    y = event->y;
    Gdk::ModifierType state = Gdk::ModifierType(event->state);
#else
    Gdk::ModifierType state = Gdk::ModifierType(0);
    window->get_pointer(x, y, state);
#endif

    if (resize.active) {
        int w = get_width();
        int k = int((x - label_width) * 128.0 / (w - label_width - 1) + 0.5);

        if (k < resize.min) k = resize.min;
        else if (k > resize.max) k = resize.max;

        if (k < 2) k = 2; // k is upper limit + 1, upper limit 0 is forbidden

        if (k != resize.pos) {
            int prevx = int((w - label_width - 1) * resize.pos / 128.0 + 0.5) + label_width;
            int x = int((w - label_width - 1) * k / 128.0 + 0.5) + label_width;
            int y = resize.dimension * h;
            int x1, x2;
            if (k > resize.pos) {
                x1 = prevx;
                x2 = x;
            } else {
                x1 = x;
                x2 = prevx;
            }
            Gdk::Rectangle rect(x1, y + 1, x2 - x1 + 1, h - 2);

            resize.pos = k;
            update_after_resize();
            get_window()->invalidate_rect(rect, false); // not sufficient ...
            queue_draw(); // ... so do a complete redraw instead.
        }
    } else {
        if (is_in_resize_zone(x, y)) {
            if (!cursor_is_resize) {
#if (GTKMM_MAJOR_VERSION == 2 && GTKMM_MINOR_VERSION < 90) || GTKMM_MAJOR_VERSION < 2
                window->set_cursor(Gdk::Cursor(Gdk::SB_H_DOUBLE_ARROW));
#else
                window->set_cursor(
# if GTKMM_MAJOR_VERSION < 3 || (GTKMM_MAJOR_VERSION == 3 && GTKMM_MINOR_VERSION < 20)
                    Gdk::Cursor::create(Gdk::SB_H_DOUBLE_ARROW)
# else
                    Gdk::Cursor::create(
                        Glib::wrap(event->device, true)->get_seat()->get_display(),
                        Gdk::SB_H_DOUBLE_ARROW
                    )
# endif
                );
#endif
                cursor_is_resize = true;
            }
        } else if (cursor_is_resize) {
            window->set_cursor();
            cursor_is_resize = false;
        }
    }
    return true;
}

bool DimRegionChooser::is_in_resize_zone(double x, double y)
{
    int w = get_width();
    if (region && y < nbDimensions * h && x >= label_width && x < w) {
        int ydim = int(y / h);
        int dim;
        int bitpos = 0;
        for (dim = 0 ; dim < region->Dimensions ; dim++) {
            if (region->pDimensionDefinitions[dim].bits == 0) continue;
            if (ydim == 0) break;
            ydim--;
            bitpos += region->pDimensionDefinitions[dim].bits;
        }
        int nbZones = region->pDimensionDefinitions[dim].zones;

        int c = 0;
        if (maindimregno >= 0) {
            int mask = ~(((1 << region->pDimensionDefinitions[dim].bits) - 1) << bitpos);
            c = maindimregno & mask; // mask away this dimension
        }
        const bool customsplits =
            ((region->pDimensionDefinitions[dim].split_type == gig::split_type_normal &&
              region->pDimensionRegions[c]->DimensionUpperLimits[dim]) ||
             (region->pDimensionDefinitions[dim].dimension == gig::dimension_velocity &&
              region->pDimensionRegions[c]->VelocityUpperLimit));

        // dimensions of split_type_bit cannot be resized
        if (region->pDimensionDefinitions[dim].split_type != gig::split_type_bit) {
            int prev_limit = 0;
            for (int iZone = 0 ; iZone < nbZones - 1 ; iZone++) {
                gig::DimensionRegion* d = region->pDimensionRegions[c + (iZone << bitpos)];
                const int upperLimit =
                    (customsplits) ?
                        (d->DimensionUpperLimits[dim]) ?
                            d->DimensionUpperLimits[dim] : d->VelocityUpperLimit
                        : (iZone+1) * (int)region->pDimensionDefinitions[dim].zone_size - 1;
                int limit = upperLimit + 1;
                int limitx = int((w - label_width - 1) * limit / 128.0 + 0.5) + label_width;
                if (x <= limitx - 2) break;
                if (x <= limitx + 2) {
                    resize.dimension = dim;
                    resize.dimensionDef = region->pDimensionDefinitions[dim];
                    resize.zone = iZone;
                    resize.pos = limit;
                    resize.min = prev_limit;

                    int dr = (maindimregno >> bitpos) &
                        ((1 << region->pDimensionDefinitions[dim].bits) - 1);
                    resize.selected = dr == iZone ? resize.left :
                        dr == iZone + 1 ? resize.right : resize.none;

                    iZone++;
                    gig::DimensionRegion* d = region->pDimensionRegions[c + (iZone << bitpos)];

                    const int upperLimit =
                        (customsplits) ?
                            (d->DimensionUpperLimits[dim]) ?
                                d->DimensionUpperLimits[dim] : d->VelocityUpperLimit
                            : (iZone+1) * (int)region->pDimensionDefinitions[dim].zone_size - 1;

                    int limit = upperLimit + 1;
                    resize.max = limit;
                    return true;
                }
                prev_limit = limit;
            }
        }
    }
    return false;
}

sigc::signal<void>& DimRegionChooser::signal_dimregion_selected()
{
    return dimregion_selected;
}

sigc::signal<void>& DimRegionChooser::signal_region_changed()
{
    return region_changed;
}

bool DimRegionChooser::on_focus(Gtk::DirectionType direction)
{
    // TODO: check that region exists etc, that is, that it's possible
    // to set focus
    if (direction == Gtk::DIR_TAB_FORWARD ||
        direction == Gtk::DIR_DOWN) {
        if (!has_focus()) {
            focus_line = 0;
            grab_focus();
            return true;
        } else {
            if (focus_line + 1 < region->Dimensions) {
                focus_line++;
                queue_draw();
                return true;
            } else {
                return false;
            }
        }
    } else if (direction == Gtk::DIR_TAB_BACKWARD ||
               direction == Gtk::DIR_UP) {
        if (!has_focus()) {
            focus_line = region->Dimensions - 1;
            grab_focus();
            return true;
        } else {
            if (focus_line > 0) {
                focus_line--;
                queue_draw();
                return true;
            } else {
                return false;
            }
        }
    } else if (!has_focus()) {
        // TODO: check that focus_line exists
        grab_focus();
        return true;
    } else {
        // TODO: increase or decrease value
    }
    return false;
}

void DimRegionChooser::split_dimension_zone() {    
    printf("split_dimension_zone() type=%d, zone=%d\n", maindimtype, maindimcase[maindimtype]);
    try {
        if (!modifyallregions) {
            region->SplitDimensionZone(maindimtype, maindimcase[maindimtype]);
        } else {
            gig::Instrument* instr = (gig::Instrument*)region->GetParent();
            gig::dimension_def_t* pMaindimdef = region->GetDimensionDefinition(maindimtype);
            assert(pMaindimdef != NULL);
            // retain structure by value since the original region will be
            // modified in the loop below as well
            gig::dimension_def_t maindimdef = *pMaindimdef;
            std::vector<gig::Region*> ignoredAll;
            std::vector<gig::Region*> ignoredMinor;
            std::vector<gig::Region*> ignoredCritical;
            gig::Region* rgn = NULL;
            for (int key = 0; key < 128; ++key) {
                if (!instr->GetRegion(key) || instr->GetRegion(key) == rgn) continue;
                rgn = instr->GetRegion(key);

                // ignore all regions which do not exactly match the dimension
                // layout of the selected region where this operation was emitted
                gig::dimension_def_t* dimdef = rgn->GetDimensionDefinition(maindimtype);
                if (!dimdef) {
                    ignoredAll.push_back(rgn);
                    ignoredMinor.push_back(rgn);
                    continue;
                }
                if (dimdef->zones != maindimdef.zones) {
                    ignoredAll.push_back(rgn);
                    ignoredCritical.push_back(rgn);
                    continue;
                }

                rgn->SplitDimensionZone(maindimtype, maindimcase[maindimtype]);
            }
            if (!ignoredAll.empty()) {
                Glib::ustring txt;
                if (ignoredCritical.empty())
                    txt = ToString(ignoredMinor.size()) + _(" regions have been ignored since they don't have that dimension type.");
                else if (ignoredMinor.empty())
                    txt = ToString(ignoredCritical.size()) + _(" regions have been ignored due to different amount of dimension zones!");
                else
                    txt = ToString(ignoredCritical.size()) + _(" regions have been ignored due to different amount of dimension zones (and ") +
                    ToString(ignoredMinor.size()) + _(" regions have been ignored since they don't have that dimension type)!");
                Gtk::MessageType type = (ignoredCritical.empty()) ? Gtk::MESSAGE_INFO : Gtk::MESSAGE_WARNING;
                Gtk::MessageDialog msg(txt, false, type);
                msg.run();
            }
        }
    } catch (RIFF::Exception e) {
        Gtk::MessageDialog msg(e.Message, false, Gtk::MESSAGE_ERROR);
        msg.run();
    } catch (...) {
        Glib::ustring txt = _("An unknown exception occurred!");
        Gtk::MessageDialog msg(txt, false, Gtk::MESSAGE_ERROR);
        msg.run();
    }
    refresh_all();
}

void DimRegionChooser::delete_dimension_zone() {
    printf("delete_dimension_zone() type=%d, zone=%d\n", maindimtype, maindimcase[maindimtype]);
    try {
        if (!modifyallregions) {
            region->DeleteDimensionZone(maindimtype, maindimcase[maindimtype]);
        } else {
            gig::Instrument* instr = (gig::Instrument*)region->GetParent();
            gig::dimension_def_t* pMaindimdef = region->GetDimensionDefinition(maindimtype);
            assert(pMaindimdef != NULL);
            // retain structure by value since the original region will be
            // modified in the loop below as well
            gig::dimension_def_t maindimdef = *pMaindimdef;
            std::vector<gig::Region*> ignoredAll;
            std::vector<gig::Region*> ignoredMinor;
            std::vector<gig::Region*> ignoredCritical;
            gig::Region* rgn = NULL;
            for (int key = 0; key < 128; ++key) {
                if (!instr->GetRegion(key) || instr->GetRegion(key) == rgn) continue;
                rgn = instr->GetRegion(key);

                // ignore all regions which do not exactly match the dimension
                // layout of the selected region where this operation was emitted
                gig::dimension_def_t* dimdef = rgn->GetDimensionDefinition(maindimtype);
                if (!dimdef) {
                    ignoredAll.push_back(rgn);
                    ignoredMinor.push_back(rgn);
                    continue;
                }
                if (dimdef->zones != maindimdef.zones) {
                    ignoredAll.push_back(rgn);
                    ignoredCritical.push_back(rgn);
                    continue;
                }

                rgn->DeleteDimensionZone(maindimtype, maindimcase[maindimtype]);
            }
            if (!ignoredAll.empty()) {
                Glib::ustring txt;
                if (ignoredCritical.empty())
                    txt = ToString(ignoredMinor.size()) + _(" regions have been ignored since they don't have that dimension type.");
                else if (ignoredMinor.empty())
                    txt = ToString(ignoredCritical.size()) + _(" regions have been ignored due to different amount of dimension zones!");
                else
                    txt = ToString(ignoredCritical.size()) + _(" regions have been ignored due to different amount of dimension zones (and ") +
                          ToString(ignoredMinor.size()) + _(" regions have been ignored since they don't have that dimension type)!");
                Gtk::MessageType type = (ignoredCritical.empty()) ? Gtk::MESSAGE_INFO : Gtk::MESSAGE_WARNING;
                Gtk::MessageDialog msg(txt, false, type);
                msg.run();
            }
        }
    } catch (RIFF::Exception e) {
        Gtk::MessageDialog msg(e.Message, false, Gtk::MESSAGE_ERROR);
        msg.run();
    } catch (...) {
        Glib::ustring txt = _("An unknown exception occurred!");
        Gtk::MessageDialog msg(txt, false, Gtk::MESSAGE_ERROR);
        msg.run();
    }
    refresh_all();
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
bool DimRegionChooser::onKeyPressed(Gdk::EventKey& _key) {
    GdkEventKey* key = _key.gobj();
#else
bool DimRegionChooser::onKeyPressed(GdkEventKey* key) {
#endif
    //printf("key down 0x%x\n", key->keyval);
    if (key->keyval == GDK_KEY_Control_L || key->keyval == GDK_KEY_Control_R)
        multiSelectKeyDown = true;
    if (key->keyval == primaryKeyL || key->keyval == primaryKeyR)
        primaryKeyDown = true;
    if (key->keyval == GDK_KEY_Shift_L || key->keyval == GDK_KEY_Shift_R)
        shiftKeyDown = true;

    //FIXME: hmm, for some reason GDKMM does not fire arrow key down events, so we are doing those handlers in the key up handler instead for now
    /*if (key->keyval == GDK_KEY_Left)
        select_prev_dimzone();
    if (key->keyval == GDK_KEY_Right)
        select_next_dimzone();
    if (key->keyval == GDK_KEY_Up)
        select_prev_dimension();
    if (key->keyval == GDK_KEY_Down)
        select_next_dimension();*/
    return false;
}

#if GTKMM_MAJOR_VERSION > 3 || (GTKMM_MAJOR_VERSION == 3 && (GTKMM_MINOR_VERSION > 91 || (GTKMM_MINOR_VERSION == 91 && GTKMM_MICRO_VERSION >= 2))) // GTKMM >= 3.91.2
bool DimRegionChooser::onKeyReleased(Gdk::EventKey& _key) {
    GdkEventKey* key = _key.gobj();
#else
bool DimRegionChooser::onKeyReleased(GdkEventKey* key) {
#endif
    //printf("key up 0x%x\n", key->keyval);
    if (key->keyval == GDK_KEY_Control_L || key->keyval == GDK_KEY_Control_R)
        multiSelectKeyDown = false;
    if (key->keyval == primaryKeyL || key->keyval == primaryKeyR)
        primaryKeyDown = false;
    if (key->keyval == GDK_KEY_Shift_L || key->keyval == GDK_KEY_Shift_R)
        shiftKeyDown = false;

    if (!has_focus()) return false;

    // avoid conflict with Ctrl+Left and Ctrl+Right accelerators on mainwindow
    // (which is supposed to switch between regions)
    if (primaryKeyDown) return false;

    // avoid conflict with Alt+Shift+Left and Alt+Shift+Right accelerators on
    // mainwindow
    if (shiftKeyDown) return false;

    if (key->keyval == GDK_KEY_Left)
        select_prev_dimzone();
    if (key->keyval == GDK_KEY_Right)
        select_next_dimzone();
    if (key->keyval == GDK_KEY_Up)
        select_prev_dimension();
    if (key->keyval == GDK_KEY_Down)
        select_next_dimension();

    return false;
}

void DimRegionChooser::resetSelectedZones() {
    this->dimzones.clear();
    if (!region) {
        queue_draw(); // redraw required parts
        return;
    }
    if (maindimregno < 0 || maindimregno >= region->DimensionRegions) {
        queue_draw(); // redraw required parts
        return;
    }
    if (!region->pDimensionRegions[maindimregno]) {
        queue_draw(); // redraw required parts
        return;
    }
    gig::DimensionRegion* dimrgn = region->pDimensionRegions[maindimregno];

    bool isValidZone;
    this->maindimcase = dimensionCaseOf(dimrgn);

    for (std::map<gig::dimension_t,int>::const_iterator it = this->maindimcase.begin();
         it != this->maindimcase.end(); ++it)
    {
        this->dimzones[it->first].insert(it->second);
    }

    // redraw required parts
    queue_draw();
}

bool DimRegionChooser::select_dimregion(gig::DimensionRegion* dimrgn) {
    if (!region) return false; //.selection failed

    for (int dr = 0; dr < region->DimensionRegions && region->pDimensionRegions[dr]; ++dr) {
        if (region->pDimensionRegions[dr] == dimrgn) {
            // reset dim region zone selection to the requested specific dim region case
            maindimregno = dr;
            resetSelectedZones();

            // emit signal that dimregion selection has changed, for external entities
            dimregion_selected();

            return true; // selection success
        }
    }

    return false; //.selection failed
}

void DimRegionChooser::select_next_dimzone(bool add) {
    select_dimzone_by_dir(+1, add);
}

void DimRegionChooser::select_prev_dimzone(bool add) {
    select_dimzone_by_dir(-1, add);
}

void DimRegionChooser::select_dimzone_by_dir(int dir, bool add) {
    if (!region) return;
    if (!region->Dimensions) return;
    if (focus_line < 0) focus_line = 0;
    if (focus_line >= region->Dimensions) focus_line = region->Dimensions - 1;

    maindimtype = region->pDimensionDefinitions[focus_line].dimension;
    if (maindimtype == gig::dimension_none) {
        printf("maindimtype -> none\n");
        return;
    }

    // commented out: re-evaluate maindimcase, since it might not been reset from a previous instrument which causes errors if it got different dimension types
    //if (maindimcase.empty()) {
        maindimcase = dimensionCaseOf(region->pDimensionRegions[maindimregno]);
        if (maindimcase.empty()) {
            printf("caseOfDimregion(%d) -> empty\n", maindimregno);
            return;
        }
    //}

    int z = (dir > 0) ? maindimcase[maindimtype] + 1 : maindimcase[maindimtype] - 1;
    if (z < 0) z = 0;
    if (z >= region->pDimensionDefinitions[focus_line].zones)
        z = region->pDimensionDefinitions[focus_line].zones - 1;

    maindimcase[maindimtype] = z;

    ::gig::DimensionRegion* dr = dimensionRegionMatching(maindimcase, region);
    if (!dr) {
        printf("select_dimzone_by_dir(%d) -> !dr\n", dir);
        return;
    }

    maindimregno = getDimensionRegionIndex(dr);

    if (!add) {
        // reset selected dimregion zones
        dimzones.clear();
    }
    for (DimensionCase::const_iterator it = maindimcase.begin();
         it != maindimcase.end(); ++it)
    {
        dimzones[it->first].insert(it->second);
    }

    dimregion_selected();

    // disabled: would overwrite dimregno with wrong value
    //refresh_all();
    // so requesting just a raw repaint instead:
    queue_draw();
}

void DimRegionChooser::select_next_dimension() {
    if (!region) return;
    focus_line++;
    if (focus_line >= region->Dimensions)
        focus_line = region->Dimensions - 1;
    this->maindimtype = region->pDimensionDefinitions[focus_line].dimension;
    queue_draw();
}

void DimRegionChooser::select_prev_dimension() {
    if (!region) return;
    focus_line--;
    if (focus_line < 0)
        focus_line = 0;
    this->maindimtype = region->pDimensionDefinitions[focus_line].dimension;
    queue_draw();
}

gig::DimensionRegion* DimRegionChooser::get_main_dimregion() const {
    if (!region) return NULL;
    return region->pDimensionRegions[maindimregno];
}
