# FPGA Laser Communication System

An FPGA-based optical wireless communication system implemented on the Nexys A7 platform using laser transmission, photodiode reception, real-time signal acquisition, and custom FPGA processing. This project explores visible-light/laser-based digital communications using FPGA hardware acceleration, PWM modulation techniques, analog signal acquisition through the XADC, and custom receiver decoding logic.


<img width="2529" height="2160" alt="IMG_1851" src="https://github.com/user-attachments/assets/e232dcda-5aa0-4eb8-a53e-c1526a42be85" />


## Project Overview

This system transmits digital data through a laser beam and receives the optical signal using a photodiode sensor connected to the FPGA’s analog-to-digital converter (XADC). The design investigates reliable short-range optical communications using FPGA-based timing control, modulation, synchronization, and signal decoding.

The project focuses on:
- FPGA-based laser modulation
- Optical signal reception using a photodiode
- PWM/PAM-style symbol encoding
- Real-time ADC sampling
- Brightness threshold decoding
- Wireless optical communication concepts
- Embedded C++ control software in Vitis
- Hardware/software co-design

## Hardware Components

- Digilent Nexys A7 FPGA Board
- Laser diode module
- Photodiode receiver module
- XADC analog input interface
- PMOD/GPIO output connections
- Optional optical filters/diffusers

## System Architecture

### Transmitter
The FPGA generates modulated PWM signals that drive the laser diode. Symbol timing and duty cycle variations represent transmitted digital information.

### Receiver
The photodiode converts incoming laser intensity into analog voltage levels. The FPGA XADC samples the signal and performs:
- brightness measurement
- threshold comparison
- symbol classification
- real-time signal monitoring

## Modulation Technique

The project explores PAM-style intensity modulation where different laser brightness levels represent digital symbols.

Example symbol mapping:

| Symbol | Relative Intensity |
|---|---|
| 00 | Off |
| 01 | Low |
| 10 | Medium |
| 11 | High |

The receiver samples incoming optical intensity levels and classifies symbols using threshold ranges.

## FPGA Features Used

- Xilinx XADC
- GPIO cores
- PWM generation
- Hardware timing logic
- Finite state machines
- UART debugging interface

## Software Stack

- Vivado
- Vitis
- Embedded C++
- Chu FPGA MMIO framework
- Custom FPGA peripheral drivers

## Current Functionality

- Laser PWM transmission
- Photodiode analog acquisition
- Real-time brightness measurement
- Symbol threshold decoding
- UART debugging output
- Adjustable symbol timing
- Experimental optical communication testing

## Future Improvements

Future work for this project includes:
- Implementing a complete custom communication protocol with dedicated start/stop synchronization words
- Adding DSP-based averaging and filtering for improved receiver stability
- Implementing error detection and correction techniques
- Developing adaptive threshold detection for changing lighting conditions
- Creating packet-based data framing
- Improving optical synchronization reliability
- Adding VGA visualization/debugging tools
- Exploring higher-order modulation techniques
- Increasing transmission distance and robustness
- Implementing FPGA DSP slices for signal smoothing and symbol recovery

## Educational Topics Demonstrated

This project combines concepts from:
- Digital communications
- FPGA design
- Embedded systems
- Optical communications
- Signal acquisition
- Hardware/software integration
- Real-time systems
- PWM modulation

## Repository Structure

```text
/src            -> HDL and C++ source files
/vivado         -> Vivado project files
/vitis          -> Vitis software workspace
/docs           -> Documentation and diagrams
/testbench      -> Simulation files
