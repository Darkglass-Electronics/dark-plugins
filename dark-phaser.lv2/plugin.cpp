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
#include <cmath>
#include <cstdio>

/**********************************************************************
 * PHASER by Krzysztof Foltman
**********************************************************************/

namespace calf_plugins {

static constexpr int kPhaserModuleDefaultStages = 6;

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
    static constexpr int par_stereo = phaser_metadata<io_count>::par_stereo;

public:
    enum { MaxStages = 12 };
    dsp::simple_phaser left;
    float x1vals[io_count][MaxStages];
    float y1vals[io_count][MaxStages];
    dsp::bypass bypass{480}; // 10ms at 48kHz
    bool reset;
    float last_r_phase = 0.5f;
    dsp::inertia<dsp::linear_ramp> fb_ramp{dsp::linear_ramp(480)}; // 10ms at 48kHz
    dsp::switcher<int> stage_switcher{2400}; // 50ms at 48kHz (25ms fade out + 25ms fade in)
    float fb_compensationgain = 1.f;
    dsp::inertia<dsp::linear_ramp> fb_compensationgain_ramp{dsp::linear_ramp(480)}; // 10ms at 48kHz

    std::conditional_t<io_count == 2, dsp::simple_phaser, unused> right;

public:
    phaser_audio_module();

    void params_changed() override {
        const auto &params = this->params;

        float rate = *params[par_rate]; // 0.01*pow(1000.0f,*params[par_rate]);
        float base_frq = *params[par_freq];
        float mod_depth = *params[par_depth];
        // map [0..10] to [0.0..0.9]
        float fb = 0.09f * (*params[par_fb]);
        if (fb > 0.45f) {
            // linearly from 0dB@0.45 to -6dB@0.9
            float fb_compensationgain_old = fb_compensationgain; 
            fb_compensationgain = std::pow(10.f, (fb - 0.45f) / 0.45f * (-0.3f)); // -0.3f = 20*(-6)
        } else {
            fb_compensationgain = 1.f;
        }
        fb_compensationgain_ramp.set_inertia(fb_compensationgain);
        int stages = (int)*params[par_stages];
        float r_phase = *params[par_stereo] * (1.f / 360.f);

        fb_ramp.set_inertia(fb);
        if (stages != stage_switcher.get_state())
            stage_switcher.set(stages);

        left.set_rate(rate);
        left.set_base_frq(base_frq);
        left.set_mod_depth(mod_depth);

        if constexpr (io_count == 2) {
            right.set_rate(rate);
            right.set_base_frq(base_frq);
            right.set_mod_depth(mod_depth);
        }

        if (reset || *params[par_reset] >= 0.5f) {
            fb_ramp.set_now(fb);
            fb_compensationgain_ramp.set_now(fb_compensationgain);
            stage_switcher.reset();
            left.reset();
            left.reset_phase(0.f);
            reset = false;
            if constexpr (io_count == 2) {
                right.reset();
                right.reset_phase(r_phase);
            }
        } else {
            if constexpr (io_count == 2) {
                if (std::fabs(r_phase - last_r_phase) > 0.0001f) {
                    right.phase = left.phase;
                    right.inc_phase(r_phase);
                    last_r_phase = r_phase;
                }
            } else {
                last_r_phase = r_phase;
            }
        }
    }

    void activate() override {
        left.reset();
        left.reset_phase(0.f);
        reset = true;

        if constexpr (io_count == 2) {
            right.reset();
            right.reset_phase(last_r_phase);
        }
    }

    void set_sample_rate(uint32_t sr) override {
        left.setup(sr);

        if constexpr (io_count == 2)
            right.setup(sr);
        
        fb_ramp.ramp.set_length(static_cast<int>(static_cast<float>(sr) * 0.01)); // 10ms
    }

    void process(uint32_t offset, uint32_t nsamples) override {
        const auto &outs = this->outs;
        const auto &ins = this->ins;
        const auto &params = this->params;

        bypass.update(*params[param_on] < 0.5f, nsamples);

        for (uint32_t i = 0; i < nsamples; ++i) {
            float current_fb = fb_ramp.get();
            float stage_switcher_amp = stage_switcher.get_ramp();
            float fb_compensationgain_current = fb_compensationgain_ramp.get();

            left.set_fb(current_fb);
            left.set_stages(stage_switcher.get_state());
            left.process(outs[0] + offset + i, ins[0] + offset + i, 1, true);
            outs[0][offset + i] *= stage_switcher_amp * fb_compensationgain_current;

            if constexpr (io_count == 2) {
                right.set_fb(current_fb);
                right.set_stages(stage_switcher.get_state());
                right.process(outs[1] + offset + i, ins[1] + offset + i, 1, true);
                outs[1][offset + i] *= stage_switcher_amp * fb_compensationgain_current;
            }
        }

        bypass.crossfade(ins, outs, io_count, offset, nsamples);

    }
};

template <>
phaser_audio_module<1>::phaser_audio_module()
    : left(kPhaserModuleDefaultStages, x1vals[0], y1vals[0])
{
    left.set_dry(1.f);
    left.set_wet(1.f);
    left.set_lfo_active(false);

    stage_switcher.set(kPhaserModuleDefaultStages);
    stage_switcher.reset();
}

template <>
phaser_audio_module<2>::phaser_audio_module()
    : left(kPhaserModuleDefaultStages, x1vals[0], y1vals[0])
    , right(kPhaserModuleDefaultStages, x1vals[1], y1vals[1])
{
    left.set_dry(1.f);
    left.set_wet(1.f);
    left.set_lfo_active(false);

    right.set_dry(1.f);
    right.set_wet(1.f);
    right.set_lfo_active(false);

    stage_switcher.set(kPhaserModuleDefaultStages);
    stage_switcher.reset();
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
