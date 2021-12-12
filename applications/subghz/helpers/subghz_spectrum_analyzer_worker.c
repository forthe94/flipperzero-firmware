/*
 * spectrum_analyzer_worker.c
 *
 *  Created on: Dec 4, 2021
 *      Author: forthe
 */
#include "subghz_spectrum_analyzer_worker.h"

#include <furi.h>
#include <cc1101.h>


struct SubGhzSpectrumAnalyzerWorker {
    FuriThread* thread;

    volatile bool worker_running;
    FrequencyRSSI rssi_buf[DOTS_COUNT];
    uint32_t freq_start;
    uint32_t freq_step;
    uint8_t calibration_values[3][DOTS_COUNT];

    bool recalibrate;
    SubGhzSpectrumAnalyzerWorkerPairCallback pair_callback;
    void* context;
};

static uint16_t bandwidths[] = {65535, 812, 650, 541, 464, 406, 325, 270, 232, 203, 162, 135, 116, 102, 81, 68, 58, 0};
//static uint16_t freq_steps_good[] = {500, 300, 250, 225, 200, 150, 125, 100, 100, 75, 50, 50, 50, 50, 25, 10};

static uint8_t get_bandwidth(uint32_t freq_step)
{
	uint8_t bw = 0;
	freq_step /= 1000;
	while((freq_step > bandwidths[bw]) | (freq_step < bandwidths[bw + 1]))
			bw++;
	return bw;

}

static void recalibrate(SubGhzSpectrumAnalyzerWorker* instance)
{
	uint32_t freq_step = instance->freq_step;
	uint32_t freq_start = instance->freq_start;
    // Calibrate and store calibration values for all
    // working frequences
    for (uint8_t i=0; i<DOTS_COUNT; i++) {

        furi_hal_subghz_set_frequency(freq_start + freq_step * i);
        furi_hal_spi_acquire(&furi_hal_spi_bus_handle_subghz);
        cc1101_read_cal_values(&furi_hal_spi_bus_handle_subghz,
                            &instance->calibration_values[0][i],
                            &instance->calibration_values[1][i],
                            &instance->calibration_values[2][i]);
        furi_hal_spi_release(&furi_hal_spi_bus_handle_subghz);

        // fill initial values

        instance->rssi_buf[i].frequency = freq_start + freq_step * i;
        instance->rssi_buf[i].rssi = -100;
    }
	instance->recalibrate = false;

}
static int32_t subghz_spectrum_analyzer_worker_thread(void* context) {
	SubGhzSpectrumAnalyzerWorker* instance = context;

    while(instance->worker_running) {
        osDelay(10);
        uint32_t freq_step = instance->freq_step;
        uint32_t freq_start = instance->freq_start;
        if (instance->recalibrate)
        {
        	// Start CC1011
			furi_hal_subghz_reset();
			furi_hal_subghz_load_preset(FuriHalSubGhzPresetOok650Async);
        	recalibrate(instance);
        	furi_hal_spi_acquire(&furi_hal_spi_bus_handle_subghz);
			uint8_t bw = get_bandwidth(freq_step);
			cc1101_write_reg(&furi_hal_spi_bus_handle_subghz, CC1101_MDMCFG4,  0x7 | (bw << 4));
			furi_hal_spi_release(&furi_hal_spi_bus_handle_subghz);
        }
        for (uint8_t i=0; i<DOTS_COUNT; i++) {
            // Fast frequency hop (chapter 28.2 of CC1101 datasheet)
            furi_hal_subghz_idle();

            furi_hal_subghz_set_frequency_and_path_fast(freq_start + freq_step * i);
            furi_hal_spi_acquire(&furi_hal_spi_bus_handle_subghz);
            cc1101_write_cal_values(&furi_hal_spi_bus_handle_subghz,
                                    instance->calibration_values[0][i],
                                    instance->calibration_values[1][i],
                                    instance->calibration_values[2][i]);
            furi_hal_spi_release(&furi_hal_spi_bus_handle_subghz);
            furi_hal_subghz_rx();
            osDelay(3);

            // Read RSSI for current channnel
            float rssi = furi_hal_subghz_get_rssi();

            // Processing out buffer

            if (rssi > instance->rssi_buf[i].rssi) {
                instance->rssi_buf[i].rssi = rssi;
            }
            else {

                instance->rssi_buf[i].rssi -= 1.0f;
            }
            instance->pair_callback(instance->context, i, freq_start + freq_step * i, instance->rssi_buf[i].rssi, false);
        }
        instance->pair_callback(instance->context, 0, freq_start, instance->rssi_buf[0].rssi, true);

    }

    //Stop CC1101
    furi_hal_subghz_idle();
    furi_hal_subghz_sleep();

    return 0;
}

SubGhzSpectrumAnalyzerWorker* subghz_spectrum_analyzer_worker_alloc() {
	SubGhzSpectrumAnalyzerWorker* instance = furi_alloc(sizeof(SubGhzSpectrumAnalyzerWorker));
	instance->recalibrate = true;
    instance->thread = furi_thread_alloc();
    furi_thread_set_name(instance->thread, "SubghzSpectrumAnalyzerWorker");
    furi_thread_set_stack_size(instance->thread, 2048);
    furi_thread_set_context(instance->thread, instance);
    furi_thread_set_callback(instance->thread, subghz_spectrum_analyzer_worker_thread);


    return instance;
}


void subghz_spectrum_analyzer_worker_free(SubGhzSpectrumAnalyzerWorker* instance) {
    furi_assert(instance);

    furi_thread_free(instance->thread);

    free(instance);
}

void subghz_spectrum_analyzer_worker_set_pair_callback(
    SubGhzSpectrumAnalyzerWorker* instance,
    SubGhzSpectrumAnalyzerWorkerPairCallback callback,
    void* context) {
    furi_assert(instance);
    furi_assert(context);
    instance->pair_callback = callback;
    instance->context = context;
}

void subghz_spectrum_analyzer_worker_start(SubGhzSpectrumAnalyzerWorker* instance) {
    furi_assert(instance);
    furi_assert(!instance->worker_running);

    instance->worker_running = true;

    furi_thread_start(instance->thread);
}

void subghz_spectrum_analyzer_worker_stop(SubGhzSpectrumAnalyzerWorker* instance) {
    furi_assert(instance);
    furi_assert(instance->worker_running);

    instance->worker_running = false;

    furi_thread_join(instance->thread);
}

bool subghz_spectrum_analyzer_worker_is_running(SubGhzSpectrumAnalyzerWorker* instance) {
    furi_assert(instance);
    return instance->worker_running;
}

void subghz_spectrum_analyzer_worker_set_params(SubGhzSpectrumAnalyzerWorker* instance, uint32_t freq_start, uint32_t freq_step)
{
	instance->freq_start = freq_start;
	instance->freq_step = freq_step;
	instance->recalibrate = true;
}
