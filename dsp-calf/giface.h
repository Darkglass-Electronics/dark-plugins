/* Calf DSP Library
 * Common plugin interface definitions (shared between LADSPA/LV2/DSSI/standalone).
 *
 * Copyright (C) 2007-2010 Krzysztof Foltman
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
#ifndef CALF_GIFACE_H
#define CALF_GIFACE_H

#include "primitives.h"
#include <complex>
#include <exception>
#include <string>
#include <vector>

namespace calf_plugins {

enum {
    MAX_SAMPLE_RUN = 256
};

/// Interface to audio processing plugins (the real things, not only metadata)
struct audio_module_iface
{
    /// Called when params are changed (before processing)
    virtual void params_changed() = 0;
    /// LADSPA-esque activate function, except it is called after ports are connected, not before
    virtual void activate() = 0;
    /// LADSPA-esque deactivate function
    virtual void deactivate() = 0;
    /// Set sample rate for the plugin
    virtual void set_sample_rate(uint32_t sr) = 0;
    /// Return the arrays of port buffer pointers
    virtual void get_port_arrays(const float *const *&ins_ptrs, float **&outs_ptrs, const float * const*&params_ptrs) = 0;
    /// Clear a part of output buffers that have 0s at mask; subdivide the buffer so that no runs > MAX_SAMPLE_RUN are fed to process function
    virtual void process_slice(uint32_t offset, uint32_t end) = 0;
    /// The audio processing loop; assumes numsamples <= MAX_SAMPLE_RUN, for larger buffers, call process_slice
    virtual void process(uint32_t offset, uint32_t numsamples) = 0;
    virtual ~audio_module_iface() {}
};

/// Empty implementations for plugin functions.
template<class Metadata>
class audio_module: public Metadata, public audio_module_iface
{
public:
    using Metadata::in_count;
    using Metadata::out_count;
    using Metadata::param_count;
    const float *ins[(Metadata::in_count != 0)  ? Metadata::in_count : 1];
    float *outs[(Metadata::out_count != 0) ? Metadata::out_count : 1];
    const float *params[Metadata::param_count];

    audio_module() {
        memset(ins, 0, sizeof(ins));
        memset(outs, 0, sizeof(outs));
        memset(params, 0, sizeof(params));
    }

    /// Called when params are changed (before processing)
    void params_changed() {}
    /// LADSPA-esque activate function, except it is called after ports are connected, not before
    void activate() {}
    /// LADSPA-esque deactivate function
    void deactivate() {}
    /// Set sample rate for the plugin
    void set_sample_rate(uint32_t sr) { }
    /// Return the array of input port pointers
    virtual void get_port_arrays(const float * const*&ins_ptrs, float **&outs_ptrs, const float * const*&params_ptrs)
    {
        ins_ptrs = ins;
        outs_ptrs = outs;
        params_ptrs = params;
    }
    /// utility function: call process, and if it returned zeros in output masks, zero out the relevant output port buffers
    void process_slice(uint32_t offset, uint32_t end)
    {
        while(offset < end)
        {
            uint32_t newend = std::min(offset + MAX_SAMPLE_RUN, end);
            process(offset, newend - offset);
            offset = newend;
        }
    }
};

};

#endif
