#include "../subghz_i.h"
#include "../views/subghz_spectrum_analyzer.h"

void subghz_scene_spectrum_analyzer_callback(SubghzCustomEvent event, void* context) {
    furi_assert(context);
    SubGhz* subghz = context;
    view_dispatcher_send_custom_event(subghz->view_dispatcher, event);
}

void subghz_scene_spectrum_analyzer_on_enter(void* context) {
    SubGhz* subghz = context;
    subghz_spectrum_analyzer_set_callback(
        subghz->subghz_spectrum_analyzer, subghz_scene_spectrum_analyzer_callback, subghz);
    view_dispatcher_switch_to_view(subghz->view_dispatcher, SubGhzViewSpectrumAnalyzer);
}

bool subghz_scene_spectrum_analyzer_on_event(void* context, SceneManagerEvent event) {
    //SubGhz* subghz = context;
    return false;
}

void subghz_scene_spectrum_analyzer_on_exit(void* context) {
    // SubGhz* subghz = context;
}
