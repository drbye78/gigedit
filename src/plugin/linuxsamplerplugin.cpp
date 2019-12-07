/*
 * Copyright (C) 2007 - 2019 Andreas Persson
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

#include "linuxsamplerplugin.h"

#ifdef LIBLINUXSAMPLER_HEADER_FILE
# include LIBLINUXSAMPLER_HEADER_FILE(plugins/InstrumentEditorFactory.h)
# include LIBLINUXSAMPLER_HEADER_FILE(engines/Engine.h)
# include LIBLINUXSAMPLER_HEADER_FILE(engines/EngineChannel.h)
#else
# include <linuxsampler/plugins/InstrumentEditorFactory.h>
# include <linuxsampler/engines/Engine.h>
# include <linuxsampler/engines/EngineChannel.h>
#endif

#include "../gigedit/gigedit.h"
#include "../gigedit/global.h"

#include <iostream>
#ifdef SIGCPP_HEADER_FILE
# include SIGCPP_HEADER_FILE(bind.h)
#else
# include <sigc++/bind.h>
#endif
#include <glibmm/main.h>
#include <set>

REGISTER_INSTRUMENT_EDITOR(LinuxSamplerPlugin)

struct LSPluginPrivate {
    std::set<gig::Region*> debounceRegionChange;
    bool debounceRegionChangedScheduled;

    LSPluginPrivate() {
        debounceRegionChangedScheduled = false;
    }
};

LinuxSamplerPlugin::LinuxSamplerPlugin() {
    pApp = new GigEdit;
    priv = new LSPluginPrivate;
}

LinuxSamplerPlugin::~LinuxSamplerPlugin() {
    if (pApp) delete static_cast<GigEdit*>(pApp);
    if (priv) delete priv;
}

int LinuxSamplerPlugin::Main(void* pInstrument, String sTypeName, String sTypeVersion, void* /*pUserData*/) {
    return Main(pInstrument, sTypeName, sTypeVersion);
}

int LinuxSamplerPlugin::Main(void* pInstrument, String sTypeName, String sTypeVersion) {
    std::cout << "Entered Gigedit Main() loop :)\n" << std::flush;
    gig::Instrument* pGigInstr = static_cast<gig::Instrument*>(pInstrument);
    GigEdit* app = static_cast<GigEdit*>(pApp);

    // connect notification signals
    app->signal_file_structure_to_be_changed().connect(
        sigc::bind(
            sigc::mem_fun(
                *this, &LinuxSamplerPlugin::NotifyDataStructureToBeChanged
            ),
            "gig::File"
        )
    );
    app->signal_file_structure_changed().connect(
        sigc::bind(
            sigc::mem_fun(
                *this, &LinuxSamplerPlugin::NotifyDataStructureChanged
            ),
            "gig::File"
        )
    );
    app->signal_samples_to_be_removed().connect(
        sigc::mem_fun(*this, &LinuxSamplerPlugin::__onSamplesToBeRemoved)
    );
    app->signal_samples_removed().connect(
        sigc::mem_fun(*this, &LinuxSamplerPlugin::NotifySamplesRemoved)
    );
    app->signal_region_to_be_changed().connect(
        sigc::bind(
            sigc::mem_fun(
                *this, &LinuxSamplerPlugin::NotifyDataStructureToBeChanged
            ),
            "gig::Region"
        )
    );
    app->signal_region_changed().connect(
        sigc::bind(
            sigc::mem_fun(
                *this, &LinuxSamplerPlugin::NotifyDataStructureChanged
            ),
            "gig::Region"
        )
    );
    app->signal_dimreg_to_be_changed().connect(
        // not connected directly anymore ...
        /*sigc::bind(
            sigc::mem_fun(
                *this, &LinuxSamplerPlugin::NotifyDataStructureToBeChanged
            ),
            "gig::DimensionRegion"
        )*/
        // ... because we are doing some event debouncing here :
        sigc::mem_fun(*this, &LinuxSamplerPlugin::__onDimRegionToBeChanged)
    );
    app->signal_dimreg_changed().connect(
        // not connected directly anymore ...
        /*sigc::bind(
            sigc::mem_fun(
                *this, &LinuxSamplerPlugin::NotifyDataStructureChanged
            ),
            "gig::DimensionRegion"
        )*/
        // ... because we are doing some event debouncing here :
        sigc::mem_fun(*this, &LinuxSamplerPlugin::__onDimRegionChanged)
    );
    app->signal_sample_changed().connect(
        sigc::bind(
            sigc::mem_fun(
                *this, &LinuxSamplerPlugin::NotifyDataStructureChanged
            ),
            "gig::Sample"
        )
    );
    app->signal_sample_ref_changed().connect(
        sigc::mem_fun(*this, &LinuxSamplerPlugin::NotifySampleReferenceChanged)
    );

    app->signal_keyboard_key_hit().connect(
        sigc::mem_fun(*this, &LinuxSamplerPlugin::__onVirtualKeyboardKeyHit)
    );
    app->signal_keyboard_key_released().connect(
        sigc::mem_fun(*this, &LinuxSamplerPlugin::__onVirtualKeyboardKeyReleased)
    );
    app->signal_switch_sampler_instrument().connect(
        sigc::mem_fun(*this, &LinuxSamplerPlugin::__requestSamplerToSwitchInstrument)
    );
    app->signal_script_to_be_changed.connect(
        sigc::bind(
            sigc::mem_fun(
                *this, &LinuxSamplerPlugin::NotifyDataStructureToBeChanged
            ),
            "gig::Script"
        )
    );
    app->signal_script_changed.connect(
        sigc::bind(
            sigc::mem_fun(
                *this, &LinuxSamplerPlugin::NotifyDataStructureChanged
            ),
            "gig::Script"
        )
    );

    // register a timeout job to gigedit's main loop, so we can poll the
    // the sampler periodically for MIDI events (I HOPE it works on all
    // archs, because gigedit is actually running in another thread than
    // the one that is calling this timeout handler register code)
    const Glib::RefPtr<Glib::TimeoutSource> timeout_source =
        Glib::TimeoutSource::create(100); // poll every 100ms
    timeout_source->connect(
        sigc::mem_fun(*this, &LinuxSamplerPlugin::__onPollPeriod)
    );
    timeout_source->attach(Glib::MainContext::get_default());

    // run gigedit application
    return app->run(pGigInstr);
}

void LinuxSamplerPlugin::__onDimRegionToBeChanged(gig::DimensionRegion* pDimRgn) {
    // instead of sending this signal per dimregion ...
    //NotifyDataStructureToBeChanged(pDimRgn, "gig::DimensionRegion");

    // ... we are rather debouncing those dimregion to be changed events, and
    // instead only send a region to be changed event, which is much faster when
    // changing a very large amount of dimregions.
    if (!pDimRgn) return;
    gig::Region* pRegion = (gig::Region*) pDimRgn->GetParent();
    const bool bIdle = priv->debounceRegionChange.empty();
    bool bRegionLocked = priv->debounceRegionChange.count(pRegion);
    if (!bRegionLocked) {
        if (bIdle)
            printf("DimRgn change event debounce BEGIN (%p)\n", pRegion);
        priv->debounceRegionChange.insert(pRegion);
        NotifyDataStructureToBeChanged(pRegion, "gig::Region");
    }
}

void LinuxSamplerPlugin::__onDimRegionChanged(gig::DimensionRegion* pDimRgn) {
    // like above, not sending this ...
    //NotifyDataStructureChanged(pDimRgn, "gig::DimensionRegion");

    // ... but rather aggressively debounce those dim region changed events and
    // sending a debounced region changed event instead.
    if (!pDimRgn) return;
    if (!priv->debounceRegionChangedScheduled) {
        priv->debounceRegionChangedScheduled = true;
        Glib::signal_idle().connect_once(
            sigc::mem_fun(*this, &LinuxSamplerPlugin::__onDimRegionChangedDebounced),
            Glib::PRIORITY_HIGH_IDLE
        );
    }
}

void LinuxSamplerPlugin::__onDimRegionChangedDebounced() {
    // Note that we are really aggressively unlocking the region here: we are
    // not even bothering whether the amount "changed" events match with the
    // previously sent amount of "to be changed" events, because this handler
    // here is only called when the app's event loop is already idle for a
    // while, which is not the case if the app is still changing instrument
    // parameters (except if the app is i.e. currently showing an error dialog
    // to the user).
    priv->debounceRegionChangedScheduled = false;
    for (std::set<gig::Region*>::const_iterator it = priv->debounceRegionChange.begin();
         it != priv->debounceRegionChange.end(); ++it)
    {
        gig::Region* pRegion = *it;
        NotifyDataStructureChanged(pRegion, "gig::Region");
    }
    priv->debounceRegionChange.clear();
    printf("DimRgn change event debounce END\n");
}

bool LinuxSamplerPlugin::__onPollPeriod() {
    #if HAVE_LINUXSAMPLER_VIRTUAL_MIDI_DEVICE
    GigEdit* app = static_cast<GigEdit*>(pApp);
    if (!NotesChanged()) return true;
    for (int iKey = 0; iKey < 128; iKey++)
        if (NoteChanged(iKey))
            NoteIsActive(iKey) ?
                app->on_note_on_event(iKey, NoteOnVelocity(iKey)) :
                app->on_note_off_event(iKey, NoteOffVelocity(iKey));
    return true;
    #else
    return false;
    #endif
}

void LinuxSamplerPlugin::__onSamplesToBeRemoved(std::list<gig::Sample*> lSamples) {
    // we have to convert the gig::Sample* list to a void* list first
    std::set<void*> samples;
    for (
        std::list<gig::Sample*>::iterator iter = lSamples.begin();
        iter != lSamples.end(); ++iter
    ) samples.insert((void*)*iter);
    // finally send notification to sampler
    NotifySamplesToBeRemoved(samples);
}

void LinuxSamplerPlugin::__onVirtualKeyboardKeyHit(int Key, int Velocity) {
    #if HAVE_LINUXSAMPLER_VIRTUAL_MIDI_DEVICE
    SendNoteOnToSampler(Key, Velocity);
    #endif
}

void LinuxSamplerPlugin::__onVirtualKeyboardKeyReleased(int Key, int Velocity) {
    #if HAVE_LINUXSAMPLER_VIRTUAL_MIDI_DEVICE
    SendNoteOffToSampler(Key, Velocity);
    #endif
}

void LinuxSamplerPlugin::__requestSamplerToSwitchInstrument(gig::Instrument* pInstrument) {
    if (!pInstrument) return;

    LinuxSampler::EngineChannel* pEngineChannel = GetEngineChannel();
    if (!pEngineChannel) return;

    LinuxSampler::Engine* pEngine = pEngineChannel->GetEngine();
    if (!pEngine) return;

    LinuxSampler::InstrumentManager* pInstrumentManager = pEngine->GetInstrumentManager();
    if (!pInstrumentManager) return;

    gig::File* pFile = (gig::File*) pInstrument->GetParent();

    // resolve instrument's index number in its gig file
    int index = -1;
    for (int i = 0; pFile->GetInstrument(i); ++i) {
        if (pFile->GetInstrument(i) == pInstrument) {
            index = i;
            break;
        }
    }
    if (index < 0) return;

    LinuxSampler::InstrumentManager::instrument_id_t id;
    id.FileName = pFile->GetFileName();
    id.Index    = index;
    pInstrumentManager->LoadInstrumentInBackground(id, pEngineChannel);
}

bool LinuxSamplerPlugin::IsTypeSupported(String sTypeName, String sTypeVersion) {
    return sTypeName == gig::libraryName() &&
           sTypeVersion == gig::libraryVersion();
}

String LinuxSamplerPlugin::Name() {
    return "gigedit";
}

String LinuxSamplerPlugin::Version() {
    return VERSION; // gigedit's version
}

String LinuxSamplerPlugin::Description() {
    return "Gigedit is an instrument editor for gig files.";
}
