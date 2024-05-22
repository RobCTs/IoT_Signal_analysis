#include "sampling_and_analyzing.h"
#include "generating_signal.h"
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

#define THRESHOLD_MULTIPLIER 1.2  // multiplier to adjust the threshold dynamically (before 1.1)
#define FFT_SIZE 40  // buffer size (should be a power of 2 and depends on your sampling rate)
#define MAX_ITERATIONS 100  // define a maximum number of iterations for the loop
#define WATCHDOG_FEED_INTERVAL 10  // interval to feed the watchdog in iterations

int current_sampling_rate = 600;  // starting with an initial "high" rate
int min_sampling_rate = 40;  // recommended minimum sampling rate
int max_sampling_rate = 6500;  // max allowable sampling rate (36k?)
int step_change = 20;  // step change for sampling rate
float previous_max_magnitude = 0; 
int initial_sampling_rate;  // to store it

float max_observed_frequency_magnitude = 0;
 const int larger_step_change = 150; 


// Shared variable to store the optimal sampling rate
int optimal_sampling_rate = 600;

int oversample() {
    int crashed_rate = 0;
    float max_observed_magnitude = 0;

    int iterations_counter = 0;  // counter to track iterations for watchdog reset


    while (true) {  // Infinite loop, no predefined crash condition
        ESP_LOGI(TAG, "Current Sampling Rate: %d Hz", current_sampling_rate);

        // Sample and analyze the signal at the current rate
        float dt = 1.0 / current_sampling_rate;  // Time interval based on the current sampling rate
        float signal[FFT_SIZE];  // Buffer to store the signal samples
        float frequency_magnitude1[FFT_SIZE / 2 + 1];  // Buffer to store frequency magnitudes from FFT

        // Simulate sampling the signal (replace get_signal_at(t) with the actual function to retrieve signal data)
        for (int i = 0; i < FFT_SIZE; i++) {
            float t = i * dt;
            signal[i] = get_signal_at(t); }

        // Perform FFT analysis on the sampled signal
        analyze_fft(signal, frequency_magnitude1);

        // Find the maximum magnitude in the frequency spectrum.
        float max_frequency_magnitude_current_rate = 0;
        for (int i = 0; i < FFT_SIZE / 2 + 1; i++) {
            if (frequency_magnitude1[i] > max_frequency_magnitude_current_rate) {
                max_frequency_magnitude_current_rate = frequency_magnitude1[i];
            }
        }

        // Update the maximum observed frequency magnitude and optimal sampling rate
        if (max_frequency_magnitude_current_rate > max_observed_magnitude) {
            max_observed_magnitude = max_frequency_magnitude_current_rate;
        }

        // Log the maximum frequency magnitude for diagnostics
        ESP_LOGI(TAG, "Current Sampling Rate: %d Hz, Current Max Frequency Magnitude: %.2f, Max Observed Frequency Magnitude: %.2f",
         current_sampling_rate, max_frequency_magnitude_current_rate, max_observed_magnitude);


        // Increase the sampling rate and handle any possible integer overflow
        if (current_sampling_rate + larger_step_change > current_sampling_rate) {
            current_sampling_rate += larger_step_change;
            crashed_rate = current_sampling_rate;
        } else {
            // If overflow occurs, log and reset sampling rate
            ESP_LOGE(TAG, "Integer overflow detected at sampling rate %d Hz. Resetting to safe value.", current_sampling_rate);
            current_sampling_rate = 600;
        }

        // Periodically reset the watchdog timer
        iterations_counter++;
        if (iterations_counter >= WATCHDOG_FEED_INTERVAL) {
            esp_task_wdt_reset();
            iterations_counter = 0;  // resetr
        }
   }

    // After exiting the loop, optionally reset sampling rate to a safe value or handle the failure
    ESP_LOGI(TAG, "Handling after system failure or stop condition.");
    current_sampling_rate = 600;  // Reset to a safe value

    return crashed_rate;
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
    float last_max_magnitude = 0;

    while (!condition_met && iterations < MAX_ITERATIONS) {  // continue looping until the condition is met
        float dt = 1.0 / current_sampling_rate;
        float signal[FFT_SIZE];  // ensure the array is large enough for FFT_SIZE
        float frequency_magnitude[FFT_SIZE / 2 + 1];  // array to store magnitudes

        //ESP_LOGI(TAG, "Initial Sampling Rate: %d Hz", current_sampling_rate);

        // Sample signal until buffer is full.
        for (int i = 0; i < FFT_SIZE; i++) {
            float t = i * dt;
            signal[i] = get_signal_at(t);
            // if (i < 10) {  // Log the first 10 samples for inspection
            //     ESP_LOGI(TAG, "Signal at t=%f: %f", t, signal[i]);
            // }
            // Feed the watchdog periodically
            // if (i % WATCHDOG_FEED_INTERVAL == 0) {
            //     esp_task_wdt_reset();
            // }
        }

        // Perform FFT analysis on the sampled signal.
        analyze_fft(signal, frequency_magnitude);

        // Feed the watchdog after FFT analysis
       // esp_task_wdt_reset();

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

        
            // ESP_LOGI(TAG, "BEFORE Previuos max magnitude: %.2f", previous_max_magnitude);
            // ESP_LOGI(TAG, "BEFORE Max frequency magnitude: %.2f", max_frequency_magnitude);
            // ESP_LOGI(TAG, "BEFORE Max observed frequency magnitude: %.2f", max_observed_frequency_magnitude);
            if (increasing) {
                if (max_frequency_magnitude > previous_max_magnitude && current_sampling_rate < max_sampling_rate) {
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
    // ESP_LOGI(TAG, "AFTER Previuos max magnitude: %.2f", previous_max_magnitude);
    // ESP_LOGI(TAG, "AFTER Max frequency magnitude: %.2f", max_frequency_magnitude);
    // ESP_LOGI(TAG, "AFTER Max observed frequency magnitude: %.2f", max_observed_frequency_magnitude);


            //ESP_LOGI(TAG, "Sampling after adjusting: %d Hz", current_sampling_rate);

           // previous_max_magnitude = max_frequency_magnitude; // Update previous max magnitude
            // dynamic_threshold = max_observed_frequency_magnitude * THRESHOLD_MULTIPLIER; // Update the dynamic threshold
            iterations++; 
            //  ESP_LOGI(TAG, "Number of iterations: %d", iterations);
            //  ESP_LOGI(TAG, "Improvements counter: %d", no_improvement_counter);

            ESP_LOGI(TAG, "Reanalyzing with new parameters...");
    }

    // Log optimization reached after exiting the loop
    ESP_LOGI(TAG, "Optimization reached with max magnitude: %.2f", max_observed_frequency_magnitude);

    // Output the final results
    ESP_LOGI(TAG, "The optimal sampling rate is: %d Hz", optimal_sampling_rate);
    ESP_LOGI(TAG, "The maximum sampling frequency observed was: %d Hz",  max_sampling_rate_observed);

    // Reset the condition flag and relevant variables for the next analysis cycle
    condition_met = false;
    current_sampling_rate = 600; 
    max_sampling_rate_observed = 600;
    max_observed_frequency_magnitude = 0;
    previous_max_magnitude = 0;

    return optimal_sampling_rate;    
}

float calculate_average_signal(float start_time, float window_length, int optimal_rate) {
    float dt = 1.0 / optimal_rate;
    int num_samples = window_length / dt;
    float sum = 0.0;
     float abs_sum = 0.0; // to store the absolut sum
    
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
    optimal_sampling_rate = 600; // reset for the next cycle
    return average_abs;
    
}

float sample_signal_and_compute_average(float window_length) {
    float start_time = 0.0;
    ESP_LOGI(TAG, "The optimal_rate from FFT to pass is %d", optimal_sampling_rate);
    return calculate_average_signal(start_time, window_length, optimal_sampling_rate);
}