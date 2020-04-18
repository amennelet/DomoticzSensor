#include <AZ3166WiFi.h>
#include "Sensor.h"
#include "MQTTClient.h"
#include "MQTTNetwork.h"
#include "AudioClass.h"

const int loopDelay = 10000;
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
int idxPressTempHumid = 95;

AudioClass &Audio = AudioClass::getInstance();
const int SAMPLE_DURATION = 3;
const int AUDIO_SIZE = 32000 * SAMPLE_DURATION + 45;
char *waveFile;
int totalSize;
int currentSoundVolume = 0;
int idxSound = 96;

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

void initSound()
{
    waveFile = (char *)malloc(AUDIO_SIZE + 1);
    memset(waveFile, 0x0, AUDIO_SIZE);
}

void setup()
{
    Serial.begin(115200);
    initWifi();
    if (hasWifi)
    {
        initSensors();
        initSound();
    }
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
    snprintf(buff, 128, "Press: %s\r\nTemp:%sC\r\nHumid:%s%%\r\n", f2s(pressure, 2), f2s(temperature, 1), f2s(humidity, 1));
    Screen.print(buff);
}

void getCurrentSoundVolume()
{
    Serial.println("Start getCurrentSoundVolume");
    Audio.format(8000, 16);
    if (Audio.startRecord(waveFile, AUDIO_SIZE, 3) != 0)
    {
        Serial.println("Audio.startRecord failed");
        return;
    }
    while (Audio.getAudioState() == AUDIO_STATE_RECORDING)
    {
        delay(100);
    }
    Audio.getWav(&totalSize);
    Serial.printf("Stereo size = %i\r\n", totalSize);

    int totalSample = 0;
    double sum = 0;
    Serial.printf("First sample at = %i\r\n", sizeof(WaveHeader));
    for (int index = sizeof(WaveHeader); index < totalSize; index += 2)
    {
        uint16_t value = ((uint16_t)waveFile[index]) | (((uint16_t)waveFile[index + 1]) << 8);
        sum += value * value;
        totalSample++;
    }
    currentSoundVolume = (int)sqrt(abs(sum) / totalSample);
    Serial.printf("Current Sound Volume = %i (%i samples, %f sum)\r\n", currentSoundVolume, totalSample, sum);

    Audio.startPlay(waveFile, totalSize);
    while (Audio.getAudioState() == AUDIO_STATE_PLAYING)
    {
        delay(100);
    }
}

void showSoundSensor()
{
    char buff[128];
    snprintf(buff, 128, "Sound: %i\r\n", currentSoundVolume);
    Screen.print(3, buff);
}

void createSoundPayload(int idx, char *payload)
{
    // idx=IDX&nvalue=0&svalue=TEMP;HUM;HUM_STAT;BAR;BAR_FOR
    sprintf(payload, "{\"idx\":%u, \"nvalue\":0, \"svalue\":\"%i\"}", idx, currentSoundVolume);
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

void loop()
{
    Serial.println("\r\n>>>Enter Loop");

    if (hasWifi && hasSensors)
    {
        getCurrentSoundVolume();
        getHumidTempSensor();
        showHumidTempSensor();
        showSoundSensor();

        Serial.println("Sending sensors MQTT");
        char payload[100];
        createPressTempHumidPayload(idxPressTempHumid, payload);
        if (sendMQTT((char *)topic, payload) == 0)
            Serial.println("sendMQTT sensors done");
        else
            Serial.println("sendMQTT sensors failed");

        Serial.println("Sending sound MQTT");
        createSoundPayload(idxSound, payload);
        if (sendMQTT((char *)topic, payload) == 0)
            Serial.println("sendMQTT sound done");
        else
            Serial.println("sendMQTT sound failed");
    }

    delay(loopDelay);
}