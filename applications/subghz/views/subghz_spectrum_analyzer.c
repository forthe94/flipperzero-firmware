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
    SubGhzSpectrumAnalyzerWorker* worker;
    SubghzSpectrumAnalyzerCallback callback;
    void* context;
};

typedef struct {
    uint32_t frequency;
    float rssi;
    FrequencyRSSI rssi_buf[DOTS_COUNT];

} SubghzSpectrumAnalyzerModel;

void subghz_spectrum_analyzer_set_callback(
    SubghzSpectrumAnalyzer* subghz_spectrum_analyzer,
    SubghzSpectrumAnalyzerCallback callback,
    void* context) {
    furi_assert(subghz_spectrum_analyzer);
    furi_assert(callback);
    subghz_spectrum_analyzer->callback = callback;
    subghz_spectrum_analyzer->context = context;
}

void subghz_spectrum_analyzer_draw(Canvas* canvas, SubghzSpectrumAnalyzerModel* model) {

    canvas_clear(canvas);

    uint32_t start_x = 0;
    for (uint32_t i = 0; i < DOTS_COUNT; ++i){

        canvas_draw_line(canvas, start_x, 0,
            start_x, abs(model->rssi_buf[i].rssi));

        start_x += 2;
    }

}

bool subghz_spectrum_analyzer_input(InputEvent* event, void* context) {
    furi_assert(context);

    SubghzSpectrumAnalyzer* instance = context;

    furi_assert(context);
    if(event->key == InputKeyOk) {
        with_view_model(
            instance->view, (SubghzSpectrumAnalyzerModel * model) {
                model->frequency = 866000000;
                return true;
            });
        return true;
    }
    if(event->key == InputKeyBack) {
        return false;
    }

    return true;
}


void subghz_spectrum_analyzer_pair_callback(
		void* context,
		uint8_t dot_num,
		uint32_t frequency,
		float rssi,
		bool redraw
)
{
	SubghzSpectrumAnalyzer* instance = context;
    with_view_model(
        instance->view, (SubghzSpectrumAnalyzerModel * model) {
            model->rssi_buf[dot_num].rssi = rssi;
            model->rssi_buf[dot_num].frequency = frequency;
            return redraw;
        });
}

void subghz_spectrum_analyzer_enter(void* context) {
    furi_assert(context);
    SubghzSpectrumAnalyzer* instance = context;

    //Start worker
    instance->worker = subghz_spectrum_analyzer_worker_alloc();
    subghz_spectrum_analyzer_worker_set_params(instance->worker, 433000000, 100000);
    subghz_spectrum_analyzer_worker_set_pair_callback(
            instance->worker,
            (SubGhzSpectrumAnalyzerWorkerPairCallback)subghz_spectrum_analyzer_pair_callback,
            instance);
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
    		model->rssi = 0;
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
