# IoT_Signal_analysis
IoT individual assignment to process signals


## Overview
This project is a modular IoT system designed for signal processing and communication using an ESP32 microcontroller. The system includes functionalities for signal processing using FFT and network communication using MQTT over Wi-Fi. It also includes the securtity and power management modules which are yet to be completed.


## Student

| **Matricola** | **GitHub** |
| :---: | :---: | :---: |
| `2162601` | [![name](https://github.com/b-rbmp/NexxGate/blob/main/docs/logos/github.png)](https://github.com/RobCTs) |



## Features

**Generating signal**: Creation of three different signals to randomly pick.
**Sampling and analyzing**: Fast Fourier Transform (FFT) analysis of a signal.
**Network**: //NOT yet fully functional.// Wi-Fi connectivity and MQTT protocol for data transmission.
**Security**: //NOT yet implemented.// Encryption for data storage and retrival.
**Transmitting**: //NOT yet implemented.// Efficient power usage management.



## Getting Started
### Prerequisites
**ESP32** development board
**USB cable** for connecting the ESP32 to your computer
**ESP-IDF** (Espressif IoT Development Framework) installed on your system
**Python** 3.6 or later

### Installation
a) Set up the ESP-IDF environment:
Follow the official ESP-IDF setup guide: [ESP-IDF Get Started](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/get-started/index.html).

b) Clone the repository:
```
git clone --recurse-submodules https://github.com/RobCTs/IoT_Signal_analysis
```

c) Configure the project:
Go to the esp-idf folder
```
cd /your/path/esp-idf
```
and run (BEWARE! This are the commands for Windows.)
```
install.bat
export.bat
```
Then, go the project directory
```
cd /your/path/IoT_Signal_analysis
```
and run
```
idf.py menuconfig
(or in alternative can suffice also)
idf.py reconfigure
```

d) Build the project:
To build the project, run the following command in the project directory:

```
idf.py build
```

e) Flashing the firmware
To flash the compiled firmware onto your ESP32, run:

```
idf.py -p flash

```

f) Running the System
After flashing, monitor the output using:

```
idf.py monitor
```

(BEWARE! If not properly configure, you may specify the PORT used, for example: idf.py -p [PORT] monitor.)
(# Replace [PORT] with the appropriate serial port for your ESP32!)

e-f)
You can also run simultaneously:

```
idf.py flash monitor
```

g) You will be prompt from the serial to input some info, for example the wifi credentials.



## Project Structure
The project is organized into several modules, each handling a specific functionality. Here is an overview of the directory structure:

```
.
├── components/
├── main
│   ├── CMakeLists.txt
│   ├── generating_signal.c
│   ├── generating_signal.h
│   ├── idf_component.yml
│   ├── main.c
│   ├── network.c
│   ├── network.h
│   ├── sampling_and_analyzing.c
│   ├── sampling_and_analyzing.h
│   ├── security.c
│   ├── security.h
│   ├── transmitting.c
│   └── transmitting.h
├── managed_components/
├── .gitignore
├── .gitmodules
├── CMakeLists.txt
├── dependencies.lock
├── README.md
└── sdkconfig
```


## Technical Details and System Design

### System Overview
This project utilizes an ESP32 microcontroller to generate, sample, analyze, and transmit signal data. The ESP32's capabilities allow for high-resolution data handling and communication, making it ideal for real-time signal processing and IoT applications.

### Maximum and Optimal Sampling Frequency
#### MSF
**Capability**: By manual, the ESP32 is capable of sampling at a frequency up to 2 MHz. This allows for precise data capture, essential for accurately representing high-frequency components in the signal analysis.
***Utilization**: While the device can sample up to 2 MHz, the actual sampling rate used is dynamically adjusted based on the signal content to optimize processing power and storage.The maximum sampling frequency of the ESP32 used in this project is XX kHz. This high sampling rate demonstrates the capability of the system to handle fast signal acquisition.

#### OSF
**Determination**: The optimal sampling rate is dynamically calculated based on the Fast Fourier Transform (FFT) analysis of the signal. The system adjusts the sampling rate to ensure it meets the Nyquist criterion, which states that the sampling frequency must be at least twice the highest frequency present in the signal to avoid aliasing.
**Adjustment Process**: The sampling rate starts at a higher rate and is adjusted downwards or upwards based on the FFT results and a predefined threshold to find the optimal rate that captures all relevant frequencies without unnecessary oversampling.

### Signal Processing
The system employs kissFFT, a lightweight **FFT** library, to transform time-domain signals into their frequency components. This transformation helps in identifying the dominant frequencies within the signal which guides the sampling rate adjustment.
A threshold multiplier is applied to dynamically adjust the decision threshold for sampling rate changes, based on the observed signal characteristics.

### Aggregate Function Over a Window
Computing the average of the signal over a specified window (default of 5 seconds) serves to smooth out transient noise and provide a stable measure of the signal's characteristics over that period.The average is calculated both as a simple mean and as a mean of absolute values, the latter providing insights into the overall energy of the signal irrespective of its direction.
This aggregate value provides a meaningful representation of the signal over time and reduces the volume of data to be transmitted.

### Communication with Edge Server
MQTT is used for data transmission to leverage its lightweight and efficient publish/subscribe model, which is suitable for IoT devices with limited bandwidth and power. Moreover, only aggregate data metrics are transmitted, reducing the amount of data sent. Data integrity and confidentiality are ensured through SSL encryption, safeguarding data during transmission to the edge server.
 
### Error Handling
ESP32's task watchdog timers are utilized to ensure system responsiveness and recovery from potential deadlocks or excessive processing delays. Through out the code the ESP_LOG features are extensively used for debugging and tracking the system's operational status, helping in quick identification and resolution of issues during development and deployment.

### Future Enhancments and Security Measures
The MQTT Communication needs to be optimize and properly set. The project also plans to integrate Advanced Encryption Standard (AES) for secure data encryption using Cipher Block Chaining (CBC) mode. I am actively developing functionalities to measure power consumption, memory utilization, and system latency. These tools will be integrated directly into the system to allow real-time monitoring and optimization. The implementation of these features is in progress


### System Performance
(As previously stated, not yet implemented no numbers yet to analyze.)
**Energy Savings**: The adaptive sampling frequency should results in significant energy savings compared to the original oversampled frequency.
**Data Volume**: The volume of data transmitted over the network is reduced by using the avarage, as only the aggregate values are sent instead of raw high-frequency data.
**End-to-End Latency**: The latency of the system, from data generation to reception by the edge server, if properly handled should ensure timely data delivery.



## License
This project is licensed under the MIT License - see the  [LICENSE](https://github.com/RobCTs/IoT_processing_signals/blob/main/LICENSE) file for details.
