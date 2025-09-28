
#include "phaser.hpp"

#include <lv2/core/lv2.h>

using namespace calf_plugins;

// --------------------------------------------------------------------------------------------------------------------

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

static void lv2_deactivate(LV2_Handle instance)
{
    auto plugin = static_cast<phaser_audio_module*>(instance);
    plugin->deactivate();
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
        .URI = "urn:darkglass:calf-phaser",
        .instantiate = lv2_instantiate,
        .connect_port = lv2_connect_port,
        .activate = lv2_activate,
        .run = lv2_run,
        .deactivate = lv2_deactivate,
        .cleanup = lv2_cleanup,
        .extension_data = lv2_extension_data
    };

    return index == 0 ? &descriptor : nullptr;
}
 
