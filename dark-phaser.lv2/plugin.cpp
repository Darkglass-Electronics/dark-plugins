/*
 * This file originates from https://github.com/calf-studio-gear/calf/blob/master/src/modules_mod.cpp
 * It is the source file for the modulation effects from the Calf Studio Gear suite of plugins.
 *
 * Modifications were made so that we only include the phaser and do our own LV2 implementation.
 * Also the code was manually cleaned up and simplified, heavily reducing its size.
 */

/* Calf DSP plugin pack
 * Modulation effect plugins
 *
 * Copyright (C) 2001-2010 Krzysztof Foltman, Markus Schmidt, Thor Harald Johansen and others
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02111-1307, USA.
 */

#include "audio_fx.cpp"
#include "bypass.h"

#include <lv2/core/lv2.h>

/**********************************************************************
 * PHASER by Krzysztof Foltman
**********************************************************************/

namespace calf_plugins {

struct phaser_metadata
{
    enum { param_on, par_reset, par_freq, par_depth, par_rate, par_fb, par_stages, par_stereo, param_count };
    enum { in_count = 2, out_count = 2 };
};

class phaser_audio_module: public audio_module<phaser_metadata>
{
public:
    enum { MaxStages = 12 };
    float last_r_phase;
    dsp::simple_phaser left, right;
    float x1vals[2][MaxStages], y1vals[2][MaxStages];
    dsp::bypass bypass;
    bool reset;

public:
    phaser_audio_module()
    : left(MaxStages, x1vals[0], y1vals[0])
    , right(MaxStages, x1vals[1], y1vals[1])
    {
        left.set_dry(1.f); right.set_dry(1.f);
        left.set_wet(1.f); right.set_wet(1.f);
        left.set_lfo_active(false); right.set_lfo_active(false);
    }

    void params_changed() override {
        float rate = *params[par_rate]; // 0.01*pow(1000.0f,*params[par_rate]);
        float base_frq = *params[par_freq];
        float mod_depth = *params[par_depth];
        float fb = *params[par_fb];
        int stages = (int)*params[par_stages];
        left.set_rate(rate); right.set_rate(rate);
        left.set_base_frq(base_frq); right.set_base_frq(base_frq);
        left.set_mod_depth(mod_depth); right.set_mod_depth(mod_depth);
        left.set_fb(fb); right.set_fb(fb);
        left.set_stages(stages); right.set_stages(stages);
        float r_phase = *params[par_stereo] * (1.f / 360.f);
        if (reset || *params[par_reset] >= 0.5f) {
            left.reset_phase(0.f);
            right.reset_phase(r_phase);
            last_r_phase = r_phase;
            reset = false;
        } else {
            if (fabs(r_phase - last_r_phase) > 0.0001f) {
                right.phase = left.phase;
                right.inc_phase(r_phase);
                last_r_phase = r_phase;
            }
        }
    }

    void activate() override {
        left.reset();
        right.reset();
        left.reset_phase(0.f);
        right.reset_phase(0.f);
        reset = true;
    }

    void set_sample_rate(uint32_t sr) override {
        left.setup(sr);
        right.setup(sr);
    }

    void process(uint32_t offset, uint32_t nsamples) override {
        left.process(outs[0] + offset, ins[0] + offset, nsamples, *params[param_on] > 0.5);
        right.process(outs[1] + offset, ins[1] + offset, nsamples, *params[param_on] > 0.5);
    }
};

}

// --------------------------------------------------------------------------------------------------------------------

using namespace calf_plugins;

static LV2_Handle lv2_instantiate(const LV2_Descriptor*, double sampleRate, const char*, const LV2_Feature* const*)
{
    auto plugin = new phaser_audio_module();
    plugin->set_sample_rate(sampleRate);
    return plugin;
}

static void lv2_cleanup(LV2_Handle instance)
{
    delete static_cast<phaser_audio_module*>(instance);
}

static void lv2_connect_port(LV2_Handle instance, uint32_t port, void *data)
{
    auto plugin = static_cast<phaser_audio_module*>(instance);

    if (port <= phaser_metadata::in_count)
    {
        plugin->ins[port] = static_cast<float*>(data);
        return;
    }
    port -= phaser_metadata::in_count;

    if (port <= phaser_metadata::out_count)
    {
        plugin->outs[port] = static_cast<float*>(data);
        return;
    }
    port -= phaser_metadata::out_count;

    if (port <= phaser_metadata::param_count)
    {
        plugin->params[port] = static_cast<float*>(data);
        return;
    }
    port -= phaser_metadata::param_count;
}

static void lv2_activate(LV2_Handle instance)
{
    auto plugin = static_cast<phaser_audio_module*>(instance);
    plugin->activate();
}

static void lv2_run(LV2_Handle instance, uint32_t nsamples)
{
    auto plugin = static_cast<phaser_audio_module*>(instance);
    plugin->params_changed();
    plugin->process(0, nsamples);
}

static const void* lv2_extension_data(const char*)
{
    return nullptr;
}

// --------------------------------------------------------------------------------------------------------------------

LV2_SYMBOL_EXPORT
const LV2_Descriptor* lv2_descriptor(uint32_t index)
{
    static const LV2_Descriptor descriptor = {
        .URI = "urn:darkglass:dark-phaser",
        .instantiate = lv2_instantiate,
        .connect_port = lv2_connect_port,
        .activate = lv2_activate,
        .run = lv2_run,
        .deactivate = nullptr,
        .cleanup = lv2_cleanup,
        .extension_data = lv2_extension_data
    };

    return index == 0 ? &descriptor : nullptr;
}
 
