/*
 * This file originates from https://github.com/ninodewit/SHIRO-Plugins/blob/master/plugins/harmless/gen_exported.cpp
 * It is a Cycling '74 Max gen~ patch exported as plugin code using Max 7.2.
 * The version is important, as the license changed in 7.3 to no longer be under BSD3/ISC/MIT-style permissible
 * license but under a new commercial license which is not considered open-source.
 *
 * Modifications were made so that it no longer depends on DPF for building, instead we do raw LV2 support directly.
 * The code also was manually cleaned up and simplified, heavily reducing its size.
 * Finally a few more tweaks for mono + stereo variants and hide some parameters.
 */

/*******************************************************************************************************************
Copyright (c) 2012 Cycling '74
Copyright (C) 2015 Filipe Coelho <falktx@falktx.com>
Copyright (C) 2015 Nino de Wit <ninodig@hotmail.com>
Copyright (C) 2025 Filipe Coelho <falktx@darkglass.com>

Permission is hereby granted, free of charge, to any person obtaining a copy of this software
and associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies
or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*******************************************************************************************************************/

#include "genlib.h"
#include "genlib_ops.h"

#include <lv2/core/lv2.h>
#include <lv2/core/lv2_util.h>

#include "control-port-state-update.h"

#include <cstring>

// The State struct contains all the state and procedures for the gendsp kernel
struct State {
	Phasor __m_phasor_10;
	t_sample m_y_1;
	t_sample m_phase_7;
	t_sample m_tone_8;
	t_sample m_rate_9;
	t_sample m_depth_6;
	t_sample samples_to_seconds;
	t_sample m_shape_5;
	t_sample m_y_2;
	t_sample m_smth_3;
	t_sample m_smth_4;
	t_sample m_smth_wet;
	t_sample m_smth_depth;
	t_sample wet;
    
	const LV2_Control_Port_State_Update* controlPortStateUpdate;
    bool update_state = true;

	State(const LV2_Control_Port_State_Update* controlPortStateUpdateInit) {
		controlPortStateUpdate = controlPortStateUpdateInit;
	};

	// re-initialize all member variables;
	inline void reset(t_param __sr) {
		m_y_1 = ((int)0);
		m_y_2 = ((int)0);
		m_smth_3 = ((int)0);
		m_smth_4 = ((int)0);
		m_smth_wet = ((int)0);
		m_shape_5 = ((t_sample)0.5);
		m_depth_6 = ((int)100);
		m_phase_7 = ((int)0);
		m_tone_8 = ((int)6000);
		m_rate_9 = ((int)4);
		wet = 1;
		samples_to_seconds = (1 / __sr);
		__m_phasor_10.reset(0);
	};
	// the signal processing routine;
	inline void perform_mono(const t_sample * __in1, t_sample * __out1, int __n) {
		if (update_state && controlPortStateUpdate != NULL)
        {
            controlPortStateUpdate->update_state(controlPortStateUpdate->handle,
                                                 7,
                                                 LV2_CONTROL_PORT_STATE_INACTIVE);
            update_state = false;
        }
		t_sample expr_1090 = (((m_tone_8 * ((int)2)) * ((t_sample)3.1415926535898)) * ((t_sample)2.0833333333333e-05));
		t_sample expr_1089 = (((t_sample)0.0027777777777778) * m_phase_7);
		t_sample wrap_3 = wrap(expr_1089, ((int)0), ((int)1));
		t_sample sin_24 = sin(expr_1090);
		t_sample clamp_25 = ((sin_24 <= ((t_sample)1e-05)) ? ((t_sample)1e-05) : ((sin_24 >= ((t_sample)0.99999)) ? ((t_sample)0.99999) : sin_24));
		// the main sample loop;
		while ((__n--)) {
			const t_sample in1 = (*(__in1++));
			m_smth_depth = (m_depth_6 + (((t_sample)0.999) * (m_smth_depth - m_depth_6)));
			t_sample mul_15 = (m_smth_depth * ((t_sample)0.01));
			t_sample mul_859 = (m_smth_depth * ((t_sample)0.005));
			t_sample add_667 = (mul_859 + ((int)1));
			t_sample mix_1 = (m_shape_5 + (((t_sample)0.999) * (m_smth_4 - m_shape_5)));
			t_sample mix_2 = (wrap_3 + (((t_sample)0.999) * (m_smth_3 - wrap_3)));
			t_sample mix_wet = (wet + (((t_sample)0.999) * (m_smth_wet - wet)));
			t_sample mix_1062 = (m_y_2 + (clamp_25 * (in1 - m_y_2)));
			t_sample sub_932 = (in1 - mix_1062);
			t_sample phasor_6 = __m_phasor_10(m_rate_9, samples_to_seconds);
			t_sample triangle_7 = triangle(phasor_6, mix_1);
			t_sample mul_958 = (mix_1062 * triangle_7);
			t_sample rsub_17 = (((int)1) - triangle_7);
			t_sample mul_971 = (sub_932 * rsub_17);
			t_sample add_984 = (mul_958 + mul_971);
			t_sample mix_1129 = (in1 + (mul_15 * (add_984 - in1)));
			t_sample out1 = (mix_1129 * add_667);
			m_smth_4 = mix_1;
			m_smth_3 = mix_2;
			m_smth_wet = mix_wet;
			m_y_2 = mix_1062;
			// assign results to output buffer;
			t_sample dry = 1.f - mix_wet;
			(*(__out1++)) = out1 * mix_wet + in1 * dry;
		}
	};
	inline void perform_stereo(const t_sample ** __ins, t_sample ** __outs, int __n) {
		const t_sample * __in1 = __ins[0];
		const t_sample * __in2 = __ins[1];
		t_sample * __out1 = __outs[0];
		t_sample * __out2 = __outs[1];
		t_sample expr_1090 = (((m_tone_8 * ((int)2)) * ((t_sample)3.1415926535898)) * ((t_sample)2.0833333333333e-05));
		t_sample expr_1089 = (((t_sample)0.0027777777777778) * m_phase_7);
		t_sample wrap_3 = wrap(expr_1089, ((int)0), ((int)1));
		t_sample sin_24 = sin(expr_1090);
		t_sample clamp_25 = ((sin_24 <= ((t_sample)1e-05)) ? ((t_sample)1e-05) : ((sin_24 >= ((t_sample)0.99999)) ? ((t_sample)0.99999) : sin_24));
		// the main sample loop;
		while ((__n--)) {
			const t_sample in1 = (*(__in1++));
			const t_sample in2 = (*(__in2++));
			m_smth_depth = (m_depth_6 + (((t_sample)0.999) * (m_smth_depth - m_depth_6)));
			t_sample mul_15 = (m_smth_depth * ((t_sample)0.01));
			t_sample mul_859 = (m_smth_depth * ((t_sample)0.005));
			t_sample add_667 = (mul_859 + ((int)1));
			t_sample mix_1 = (m_shape_5 + (((t_sample)0.999) * (m_smth_4 - m_shape_5)));
			t_sample mix_2 = (wrap_3 + (((t_sample)0.999) * (m_smth_3 - wrap_3)));
			t_sample mix_wet = (wet + (((t_sample)0.999) * (m_smth_wet - wet)));
			t_sample mix_1062 = (m_y_2 + (clamp_25 * (in1 - m_y_2)));
			t_sample mix_1049 = (m_y_1 + (clamp_25 * (in2 - m_y_1)));
			t_sample sub_932 = (in1 - mix_1062);
			t_sample sub_1036 = (in2 - mix_1049);
			t_sample phasor_6 = __m_phasor_10(m_rate_9, samples_to_seconds);
			t_sample triangle_7 = triangle(phasor_6, mix_1);
			t_sample mul_958 = (mix_1062 * triangle_7);
			t_sample rsub_17 = (((int)1) - triangle_7);
			t_sample mul_971 = (sub_932 * rsub_17);
			t_sample add_984 = (mul_958 + mul_971);
			t_sample mix_1129 = (in1 + (mul_15 * (add_984 - in1)));
			t_sample out1 = (mix_1129 * add_667);
			t_sample triangle_5 = triangle((mix_2 + phasor_6), mix_1);
			t_sample mul_1023 = (mix_1049 * triangle_5);
			t_sample rsub_9 = (((int)1) - triangle_5);
			t_sample mul_1010 = (sub_1036 * rsub_9);
			t_sample add_997 = (mul_1023 + mul_1010);
			t_sample mix_1130 = (in2 + (mul_15 * (add_997 - in2)));
			t_sample out2 = (mix_1130 * add_667);
			m_smth_4 = mix_1;
			m_smth_3 = mix_2;
			m_smth_wet = mix_wet;
			m_y_2 = mix_1062;
			m_y_1 = mix_1049;
			// assign results to output buffer;
			t_sample dry = 1.f - mix_wet;
			(*(__out1++)) = out1 * mix_wet + in1 * dry;
			(*(__out2++)) = out2 * mix_wet + in2 * dry;
		}
	};
	inline void set_shape(t_param _value) {
		m_shape_5 = (_value < 0.01 ? 0.01 : (_value > 0.99 ? 0.99 : _value));
	};
	inline void set_depth(t_param _value) {
		m_depth_6 = (_value < 0 ? 0 : (_value > 100 ? 100 : _value));
	};
	inline void set_phase(t_param _value) {
		m_phase_7 = (_value < -180 ? -180 : (_value > 180 ? 180 : _value));
	};
	inline void set_tone(t_param _value) {
		m_tone_8 = (_value < 500 ? 500 : (_value > 6000 ? 6000 : _value));
	};
	inline void set_rate(t_param _value) {
		m_rate_9 = (_value < 0.1 ? 0.1 : (_value > 20 ? 20 : _value));
	};
	// lv2 specific details
	union {
		struct {
			const float* in[2];
			float* out[2];
			const float* ctrls[6];
		} ports;
		void* ptrs[11];
	} lv2 = {};
	inline void lv2_reset() {
		// memory reset
		m_y_1 = ((int)0);
		m_y_2 = ((int)0);
		__m_phasor_10.reset(0);
		// set editable to targets -> skip any smoothing
		m_smth_wet = wet;
		m_smth_depth = m_depth_6;
		m_smth_4 = m_shape_5;
	}
	inline void lv2_prerun() {
		wet = *lv2.ports.ctrls[0] > 0.5f ? 1.f : 0.f;
		set_rate(*lv2.ports.ctrls[2]);
		// map [0..10] to [0.1..0.9]
		set_shape((*lv2.ports.ctrls[3]) * 0.08f + 0.1f);
		set_depth(*lv2.ports.ctrls[4]);
		set_phase(*lv2.ports.ctrls[5]);
		// reset after parameter setting so that values jump directly to targets
		if (*lv2.ports.ctrls[1] > 0.5f)
			lv2_reset();
	}
	inline void lv2_run_mono(uint32_t nsamples) {
		lv2_prerun();
		perform_mono(lv2.ports.in[0], lv2.ports.out[0], nsamples);
	}
	inline void lv2_run_stereo(uint32_t nsamples) {
		lv2_prerun();
		perform_stereo(lv2.ports.in, lv2.ports.out, nsamples);
	}
};

// --------------------------------------------------------------------------------------------------------------------

static LV2_Handle lv2_instantiate(const LV2_Descriptor* desc, double sampleRate, const char*, const LV2_Feature* const* const features) {
    const LV2_Control_Port_State_Update* controlPortStateUpdate = NULL;
    lv2_features_query(features,
                       LV2_CONTROL_PORT_STATE_UPDATE_URI, &controlPortStateUpdate, false,
                       NULL);
	auto plugin = new State(controlPortStateUpdate);
	plugin->reset(sampleRate);
	if (std::strcmp(desc->URI, "urn:darkglass:dark-tremolo#stereo") == 0)
		plugin->m_phase_7 = 180;
	return plugin;
}

static void lv2_cleanup(LV2_Handle instance) {
	delete static_cast<State*>(instance);
}

static void lv2_connect_port_mono(LV2_Handle instance, uint32_t port, void *data) {
	uint32_t offset;
	switch (port) {
		case 0: offset = 0; break;
		case 1: offset = 1; break;
		default: offset = 2; break;
	}
	static_cast<State*>(instance)->lv2.ptrs[port + offset] = data;
}

static void lv2_connect_port_stereo(LV2_Handle instance, uint32_t port, void *data) {
	static_cast<State*>(instance)->lv2.ptrs[port] = data;
}

static void lv2_activate(LV2_Handle instance) {
	static_cast<State*>(instance)->lv2_reset();
}

static void lv2_run_mono(LV2_Handle instance, uint32_t nsamples) {
	static_cast<State*>(instance)->lv2_run_mono(nsamples);
}

static void lv2_run_stereo(LV2_Handle instance, uint32_t nsamples) {
	static_cast<State*>(instance)->lv2_run_stereo(nsamples);
}

// --------------------------------------------------------------------------------------------------------------------

LV2_SYMBOL_EXPORT
const LV2_Descriptor* lv2_descriptor(uint32_t index)
{
	static constexpr const LV2_Descriptor descriptorMono = {
		.URI = "urn:darkglass:dark-tremolo",
		.instantiate = lv2_instantiate,
		.connect_port = lv2_connect_port_mono,
		.activate = lv2_activate,
		.run = lv2_run_mono,
		.deactivate = nullptr,
		.cleanup = lv2_cleanup,
		.extension_data = nullptr
	};
	static constexpr const LV2_Descriptor descriptorStereo = {
		.URI = "urn:darkglass:dark-tremolo#stereo",
		.instantiate = lv2_instantiate,
		.connect_port = lv2_connect_port_stereo,
		.activate = lv2_activate,
		.run = lv2_run_stereo,
		.deactivate = nullptr,
		.cleanup = lv2_cleanup,
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
