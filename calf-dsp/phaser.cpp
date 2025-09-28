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
 * Boston, MA  02110-1301  USA
 */

#include "phaser.hpp"

namespace calf_plugins {

/**********************************************************************
 * PHASER by Krzysztof Foltman
**********************************************************************/

phaser_audio_module::phaser_audio_module()
: left(MaxStages, x1vals[0], y1vals[0])
, right(MaxStages, x1vals[1], y1vals[1])
{
    left.set_dry(1.f); right.set_dry(1.f);
    left.set_wet(1.f); right.set_wet(1.f);
    left.set_lfo_active(false); right.set_lfo_active(false);
}

void phaser_audio_module::set_sample_rate(uint32_t sr)
{
    left.setup(sr);
    right.setup(sr);
}

void phaser_audio_module::activate()
{
    left.reset();
    right.reset();
    left.reset_phase(0.f);
    right.reset_phase(0.f);
    reset = true;
}

void phaser_audio_module::deactivate()
{
}

void phaser_audio_module::params_changed()
{
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

}
