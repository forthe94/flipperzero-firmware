#pragma once

#include <gui/view.h>
#include "../helpers/subghz_custom_event.h"

#define DOTS_COUNT (50)
typedef struct SubghzSpectrumAnalyzer SubghzSpectrumAnalyzer;
//
//typedef void (*SubghzFrequencyAnalyzerCallback)(SubghzCustomEvent event, void* context);
//
//void subghz_frequency_analyzer_set_callback(
//    SubghzFrequencyAnalyzer* subghz_frequency_analyzer,
//    SubghzFrequencyAnalyzerCallback callback,
//    void* context);
//
SubghzSpectrumAnalyzer* subghz_spectrum_analyzer_alloc();

void subghz_spectrum_analyzer_free(SubghzSpectrumAnalyzer* subghz_static);

View* subghz_spectrum_analyzer_get_view(SubghzSpectrumAnalyzer* subghz_static);
