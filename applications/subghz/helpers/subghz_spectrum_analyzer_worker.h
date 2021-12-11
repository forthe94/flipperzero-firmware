/*
 * spectrum_analyzer_worker.h
 *
 *  Created on: Dec 4, 2021
 *      Author: forthe
 */

#pragma once 

#include <furi-hal.h>

#define DOTS_COUNT (50)

typedef struct {
    uint32_t frequency;
    float rssi;
} FrequencyRSSI;

typedef struct  {
    FuriThread* thread;

    volatile bool worker_running;
    FrequencyRSSI rssi_buf[DOTS_COUNT];
    uint32_t start_freq;
    uint32_t bandwidth;

    void* context;
} SubghzSpectrumAnalyzerWorker;



SubghzSpectrumAnalyzerWorker* subghz_spectrum_analyzer_worker_alloc();
void subghz_spectrum_analyzer_worker_free(SubghzSpectrumAnalyzerWorker* instance);
void subghz_spectrum_analyzer_worker_start(SubghzSpectrumAnalyzerWorker* instance);
void subghz_spectrum_analyzer_worker_stop(SubghzSpectrumAnalyzerWorker* instance);
bool subghz_spectrum_analyzer_worker_is_running(SubghzSpectrumAnalyzerWorker* instance);

