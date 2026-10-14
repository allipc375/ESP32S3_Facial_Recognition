# Low-Cost Facial Recognition System with ESP32-S3 and Neural Networks

ESP-DL (Espressif Deep Learning Library) is Espressif’s lightweight, high‑performance neural network inference framework designed specifically for ESP32‑series chips, enabling AI applications directly on devices like the ESP32‑S3 microcontroller

It is optimized for low power, low cost, and constrained embedded environments, making it ideal for AIoT (AI + Internet of Things) use cases such as object detection, image classification, and tracking​. In the context of facial recognition, ESP-DL is currently limited in these aspects

  1) Less customization properties 
  2) Supports a limited set of pre‑configured models
 

## Objectives:

- Test the OV3660 camera and resolution abilities with Arduino IDE
- Use VS Code with a PlatformIO extension to store code
- Run ESP-DL with the microcontroller to test facial recognition via a known method
- Use neural networks (MobileFaceNet for facial recognition and ArcFace for training)
- Implementing neural networks as much as possible on the board rather than solely the computer​ to test the microcontroller's capabilities
- Solder on a 3.7 Lithium-Ion battery to the microcontroller for remote use and testing
- Model and print a 3D enclosure for the system to consolidate all the elements and provide a complete project 


## General Goals:
- Compare Espressif's built-in recognition model against a MobileFaceNet model trained with ArcFace loss
- Explore whether different training data, embedding sizes, or model configurations improve recognition for my intended conditions
- Demonstrate model conversion, quantization, inference optimization, and quantitative evaluation


## Conclusion/Results:
This project is currently in development for the Fall 2026 semester.
