#include "generating_signal.h"
#include <stdint.h> 
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <stdarg.h>
#include <signal.h>
#include <time.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_system.h"


#define PI 3.14159265358979323846
#define SAMPLING_RATE 40 // sampling rate in Hz; The number of samples taken per second from a continuous signal to make a discrete signal..
#define SIGNAL_LENGTH 9  // length of signal in seconds

// Queue handle
QueueHandle_t signalQueue = NULL;
#define QUEUE_SIZE 1000  // This needs to be at least as large as SAMPLING_RATE * SIGNAL_LENGTH

// Define tags for ESP logging
static const char* TAG = "SignalGenerator";
int chosenSignal = -1;  // globally track the current signal function index

// Signal generation functions
float signal1(float t) {
    return 2 * sin(2 * PI * 3 * t) + 4 * sin(2 * PI * 5 * t);
    //Frequencies between 3 Hz and 5 Hz
}

float signal2(float t) {
    return 3 * sin(2 * PI * 2 * t) + 2 * sin(2 * PI * 8 * t);
    //Frequencies between 2 Hz and 8 Hz
}

float signal3(float t) {
    return 5 * sin(2 * PI * 1 * t) + 1 * sin(2 * PI * 10 * t);
    //Frequencies between 1 Hz and 10 Hz
    //Highest frequency is 10 Hz
    //Min. sampling rate frequency: 2x10 Hz = 20 Hz -> raccomanded 40 Hz
}

// Array of function pointers
float (*signalFunctions[3])(float) = {signal1, signal2, signal3};


void setup_signal_queue() {
    signalQueue = xQueueCreate(QUEUE_SIZE, sizeof(float));
    if (signalQueue == NULL) {
        ESP_LOGE(TAG, "Failed to create queue");
    }
}

void generate_random_signal() {
    chosenSignal = rand() % 3;  // randomly choose between 0, 1, and 2

    float dt = 1.0 / SAMPLING_RATE;
    ESP_LOGI(TAG, "Generating signal %d", chosenSignal + 1);
    for (int i = 0; i < SAMPLING_RATE * SIGNAL_LENGTH; i++) {
        //"dt" represents the time interval between consecutive samples.
        float t = i * dt;
        float signal = signalFunctions[chosenSignal](t);
        if (xQueueSend(signalQueue, &signal, 0) != pdPASS) {
            ESP_LOGE(TAG, "Failed to send data to the queue");
        }
        if (i == 0 || i == (SAMPLING_RATE * SIGNAL_LENGTH) - 1) {  // limit logging to reduce delay
            ESP_LOGI(TAG, "t = %.3f, signal = %.3f", t, signal);}
        }
    }

//"t" represents the specific time at which the signal value is being computed
float get_signal_at(float t) {
    //ESP_LOGI(TAG, "chosenSignal is now %d", chosenSignal + 1);
    if (chosenSignal == -1) {
        ESP_LOGE(TAG, "No signal has been selected yet.");
        return 0;
    }
    float signal_value = signalFunctions[chosenSignal](t); // store the result in a variable
    // ESP_LOGI(TAG, "The signal at time %.3f is now = %.3f", t, signal_value); 
    return signal_value; // Return the stored value
}
