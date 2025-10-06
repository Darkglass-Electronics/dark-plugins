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

#include <cstring>

/**********************************************************************
 * PHASER by Krzysztof Foltman
**********************************************************************/

namespace calf_plugins {

struct unused
{
};

template <int io_count>
struct phaser_metadata
{
    enum { param_on, par_reset, par_freq, par_depth, par_rate, par_fb, par_stages, par_stereo, param_count };
    enum { in_count = io_count, out_count = io_count };
};

template <int io_count>
class phaser_audio_module: public audio_module<phaser_metadata<io_count>>
{
    static constexpr int param_on = phaser_metadata<io_count>::param_on;
    static constexpr int par_reset = phaser_metadata<io_count>::par_reset;
    static constexpr int par_freq = phaser_metadata<io_count>::par_freq;
    static constexpr int par_depth = phaser_metadata<io_count>::par_depth;
    static constexpr int par_rate = phaser_metadata<io_count>::par_rate;
    static constexpr int par_fb = phaser_metadata<io_count>::par_fb;
    static constexpr int par_stages = phaser_metadata<io_count>::par_stages;

public:
    enum { MaxStages = 12 };
    dsp::simple_phaser left;
    float x1vals[io_count][MaxStages];
    float y1vals[io_count][MaxStages];
    dsp::bypass bypass;
    bool reset;

    std::conditional_t<io_count == 2, dsp::simple_phaser, unused> right;

public:
    phaser_audio_module();

    void params_changed() override {
        const auto &params = this->params;

        float rate = *params[par_rate]; // 0.01*pow(1000.0f,*params[par_rate]);
        float base_frq = *params[par_freq];
        float mod_depth = *params[par_depth];
        float fb = *params[par_fb];
        int stages = (int)*params[par_stages];

        left.set_rate(rate);
        left.set_base_frq(base_frq);
        left.set_mod_depth(mod_depth);
        left.set_fb(fb);
        left.set_stages(stages);

        if constexpr (io_count == 2) {
            right.set_rate(rate);
            right.set_base_frq(base_frq);
            right.set_mod_depth(mod_depth);
            right.set_fb(fb);
            right.set_stages(stages);
        }

        if (reset || *params[par_reset] >= 0.5f) {
            left.reset_phase(0.f);
            reset = false;
            if constexpr (io_count == 2)
                right.reset_phase(0.5f);
        }
    }

    void activate() override {
        left.reset();
        left.reset_phase(0.f);
        reset = true;

        if constexpr (io_count == 2) {
            right.reset();
            right.reset_phase(0.5f);
        }
    }

    void set_sample_rate(uint32_t sr) override {
        left.setup(sr);

        if constexpr (io_count == 2)
            right.setup(sr);
    }

    void process(uint32_t offset, uint32_t nsamples) override {
        const auto &outs = this->outs;
        const auto &ins = this->ins;
        const auto &params = this->params;

        bypass.update(*params[param_on] < 0.5f, nsamples);

        left.process(outs[0] + offset, ins[0] + offset, nsamples, true);

        if constexpr (io_count == 2)
            right.process(outs[1] + offset, ins[1] + offset, nsamples, true);

        bypass.crossfade(ins, outs, io_count, offset, nsamples);

    }
};

template <>
phaser_audio_module<1>::phaser_audio_module()
    : left(MaxStages, x1vals[0], y1vals[0])
{
    left.set_dry(1.f);
    left.set_wet(1.f);
    left.set_lfo_active(false);
}

template <>
phaser_audio_module<2>::phaser_audio_module()
    : left(MaxStages, x1vals[0], y1vals[0])
    , right(MaxStages, x1vals[1], y1vals[1])
{
    left.set_dry(1.f);
    left.set_wet(1.f);
    left.set_lfo_active(false);

    right.set_dry(1.f);
    right.set_wet(1.f);
    right.set_lfo_active(false);
}

}

// --------------------------------------------------------------------------------------------------------------------

using namespace calf_plugins;

template <int io_count>
static LV2_Handle lv2_instantiate(const LV2_Descriptor*, double sampleRate, const char* uri, const LV2_Feature* const*)
{
    auto plugin = new phaser_audio_module<io_count>();
    plugin->set_sample_rate(sampleRate);
    return plugin;
}

template <int io_count>
static void lv2_cleanup(LV2_Handle instance)
{
    delete static_cast<phaser_audio_module<io_count>*>(instance);
}

template <int io_count>
static void lv2_connect_port(LV2_Handle instance, uint32_t port, void *data)
{
    auto plugin = static_cast<phaser_audio_module<io_count>*>(instance);

    if (port <= io_count) {
        plugin->ins[port] = static_cast<float*>(data);
        return;
    }
    port -= io_count;

    if (port <= io_count) {
        plugin->outs[port] = static_cast<float*>(data);
        return;
    }
    port -= io_count;

    if (port <= phaser_metadata<io_count>::param_count)
        plugin->params[port] = static_cast<float*>(data);
}

template <int io_count>
static void lv2_activate(LV2_Handle instance)
{
    auto plugin = static_cast<phaser_audio_module<io_count>*>(instance);
    plugin->activate();
}

template <int io_count>
static void lv2_run(LV2_Handle instance, uint32_t nsamples)
{
    auto plugin = static_cast<phaser_audio_module<io_count>*>(instance);
    plugin->params_changed();
    plugin->process(0, nsamples);
}

// --------------------------------------------------------------------------------------------------------------------

LV2_SYMBOL_EXPORT
const LV2_Descriptor* lv2_descriptor(uint32_t index)
{
    static constexpr const LV2_Descriptor descriptorMono = {
        .URI = "urn:darkglass:dark-phaser",
        .instantiate = lv2_instantiate<1>,
        .connect_port = lv2_connect_port<1>,
        .activate = lv2_activate<1>,
        .run = lv2_run<1>,
        .deactivate = nullptr,
        .cleanup = lv2_cleanup<1>,
        .extension_data = nullptr
    };
    static constexpr const LV2_Descriptor descriptorStereo = {
        .URI = "urn:darkglass:dark-phaser#stereo",
        .instantiate = lv2_instantiate<2>,
        .connect_port = lv2_connect_port<2>,
        .activate = lv2_activate<2>,
        .run = lv2_run<2>,
        .deactivate = nullptr,
        .cleanup = lv2_cleanup<2>,
        .extension_data = nullptr
    };

    switch (index) {
    case 0:
        return &descriptorMono;
    case 1:
        return &descriptorStereo;
    default:
        return nullptr;
    }
}
