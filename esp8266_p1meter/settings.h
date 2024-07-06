// **********************************
// * Settings                       *
// * Settings                       *
// **********************************

//Reset wifi settings:
// *Number of seconds after reset during which a
// *subseqent reset will be considered a double reset.
#define DRD_TIMEOUT 10

// *RTC Memory Address for the DoubleResetDetector to use
#ifdef ARDUINO_ESP8266_WEMOS_D1MINI // WeMos mini and D1 R2
#define DRD_ADDRESS 1
#elif ARDUINO_ESP8266_NODEMCU // Wio Link and NodeMCU 1.0 (also 0.9), use for ESP12
#define DRD_ADDRESS 0 // FLASH BUTTON
#else 
#define DRD_ADDRESS 1
#endif

// Update treshold in milliseconds, messages will only be sent on this interval
//#define UPDATE_INTERVAL 60000  // 1 minute
#define UPDATE_INTERVAL 30000 // 30s
//#define UPDATE_INTERVAL 300000 // 5 minutes

// * Baud rate for both hardware and software
#define BAUD_RATE 115200
//#define BAUD_RATE 9600

// The used serial pins, note that this can only be UART0, as other serial port doesn't support inversion
// By default the UART0 serial will be used. These settings displayed here just as a reference.
// #define SERIAL_RX RX
// #define SERIAL_TX TX

// * Max telegram length
#define P1_MAXLINELENGTH 2048 // 4096 longest normal line is 47 char (+3 for \r\n\0)

// * The hostname of our little creature
#define HOSTNAME "p1meter"

// * The password used for OTA
#define OTA_PASSWORD "rXqEqAY7D8L9n2"

// * Wifi timeout in milliseconds
#define WIFI_TIMEOUT 30000

// * MQTT network settings
#define MQTT_MAX_RECONNECT_TRIES 10

// * MQTT root topic
#define MQTT_ROOT_TOPIC "sensors/power/p1meter"

// * MQTT Last reconnection counter
long LAST_RECONNECT_ATTEMPT = 0;

long LAST_UPDATE_SENT = 0;

// * To be filled with EEPROM data
char MQTT_HOST[64] = "";
char MQTT_PORT[6] = "";
char MQTT_USER[32] = "";
char MQTT_PASS[32] = "";

// * Set to store received telegram
char telegram[P1_MAXLINELENGTH];

// * Set to store the data values read
long CONSUMPTION_LOW_TARIF;
long CONSUMPTION_HIGH_TARIF;

long RETURNDELIVERY_LOW_TARIF;
long RETURNDELIVERY_HIGH_TARIF;

long ACTUAL_CONSUMPTION;
long ACTUAL_RETURNDELIVERY;
long GAS_METER_M3;

long L1_INSTANT_POWER_USAGE;
long L2_INSTANT_POWER_USAGE;
long L3_INSTANT_POWER_USAGE;
long L1_INSTANT_POWER_CURRENT;
long L2_INSTANT_POWER_CURRENT;
long L3_INSTANT_POWER_CURRENT;
long L1_VOLTAGE;
long L2_VOLTAGE;
long L3_VOLTAGE;

// Set to store data counters read
long ACTUAL_TARIF;
long SHORT_POWER_OUTAGES;
long LONG_POWER_OUTAGES;
long SHORT_POWER_DROPS;
long SHORT_POWER_PEAKS;

long mActualAverage15mPeak = 0;
long mMax15mPeakThisMonth = 0;
long mAverage15mPeakLast13months = 0;

StaticJsonDocument<256> Last13MonthsPeaks_json; //max buffer of PubSubClient is 256 byte. so no longer JSON messages allowed!

// * Set during CRC checking
unsigned int currentCRC = 0;
