#include "sampling_and_analyzing.h"
#include "generating_signal.h"
#include "transmitting.h"
#include "esp_log.h"
#include "math.h"
#include "kiss_fft.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include "esp_task_wdt.h"



static const char* TAG = "DynamicSampler";
static bool condition_met = false; 

#define THRESHOLD_MULTIPLIER 1.1  // multiplier to adjust the threshold dynamically (lower=more sensitive to smaller inputs)
#define FFT_SIZE 64  // buffer size (should be a power of 2 and depends on your sampling rate 32, 64, 128, 256, 512, 1024, etc.
// Increasing improves frequency resolution but requires more computational power and memory.
#define MAX_ITERATIONS 100  // define a maximum number of iterations for the loop
#define WATCHDOG_FEED_INTERVAL 10 // interval to feed the watchdog in iterations
//#define PAUSE_DURATION_MS 1000  // in milliseconds

int current_sampling_rate = 600;  // starting with an initial "high" rate
int min_sampling_rate = 40;  // recommended minimum sampling rate
int max_sampling_rate = 9000000;  // max allowable sampling rate
int step_change = 20;  // step change for sampling rate
float previous_max_magnitude = 0; 
int initial_sampling_rate;  // to store it

float max_observed_frequency_magnitude = 0;
const int larger_step_change = 150; 

// Shared variable to store the optimal sampling rate
int optimal_sampling_rate = 600;


int oversample() {
    int oversampling_rate = 600;
    const int oversample_jump = 500000;
    int iterations_counter = 0;  // counter to track iterations for watchdog reset

    while (true) {  
        float signal[FFT_SIZE];  

        // Retrieve signal from queue
        for (int i = 0; i < FFT_SIZE; i++) {
             if (xQueueReceive(signalQueue, &signal[i], portMAX_DELAY) != pdPASS) {
                ESP_LOGE(TAG, "Failed to receive data from the queue");
                return -1;
            }
            if (i % (FFT_SIZE / 10) == 0) { // more frequent resets based on loop iteration
                esp_task_wdt_reset();
            }}

       
         // Log the current oversampling rate for diagnostics
        ESP_LOGI(TAG, "Current Oversampling Rate: %d Hz", oversampling_rate);

        // Break the loop if the sampling rate reaches or exceeds the maximum sampling rate
        if (oversampling_rate >= max_sampling_rate) {
            ESP_LOGI(TAG, "Reached maximum sampling rate of %d Hz. Exiting loop.", max_sampling_rate);
            char status_message[256];
            snprintf(status_message, sizeof(status_message), "Reached maximum sampling rate of %d Hz, exiting loop.", max_sampling_rate);
            mqtt_publish("/status", status_message);
            break;
        }

        // Increase the sampling rate
        oversampling_rate += oversample_jump;

         // Periodically reset the watchdog timer
        iterations_counter++;
        if (iterations_counter % WATCHDOG_FEED_INTERVAL == 0) {
            esp_task_wdt_reset();
            //ESP_LOGI(TAG, "Oversampling...");
            iterations_counter = 0;  // reset
        }
    }
    return oversampling_rate;
    oversampling_rate = 600;  // reset
    esp_task_wdt_reset(); 
}


// Analyze the FFT of the provided signal and store magnitude results.
void analyze_fft(const float* signal, float* frequency_magnitude) {
    kiss_fft_cfg cfg;
    kiss_fft_cpx in[FFT_SIZE], out[FFT_SIZE]; // complex arrays for FFT input and output.

    // Allocate memory and prepare FFT configuration
    if ((cfg = kiss_fft_alloc(FFT_SIZE, 0, NULL, NULL)) != NULL) {
       // ESP_LOGI(TAG, "FFT allocation successful.");
        // Initialize input for FFT: real part from signal, imaginary part as zero
        for (int i = 0; i < FFT_SIZE; i++) {
            in[i].r = signal[i];
            in[i].i = 0;
        }

        // Perform FFT
        kiss_fft(cfg, in, out);
        free(cfg); // free the FFT configuration.

        // Calculate magnitude of each frequency component
        for (int i = 0; i < FFT_SIZE / 2 + 1; i++) {
            frequency_magnitude[i] = sqrt(out[i].r * out[i].r + out[i].i * out[i].i);
        }
    } else {
        // If FFT configuration fails, populate magnitude array with zeros.
        ESP_LOGE(TAG, "FFT allocation failed.");
        for (int i = 0; i < FFT_SIZE / 2 + 1; i++) {
            frequency_magnitude[i] = 0;
        }
    }
}


int sample_and_analyze_signal() {
    int iterations = 0;  
    bool increasing = true;  
    bool decreasing = false; 

    initial_sampling_rate = current_sampling_rate;  // store the initial sampling rate
    int max_sampling_rate_observed = current_sampling_rate;

    int increase_retries = 0; 
    const int max_increase_retries = 3; 

    int no_improvement_counter = 0;
    float improvement_ratio = 1.05; // ratio to determine significant improvement (e.g., 5% improvement)
    // sensitivity of the optimization process
    float last_max_magnitude = 0;
    char message[256];  // buffer for MQTT messages


    while (!condition_met && iterations < MAX_ITERATIONS) {  // continue looping until the condition is met
        float dt = 1.0 / current_sampling_rate;
        float signal[FFT_SIZE];  // ensure the array is large enough for FFT_SIZE
        float frequency_magnitude[FFT_SIZE / 2 + 1];  // array to store magnitudes

        //ESP_LOGI(TAG, "Initial Sampling Rate: %d Hz", current_sampling_rate);

        // Sample signal until buffer is full.
        for (int i = 0; i < FFT_SIZE; i++) {
            float t = i * dt;
            signal[i] = get_signal_at(t);
            if (i % (FFT_SIZE / 10) == 0) { // more frequent resets based on loop iteration
                esp_task_wdt_reset();
            }
            // if (i < 10) {  // Log the first 10 samples for inspection
            //     ESP_LOGI(TAG, "Signal at t=%f: %f", t, signal[i]);
            // }
        }

        // Perform FFT analysis on the sampled signal.
        analyze_fft(signal, frequency_magnitude);
        esp_task_wdt_reset();

        // Find the maximum magnitude in the frequency spectrum.
        float max_frequency_magnitude = 0;
        for (int i = 0; i < FFT_SIZE / 2 + 1; i++) {
            if (frequency_magnitude[i] > max_frequency_magnitude) {
                max_frequency_magnitude = frequency_magnitude[i];
            }
        }

        ESP_LOGI(TAG, "Max Frequency Magnitude: %.2f", max_frequency_magnitude);

        // Update the maximum observed frequency magnitude and optimal sampling rate
        if (max_frequency_magnitude > max_observed_frequency_magnitude) {
            max_observed_frequency_magnitude = max_frequency_magnitude;
            optimal_sampling_rate = current_sampling_rate;
            no_improvement_counter = 0; // reset counter on significant improvement
        } else {
            no_improvement_counter++;
        }

        // Track the maximum sampling frequency observed
        if (current_sampling_rate > max_sampling_rate_observed) {
            max_sampling_rate_observed = current_sampling_rate;
        }

        // Check for significant improvement
        if (max_frequency_magnitude > last_max_magnitude * improvement_ratio) {
            last_max_magnitude = max_frequency_magnitude;
        } else {
            no_improvement_counter++;
        }

            if (increasing) {
                if (max_frequency_magnitude > previous_max_magnitude && current_sampling_rate < max_sampling_rate_observed) {
                    current_sampling_rate += step_change; // use normal step
                    ESP_LOGI(TAG, "Increasing sampling rate to: %d Hz", current_sampling_rate);
                    increase_retries = 0; // reset retries on successful increase
                } else {
                    if (increase_retries < max_increase_retries) {
                        current_sampling_rate += larger_step_change; // attempt with larger step
                        ESP_LOGI(TAG, "Retrying with larger step. Increasing sampling rate to: %d Hz", current_sampling_rate);
                        increase_retries++;
                    } else {
                        increasing = false;
                        decreasing = true;
                        current_sampling_rate = initial_sampling_rate; // reset for decreasing
                        ESP_LOGI(TAG, "Switching to decreasing mode. Resetting sampling rate to: %d Hz", current_sampling_rate);
                        increase_retries = 0;
                    }
                }
            } else if (decreasing) {
                if (increase_retries < max_increase_retries && max_frequency_magnitude <= previous_max_magnitude && current_sampling_rate > min_sampling_rate) {
                    current_sampling_rate -= larger_step_change; // use normal step
                    ESP_LOGI(TAG, "Decreasing sampling rate with a larger step to: %d Hz", current_sampling_rate);
                } else {
                    if (max_frequency_magnitude > previous_max_magnitude) {
                        current_sampling_rate -= step_change; // attempt with larger step
                        ESP_LOGI(TAG, "Retrying with smaller step. Decreasing sampling rate to: %d Hz", current_sampling_rate);
                        increase_retries++;
                    } else {
                        decreasing = false;
                        ESP_LOGI(TAG, "Exiting decreasing mode.");
                        condition_met = true; // confirm exit
                    }
                }
            }
    previous_max_magnitude = max_frequency_magnitude; // update magnitude reference

            //ESP_LOGI(TAG, "Sampling after adjusting: %d Hz", current_sampling_rate);
            iterations++;
            esp_task_wdt_reset(); 
            //  ESP_LOGI(TAG, "Number of iterations: %d", iterations);
            ESP_LOGI(TAG, "Reanalyzing with new parameters...");
    }

    // Log optimization reached after exiting the loop
    ESP_LOGI(TAG, "Optimization reached with max magnitude: %.2f", max_observed_frequency_magnitude);
    // Transmit the final optimal rate and analysis results
    snprintf(message, sizeof(message), "Optimization reached with max magnitude: %.2f", max_observed_frequency_magnitude);
    mqtt_publish("/optimization_result", message);
     
    // Output the final results
    ESP_LOGI(TAG, "The optimal sampling rate is: %d Hz", optimal_sampling_rate);
    // Transmit the final optimal rate and analysis results
    snprintf(message, sizeof(message), "The optimal sampling rate is: %d Hz", optimal_sampling_rate);
    mqtt_publish("/optimal_rate result", message);
    ESP_LOGI(TAG, "The maximum sampling frequency observed was: %d Hz",  max_sampling_rate_observed);
    // Transmit the final optimal rate and analysis results
    snprintf(message, sizeof(message), "The maximum sampling frequency observed was: %d Hz",  max_sampling_rate_observed);
    mqtt_publish("/max_sampling_rate_observed_result", message);


    // Reset the condition flag and relevant variables for the next analysis cycle
    condition_met = false;
    current_sampling_rate = 600; 
    max_sampling_rate_observed = 0;
    max_observed_frequency_magnitude = 0;
    previous_max_magnitude = 0;
    esp_task_wdt_reset();

    return optimal_sampling_rate;    
}

//Not realistic: rolling maybe, but with real signals you would observe
float calculate_average_signal(float start_time, float window_length, int optimal_rate) {
    float dt = 1.0 / optimal_rate;
    int num_samples = window_length / dt;
    float sum = 0.0;
    float abs_sum = 0.0; // to store the absolut sum
    char message[256];  // buffer for MQTT messages

    
    // ESP_LOGI(TAG, "Dt is %.5f", dt); 
    // ESP_LOGI(TAG, "Num samples is %d", num_samples);

    for (int i = 0; i < num_samples; i++) {
        float t = start_time + i * dt;
        float signal_value = get_signal_at(t);
        sum += signal_value;
        abs_sum += fabs(signal_value); // sum of absolute values
        //ESP_LOGI(TAG, "t=%.3f, signal_value=%.3f, sum=%.3f", t, signal_value, sum);
       // ESP_LOGI(TAG, "Intermediate sum is %.3f the absolute is %.3f", sum, abs_sum);
    }
    ESP_LOGI(TAG, "Final sum is %.3f", sum); //a symmetric signal around zero will have an average close to zero.
    ESP_LOGI(TAG, "Final absolute sum is %.3f", abs_sum); //the average of absolute values provides insight into the signal's magnitude.
    float average = sum / num_samples;
    ESP_LOGI(TAG, "Average signal from t=%.3f to t=%.3f is %.3f", start_time, start_time + window_length, average);
    float average_abs = abs_sum / num_samples;
    ESP_LOGI(TAG, "Average absolute signal from t=%.3f to t=%.3f is %.3f", start_time, start_time + window_length, average_abs);

    // Transmit the final optimal rate and analysis results
    snprintf(message, sizeof(message), "Average absolute signal from t=%.3f to t=%.3f is %.3f", start_time, start_time + window_length, average_abs);
    mqtt_publish("/avarage_analysis_result", message);
    optimal_sampling_rate = 600; // reset for the next cycle
    return average_abs;
    
}

float sample_signal_and_compute_average(float window_length) {
    float start_time = 0.0;
    ESP_LOGI(TAG, "The optimal_rate from FFT to pass is %d", optimal_sampling_rate);
    return calculate_average_signal(start_time, window_length, optimal_sampling_rate);
}