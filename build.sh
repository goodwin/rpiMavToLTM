#gcc -Wall -fPIC -shared Led.cpp -lwiringPi -I/home/pi/rpi_ws281x -I/home/pi/rpiMavToLTM/AP_Common -I/home/pi/rpiMavToLTM/AP_Math -I/home/pi/rpiMavToLTM/GCS_MAVLink -I/home/pi/rpiMavToLTM/GCS_MAVLink/include/mavlink/v1.0 -I/home/pi/rpiMavToLTM/GCS_MAVLink/include/mavlink/v1.0/common -lm -lpthread -lws2811 -lstdc++ -std=c++11 -o libLed.so
gcc -Wall main.cpp -lwiringPi -I/home/pi/rpi_ws281x -I/home/pi/rpiMavToLTM/AP_Common -I/home/pi/rpiMavToLTM/AP_Math -I/home/pi/rpiMavToLTM/GCS_MAVLink -I/home/pi/rpiMavToLTM/GCS_MAVLink/include/mavlink/v1.0 -I/home/pi/rpiMavToLTM/GCS_MAVLink/include/mavlink/v1.0/common -lm -lpthread -lws2811 -std=c++11 -lstdc++ -o rpiMavToLTM
