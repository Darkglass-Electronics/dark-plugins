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

#include "calf-dsp/audio_fx.h"
#include "calf-dsp/bypass.h"
#include "calf-dsp/giface.h"

namespace calf_plugins {

struct phaser_metadata
{
    enum { param_on, par_reset, par_freq, par_depth, par_rate, par_fb, par_stages, par_stereo, param_count };
    enum { in_count = 2, out_count = 2 };
};

/**********************************************************************
 * PHASER by Krzysztof Foltman
**********************************************************************/

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
    phaser_audio_module();
    void params_changed();
    void activate();
    void set_sample_rate(uint32_t sr);
    void deactivate();
    void process(uint32_t offset, uint32_t nsamples) override {
        left.process(outs[0] + offset, ins[0] + offset, nsamples, *params[param_on] > 0.5);
        right.process(outs[1] + offset, ins[1] + offset, nsamples, *params[param_on] > 0.5);
    }
};

}
