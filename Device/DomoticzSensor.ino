#include <AZ3166WiFi.h>

bool hasWifi = false;

void initWifi()
{
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

void setup()
{
    Serial.begin(115200);
    initWifi();
}

void loop()
{
    Serial.println("\r\n>>>Enter Loop");

    if (hasWifi)
    {
        Screen.clean();
        Screen.print(0, "Wellcome Alex");
    }

    delay(5000);
}