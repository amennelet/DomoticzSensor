#include <AZ3166WiFi.h>
#include "Sensor.h"
#include "MQTTClient.h"
#include "MQTTNetwork.h"

const int loopDelay = 60000;
bool hasWifi = false;

bool hasSensors = false;
DevI2C *ext_i2c;
HTS221Sensor *ht_sensor;
LPS22HBSensor *pressureSensor;
float temperature = 0;
float humidity = 0;
float pressure = 0;

const char *mqttServer = "domoticz";
int port = 1883;
const char *clientID = "InternalSensors";
const char *topic = "domoticz/in";
int idx = 95;
const int updateEveryNLoop = 10;

void initWifi()
{
    Screen.clean();
    Screen.print(0, "Wi-Fi Connecting");
    Serial.print("Attempting to connect to Wi-Fi");

    if (WiFi.begin() == WL_CONNECTED)
    {
        IPAddress ip = WiFi.localIP();
        Screen.print(0, "Wi-Fi Connected");
        Screen.print(1, ip.get_address());
        hasWifi = true;
        Screen.print(2, "Running... \r\n");
    }
    else
    {
        Screen.print(1, "No Wi-Fi\r\n ");
    }
}

void initSensors()
{
    Screen.clean();
    Screen.print(0, "Init sensors");
    ext_i2c = new DevI2C(D14, D15);
    ht_sensor = new HTS221Sensor(*ext_i2c);
    pressureSensor = new LPS22HBSensor(*ext_i2c);
    hasSensors = ht_sensor->init(NULL) == 0 || pressureSensor->init(NULL) == 0;
    if (hasSensors)
        Screen.print(1, "Init sensors done");
    else
        Screen.print(1, "Init sensors failed");
}

void setup()
{
    Serial.begin(115200);
    initWifi();
    if (hasWifi)
        initSensors();
}

void getHumidTempSensor()
{
    ht_sensor->reset();
    ht_sensor->getTemperature(&temperature);
    ht_sensor->getHumidity(&humidity);
    pressureSensor->getPressure(&pressure);
}

void showHumidTempSensor()
{
    char buff[128];
    snprintf(buff, 128, "Environement\r\nPress: %s\r\nTemp:%sC\r\nHumid:%s%%\r\n", f2s(pressure, 2), f2s(temperature, 1), f2s(humidity, 1));
    Screen.print(buff);
}

int sendMQTT(char *topic, char *payload)
{
    MQTTNetwork mqttNetwork;
    MQTT::Client<MQTTNetwork, Countdown> client = MQTT::Client<MQTTNetwork, Countdown>(mqttNetwork);

    char msgBuf[100];
    sprintf(msgBuf, "Connecting to MQTT server %s:%d", mqttServer, port);
    Serial.println(msgBuf);

    int rc = mqttNetwork.connect(mqttServer, port);
    if (rc != 0)
    {
        Serial.println("Connected to MQTT server failed");
        return -1;
    }
    else
    {
        Serial.println("Connected to MQTT server successfully");
    }

    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
    data.MQTTVersion = 3;
    data.clientID.cstring = (char *)clientID;

    if ((rc = client.connect(data)) != 0)
    {
        Serial.println("MQTT client connect to server failed");
        return -1;
    }

    MQTT::Message message;
    message.qos = MQTT::QOS0;
    message.retained = false;
    message.dup = false;
    message.payload = (void *)payload;
    message.payloadlen = strlen(payload) + 1;
    rc = client.publish(topic, message);

    if ((rc = client.disconnect()) != 0)
    {
        Serial.println("MQTT client disconnect from server failed");
        return -1;
    }

    mqttNetwork.disconnect();
    Serial.print("Finish message count: ");
    return 0;
}

void createPressTempHumidPayload(int idx, char *payload)
{
    // idx=IDX&nvalue=0&svalue=TEMP;HUM;HUM_STAT;BAR;BAR_FOR
    sprintf(payload, "{\"idx\":%u, \"nvalue\":0, \"svalue\":\"%s;%s;0;%s;0\"}", idx, f2s(temperature, 1), f2s(humidity, 1), f2s(pressure, 2));
}

int loopCounter = 0;

void loop()
{
    Serial.println("\r\n>>>Enter Loop");

    if (hasWifi && hasSensors)
    {
        getHumidTempSensor();
        showHumidTempSensor();
        if (loopCounter == 0)
        {
            Serial.println("Sending MQTT");
            char payload[100];
            createPressTempHumidPayload(idx, payload);
            if (sendMQTT((char *)topic, payload) == 0)
                Serial.println("sendMQTT done");
            else
                Serial.println("sendMQTT failed");

            loopCounter = updateEveryNLoop;
        }
    }

    delay(loopDelay);
    loopCounter--;
}