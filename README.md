# Genetic Algorithm for Rescue Operations

## Project Description
This project implements a genetic algorithm-based application to optimize rescue operations in collapsed buildings using multi-processing and IPC techniques in Linux.

## Team Information
- **Course**: ENCS4330 - Real-Time Applications & Embedded Systems
- **Instructor**: Dr. Hanna Bullata
- **Due Date**: December 12, 2025
- **University**: Birzeit University

## Project Overview
The application optimizes paths for tiny rescue robots to deliver nutrition and beverages to trapped survivors in a 3D collapsed building environment. It uses genetic algorithms with multi-processing to find efficient paths that maximize survivor retrieval while minimizing time and risks.

## Features
- 3D grid environment modeling
- Genetic algorithm optimization
- Multi-processing with IPC (Shared Memory + Semaphores)
- Configurable parameters via config file
- Collision detection and risk assessment
- Elitism preservation (top 10% solutions)

## System Requirements
- Linux operating system (Ubuntu/Debian recommended)
- GCC compiler (version 7.0 or higher)
- Make utility
- GDB debugger (for development)
- POSIX threads and IPC support

## Project Structure