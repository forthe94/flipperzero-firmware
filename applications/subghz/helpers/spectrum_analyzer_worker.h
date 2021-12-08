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
} SpectrumAnalyzerWorker;



SpectrumAnalyzerWorker* spectrum_analyzer_worker_alloc();
void spectrum_analyzer_worker_free(SpectrumAnalyzerWorker* instance);
void spectrum_analyzer_worker_start(SpectrumAnalyzerWorker* instance);
void spectrum_analyzer_worker_stop(SpectrumAnalyzerWorker* instance);

