#include "subghz_spectrum_analyzer.h"
#include "../subghz_i.h"

#include <math.h>
#include <furi.h>
#include <furi-hal.h>
#include <input/input.h>
#include <notification/notification-messages.h>
#include <lib/subghz/protocols/subghz_protocol_princeton.h>
#include "../helpers/subghz_spectrum_analyzer_worker.h"
#include <gui/gui_i.h>

#include <assets_icons.h>

struct SubghzSpectrumAnalyzer {
    View* view;
    SubGhzSpectrumAnalyzerWorker* worker;
    SubghzSpectrumAnalyzerCallback callback;
    void* context;
};

typedef struct {
    uint32_t frequency;
    uint32_t frequency_end;
    uint32_t freq_step;
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

static void set_model_params(SubghzSpectrumAnalyzer* instance, uint32_t freq, uint32_t freq_step)
{
	uint32_t freq_end = freq + freq_step * DOTS_COUNT;

	if (furi_hal_subghz_is_frequency_valid(freq) & furi_hal_subghz_is_frequency_valid(freq_end))
	{
		with_view_model(
			instance->view, (SubghzSpectrumAnalyzerModel * model) {
				model->frequency = freq;
				model->freq_step = freq_step;
				model->frequency_end = freq_end;
				return false;
			});
		subghz_spectrum_analyzer_worker_set_params(instance->worker, freq, freq_step);

	}
}
void subghz_spectrum_analyzer_draw(Canvas* canvas, SubghzSpectrumAnalyzerModel* model) {
    char buffer[64];

    canvas_clear(canvas);

    uint32_t start_x = 0;
    for (uint32_t i = 0; i < DOTS_COUNT; ++i){
    	float val = (model->rssi_buf[i].rssi < -100) ? 0 : model->rssi_buf[i].rssi + 100;
        canvas_draw_line(canvas, start_x, GUI_DISPLAY_HEIGHT-10,
            start_x, GUI_DISPLAY_HEIGHT - val - 10);

        start_x += 2;
    }
    canvas_set_font(canvas, FontPrimary);
	snprintf(
		buffer,
		sizeof(buffer),
		"%03ld.%03ld",
		model->frequency / 1000000 % 1000,
		model->frequency / 1000 % 1000);
	canvas_draw_str(canvas, 0, GUI_DISPLAY_HEIGHT, buffer);
	snprintf(
		buffer,
		sizeof(buffer),
		"%03ld.%03ld",
		(model->frequency_end) / 1000000 % 1000,
		(model->frequency_end) / 1000 % 1000);
	canvas_draw_str(canvas, 61, GUI_DISPLAY_HEIGHT, buffer);

}

bool subghz_spectrum_analyzer_input(InputEvent* event, void* context) {
    furi_assert(context);

    SubghzSpectrumAnalyzer* instance = context;
	SubghzSpectrumAnalyzerModel* model = view_get_model(instance->view);

    furi_assert(context);
    if(event->key == InputKeyOk) {

        return true;
    }
    if(event->key == InputKeyBack) {
        return false;
    }
    else if ((event->key == InputKeyRight) & (event->type == InputTypePress)) {
    	set_model_params(instance, model->frequency + model->freq_step, model->freq_step);
    	return true;
    }
    else if ((event->key == InputKeyLeft) & (event->type == InputTypePress)) {
    	set_model_params(instance, model->frequency - model->freq_step, model->freq_step);
    	return true;
    }
    else if ((event->key == InputKeyUp) & (event->type == InputTypePress)) {
    	if (model->freq_step <= 100000)
    		return true;
    	set_model_params(instance, model->frequency, model->freq_step - 100000);
    	return true;
    }
    else if ((event->key == InputKeyDown) & (event->type == InputTypePress)) {
    	set_model_params(instance, model->frequency, model->freq_step + 100000);
    	return true;
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
        instance->view, ViewModelTypeLockFree, sizeof(SubghzSpectrumAnalyzerModel));
    view_set_context(instance->view, instance);
    view_set_draw_callback(instance->view, (ViewDrawCallback)subghz_spectrum_analyzer_draw);
    view_set_input_callback(instance->view, subghz_spectrum_analyzer_input);
    view_set_enter_callback(instance->view, subghz_spectrum_analyzer_enter);
    view_set_exit_callback(instance->view, subghz_spectrum_analyzer_exit);

    with_view_model(
        instance->view, (SubghzSpectrumAnalyzerModel * model) {
    		for (uint8_t i = 0; i<DOTS_COUNT;i++)
    		{
                model->rssi_buf[i].rssi = -100;
    		}
            model->frequency = 866000000;
            model->freq_step = 100000;
            model->frequency_end = 866000000 + 100000 * 50;
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
