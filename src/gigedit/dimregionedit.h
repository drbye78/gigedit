/*                                                         -*- c++ -*-
 * Copyright (C) 2006-2019 Andreas Persson
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

#ifndef GIGEDIT_DIMREGIONEDIT_H
#define GIGEDIT_DIMREGIONEDIT_H

#ifdef LIBGIG_HEADER_FILE
# include LIBGIG_HEADER_FILE(gig.h)
#else
# include <gig.h>
#endif

#include "compat.h"

#include <cairomm/context.h>
#include <gtkmm/box.h>
#include <gtkmm/drawingarea.h>
#include <gtkmm/entry.h>
#include <gtkmm/label.h>
#include <gtkmm/notebook.h>
#if USE_GTKMM_GRID
# include <gtkmm/grid.h>
#else
# include <gtkmm/table.h>
#endif

#ifdef LIBLINUXSAMPLER_HEADER_FILE
# include LIBLINUXSAMPLER_HEADER_FILE(engines/LFO.h)
#else
# include <linuxsampler/engines/LFO.h>
#endif

#include <set>

#include "paramedit.h"
#include "global.h"
#include "ScriptPatchVars.h"
#include "wrapLabel.hh"

class VelocityCurve : public Gtk::DrawingArea {
public:
    VelocityCurve(double (gig::DimensionRegion::*getter)(uint8_t));
    void set_dim_region(gig::DimensionRegion* d) { dimreg = d; }

protected:
#if (GTKMM_MAJOR_VERSION == 2 && GTKMM_MINOR_VERSION < 90) || GTKMM_MAJOR_VERSION < 2
    bool on_expose_event(GdkEventExpose* e);
#else
    bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr);
#endif

private:
    double (gig::DimensionRegion::* const getter)(uint8_t);
    gig::DimensionRegion* dimreg;
};

class CrossfadeCurve : public Gtk::DrawingArea {
public:
    CrossfadeCurve();
    void set_dim_region(gig::DimensionRegion* d) { dimreg = d; }

protected:
#if (GTKMM_MAJOR_VERSION == 2 && GTKMM_MINOR_VERSION < 90) || GTKMM_MAJOR_VERSION < 2
    bool on_expose_event(GdkEventExpose* e);
#else
    bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr);
#endif

private:
    gig::DimensionRegion* dimreg;
    void draw_one_curve(const Cairo::RefPtr<Cairo::Context>& cr,
                        const gig::DimensionRegion* d,
                        bool sensitive);
};

class LFOGraph : public Gtk::DrawingArea {
public:
    LFOGraph();
    void set_dim_region(gig::DimensionRegion* d) { dimreg = d; }

protected:
#if (GTKMM_MAJOR_VERSION == 2 && GTKMM_MINOR_VERSION < 90) || GTKMM_MAJOR_VERSION < 2
    bool on_expose_event(GdkEventExpose* e);
#else
    bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr);
#endif

    virtual LinuxSampler::LFO::wave_t waveType() const = 0;
    virtual float frequency() const = 0;
    virtual float phase() const = 0;
    virtual uint internalDepth() const = 0;
    virtual uint controllerDepth() const = 0;
    virtual bool flipPolarity() const = 0;
    virtual bool signedRange() const = 0;
    virtual LinuxSampler::LFO::start_level_t startLevel() const = 0;
    virtual bool hasControllerAssigned() const = 0;

    gig::DimensionRegion* dimreg;
    LinuxSampler::LFO lfo;
};

class LFO1Graph : public LFOGraph {
public:
    LinuxSampler::LFO::wave_t waveType() const OVERRIDE {
        // simply assuming here libgig's and LS's enums are equally value mapped
        return (LinuxSampler::LFO::wave_t) dimreg->LFO1WaveForm;
    }
    float frequency() const OVERRIDE { return dimreg->LFO1Frequency; }
    float phase() const OVERRIDE { return dimreg->LFO1Phase; }
    uint internalDepth() const OVERRIDE {
        const gig::lfo1_ctrl_t ctrl = dimreg->LFO1Controller;
        const bool hasInternalDepth = (
            ctrl != gig::lfo1_ctrl_modwheel && ctrl != gig::lfo1_ctrl_breath
        );
        return (hasInternalDepth) ? dimreg->LFO1InternalDepth : 0;
    }
    uint controllerDepth() const OVERRIDE { return dimreg->LFO1ControlDepth; }
    bool flipPolarity() const OVERRIDE { return dimreg->LFO1FlipPhase; }
    bool signedRange() const OVERRIDE { return false; }
    virtual LinuxSampler::LFO::start_level_t startLevel() const OVERRIDE { return LinuxSampler::LFO::start_level_mid; } // see https://sourceforge.net/p/linuxsampler/mailman/linuxsampler-devel/thread/2189307.cNP0Xbctxq%40silver/#msg36774029
    bool hasControllerAssigned() const OVERRIDE { return dimreg->LFO1Controller; }
};

class LFO2Graph : public LFOGraph {
public:
    LinuxSampler::LFO::wave_t waveType() const OVERRIDE {
        // simply assuming here libgig's and LS's enums are equally value mapped
        return (LinuxSampler::LFO::wave_t) dimreg->LFO2WaveForm;
    }
    float frequency() const OVERRIDE { return dimreg->LFO2Frequency; }
    float phase() const OVERRIDE { return dimreg->LFO2Phase; }
    uint internalDepth() const OVERRIDE {
        const gig::lfo2_ctrl_t ctrl = dimreg->LFO2Controller;
        const bool hasInternalDepth = (
            ctrl != gig::lfo2_ctrl_modwheel && ctrl != gig::lfo2_ctrl_foot
        );
        return (hasInternalDepth) ? dimreg->LFO2InternalDepth : 0;
    }
    uint controllerDepth() const OVERRIDE { return dimreg->LFO2ControlDepth; }
    bool flipPolarity() const OVERRIDE { return dimreg->LFO2FlipPhase; }
    bool signedRange() const OVERRIDE { return false; }
    virtual LinuxSampler::LFO::start_level_t startLevel() const OVERRIDE { return LinuxSampler::LFO::start_level_mid; } // see https://sourceforge.net/p/linuxsampler/mailman/linuxsampler-devel/thread/2189307.cNP0Xbctxq%40silver/#msg36774029
    bool hasControllerAssigned() const OVERRIDE { return dimreg->LFO2Controller; }
};

class LFO3Graph : public LFOGraph {
public:
    LinuxSampler::LFO::wave_t waveType() const OVERRIDE {
        // simply assuming here libgig's and LS's enums are equally value mapped
        return (LinuxSampler::LFO::wave_t) dimreg->LFO3WaveForm;
    }
    float frequency() const OVERRIDE { return dimreg->LFO3Frequency; }
    float phase() const OVERRIDE { return dimreg->LFO3Phase; }
    uint internalDepth() const OVERRIDE {
        const gig::lfo3_ctrl_t ctrl = dimreg->LFO3Controller;
        const bool hasInternalDepth = (
            ctrl != gig::lfo3_ctrl_modwheel && ctrl != gig::lfo3_ctrl_aftertouch
        );
        return (hasInternalDepth) ? dimreg->LFO3InternalDepth : 0;
    }
    uint controllerDepth() const OVERRIDE { return dimreg->LFO3ControlDepth; }
    bool flipPolarity() const OVERRIDE { return dimreg->LFO3FlipPhase; }
    bool signedRange() const OVERRIDE { return true; }
    virtual LinuxSampler::LFO::start_level_t startLevel() const OVERRIDE { return LinuxSampler::LFO::start_level_max; } // see https://sourceforge.net/p/linuxsampler/mailman/linuxsampler-devel/thread/2189307.cNP0Xbctxq%40silver/#msg36774029
    bool hasControllerAssigned() const OVERRIDE { return dimreg->LFO3Controller; }
};

class EGStateOptions : public HBox {
public:
    Gtk::Label label;
    BoolBox checkBoxAttack;
    BoolBox checkBoxAttackHold;
    BoolBox checkBoxDecay1;
    BoolBox checkBoxDecay2;
    BoolBox checkBoxRelease;

    EGStateOptions();
    void on_show_tooltips_changed();
};

class DimRegionEdit : public Gtk::Notebook
{
public:
    DimRegionEdit();
    virtual ~DimRegionEdit();
    void set_dim_region(gig::DimensionRegion* d);
    bool set_sample(gig::Sample* sample, bool copy_sample_unity, bool copy_sample_tune, bool copy_sample_loop);
    bool set_sample(gig::DimensionRegion* dimreg, gig::Sample* sample, bool copy_sample_unity, bool copy_sample_tune, bool copy_sample_loop);
    Gtk::Entry* wSample;
    Gtk::Button* buttonNullSampleReference;
    sigc::signal<void, gig::DimensionRegion*>& signal_dimreg_to_be_changed();
    sigc::signal<void, gig::DimensionRegion*>& signal_dimreg_changed();
    sigc::signal<void, gig::Sample*/*old*/, gig::Sample*/*new*/>& signal_sample_ref_changed();
    sigc::signal<void, gig::Sample*>& signal_select_sample();

    std::set<gig::DimensionRegion*> dimregs;

    ScriptPatchVars scriptVars;
    Gtk::Button editScriptSlotsButton;

protected:
    sigc::signal<void, gig::DimensionRegion*> dimreg_to_be_changed_signal;
    sigc::signal<void, gig::DimensionRegion*> dimreg_changed_signal;
    sigc::signal<void, gig::Sample*/*old*/, gig::Sample*/*new*/> sample_ref_changed_signal;
    sigc::signal<void> instrument_changed;
    sigc::signal<void, gig::Sample*> select_sample_signal;

    /**
     * Ensures that the 2 signals DimRegionEdit::dimreg_to_be_changed_signal and
     * DimRegionEdit::dimreg_changed_signal are always triggered correctly as a
     * pair. It behaves similar to a "mutex lock guard" design pattern.
     */
    class DimRegionChangeGuard : public SignalGuard<gig::DimensionRegion*> {
    public:
        DimRegionChangeGuard(DimRegionEdit* edit, gig::DimensionRegion* pDimReg) :
            SignalGuard<gig::DimensionRegion*>(edit->dimreg_to_be_changed_signal, edit->dimreg_changed_signal, pDimReg)
        {
        }
    };

    gig::DimensionRegion* dimregion;

#ifdef OLD_TOOLTIPS
    Gtk::Tooltips tooltips;
#endif

    static const int tableSize = 10;
#if USE_GTKMM_GRID
    Gtk::Grid* table[tableSize];
#else
    Gtk::Table* table[tableSize];
#endif

    Gtk::Label* lSample;

    VelocityCurve velocity_curve;
    VelocityCurve release_curve;
    VelocityCurve cutoff_curve;
    CrossfadeCurve crossfade_curve;
    LFO1Graph lfo1Graph; ///< Graphic of Amplitude (Volume) LFO waveform.
    LFO2Graph lfo2Graph; ///< Graphic of Filter Cutoff LFO waveform.
    LFO3Graph lfo3Graph; ///< Graphic of Pitch LFO waveform.

    NumEntryPermille eEG1PreAttack;
    NumEntryTemp<double> eEG1Attack;
    NumEntryTemp<double> eEG1Decay1;
    NumEntryTemp<double> eEG1Decay2;
    BoolEntry eEG1InfiniteSustain;
    NumEntryPermille eEG1Sustain;
    NumEntryTemp<double> eEG1Release;
    BoolEntry eEG1Hold;
    ChoiceEntryLeverageCtrl eEG1Controller;
    BoolEntry eEG1ControllerInvert;
    NumEntryTemp<uint8_t> eEG1ControllerAttackInfluence;
    NumEntryTemp<uint8_t> eEG1ControllerDecayInfluence;
    NumEntryTemp<uint8_t> eEG1ControllerReleaseInfluence;
    EGStateOptions eEG1StateOptions;
    ChoiceEntryLfoWave eLFO1Wave;
    NumEntryTemp<double> eLFO1Frequency;
    NumEntryTemp<double> eLFO1Phase;
    NumEntryTemp<uint16_t> eLFO1InternalDepth;
    NumEntryTemp<uint16_t> eLFO1ControlDepth;
    ChoiceEntry<gig::lfo1_ctrl_t> eLFO1Controller;
    BoolEntry eLFO1FlipPhase;
    BoolEntry eLFO1Sync;
    NumEntryPermille eEG2PreAttack;
    NumEntryTemp<double> eEG2Attack;
    NumEntryTemp<double> eEG2Decay1;
    NumEntryTemp<double> eEG2Decay2;
    BoolEntry eEG2InfiniteSustain;
    NumEntryPermille eEG2Sustain;
    NumEntryTemp<double> eEG2Release;
    ChoiceEntryLeverageCtrl eEG2Controller;
    BoolEntry eEG2ControllerInvert;
    NumEntryTemp<uint8_t> eEG2ControllerAttackInfluence;
    NumEntryTemp<uint8_t> eEG2ControllerDecayInfluence;
    NumEntryTemp<uint8_t> eEG2ControllerReleaseInfluence;
    EGStateOptions eEG2StateOptions;
    ChoiceEntryLfoWave eLFO2Wave;
    NumEntryTemp<double> eLFO2Frequency;
    NumEntryTemp<double> eLFO2Phase;
    NumEntryTemp<uint16_t> eLFO2InternalDepth;
    NumEntryTemp<uint16_t> eLFO2ControlDepth;
    ChoiceEntry<gig::lfo2_ctrl_t> eLFO2Controller;
    BoolEntry eLFO2FlipPhase;
    BoolEntry eLFO2Sync;
    NumEntryTemp<double> eEG3Attack;
    NumEntryTemp<int16_t> eEG3Depth;
    ChoiceEntryLfoWave eLFO3Wave;
    NumEntryTemp<double> eLFO3Frequency;
    NumEntryTemp<double> eLFO3Phase;
    NumEntryTemp<int16_t> eLFO3InternalDepth;
    NumEntryTemp<int16_t> eLFO3ControlDepth;
    ChoiceEntry<gig::lfo3_ctrl_t> eLFO3Controller;
    BoolEntry eLFO3FlipPhase;
    BoolEntry eLFO3Sync;
    BoolEntry eVCFEnabled;
    ChoiceEntry<gig::vcf_type_t> eVCFType;
    ChoiceEntry<gig::vcf_cutoff_ctrl_t> eVCFCutoffController;
    BoolEntry eVCFCutoffControllerInvert;
    NumEntryTemp<uint8_t> eVCFCutoff;
    ChoiceEntry<gig::curve_type_t> eVCFVelocityCurve;
    NumEntryTemp<uint8_t> eVCFVelocityScale;
    NumEntryTemp<uint8_t> eVCFVelocityDynamicRange;
    NumEntryTemp<uint8_t> eVCFResonance;
    BoolEntry eVCFResonanceDynamic;
    ChoiceEntry<gig::vcf_res_ctrl_t> eVCFResonanceController;
    BoolEntry eVCFKeyboardTracking;
    NumEntryTemp<uint8_t> eVCFKeyboardTrackingBreakpoint;
    ChoiceEntry<gig::curve_type_t> eVelocityResponseCurve;
    NumEntryTemp<uint8_t> eVelocityResponseDepth;
    NumEntryTemp<uint8_t> eVelocityResponseCurveScaling;
    ChoiceEntry<gig::curve_type_t> eReleaseVelocityResponseCurve;
    NumEntryTemp<uint8_t> eReleaseVelocityResponseDepth;
    NumEntryTemp<uint8_t> eReleaseTriggerDecay;
    NumEntryTemp<uint8_t> eCrossfade_in_start;
    NumEntryTemp<uint8_t> eCrossfade_in_end;
    NumEntryTemp<uint8_t> eCrossfade_out_start;
    NumEntryTemp<uint8_t> eCrossfade_out_end;
    BoolEntry ePitchTrack;
    ChoiceEntry<gig::sust_rel_trg_t> eSustainReleaseTrigger;
    BoolEntry eNoNoteOffReleaseTrigger;
    ChoiceEntry<gig::dim_bypass_ctrl_t> eDimensionBypass;
    NumEntryTemp<int8_t> ePan;
    BoolEntry eSelfMask;
    ChoiceEntryLeverageCtrl eAttenuationController;
    BoolEntry eInvertAttenuationController;
    NumEntryTemp<uint8_t> eAttenuationControllerThreshold;
    NumEntryTemp<uint8_t> eChannelOffset;
    BoolEntry eSustainDefeat;
    BoolEntry eMSDecode;
    NumEntryTemp<uint16_t> eSampleStartOffset;
    NoteEntry eUnityNote;
    ReadOnlyLabelWidget eSampleGroup;
    ReadOnlyLabelWidget eSampleFormatInfo;
    ReadOnlyLabelWidget eSampleID;
    ReadOnlyLabelWidget eChecksum;
    NumEntryTemp<int16_t> eFineTune;
    NumEntryGain eGain;
    BoolEntry eSampleLoopEnabled;
    NumEntryTemp<uint32_t> eSampleLoopStart;
    NumEntryTemp<uint32_t> eSampleLoopLength;
    ChoiceEntry<uint32_t> eSampleLoopType;
    BoolEntry eSampleLoopInfinite;
    NumEntryTemp<uint32_t> eSampleLoopPlayCount;
    Gtk::Label* lEG2;
    Gtk::Label* lLFO2;

    Gtk::Button buttonSelectSample;

    Gtk::HBox scriptVarsDescrBox;
#if GTKMM_MAJOR_VERSION < 3
    view::WrapLabel m_labelPatchVarsDescr;
#else
    MultiLineLabel  m_labelPatchVarsDescr;
#endif

    int rowno;
    int pageno;
    int firstRowInBlock;


    void addProp(BoolEntry& boolentry);
    void addProp(LabelWidget& labelwidget);
    void addLine(HBox& line);
    void addString(const char* labelText, Gtk::Label*& label,
                   Gtk::Entry*& widget);
    void addString(const char* labelText, Gtk::Label*& label,
                   Gtk::Entry*& widget, Gtk::Button*& button);
    Gtk::Label* addHeader(const char* text);
    void addRightHandSide(Gtk::Widget& widget);
    void nextPage();

    void VCFEnabled_toggled();
    void VCFCutoffController_changed();
    void VCFResonanceController_changed();
    void EG1InfiniteSustain_toggled();
    void EG2InfiniteSustain_toggled();
    void EG1Controller_changed();
    void EG2Controller_changed();
    void AttenuationController_changed();
    void LFO1Controller_changed();
    void LFO2Controller_changed();
    void LFO3Controller_changed();
    void crossfade1_changed();
    void crossfade2_changed();
    void crossfade3_changed();
    void crossfade4_changed();
    void update_loop_elements();
    void loop_start_changed();
    void loop_length_changed();
    void loop_infinite_toggled();
    void nullOutSampleReference();
    void on_show_tooltips_changed();

    int update_model;

    // connect a widget to a setter function in DimRegionEdit
    template<typename C, typename T>
    void connect(C& widget,
                 void (DimRegionEdit::*setter)(gig::DimensionRegion&, T)) {
        connect<C, T>(widget,
                      sigc::mem_fun(setter));
    }

    // connect a widget to a member variable in gig::DimensionRegion
    template<typename C, typename T>
    void connect(C& widget, T gig::DimensionRegion::* member) {
        connect<C, T>(widget,
                      sigc::bind(sigc::mem_fun(&DimRegionEdit::set_member<T>), member));
    }

    // connect a widget to a member of a struct member in gig::DimensionRegion
    template<typename C, typename T, typename S>
    void connect(C& widget, S gig::DimensionRegion::* member, T S::* member2) {
        connect<C, T>(widget,
                      sigc::bind(sigc::mem_fun(&DimRegionEdit::set_sub_member<T, S>), member, member2));
    }

    // connect a widget to a setter function in gig::DimensionRegion
    template<typename C, typename T>
    void connect(C& widget,
                 void (gig::DimensionRegion::*setter)(T)) {
        connect<C, T>(widget,
                      sigc::hide<0>(sigc::mem_fun(setter)));
    }

    // helper function for the connect functions above
    template<typename C, typename T>
    void connect(C& widget,
                 sigc::slot<void, DimRegionEdit&, gig::DimensionRegion&, T> setter) {
        widget.signal_value_changed().connect(
            sigc::compose(sigc::bind(sigc::mem_fun(*this, &DimRegionEdit::set_many<T>), setter),
                          sigc::mem_fun(widget, &C::get_value)));
    }

    // loop through all dimregions being edited and set a value in
    // each of them
    template<typename T>
    void set_many(T value,
                  sigc::slot<void, DimRegionEdit&, gig::DimensionRegion&, T> setter) {
        if (update_model == 0) {
            for (std::set<gig::DimensionRegion*>::iterator i = dimregs.begin() ;
                 i != dimregs.end() ; ++i)
            {
                DimRegionChangeGuard(this, *i);
                setter(*this, **i, value);
            }
        }
    }

    // set a value of a member variable in the given dimregion
    template<typename T>
    void set_member(gig::DimensionRegion& d, T value,
                    T gig::DimensionRegion::* member) {
        d.*member = value;
    }

    // set a value of a member of a struct member variable in the given dimregion
    template<typename T, typename S>
    void set_sub_member(gig::DimensionRegion& d, T value,
                        S gig::DimensionRegion::* member, T S::* member2) {
        d.*member.*member2 = value;
    }

    // setters for specific dimregion parameters

    void set_UnityNote(gig::DimensionRegion& d, uint8_t value);
    void set_FineTune(gig::DimensionRegion& d, int16_t value);
    void set_Crossfade_in_start(gig::DimensionRegion& d, uint8_t value);
    void set_Crossfade_in_end(gig::DimensionRegion& d, uint8_t value);
    void set_Crossfade_out_start(gig::DimensionRegion& d, uint8_t value);
    void set_Crossfade_out_end(gig::DimensionRegion& d, uint8_t value);
    void set_Gain(gig::DimensionRegion& d, int32_t value);
    void set_LoopEnabled(gig::DimensionRegion& d, bool value);
    void set_LoopType(gig::DimensionRegion& d, uint32_t value);
    void set_LoopStart(gig::DimensionRegion& d, uint32_t value);
    void set_LoopLength(gig::DimensionRegion& d, uint32_t value);
    void set_LoopInfinite(gig::DimensionRegion& d, bool value);
    void set_LoopPlayCount(gig::DimensionRegion& d, uint32_t value);

    void onButtonSelectSamplePressed();
};

#endif
