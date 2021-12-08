/*
 * spectrum_analyzer_worker.c
 *
 *  Created on: Dec 4, 2021
 *      Author: forthe
 */
#include "spectrum_analyzer_worker.h"

#include <furi.h>
#include <cc1101.h>

// TODO: dynamically allocated
uint8_t calibration_values[3][DOTS_COUNT];
float spect_buf[DOTS_COUNT];


//static uint16_t freq_steps[] = {812, 650, 541, 464, 406, 325, 270, 232, 203, 162, 135, 116, 102, 81, 68, 58};
static uint16_t freq_steps_good[] = {500, 300, 250, 225, 200, 150, 125, 100, 100, 75, 50, 50, 50, 50, 25, 10};

static int32_t spectrum_analyzer_worker_thread(void* context) {
	SpectrumAnalyzerWorker* instance = context;

    uint32_t freq_step = freq_steps_good[instance->bandwidth]*1000;
    // Start CC1011
    furi_hal_subghz_reset();
    furi_hal_subghz_load_preset(FuriHalSubGhzPresetOok650Async);
    furi_hal_spi_acquire(&furi_hal_spi_bus_handle_subghz);

    cc1101_write_reg(&furi_hal_spi_bus_handle_subghz, CC1101_MDMCFG4,  0x7 | (instance->bandwidth << 4));
    furi_hal_spi_release(&furi_hal_spi_bus_handle_subghz);

    // Calibrate and store calibration values for all
    // working frequences 
    for (uint8_t i=0; i<DOTS_COUNT; i++) {

        furi_hal_subghz_set_frequency(instance->start_freq + freq_step * i);
        furi_hal_spi_acquire(&furi_hal_spi_bus_handle_subghz);
        cc1101_read_cal_values(&furi_hal_spi_bus_handle_subghz,
                            &calibration_values[0][i],
                            &calibration_values[1][i],
                            &calibration_values[2][i]);
        furi_hal_spi_release(&furi_hal_spi_bus_handle_subghz);

        // fill initial values


        instance->rssi_buf[i].frequency = instance->start_freq + freq_step * i;
        instance->rssi_buf[i].rssi = -100;
    }
    
    while(instance->worker_running) {
        
        for (uint8_t i=0; i<DOTS_COUNT; i++) {
            // Fast frequency hop (chapter 28.2 of CC1101 datasheet)
            furi_hal_subghz_idle();
            
            furi_hal_subghz_set_frequency_and_path_fast(instance->start_freq + freq_step * i);
            furi_hal_spi_acquire(&furi_hal_spi_bus_handle_subghz);
            cc1101_write_cal_values(&furi_hal_spi_bus_handle_subghz,
                                    calibration_values[0][i],
                                    calibration_values[1][i],
                                    calibration_values[2][i]);
            furi_hal_spi_release(&furi_hal_spi_bus_handle_subghz);
            furi_hal_subghz_rx();
            osDelay(3);
            
            // Read RSSI for current channnel
            spect_buf[i] = furi_hal_subghz_get_rssi();

            // Processing out buffer

            if (spect_buf[i] > instance->rssi_buf[i].rssi) {
                instance->rssi_buf[i].rssi = spect_buf[i];
            }  
            else {

                instance->rssi_buf[i].rssi -= 1.0f;
            }
        }
    }


    return 0;
}

SpectrumAnalyzerWorker* spectrum_analyzer_worker_alloc() {
	SpectrumAnalyzerWorker* instance = furi_alloc(sizeof(SpectrumAnalyzerWorker));

    instance->thread = furi_thread_alloc();
    furi_thread_set_name(instance->thread, "SpectrumAnalyzerWorker");
    furi_thread_set_stack_size(instance->thread, 2048);
    furi_thread_set_context(instance->thread, instance);
    furi_thread_set_callback(instance->thread, spectrum_analyzer_worker_thread);


    return instance;
}


void spectrum_analyzer_worker_free(SpectrumAnalyzerWorker* instance) {
    furi_assert(instance);

    furi_thread_free(instance->thread);

    free(instance);
}

void spectrum_analyzer_worker_start(SpectrumAnalyzerWorker* instance) {
    furi_assert(instance);
    furi_assert(!instance->worker_running);

    instance->worker_running = true;

    furi_thread_start(instance->thread);
}

void spectrum_analyzer_worker_stop(SpectrumAnalyzerWorker* instance) {
    furi_assert(instance);
    furi_assert(instance->worker_running);

    instance->worker_running = false;

    furi_thread_join(instance->thread);
}

bool spectrum_analyzer_worker_is_running(SpectrumAnalyzerWorker* instance) {
    furi_assert(instance);
    return instance->worker_running;
}
