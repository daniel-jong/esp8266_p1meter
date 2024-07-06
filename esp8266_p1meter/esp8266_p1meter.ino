#include <FS.h>
#include <EEPROM.h>
#include <DNSServer.h>
#include <ESP8266WiFi.h>
#include <Ticker.h>
#include <WiFiManager.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>
#include <DoubleResetDetector.h>
#include <string>
#include <vector>
#include <ArduinoJson.h>

using namespace std;

// * Include settings
#include "settings.h"

// * Initiate Double resetDetector
DoubleResetDetector drd(DRD_TIMEOUT, DRD_ADDRESS);

// * Initiate led blinker library
Ticker ticker;

// * Initiate WIFI client
WiFiClient espClient;

// * Initiate MQTT client
PubSubClient mqtt_client(espClient);

// **********************************
// * WIFI                           *
// **********************************

// * Gets called when WiFiManager enters configuration mode
void configModeCallback(WiFiManager *myWiFiManager)
{
    Serial.println(F("Entered config mode"));
    Serial.println(WiFi.softAPIP());

    // * If you used auto generated SSID, print it
    Serial.println(myWiFiManager->getConfigPortalSSID());

    // * Entered config mode, make led toggle faster
    ticker.attach(0.2, tick);
}

// **********************************
// * Ticker (System LED Blinker)    *
// **********************************

// * Blink on-board Led
void tick()
{
    // * Toggle state
    int state = digitalRead(LED_BUILTIN); // * Get the current state of GPIO1 pin
    digitalWrite(LED_BUILTIN, !state);    // * Set pin to the opposite state
}

// **********************************
// * MQTT                           *
// **********************************

// * Send a message to a broker topic
void send_mqtt_message(const char *topic, char *payload)
{
    bool result = mqtt_client.publish(topic, payload, false);

    if (!result)
    {
        Serial.printf("MQTT publish to topic %s failed\n", topic);
    }
}

// * send a Json message to a broker topic
void send_mqtt_message(const char *topic, StaticJsonDocument<256> doc)
{
    char buffer[256]; //max buffer of PubSubClient is 256 byte.
    size_t n = serializeJson(doc, buffer);
    mqtt_client.publish(topic, buffer, n);
    doc.clear(); // clear memory for the Json Document in this method.
}

// * Reconnect to MQTT server and subscribe to in and out topics
bool mqtt_reconnect()
{
    // * Loop until we're reconnected
    int MQTT_RECONNECT_RETRIES = 0;

    while (!mqtt_client.connected() && MQTT_RECONNECT_RETRIES < MQTT_MAX_RECONNECT_TRIES)
    {
        MQTT_RECONNECT_RETRIES++;
        Serial.printf("MQTT connection attempt %d / %d ...\n", MQTT_RECONNECT_RETRIES, MQTT_MAX_RECONNECT_TRIES);

        // * Attempt to connect
        if (mqtt_client.connect(HOSTNAME, MQTT_USER, MQTT_PASS))
        {
            Serial.println(F("MQTT connected!"));

            // * Once connected, publish an announcement...
            char *message = new char[16 + strlen(HOSTNAME) + 1];
            strcpy(message, "p1 meter alive: ");
            strcat(message, HOSTNAME);
            mqtt_client.publish("hass/status", message);

            Serial.printf("MQTT root topic: %s\n", MQTT_ROOT_TOPIC);
        }
        else
        {
            Serial.print(F("MQTT Connection failed: rc="));
            Serial.println(mqtt_client.state());
            Serial.println(F(" Retrying in 5 seconds"));
            Serial.println("");

            // * Wait 5 seconds before retrying
            delay(5000);
        }
    }

    if (MQTT_RECONNECT_RETRIES >= MQTT_MAX_RECONNECT_TRIES)
    {
        Serial.printf("*** MQTT connection failed, giving up after %d tries ...\n", MQTT_RECONNECT_RETRIES);
        return false;
    }

    return true;
}

void send_metric(String name, long metric)
{
    char output[10];
    ltoa(metric, output, sizeof(output));

    String topic = String(MQTT_ROOT_TOPIC) + "/" + name;
    send_mqtt_message(topic.c_str(), output);
}

void send_data_to_broker()
{
    send_metric("consumption_low_tarif", CONSUMPTION_LOW_TARIF);
    send_metric("consumption_high_tarif", CONSUMPTION_HIGH_TARIF);
    send_metric("returndelivery_low_tarif", RETURNDELIVERY_LOW_TARIF);
    send_metric("returndelivery_high_tarif", RETURNDELIVERY_HIGH_TARIF);
    send_metric("actual_consumption", ACTUAL_CONSUMPTION);
    send_metric("actual_returndelivery", ACTUAL_RETURNDELIVERY);

    send_metric("l1_instant_power_usage", L1_INSTANT_POWER_USAGE);
    send_metric("l2_instant_power_usage", L2_INSTANT_POWER_USAGE);
    send_metric("l3_instant_power_usage", L3_INSTANT_POWER_USAGE);
    send_metric("l1_instant_power_current", L1_INSTANT_POWER_CURRENT);
    send_metric("l2_instant_power_current", L2_INSTANT_POWER_CURRENT);
    send_metric("l3_instant_power_current", L3_INSTANT_POWER_CURRENT);
    send_metric("l1_voltage", L1_VOLTAGE);
    send_metric("l2_voltage", L2_VOLTAGE);
    send_metric("l3_voltage", L3_VOLTAGE);

    send_metric("gas_meter_m3", GAS_METER_M3);

    send_metric("actual_tarif_group", ACTUAL_TARIF);
    send_metric("short_power_outages", SHORT_POWER_OUTAGES);
    send_metric("long_power_outages", LONG_POWER_OUTAGES);
    send_metric("short_power_drops", SHORT_POWER_DROPS);
    send_metric("short_power_peaks", SHORT_POWER_PEAKS);

    send_metric("actual_average_15m_peak", mActualAverage15mPeak);
    send_metric("thismonth_max_15m_peak", mMax15mPeakThisMonth);
    send_metric("last13months_average_15m_peak", mAverage15mPeakLast13months);
    send_mqtt_message((String(MQTT_ROOT_TOPIC) + "/" + "last13months_peaks_json").c_str(), Last13MonthsPeaks_json); // bypassed send_metric() for Json message.
    Last13MonthsPeaks_json.clear(); // clear memory for the GLOBAL JsonDocument.
}

// **********************************
// * P1                             *
// **********************************

unsigned int CRC16(unsigned int crc, unsigned char *buf, int len)
{
    for (int pos = 0; pos < len; pos++)
    {
        crc ^= (unsigned int)buf[pos]; // * XOR byte into least sig. byte of crc

        for (int i = 8; i != 0; i--) // * Loop over each bit
        {
            // * If the LSB is set
            if ((crc & 0x0001) != 0)
            {
                // * Shift right and XOR 0xA001
                crc >>= 1;
                crc ^= 0xA001;
                // crc ^= 0x8005;
            }
            // * Else LSB is not set
            else
                // * Just shift right
                crc >>= 1;
        }
    }
    return crc;
}

bool isNumber(char *res, int len)
{
    for (int i = 0; i < len; i++)
    {
        if (((res[i] < '0') || (res[i] > '9')) && (res[i] != '.' && res[i] != 0))
            return false;
    }
    return true;
}

int FindCharInArrayRev(char array[], char c, int len)
{
    for (int i = len - 1; i >= 0; i--)
    {
        if (array[i] == c)
            return i;
    }
    return -1;
}

long getValue(char *buffer, int maxlen, char startchar, char endchar) // should have more than 4 chars.
{
    int s = FindCharInArrayRev(buffer, startchar, maxlen - 2);
    int l = FindCharInArrayRev(buffer, endchar, maxlen - 2) - s - 1;

    char res[16];
    memset(res, 0, sizeof(res));

    if (strncpy(res, buffer + s + 1, l))
    {
        if (endchar == '*')
        {
            if (isNumber(res, l))
                // * Lazy convert float to long
                return (1000 * atof(res));
        }
        else if (endchar == ')')
        {
            if (isNumber(res, l))
                return atof(res);
        }
    }
    return 0;
}

char *stringToCharArray(string s)
{
    const int length = s.length();
    char *char_array = new char[length + 1];
    return strcpy(char_array, s.c_str());
}

// returns an array of values '(02.314)'
vector<string> parseStringIntoVectorArray(char *input)
{
    string s(input);

    vector<string> result;
    size_t start = s.find("(");
    while (start != string::npos)
    {
        size_t end = s.find(")", start + 1);
        if (end == string::npos)
        {
            break;
        }
        // result.push_back(s.substr(start + 1, end - start - 1));
        result.push_back(s.substr(start, end - start + 1)); // keep the emphases in the result. (12)
        start = s.find("(", end + 1);
    }
    return result;
}

bool decode_telegram(int len)
{
    int startChar = FindCharInArrayRev(telegram, '/', len);
    int endChar = FindCharInArrayRev(telegram, '!', len);
    bool validCRCFound = false;

    // debug
    // Serial.printf("\nstartchar = %d; endchar = %d; \n", startChar, endChar);

    Serial.print("telegram = __");
    for (int cnt = 0; cnt < len; cnt++)
    {
        Serial.print(telegram[cnt]);
    }
    // Serial.print("__\n");

    if (startChar >= 0)
    {
        // debug
        // Serial.println("Branch 1");

        // * Start found. Reset CRC calculation
        currentCRC = CRC16(0x0000, (unsigned char *)telegram + startChar, len - startChar);
    }
    else if (endChar >= 0)
    {
        // debug
        Serial.println("Branch 2");

        // * Add to crc calc
        currentCRC = CRC16(currentCRC, (unsigned char *)telegram + endChar, 1);

        char messageCRC[5];
        strncpy(messageCRC, telegram + endChar + 1, 4);

        messageCRC[4] = 0; // * Thanks to HarmOtten (issue 5)

        // debug
        Serial.printf("\nmessageCRC = %s", messageCRC);

        validCRCFound = ((unsigned int)strtol(messageCRC, NULL, 16) == currentCRC);

        // debug
        Serial.printf("\ncurrentCRC = %d", currentCRC);

        if (validCRCFound)
            Serial.println(F("CRC Valid!"));
        else
            Serial.println(F("CRC Invalid!"));

        currentCRC = 0;
    }
    else
    {
        // debug
        // Serial.println("Branch 3");

        currentCRC = CRC16(currentCRC, (unsigned char *)telegram, len);
    }

    // debug
    // Serial.printf("\ncurrentCRC = %d", currentCRC);

    // 1-0:1.8.1(000992.992*kWh)
    // 1-0:1.8.1 = Elektra verbruik DAG  tarief (DSMR v4.0)
    if (strncmp(telegram, "1-0:1.8.1", strlen("1-0:1.8.1")) == 0)
    {
        CONSUMPTION_HIGH_TARIF = getValue(telegram, len, '(', '*');
    }

    // 1-0:1.8.2(000560.157*kWh)
    // 1-0:1.8.2 = Elektra verbruik NACHT tarief (DSMR v4.0)
    if (strncmp(telegram, "1-0:1.8.2", strlen("1-0:1.8.2")) == 0)
    {
        CONSUMPTION_LOW_TARIF = getValue(telegram, len, '(', '*');
    }

    // 1-0:2.8.1(000560.157*kWh)
    // 1-0:2.8.1 = Elektra opbrengst dagtarief (Fluvius) - Totale injectie van energie in kWh dagtarief
    if (strncmp(telegram, "1-0:2.8.1", strlen("1-0:2.8.1")) == 0)
    {
        RETURNDELIVERY_HIGH_TARIF = getValue(telegram, len, '(', '*');
    }

    // 1-0:2.8.2(000560.157*kWh)
    // 1-0:2.8.2 = Elektra opbrengst nachttarief (Fluvius) - Totale injectie van energie in kWh nachttarief
    if (strncmp(telegram, "1-0:2.8.2", strlen("1-0:2.8.2")) == 0)
    {
        RETURNDELIVERY_LOW_TARIF = getValue(telegram, len, '(', '*');
    }

    // 1-0:1.7.0(00.424*kW) Actueel verbruik (Fluvius) - Afgenomen ogenblikkelijk vermogen in kW
    // 1-0:1.7.x = Electricity consumption actual usage (DSMR v4.0)
    if (strncmp(telegram, "1-0:1.7.0", strlen("1-0:1.7.0")) == 0)
    {
        ACTUAL_CONSUMPTION = getValue(telegram, len, '(', '*');
    }

    // 1-0:2.7.0(00.000*kW) Actuele teruglevering (Fluvius) - Geïnjecteerd ogenblikkelijk vermogen in kW
    if (strncmp(telegram, "1-0:2.7.0", strlen("1-0:2.7.0")) == 0)
    {
        ACTUAL_RETURNDELIVERY = getValue(telegram, len, '(', '*');
    }

    // undocumented --> remove
    /*
    // 1-0:21.7.0(00.378*kW)
    // 1-0:21.7.0 = Instantaan vermogen Elektriciteit levering L1
    if (strncmp(telegram, "1-0:21.7.0", strlen("1-0:21.7.0")) == 0)
    {
        L1_INSTANT_POWER_USAGE = getValue(telegram, len, '(', '*');
    }

    // 1-0:41.7.0(00.378*kW)
    // 1-0:41.7.0 = Instantaan vermogen Elektriciteit levering L2
    if (strncmp(telegram, "1-0:41.7.0", strlen("1-0:41.7.0")) == 0)
    {
        L2_INSTANT_POWER_USAGE = getValue(telegram, len, '(', '*');
    }

    // 1-0:61.7.0(00.378*kW)
    // 1-0:61.7.0 = Instantaan vermogen Elektriciteit levering L3
    if (strncmp(telegram, "1-0:61.7.0", strlen("1-0:61.7.0")) == 0)
    {
        L3_INSTANT_POWER_USAGE = getValue(telegram, len, '(', '*');
    }
*/
    // 1-0:31.7.0(002*A)
    // 1-0:31.7.0 = Instantant stroom Elektriciteit L1
    if (strncmp(telegram, "1-0:31.7.0", strlen("1-0:31.7.0")) == 0)
    {
        L1_INSTANT_POWER_CURRENT = getValue(telegram, len, '(', '*');
    }
    // 1-0:51.7.0(002*A)
    // 1-0:51.7.0 = Instantant stroom Elektriciteit L2
    if (strncmp(telegram, "1-0:51.7.0", strlen("1-0:51.7.0")) == 0)
    {
        L2_INSTANT_POWER_CURRENT = getValue(telegram, len, '(', '*');
    }
    // 1-0:71.7.0(002*A)
    // 1-0:71.7.0 = Instantant stroom Elektriciteit L3
    if (strncmp(telegram, "1-0:71.7.0", strlen("1-0:71.7.0")) == 0)
    {
        L3_INSTANT_POWER_CURRENT = getValue(telegram, len, '(', '*');
    }

    // 1-0:32.7.0(232.0*V)
    // 1-0:32.7.0 = Voltage L1
    if (strncmp(telegram, "1-0:32.7.0", strlen("1-0:32.7.0")) == 0)
    {
        L1_VOLTAGE = getValue(telegram, len, '(', '*');
    }
    // 1-0:52.7.0(232.0*V)
    // 1-0:52.7.0 = Voltage L2
    if (strncmp(telegram, "1-0:52.7.0", strlen("1-0:52.7.0")) == 0)
    {
        L2_VOLTAGE = getValue(telegram, len, '(', '*');
    }
    // 1-0:72.7.0(232.0*V)
    // 1-0:72.7.0 = Voltage L3
    if (strncmp(telegram, "1-0:72.7.0", strlen("1-0:72.7.0")) == 0)
    {
        L3_VOLTAGE = getValue(telegram, len, '(', '*');
    }

    // 0-1:24.2.3(150531200000S)(00811.923*m3)
    // 0-1:24.2.3 = Gas (DSMR v5.0 - fluvius)
    if (strncmp(telegram, "0-1:24.2.3", strlen("0-1:24.2.3")) == 0)
    {
        GAS_METER_M3 = getValue(telegram, len, '(', '*');
    }

    // 0-0:96.14.0(0001)
    // 0-0:96.14.0 = Actual Tarif
    if (strncmp(telegram, "0-0:96.14.0", strlen("0-0:96.14.0")) == 0)
    {
        ACTUAL_TARIF = getValue(telegram, len, '(', ')');
    }

    // 0-0:96.7.21(00003)
    // 0-0:96.7.21 = Aantal onderbrekingen Elektriciteit
    if (strncmp(telegram, "0-0:96.7.21", strlen("0-0:96.7.21")) == 0)
    {
        SHORT_POWER_OUTAGES = getValue(telegram, len, '(', ')');
    }

    // 0-0:96.7.9(00001)
    // 0-0:96.7.9 = Aantal lange onderbrekingen Elektriciteit
    if (strncmp(telegram, "0-0:96.7.9", strlen("0-0:96.7.9")) == 0)
    {
        LONG_POWER_OUTAGES = getValue(telegram, len, '(', ')');
    }

    // 1-0:32.32.0(00000)
    // 1-0:32.32.0 = Aantal korte spanningsdalingen Elektriciteit in fase 1
    if (strncmp(telegram, "1-0:32.32.0", strlen("1-0:32.32.0")) == 0)
    {
        SHORT_POWER_DROPS = getValue(telegram, len, '(', ')');
    }

    // 1-0:32.36.0(00000)
    // 1-0:32.36.0 = Aantal korte spanningsstijgingen Elektriciteit in fase 1
    if (strncmp(telegram, "1-0:32.36.0", strlen("1-0:32.36.0")) == 0)
    {
        SHORT_POWER_PEAKS = getValue(telegram, len, '(', ')');
    }

#pragma region UPDATE 1.7.1 PEAK TARRIFF

    // 1-0:1.4.0(02.351*kW)
    // 1-0:1.4.0 = quart_hourly_current_average_peak_consumption kW - Current rolling avg of the last 15 minutes
    if (strncmp(telegram, "1-0:1.4.0", strlen("1-0:1.4.0")) == 0)
        mActualAverage15mPeak = getValue(telegram, len, '(', '*');

    // 1-0:1.6.0(200509134558S)(02.589*kW)
    // 1-0:1.6.0 = quart_hourly_max_peak_this_month kW
    if (strncmp(telegram, "1-0:1.6.0", strlen("1-0:1.6.0")) == 0)
        mMax15mPeakThisMonth = getValue(telegram, len, '(', '*');

    // 0-0:98.1.0 = quart_hourly_peak_consumption_last_13months
    // 0-0:98.1.0(3)(1-0:1.6.0)(1-0:1.6.0)(200501000000S)(200423192538S)(03.695*kW)(200401000000S)(200305122139S)(05.980*kW)(200301000000S)(200210035421W)(04.318*kW)
    /*
     * Line format:
        'ID (Count) (ID) (ID) (TST) (TST) (Mv1*U1)'
         1  2       3    4    5     6     7
        1) OBIS Reduced ID-code
        2) Amount of values in the response
        3) ID of the source
        4) ^^
        5) Time Stamp (TST) of the month
        6) Time Stamp (TST) when the max demand occured
        6) Measurement value 1 (most recent entry of buffer attribute without unit)
        7) Unit of measurement values (Unit of capture objects attribute)
    */
    if (strncmp(telegram, "0-0:98.1.0", strlen("0-0:98.1.0")) == 0)
    {
        vector<string> output = parseStringIntoVectorArray(telegram); // parse telegram into array of strings based on the parentheses '(' ')'

        unsigned long count = stol(output[0].substr(1, output[0].size() - 2));
        mAverage15mPeakLast13months = count;
        vector<long> valuesArray;

        // create JsonObject from values telegram
        Last13MonthsPeaks_json["count"] = count;
        Last13MonthsPeaks_json["unit"] = "W";
        JsonArray peakvalues = Last13MonthsPeaks_json.createNestedArray("values");

        for (unsigned int i = 3; i < output.size(); i += 3)
        {
            //to filter only the VALUES of the last 12months, I need to loop through the values starting from i=3 and only take the 3th value.
            // '(Count) (ID) (ID) (TST) (TST) (Mv1*U1) (TST) (TST) (Mv2*U2)'
            //  0       1    2    3     4     5         6     7    8       ..... 41 (for 12months)
            valuesArray.push_back(getValue(stringToCharArray(output[i + 2]), output[i + 2].size(), '(', '*')); // capture only values in the valuesArray as long.

            JsonObject peakvalue = peakvalues.createNestedObject();
            peakvalue["value"] = getValue(stringToCharArray(output[i + 2]), output[i + 2].size(), '(', '*'); // remove "*kW" from the string
        }

        // debug
        // serializeJsonPretty(Last13MonthsPeaks_json, Serial);

        //calculate average peak value.
        long sum = 0;
        for (unsigned int i = 0; i < valuesArray.size(); i++)
            sum += valuesArray[i];
        long average = sum / valuesArray.size();

        if (count == valuesArray.size())
            mAverage15mPeakLast13months = average;
        else
            mAverage15mPeakLast13months = -1; // detect error value over MQTT.

        /* */
    }

#pragma endregion

    return validCRCFound;
}

void read_p1_hardwareserial()
{

    if (Serial.available())
    {
        memset(telegram, 0, sizeof(telegram));

        while (Serial.available())
        {
            // ESP.wdtDisable();
            int len = Serial.readBytesUntil('\n', telegram, P1_MAXLINELENGTH);

            // ESP.wdtEnable(1);

            // debug entire telegram
            // Serial.printf(">>> %s\n", telegram);

            processLine(len);

            memset(telegram, 0, sizeof(telegram));
        }
    }
}

void processLine(int len)
{
    telegram[len] = '\n';
    telegram[len + 1] = 0;
    yield();

    // DEBUG
    bool result = decode_telegram(len + 1);

    if (result)
    {
        send_data_to_broker();
        LAST_UPDATE_SENT = millis();
    }
}

// **********************************
// * EEPROM helpers                 *
// **********************************

String read_eeprom(int offset, int len)
{
    Serial.print(F("read_eeprom()"));

    String res = "";
    for (int i = 0; i < len; ++i)
    {
        res += char(EEPROM.read(i + offset));
    }
    return res;
}

void write_eeprom(int offset, int len, String value)
{
    Serial.println(F("write_eeprom()"));
    for (int i = 0; i < len; ++i)
    {
        if ((unsigned)i < value.length())
        {
            EEPROM.write(i + offset, value[i]);
        }
        else
        {
            EEPROM.write(i + offset, 0);
        }
    }
}

// ******************************************
// * Callback for saving WIFI config        *
// ******************************************

bool shouldSaveConfig = false;

// * Callback notifying us of the need to save config
void save_wifi_config_callback()
{
    Serial.println(F("Should save config"));
    shouldSaveConfig = true;
}

// ******************************************
// * Callback for resetting Wifi settings   *
// ******************************************
void resetWifi()
{
    Serial.println("RST was pushed twice...");
    Serial.println("Erasing stored WiFi credentials.");

    // clear WiFi creds.
    WiFiManager wifiManager;
    wifiManager.resetSettings();

    Serial.println("Restarting...");
    ESP.restart(); // builtin, safely restarts the ESP.
}

// **********************************
// * Setup OTA                      *
// **********************************

void setup_ota()
{
    Serial.println(F("Arduino OTA activated."));

    // * Port defaults to 8266
    ArduinoOTA.setPort(8266);

    // * Set hostname for OTA
    ArduinoOTA.setHostname(HOSTNAME);
    ArduinoOTA.setPassword(OTA_PASSWORD);

    ArduinoOTA.onStart([]()
    {
        Serial.println(F("Arduino OTA: Start"));
    });

    ArduinoOTA.onEnd([]()
    {
        Serial.println(F("Arduino OTA: End (Running reboot)"));
    });

    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
    {
        Serial.printf("Arduino OTA Progress: %u%%\r", (progress / (total / 100)));
    });

    ArduinoOTA.onError([](ota_error_t error)
    {
        Serial.printf("Arduino OTA Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR)
            Serial.println(F("Arduino OTA: Auth Failed"));
        else if (error == OTA_BEGIN_ERROR)
            Serial.println(F("Arduino OTA: Begin Failed"));
        else if (error == OTA_CONNECT_ERROR)
            Serial.println(F("Arduino OTA: Connect Failed"));
        else if (error == OTA_RECEIVE_ERROR)
            Serial.println(F("Arduino OTA: Receive Failed"));
        else if (error == OTA_END_ERROR)
            Serial.println(F("Arduino OTA: End Failed"));
    });

    ArduinoOTA.begin();
    Serial.println(F("Arduino OTA finished"));
}

// **********************************
// * Setup MDNS discovery service   *
// **********************************

void setup_mdns()
{
    Serial.println(F("Starting MDNS responder service"));

    bool mdns_result = MDNS.begin(HOSTNAME);
    if (mdns_result)
    {
        MDNS.addService("http", "tcp", 80);
    }
}

// **********************************
// * Setup Main                     *
// **********************************

void setup()
{
    // * Configure EEPROM
    EEPROM.begin(512);

    // Setup a hw serial connection for communication with the P1 meter and logging (not using inversion)
    Serial.begin(BAUD_RATE, SERIAL_8N1, SERIAL_FULL);
    Serial.setRxBufferSize(1024);
    Serial.println("");
    Serial.println("Swapping UART0 RX to inverted");
    Serial.flush();

    // Invert the RX serialport by setting a register value, this way the TX might continue normally allowing the serial monitor to read println's
    USC0(UART0) = USC0(UART0) | BIT(UCRXI);
    Serial.println("Serial port is ready to recieve.");

    // * Set led pin as output
    pinMode(LED_BUILTIN, OUTPUT);

    // * Setup Double reset detection
    if (drd.detectDoubleReset())
    {
        Serial.println("DRD: Double Reset Detected");
        Serial.println("DRD: RESET WIFI Initiated");
        resetWifi();
    }
    else
    {
        Serial.println("DRD: No Double Reset Detected");
    }

    // * Start ticker with 0.5 because we start in AP mode and try to connect
    ticker.attach(0.6, tick);

    // * Get MQTT Server settings
    String settings_available = read_eeprom(134, 1);

    if (settings_available == "1")
    {
        read_eeprom(0, 64).toCharArray(MQTT_HOST, 64);   // * 0-63
        read_eeprom(64, 6).toCharArray(MQTT_PORT, 6);    // * 64-69
        read_eeprom(70, 32).toCharArray(MQTT_USER, 32);  // * 70-101
        read_eeprom(102, 32).toCharArray(MQTT_PASS, 32); // * 102-133
    }

    WiFiManagerParameter CUSTOM_MQTT_HOST("host", "MQTT hostname", MQTT_HOST, 64);
    WiFiManagerParameter CUSTOM_MQTT_PORT("port", "MQTT port", MQTT_PORT, 6);
    WiFiManagerParameter CUSTOM_MQTT_USER("user", "MQTT user", MQTT_USER, 32);
    WiFiManagerParameter CUSTOM_MQTT_PASS("pass", "MQTT pass", MQTT_PASS, 32);

    // * WiFiManager local initialization. Once its business is done, there is no need to keep it around
    WiFiManager wifiManager;

    // * Reset settings - uncomment for testing
    // wifiManager.resetSettings();

    // * Set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
    wifiManager.setAPCallback(configModeCallback);

    // * Set timeout
    wifiManager.setConfigPortalTimeout(WIFI_TIMEOUT);

    // * Set save config callback
    wifiManager.setSaveConfigCallback(save_wifi_config_callback);

    // * Add all your parameters here
    wifiManager.addParameter(&CUSTOM_MQTT_HOST);
    wifiManager.addParameter(&CUSTOM_MQTT_PORT);
    wifiManager.addParameter(&CUSTOM_MQTT_USER);
    wifiManager.addParameter(&CUSTOM_MQTT_PASS);

    // * Fetches SSID and pass and tries to connect
    // * Reset when no connection after 10 seconds
    if (!wifiManager.autoConnect())
    {
        Serial.println(F("Failed to connect to WIFI and hit timeout"));

        // * Reset and try again, or maybe put it to deep sleep
        ESP.reset();
        delay(WIFI_TIMEOUT);
    }

    // * Read updated parameters
    strcpy(MQTT_HOST, CUSTOM_MQTT_HOST.getValue());
    strcpy(MQTT_PORT, CUSTOM_MQTT_PORT.getValue());
    strcpy(MQTT_USER, CUSTOM_MQTT_USER.getValue());
    strcpy(MQTT_PASS, CUSTOM_MQTT_PASS.getValue());

    // * Save the custom parameters to EEPROM
    if (shouldSaveConfig)
    {
        Serial.println(F("Saving WiFiManager config"));

        write_eeprom(0, 64, MQTT_HOST);   // * 0-63
        write_eeprom(64, 6, MQTT_PORT);   // * 64-69
        write_eeprom(70, 32, MQTT_USER);  // * 70-101
        write_eeprom(102, 32, MQTT_PASS); // * 102-133
        write_eeprom(134, 1, "1");        // * 134 --> always "1"
        EEPROM.commit();
    }

    // * If you get here you have connected to the WiFi
    Serial.println(F("Connected to WIFI..."));

    // * Keep LED on
    ticker.detach();
    digitalWrite(LED_BUILTIN, LOW);

    // * Configure OTA
    setup_ota();

    // * Startup MDNS Service
    setup_mdns();

    // * Setup MQTT
    Serial.printf("MQTT connecting to: %s:%s\n", MQTT_HOST, MQTT_PORT);

    mqtt_client.setServer(MQTT_HOST, atoi(MQTT_PORT));
}

// **********************************
// * Loop                           *
// **********************************

void loop()
{
    // every 2days --> restart board.
    const unsigned long RESTART_INTERVAL = 2ul * 24ul * 60ul * 60ul * 1000ul;
    if (millis() > RESTART_INTERVAL)
    {
        Serial.println("++++++++++++++++++++++++++++++++++++++++");
        Serial.print("trying to restart after 2 days or ms: ");
        Serial.print(RESTART_INTERVAL);
        Serial.println("");
        Serial.println("++++++++++++++++++++++++++++++++++++++++");
        ESP.restart();
    }

    ArduinoOTA.handle();
    long now = millis();

    if (!mqtt_client.connected())
    {

        if (now - LAST_RECONNECT_ATTEMPT > 5000)
        {
            LAST_RECONNECT_ATTEMPT = now;

            if (mqtt_reconnect())
            {
                LAST_RECONNECT_ATTEMPT = 0;
            }
        }
    }
    else
    {
        // Serial.println("start mqtt Loop");
        mqtt_client.loop();
        // Serial.println("End mqtt Loop");
    }

    if (now - LAST_UPDATE_SENT > UPDATE_INTERVAL)
    {
        // Serial.println("start reading serial");
        read_p1_hardwareserial();
    }

    // //debug
    // Serial.println("End Loop");
    // Serial.println("--------");

    // Call the double reset detector loop method every so often,
    // so that it can recognise when the timeout expires.
    // You can also call drd.stop() when you wish to no longer
    // consider the next reset as a double reset.
    drd.loop();
}
