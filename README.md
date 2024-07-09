## โครงการการเชื่อมต่ออุปกรณ์กับ IoT เพื่อประยุกต์ใช้กับ OASYS (Amazon)

### Main feature
- ESP32 can sub and pub simultaneously.
- ESP32 can pub the temperature and humidity to the dashboard by using MQTT protocol.
- ESP32 communicates to the iTSD through a serial port.
- ESP32 connects to free cloud of HIVEMQ.
- ESP32 can reconnect to the broker(server) and resubscribe when it fails to connect. If our ESP32 cannot connect to the broker, we can identify the cause of failure.

### Added feature
- We set ESP32 to be a client and server simultaneously. (WiFi mode and Station mode)
- We can input SSID and password of WiFi so as to make ESP32 connecting to our wifi network. (We have a webpage to input SSID and password by having ESP32 as a webserver.)
- We can upload firmware to our ESP32 board with an OTA way. However, Uploader device must have the same wifi network with the ESP32.
- We can reset the SSID and password of wifi by pressing button that we designed.
