Most of driver code is taken from https://github.com/igrr/esp32-cam-demo by Ivan Grokhotkov

This sketch demonstrates how to get http server running and serving 320*240 images with a $2 aliexpress OV7670 camera (no-fifo, there isn't ayny issue with serving larger images with this camera that has buffering)

Original drivers were modified a bit.
Make sure you have proper esp32 SDK installed for your Arduino IDE.
Demo sends "MOTION" text over mosquitto message broker when motion is detected by sensor.
