Very simple library that converts joystick events to kafka topics

Install Dependencies
```
apt install librdkafka-dev
```
Configuration file
```
{
    "kafkaIp" : "localhost",
    "kafkaPort" : 9094,
    "kafkaTopic" : "joystick-event",
    "joystick" : "/dev/input/js0"
}
```
Kafka output is a json packet in the following format
```
{"isAxis":true,"number":4,"time":676967964,"value":1036}
```
isAxis - true indicates a analog button or stick
number - the button or axis
time - timestamp in ms
value - the sensor value. 0 to 1 for buttons
