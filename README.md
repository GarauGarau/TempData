# ESP32 Sensor Dashboard

This repository contains the Arduino code and web components for a project that integrates an ESP32 microcontroller with Bosch sensors to measure humidity, temperature, and Indoor Air Quality (IAQ). The system captures real-time sensor data and visualizes it through an interactive web dashboard. The project demonstrates a full-stack IoT solution, including data collection, cloud integration, and web-based presentation.
How It Works
Data Collection

# The ESP32 microcontroller reads data from Bosch sensors.
Supported measurements include:
   - Humidity
   - Temperature
   - IAQ (Indoor Air Quality Index)

# Data Transmission

The ESP32 sends the sensor data to a Google Apps Script Web App endpoint.
The web app processes the incoming data and appends it to a Google Sheet, effectively creating a real-time database.

# Data Visualization

The data from the Google Sheet is fetched and displayed on a custom web dashboard, providing a user-friendly interface for monitoring the collected data in real-time. The dashboard is hosted on GitHub Pages and is freely accessible at: https://garaugarau.github.io/TempData/
