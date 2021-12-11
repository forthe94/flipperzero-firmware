#include "subghz_spectrum_analyzer.h"
#include "../subghz_i.h"

#include <math.h>
#include <furi.h>
#include <furi-hal.h>
#include <input/input.h>
#include <notification/notification-messages.h>
#include <lib/subghz/protocols/subghz_protocol_princeton.h>
#include "../helpers/subghz_spectrum_analyzer_worker.h"

#include <assets_icons.h>

struct SubghzSpectrumAnalyzer {
    View* view;
    SubghzSpectrumAnalyzerWorker* worker;
    void* context;
};

typedef struct {
    uint32_t frequency;
    float rssi;
} SubghzSpectrumAnalyzerModel;

//void subghz_frequency_analyzer_set_callback(
//    SubghzFrequencyAnalyzer* subghz_frequency_analyzer,
//    SubghzFrequencyAnalyzerCallback callback,
//    void* context) {
//    furi_assert(subghz_frequency_analyzer);
//    furi_assert(callback);
//    subghz_frequency_analyzer->callback = callback;
//    subghz_frequency_analyzer->context = context;
//}

void subghz_spectrum_analyzer_draw(Canvas* canvas, SubghzSpectrumAnalyzerModel* model) {

}

bool subghz_spectrum_analyzer_input(InputEvent* event, void* context) {
    furi_assert(context);


    return true;
}

void subghz_spectrum_analyzer_enter(void* context) {
    furi_assert(context);
    SubghzSpectrumAnalyzer* instance = context;

    //Start worker
    instance->worker = subghz_spectrum_analyzer_worker_alloc();

    subghz_spectrum_analyzer_worker_start(instance->worker);

    with_view_model(
        instance->view, (SubghzSpectrumAnalyzerModel * model) {

            return true;
        });
}

void subghz_spectrum_analyzer_exit(void* context) {
    furi_assert(context);
    SubghzSpectrumAnalyzer* instance = context;

    //Stop worker
    if(subghz_spectrum_analyzer_worker_is_running(instance->worker)) {
        subghz_spectrum_analyzer_worker_stop(instance->worker);
    }
    subghz_spectrum_analyzer_worker_free(instance->worker);

    with_view_model(
        instance->view, (SubghzSpectrumAnalyzerModel * model) {
            return true;
        });
}

SubghzSpectrumAnalyzer* subghz_spectrum_analyzer_alloc() {
    SubghzSpectrumAnalyzer* instance = furi_alloc(sizeof(SubghzSpectrumAnalyzer));

    // View allocation and configuration
    instance->view = view_alloc();
    view_allocate_model(
        instance->view, ViewModelTypeLocking, sizeof(SubghzSpectrumAnalyzerModel));
    view_set_context(instance->view, instance);
    view_set_draw_callback(instance->view, (ViewDrawCallback)subghz_spectrum_analyzer_draw);
    view_set_input_callback(instance->view, subghz_spectrum_analyzer_input);
    view_set_enter_callback(instance->view, subghz_spectrum_analyzer_enter);
    view_set_exit_callback(instance->view, subghz_spectrum_analyzer_exit);

    with_view_model(
        instance->view, (SubghzSpectrumAnalyzerModel * model) {
            return true;
        });

    return instance;
}

void subghz_spectrum_analyzer_free(SubghzSpectrumAnalyzer* instance) {
    furi_assert(instance);

    view_free(instance->view);
    free(instance);
}

View* subghz_spectrum_analyzer_get_view(SubghzSpectrumAnalyzer* instance) {
    furi_assert(instance);
    return instance->view;
}
