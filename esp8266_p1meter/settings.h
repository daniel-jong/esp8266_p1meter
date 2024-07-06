// **********************************
// * Settings                       *
// **********************************

// Update treshold in milliseconds, messages will only be sent on this interval
#define UPDATE_INTERVAL 60000  // 1 minute
//#define UPDATE_INTERVAL 300000 // 5 minutes

// * Baud rate for both hardware and software 
#define BAUD_RATE 115200

// * Change to 0 if you have a hardware inverter
#define SERIAL_RX_INVERTED 1

// The used serial pins, note that this can only be UART0, as other serial port doesn't support inversion
// By default the UART0 serial will be used. These settings displayed here just as a reference. 
// #define SERIAL_RX RX
// #define SERIAL_TX TX

// * Max telegram length
#define P1_MAXLINELENGTH 1050

// * The hostname of our little creature
#define HOSTNAME "p1meter"

// * The password used for OTA
#define OTA_PASSWORD "admin"

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
char MQTT_PORT[6]  = "";
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

// * Set during CRC checking
unsigned int currentCRC = 0;
