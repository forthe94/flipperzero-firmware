/*
 * spectrum_analyzer_worker.h
 *
 *  Created on: Dec 4, 2021
 *      Author: forthe
 */

#pragma once 

#include <furi-hal.h>

#define DOTS_COUNT (50)

typedef struct SubGhzSpectrumAnalyzerWorker SubGhzSpectrumAnalyzerWorker;
typedef void (
    *SubGhzSpectrumAnalyzerWorkerPairCallback)(
    		void* context,
    		uint8_t dot_num,
    		uint32_t frequency,
    		float rssi,
    		bool redraw
    );

typedef struct {
    uint32_t frequency;
    float rssi;
} FrequencyRSSI;





SubGhzSpectrumAnalyzerWorker* subghz_spectrum_analyzer_worker_alloc();
void subghz_spectrum_analyzer_worker_free(SubGhzSpectrumAnalyzerWorker* instance);
void subghz_spectrum_analyzer_worker_start(SubGhzSpectrumAnalyzerWorker* instance);
void subghz_spectrum_analyzer_worker_stop(SubGhzSpectrumAnalyzerWorker* instance);
bool subghz_spectrum_analyzer_worker_is_running(SubGhzSpectrumAnalyzerWorker* instance);
void subghz_spectrum_analyzer_worker_set_params(SubGhzSpectrumAnalyzerWorker* instance, uint32_t freq_start, uint32_t freq_step);
void subghz_spectrum_analyzer_worker_set_pair_callback(
    SubGhzSpectrumAnalyzerWorker* instance,
    SubGhzSpectrumAnalyzerWorkerPairCallback callback,
    void* context);
