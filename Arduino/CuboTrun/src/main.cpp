#include <Arduino.h>
#include <SPI.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <MicroOscUdp.h>
#include <Adafruit_BNO055.h>
#include <NeoPixelBus.h>

// neopixel


void wifiEvent(WiFiEvent_t event);
void updateOsc(MicroOscMessage& oscMessage);
void sendConnected();
void updateBno(uint32_t currentTime);
void updateLed(uint32_t currentTime, int state);
void fillNeopixel(uint8_t r, uint8_t g, uint8_t b);

uint8_t id = 0;
const int ID_OFFSET = 60;
const int ID_PIN[] = {36, 39};
// WiFi
const char* ssid = "FINALBY-g";
const char* password = "733kifnthx37t";
IPAddress ip(10, 0, 0, ID_OFFSET);
const IPAddress gateway(10, 0, 0, 1);
const IPAddress subnet(255, 255, 255, 0);
const IPAddress broadcastIp(255, 255, 255, 255);

// OSC
WiFiUDP udp;
int dstPort = 50000 + ID_OFFSET;
const int srcPort = 50000;
IPAddress dstIp(10, 0, 0, 10);
MicroOscUdp<2048> microOsc(&udp, dstIp, dstPort);

// led
uint32_t LED_INTERVAL_DEFAULT = 200;
uint32_t LED_INTERVAL_WIFI = 1000;
int LED_PIN = 33;
bool ledState = false;
uint32_t ledTimestamp = 0;

// bno055
const int BNO_SDA = 21;  // 21
const int BNO_SCL = 22;  // 22
const int BNO_INTERVAL = 10;
const int BNO_CALIBRATION_INTERVAL = 1000;
Adafruit_BNO055 bno = Adafruit_BNO055(55, 0x28, &Wire);
bool bSendBno = true;
bool bSendBnoRaw = false;
uint32_t bnoTimestamp;
uint32_t bnoCalibrationTimestamp;
uint8_t calibration[4];

// neo pixel
const uint8_t NUM_NEOPIXEL = 7;
const int NEOPIXEL_PIN = 14;

NeoPixelBus<NeoGrbFeature, NeoWs2812xMethod> strip(NUM_NEOPIXEL, NEOPIXEL_PIN);


float mapfloat(float x, float in_min, float in_max, float out_min,
               float out_max) {
    return (float)(x - in_min) * (out_max - out_min) /
               (float)(in_max - in_min) +
           out_min;
}
float mapfloatexp(float x, float in_min, float in_max, float out_min,
                  float out_max) {
    return out_min * pow(2.0, ((float)(x - in_min) / (float)(in_max - in_min)) *
                                  log(out_max / out_min) / log(2.0));
}

void setup() {
    Serial.begin(115200);
    pinMode(LED_PIN, OUTPUT);
    for (uint8_t i = 0; i < 2; i++) {
        pinMode(ID_PIN[i], INPUT);
    }
    delay(10);
    for (uint8_t i = 0; i < 2; i++) {
        id += digitalRead(ID_PIN[i]) << i;
    }
    // We start by connecting to a WiFi network
    ip[3] += id;
    dstPort += id;

    Serial.println();
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);

    WiFi.onEvent(wifiEvent);
    WiFi.setAutoReconnect(true);
    WiFi.config(ip, gateway, subnet);
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());

    Wire.begin(BNO_SDA, BNO_SCL, 400000);
    if (!bno.begin()) {
        /* There was a problem detecting the BNO055 ... check your connections
         */
        Serial.print(
            "Ooops, no BNO055 detected ... Check your wiring or I2C ADDR!");
        while (1);
    }

    delay(1000);
    /* Use external crystal for better accuracy */
    bno.setExtCrystalUse(true);

    strip.Begin();
}

void loop() {
    uint32_t currentTime = millis();
    int currentState = 0;
    if (WiFi.status() == WL_CONNECTED) {
        microOsc.onOscMessageReceived(updateOsc);
        currentState = 1;
    }

    updateBno(currentTime);
    updateLed(currentTime, currentState);
}
void wifiEvent(WiFiEvent_t event) {
    switch (event) {
        case SYSTEM_EVENT_STA_GOT_IP:
            // When connected set
            Serial.print("WiFi connected! IP address: ");
            Serial.println(WiFi.localIP());
            Serial.print("srcPort: ");
            Serial.println(srcPort);
            Serial.print("dstPort: ");
            Serial.println(dstPort);
            // initializes the UDP state
            // This initializes the transfer buffer
            udp.begin(WiFi.localIP(), srcPort);
            sendConnected();
            break;
        case SYSTEM_EVENT_STA_DISCONNECTED:
            Serial.println("WiFi lost connection");
            break;
        default:
            break;
    }
}
void sendConnected() {
    //   OSCMessage msg("/connected");
    //   msg.add((int32_t)id);
    //   udp.beginPacket(broadcastIp, dstPort);
    //   msg.send(udp);
    //   udp.endPacket();
    //   msg.empty();

    microOsc.setDestination(broadcastIp, dstPort);
    microOsc.sendInt("/connected", id);
}

void updateOsc(MicroOscMessage& oscMessage) {
    // CHECK THE ADDRESS OF THE OSC MESSAGE
    if (oscMessage.checkOscAddress("/setDstIp")) {
        dstIp = udp.remoteIP();
        microOsc.setDestination(dstIp, dstPort);
        microOsc.sendInt("/dstIp", id);
    } else if (oscMessage.checkOscAddress("/ping")) {
        bool bMatch = true;
        for (uint8_t i = 0; i < 4; i++) {
            if (dstIp[i] != udp.remoteIP()[i]) {
                bMatch = false;
            }
        }
        microOsc.setDestination(udp.remoteIP(), dstPort);
        microOsc.sendInt("/pong", (int32_t)bMatch);
        microOsc.setDestination(dstIp, dstPort);
    } else if (oscMessage.checkOscAddress("/sendQuat")) {
        int val = oscMessage.nextAsInt();
        if (val) {
            bSendBno = true;
        } else {
            bSendBno = false;
        }
    } else if (oscMessage.checkOscAddress("/sendImuRaw")) {
        int val = oscMessage.nextAsInt();
        if (val) {
            bSendBnoRaw = true;
        } else {
            bSendBno = false;
        }
    } else if (oscMessage.checkOscAddress("/neopixelFill")) {
        int r = oscMessage.nextAsInt();
        int g = oscMessage.nextAsInt();
        int b = oscMessage.nextAsInt();
        fillNeopixel(r, g, b);
    }
}
void updateBno(uint32_t currentTime) {
    if (BNO_INTERVAL <= currentTime - bnoTimestamp) {
        if (bSendBno) {
            sensors_event_t event;
            bno.getEvent(&event);
            imu::Quaternion quat = bno.getQuat();

            microOsc.setDestination(dstIp, dstPort);
            microOsc.sendMessage("/quat", "ffffi", (float)quat.x(),
                                 (float)quat.y(), (float)quat.z(),
                                 (float)quat.w(), (int32_t)event.timestamp);
        }
        if (bSendBnoRaw) {
            sensors_event_t orientationData, angVelocityData, linearAccelData,
                magnetometerData, accelerometerData, gravityData;
            bno.getEvent(&orientationData, Adafruit_BNO055::VECTOR_EULER);
            bno.getEvent(&angVelocityData, Adafruit_BNO055::VECTOR_GYROSCOPE);
            bno.getEvent(&linearAccelData, Adafruit_BNO055::VECTOR_LINEARACCEL);
            bno.getEvent(&magnetometerData,
                         Adafruit_BNO055::VECTOR_MAGNETOMETER);
            bno.getEvent(&accelerometerData,
                         Adafruit_BNO055::VECTOR_ACCELEROMETER);
            bno.getEvent(&gravityData, Adafruit_BNO055::VECTOR_GRAVITY);
            microOsc.setDestination(dstIp, dstPort);
            microOsc.sendMessage("/imu", "ffffffffffffffffffi",
                                 (float)orientationData.orientation.x,
                                 (float)orientationData.orientation.y,
                                 (float)orientationData.orientation.z,
                                 // SENSOR_TYPE_GYROSCOPE
                                 (float)angVelocityData.gyro.x,
                                 (float)angVelocityData.gyro.y,
                                 (float)angVelocityData.gyro.z,
                                 // SENSOR_TYPE_LINEAR_ACCELERATION
                                 (float)linearAccelData.acceleration.x,
                                 (float)linearAccelData.acceleration.y,
                                 (float)linearAccelData.acceleration.z,
                                 // SENSOR_TYPE_MAGNETIC_FIELD
                                 (float)magnetometerData.magnetic.x,
                                 (float)magnetometerData.magnetic.y,
                                 (float)magnetometerData.magnetic.z,
                                 // SENSOR_TYPE_ACCELEROMETER
                                 (float)accelerometerData.acceleration.x,
                                 (float)accelerometerData.acceleration.y,
                                 (float)accelerometerData.acceleration.z,
                                 // SENSOR_TYPE_GRAVITY
                                 (float)gravityData.acceleration.x,
                                 (float)gravityData.acceleration.y,
                                 (float)gravityData.acceleration.z,
                                 (int32_t)orientationData.timestamp);
        }
        bnoTimestamp = currentTime;
    }
}
void updateLed(uint32_t currentTime, int state) {
    switch (state) {
        case 0:
            if (LED_INTERVAL_DEFAULT <= currentTime - ledTimestamp) {
                ledState = !ledState;
                // Serial.println(state);
                // Serial.println(ledState);
                digitalWrite(LED_PIN, ledState);
                ledTimestamp = currentTime;
            }
            break;
        case 1:
            if (LED_INTERVAL_WIFI <= currentTime - ledTimestamp) {
                ledState = !ledState;
                // Serial.println(state);
                // Serial.println(ledState);
                digitalWrite(LED_PIN, ledState);
                ledTimestamp = currentTime;
            }
            break;
    }
}

void fillNeopixel(uint8_t r, uint8_t g, uint8_t b) {
    for (int i = 0; i < NUM_NEOPIXEL; i++) {
        strip.SetPixelColor(i, RgbColor(r, g, b));
    }
    
    strip.Show();
}