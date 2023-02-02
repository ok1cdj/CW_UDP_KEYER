//
// 2021-10 CW UDP Keyer with OLED Display
// by Jan Uhrin, OM2JU and Ondrej Kolonicny, OK1CDJ
//
// Generate CW in Straight and Iambic keyer mode
// Decode generated CW on the fly, the decoding is auto adjusted to played speed
// Optionally, if remote IP is defined, the decoded Morse characters are sent via UDP port 6789
//
//
// When TTGO_T3_BOARD is     defined the Keyer will work with TTGO T7 v1.4 mini32 board (ESP32 with OLED display)
// When TTGO_T3_BOARD is not defined the Keyer will work with hardware designed by OK1CDJ/OM2JU (ESP32 module, Monochrome OLED 128x64, SX1280 module)
#define TTGO_T3_BOARD

// When we use TTGO_T3_BOARD we will use keyer pads to enter input data (instead of rotary encoder)
// Left pad = down, rigth pad = up
#ifdef TTGO_T3_BOARD
#define USE_PADS_INSTEAD_OF_ROTARY_ENC
#define TTGO_BOARD_PUSH_LEFT  35
#define TTGO_BOARD_PUSH_RIGHT 0
#define ReadPushBtnVal()   pushBtnVal=digitalRead(TTGO_BOARD_PUSH_LEFT)
#define WAIT_Push_Btn_Release(milisec)  delay(milisec);while(digitalRead(TTGO_BOARD_PUSH_LEFT)==0){}
#define KEYER_DOT       12    // Morse keyer DOT
#define KEYER_DASH      13    // Morse keyer DASH
#else
#define ReadPushBtnVal()   pushBtnVal=digitalRead(ROTARY_ENC_PUSH)
#define WAIT_Push_Btn_Release(milisec)  delay(milisec);while(digitalRead(ROTARY_ENC_PUSH)==0){}
#define KEYER_DOT       34    // Morse keyer DOT          NOTE: GPIOs 34 to 39 INPUTs only / No internal pullup / No int. pulldown
#define KEYER_DASH      35    // Morse keyer DASH
#endif

#define TIMER_PERIOD_USEC 3500     // 2500 = 2.5 msec, 3500 = 3.5 msec

#include <Adafruit_SSD1306.h>
#include <Preferences.h>

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncUDP.h>
#include <SPIFFS.h>


#include <stdlib.h>
#include "time.h"
#include <SPI.h>                                               //the lora device is SPI based so load the SPI library                                         
#include <SX128XLT.h>                                          //include the appropriate library  
#include "Keyer_Settings.h"                                    //include the setiings file, frequencies, LoRa settings etc   


#define UDP_PORT 6789 // UDP port for CW 

#define PUSH_BTN_PRESSED 0

// Display related defines
#ifdef TTGO_T3_BOARD
#include <TFT_eSPI.h> // Graphics and font library for ST7735 driver chip
#define SCREEN_WIDTH 240 // OLED display width, in pixels
#define SCREEN_HEIGHT 135 // OLED display height, in pixels
#define MY_TEXT_SIZE 2
#define MY_SCALE 2
#define MY_WHITE TFT_WHITE
#define MY_ORANGE TFT_ORANGE
#define MY_GOLD TFT_GOLD
#define MY_GREEN TFT_GREEN
#define MY_BLACK TFT_BLACK
TFT_eSPI     display   = TFT_eSPI();  // Invoke library, pins defined in User_Setup.h
#else
#include <Adafruit_GFX.h>
// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define MY_TEXT_SIZE 1
#define MY_SCALE 1
#define MY_WHITE WHITE
#define MY_ORANGE WHITE
#define MY_GOLD WHITE
#define MY_GREEN WHITE
#define MY_BLACK BLACK
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
#endif

// Webserver
AsyncWebServer server(80);
AsyncUDP udp;
QueueHandle_t queue;

String ssid       = "";
String ssid_ap    = "CW_UDP_KEYER_AP";
String password   = "";
String sRemote_ip  = "";
String apikey     = "";
String sIP        = "";
String sGateway   = "";
String sSubnet    = "";
String sPrimaryDNS = "";
String wifiNetworkList = "";
String sSecondaryDNS = "";
String spwr = "";

IPAddress AP_IP      = IPAddress(192,168,4,4);
IPAddress AP_gateway = IPAddress(192,168,4,4);
IPAddress AP_subnet  = IPAddress(255,255,255,0);


String message;
String cq_message;
String sspeed = "20";
String sfreq;
String scmd;
uint32_t speed = 20;

// web server requests
const char* PARAM_MESSAGE = "message";
const char* PARAM_FRQ_INDEX = "frq_index";
const char* PARAM_SPEED = "speed";
const char* PARAM_PWR = "pwr";
const char* PARAM_SSID = "ssid";
const char* PARAM_password = "password";
const char* PARAM_APIKEY = "apikey";
const char* PARAM_DHCP = "dhcp";
const char* PARAM_CON = "con";
const char* PARAM_SUBNET = "subnet";
const char* PARAM_GATEWAY = "gateway";
const char* PARAM_PDNS = "pdns";
const char* PARAM_SDNS = "sdns";
const char* PARAM_LOCALIP = "localip";
const char* PARAM_CMD_B = "cmd_B";
const char* PARAM_CMD_C = "cmd_C";
const char* PARAM_CMD_D = "cmd_D";
const char* PARAM_CMD_T = "cmd_T";


bool wifiConfigRequired = false;
bool wifiSoftAP = false;
bool dhcp = true; // dhcp enable disble
bool con = false; // connection type wifi false
bool remote_ip_valid = false;
bool udp_sending_enabled = false;
bool cq_running = false;
bool keyerEnabled = true;

IPAddress IP;
IPAddress IP_REMOTE;
IPAddress gateway;
IPAddress subnet;
IPAddress primaryDNS;
IPAddress secondaryDNS;

// JU Perl Script:
// for($i=-1;$i<13;$i++) { print $i+1 . " "; print 18.26+0.7*$i . " " . 10**((18.26+0.7*$i)/10). "\n"; }
// 0 17.56 57.0164272280748
// 1 18.26 66.9884609416526
// 2 18.96 78.7045789695099
// 3 19.66 92.4698173938223
// 4 20.36 108.642562361707
// 5 21.06 127.643880881134
// 6 21.76 149.968483550237
// 7 22.46 176.197604641163
// 8 23.16 207.014134879104
// 9 23.86 243.220400907382
// 10 24.56 285.759054337495
// 11 25.26 335.737614242955
// 12 25.96 394.457302075279
// 13 26.66 463.446919736288
//
#define PowerArrayMiliWatt_Size 6
// { Power_mW, Reg-setting }
const uint32_t PowerArrayMiliWatt [][2] = {
  {  0,  0 },   // 0mW = no output power, special case
  { 50,  0 },   // cca 50 mW
  { 100, 4 },   // cca 100 mW
  { 200, 8 },   // cca 200 mW
  { 330, 11 },  // cca 300 mW
  { 460, 13 }   // cca 460 mW       13+18 = 31  --> max value
};
//
// Program states
enum state_t {
  S_RUN = 0,
  S_RUN_RUN,
  S_CHANGE_FREQ_WITH_PADS,
  S_TOP_MENU_ITEMS,
  S_RUN_CQ_BLOCKING,         // CQ is generated solely in this state
  S_RUN_CQ_SENDMORSE_TASK,   // CQ is generated by filling in "message" buffer which is then played in Sendmorse task
  S_SEND_CQ_UDP,             // CQ is generated by sending UDP packet to remote IP, no local playing
  S_SET_SPEED_WPM,
  S_SET_OUTPUT_POWER,
  S_SAVE_SPEED_AND_POWER,    // Save them to FLASH as the Speed WPM and Power change without saving in previous states
  S_SET_KEYER_TYPE,
  S_SET_BUZZER_FREQ,
  S_SET_TEXT_GENERIC,
  S_SET_DISP_ORIENTATION,
  S_WIFI_RECONNECT
} program_state;

// Sub-states related to text-setting actions
enum set_text_state_t {
  S_SET_MY_CALL = 0,
  S_SET_WIFI_SSID,
  S_SET_WIFI_PWD,
  S_SET_sRemote_ip
} set_text_state;

// Displayed items when pressed pushutton and rotating the knob
const char * TopMenuArray[] = {
  "1. << Back        ",
  "2. CQ...          ",
  "3. Set WPM        ",
  "4. Set Out Power  ",
  "5. Save WPM & Pwr ",
  "6. Set Keyer Typ  ",
  "7. Set Buzzer Freq",
  "8. Set My Call    ",
  "9. Set WiFi SSID  ",
  "10. Set WiFi PWD  ",
  "11. Set Remote IP ",
  "12. Set Disp Orien",
  "13. WiFi Reconn.  "
};
//
// Rotary Encoder structure
struct RotaryEncounters
{
  int32_t cntVal;
  int32_t cntMin;
  int32_t cntMax;
  int32_t cntIncr;
  int32_t cntValOld;
};

hw_timer_t * timer = NULL;

uint8_t  rotaryA_Val = 0xFF;
uint8_t  rotaryB_Val = 0xFF;
uint8_t  ISR_cnt = 0;
uint8_t  cntIncrISR;
uint32_t loopCnt = 0;
uint32_t menuIndex;
uint32_t timeout_cnt = 0;
uint8_t  keyerVal      = 1;
uint8_t  squeezedFlag  = 0;  // paddles squeezed flag
uint8_t  iambicBfinishFlag  = 0;

#define DOT  0
#define DASH 1
uint8_t  lastPlayedPad = DOT;
uint8_t  pushBtnVal    = 1;
uint8_t  pushBtnValR   = 1;
uint8_t  pushBtnValL   = 1;
uint8_t  sentCnt       = 0;
uint8_t  keyerCWstarted = 0;
uint8_t  pushBtnPressed = 0;
uint8_t stop = 0;

// Variables related to Morse decoding
#define KEYER_OFF   0x00
#define KEYER_ON    0x01
#define LAST_WAS_NOTHING	0x00
#define LAST_WAS_DOT	    0x01
#define LAST_WAS_DASH	    0x02
#define LAST_WAS_SPACE 	0x03
//
uint8_t  CwToneOn = 0;
uint8_t  keyerState             = 0;
uint8_t  keyerStateLast         = 0;
uint32_t keyerEventTime         = 0;
uint32_t keyerEventTimeBuf[256];  // For buffer length 256 we can use 8-bit pointers 
uint8_t  keyerEventTimeBufLen   = 0;
uint8_t  keyerEventTimeBufPtr   = 0;
uint8_t  keyerEventTimeBufPtrRd = 0;
uint32_t keyerTimeCnt           = 0;
uint32_t tOnPauseTreshold       = 20;
uint32_t tOn;
uint32_t tOnLast = 10;
uint32_t tOnDot, tOnDash, tOnTmp, tOnTmp1;
uint32_t tOnDotTreshold  = 10;
uint32_t tOnDotTreshold1 = 10;
uint32_t tOnDotTreshold2 = 10;
uint8_t  decodeOffset  = 32;
uint8_t  decodeOffsetK = 32;
uint8_t  decodeIndexK  = 64;
uint8_t  decodeLenK    = 0;
uint8_t  spaceOkK      = 80;
uint8_t  lastPlayedK = LAST_WAS_NOTHING;        // Last played keyer
uint8_t  decodedChar;
uint8_t  decodedCharShown;
uint8_t  decodedCharCnt = 0;
char     decodedCharBuf[8];
//
uint8_t bufAsyncMsg[10];
AsyncUDPMessage AsyncUDPmsg;
//

uint32_t FreqWord = 0;
uint32_t FreqWordNoOffset = 0;
uint32_t WPM_dot_delay;
uint32_t WPM_dash_delay;

char   freq_ascii_buf[20];   // Buffer for formatting of FREQ value
char   general_ascii_buf[40];
uint8_t general_ascii_buf_index = 0;
char   mycall_ascii_buf[40];
char   wifi_ssid_ascii_buf[40];
char   wifi_pwd_ascii_buf[40];
char   sRemote_ip_ascii_buf[40];

String s_mycall_ascii_buf;
//String s_wifi_ssid_ascii_buf;
//String s_wifi_pwd_ascii_buf;
String s_general_ascii_buf;


char    cq_message_buf[]     = " CQ CQ DE ";
char    cq_message_end_buf[] = " +K";
char    eee_message_buf[]    =    "E E E E E E E E E E E E E E E E E E E E E E E E E";

char    cw_decoded_buf[16];
char    cw_decoded_buf_ordered[17];
uint8_t cw_decoded_buf_ptr = 0;

RotaryEncounters RotaryEnc_FreqWord;
RotaryEncounters RotaryEnc_MenuSelection;
RotaryEncounters RotaryEnc_KeyerSpeedWPM;
RotaryEncounters RotaryEnc_KeyerType;
RotaryEncounters RotaryEnc_DispOrientation;
RotaryEncounters RotaryEnc_BuzzerFreq;
RotaryEncounters RotaryEnc_OutPowerMiliWatt;
RotaryEncounters RotaryEnc_TextInput_Char_Index;
RotaryEncounters RotaryEncISR;

Preferences preferences;

// Copy values of variable pointed by r1 to RotaryEncISR variable
void RotaryEncPush(RotaryEncounters *r1) {
  RotaryEncISR = *r1;
  RotaryEncISR.cntValOld = RotaryEncISR.cntVal + 1;  // Make Old value different from actual in order to e.g. force initial update of display
}

// Copy values of RotaryEncISR variable to variable pointed by r1
void RotaryEncPop(RotaryEncounters *r1) {
  *r1 = RotaryEncISR;
}

void display_display() {
#ifndef TTGO_T3_BOARD
  display.display();
#endif
}

// Display - Prepare box and cursor for main field
void display_mainfield_begin(uint8_t x) {
  display.setTextColor(MY_GOLD);
  //display.clearDisplay();
  display.fillRect(1*MY_SCALE, 10*MY_SCALE, SCREEN_WIDTH - 2*MY_SCALE, 35*MY_SCALE, MY_BLACK); // x, y, width, height
  display.setTextSize(MY_TEXT_SIZE);
  display.setCursor(x*MY_SCALE, 16*MY_SCALE);
}

// Display - Prepare box and cursor for value field
void display_valuefield_begin () {
  display.fillRect(4*MY_SCALE, 33*MY_SCALE, SCREEN_WIDTH - 10*MY_SCALE, 10*MY_SCALE, MY_WHITE); // x, y, width, height
  display.fillRect(5*MY_SCALE, 34*MY_SCALE, SCREEN_WIDTH - 12*MY_SCALE, 8*MY_SCALE, MY_BLACK); // x, y, width, height
  display.setTextColor(MY_ORANGE);
  display.setTextSize(MY_TEXT_SIZE);
  display.setCursor(20*MY_SCALE, 35*MY_SCALE);
  //display.fillRect(10,30,90,50,MY_BLACK);
}

// Display - Clear the valuefield box
void display_valuefield_clear () {
  display.fillRect(4*MY_SCALE, 33*MY_SCALE, SCREEN_WIDTH - 10*MY_SCALE, 12*MY_SCALE, MY_BLACK); // x, y, width, height
}


void display_decodedmorse_field_begin() {
  display.fillRect(1*MY_SCALE, 33*MY_SCALE, SCREEN_WIDTH - 2*MY_SCALE, 10*MY_SCALE, MY_BLACK); // x, y, width, height
  display.setTextColor(MY_WHITE);
  display.setTextSize(MY_TEXT_SIZE);
  display.setCursor(5*MY_SCALE, 33*MY_SCALE);
}

void display_valuefield_begin2 (String buf) {
  display.setTextColor(MY_ORANGE);
  display.setTextSize(MY_TEXT_SIZE);
  display.setCursor(20*MY_SCALE, 35*MY_SCALE);
  display.print(buf);
  display_display();
}

void display_valuefield_clear2 (String buf) {
  display.setTextColor(MY_BLACK);
  display.setTextSize(MY_TEXT_SIZE);
  display.setCursor(20*MY_SCALE, 35*MY_SCALE);
  display.print(buf);
  display_display();
}

// Display - show values on status bar
void display_status_bar () {
  // Delete old info by drawing rectangle
  display.fillRect(1*MY_SCALE, 1*MY_SCALE,                  SCREEN_WIDTH-2*MY_SCALE,  10*MY_SCALE, MY_BLACK); // x, y, width, height
  display.fillRect(1*MY_SCALE, SCREEN_HEIGHT - 18*MY_SCALE, SCREEN_WIDTH-2*MY_SCALE,  17*MY_SCALE, MY_BLACK); // x, y, width, height
  display.drawFastHLine(0, 11*MY_SCALE, SCREEN_WIDTH, MY_WHITE);
  display.drawFastHLine(0, SCREEN_HEIGHT - 20*MY_SCALE, SCREEN_WIDTH, MY_WHITE);

  display.setTextColor(MY_WHITE);
  display.setTextSize(MY_TEXT_SIZE);
  //
  // Display MY_CALL, configured WPM (or Straight keyer) and configured power in mW
  display.setCursor(3*MY_SCALE, 2*MY_SCALE); // 7 is the font height
  display.print(s_mycall_ascii_buf);
  display.setCursor(52*MY_SCALE, 2*MY_SCALE); // 7 is the font height
  if ((RotaryEnc_KeyerType.cntVal) == 1) {
    display.print("Strgh,");
  } else {
    display.print(RotaryEnc_KeyerSpeedWPM.cntVal);
    if ((RotaryEnc_KeyerType.cntVal) == 0) {
      display.print("wpA,");
    } else {
      display.print("wpB,");
    }
  }
  display.print(PowerArrayMiliWatt[RotaryEnc_OutPowerMiliWatt.cntVal][0]);
  display.print("mW");
  // Display WiFi SSID and IP address
  display.setCursor(3*MY_SCALE, SCREEN_HEIGHT - 18*MY_SCALE); // 7 is the font height
  display.print(wifiSoftAP == false ? ssid.c_str() : ssid_ap.c_str());
  display.setCursor(3*MY_SCALE, SCREEN_HEIGHT - 9*MY_SCALE); // 7 is the font height
  display.print(IP);
  display_display();
}


// Limit values of RotaryEncISR.cntVal
void limitRotaryEncISR_values() {
  if (RotaryEncISR.cntVal >= RotaryEncISR.cntMax) {
    RotaryEncISR.cntVal = RotaryEncISR.cntMax;
  }
  if (RotaryEncISR.cntVal <= RotaryEncISR.cntMin) {
    RotaryEncISR.cntVal = RotaryEncISR.cntMin;
  }
}

// Calculate duration of DOT from WPM
void Calc_WPM_dot_delay ( uint32_t wpm) {
  WPM_dot_delay  = (uint32_t) (double(1200.0) / (double) wpm);
  WPM_dash_delay = WPM_dot_delay+WPM_dot_delay+WPM_dot_delay;
}

void SendParamsUDPtoRemoteIP(int32_t freq, int32_t wpm, int32_t pwr) {
  if (remote_ip_valid == true) {
    // Send UDP Packet
    bufAsyncMsg[0] = 27;    // Command
    bufAsyncMsg[1] = 0xCE;  // Config packEt
    bufAsyncMsg[2] = freq     & 0x000000FF;
    bufAsyncMsg[3] = freq>>8  & 0x000000FF;
    bufAsyncMsg[4] = freq>>16 & 0x000000FF;
    bufAsyncMsg[5] = freq>>24 & 0x000000FF;
    bufAsyncMsg[6] = wpm & 0x000000FF;
    bufAsyncMsg[7] = pwr & 0x000000FF;
    bufAsyncMsg[8] = 0;
    AsyncUDPmsg.write(bufAsyncMsg, 8);
    udp.sendTo(AsyncUDPmsg, IP_REMOTE, UDP_PORT);
    AsyncUDPmsg.flush();
    vTaskDelay(20);
    // Repeat packet
    AsyncUDPmsg.write(bufAsyncMsg, 8);
    udp.sendTo(AsyncUDPmsg, IP_REMOTE, UDP_PORT);
  } else {
    display_valuefield_begin();
    display.print("Invalid remote IP !");
    display_display();
    delay(1000);
  }
}

// startCW - no real keying, just setting the CwToneOn flag
void startCW() {
  digitalWrite(LED1, HIGH);
  CwToneOn = 1;
  ledcWriteTone(3, RotaryEnc_BuzzerFreq.cntVal);
}

// startCW - no real keying, just clearing the CwToneOn flag
void stopCW() {
  digitalWrite(LED1, LOW);
  ledcWriteTone(3, 0);
  CwToneOn = 0;
}

//-----------------------------------------------------------------------------
// Encoding a character to Morse code and playing it
void morseEncode ( unsigned char rxd ) {
  uint8_t i, j, m, mask, morse_len;

  if (rxd >= 97 && rxd < 123) {   // > 'a' && < 'z'
    rxd = rxd - 32;         // make the character uppercase
  }
  //
  if ((rxd < 97) && (rxd > 12)) {   // above 96 no valid Morse characters
    m   = Morse_Coding_table[rxd - 32];
    morse_len = (m >> 5) & 0x07;
    mask = 0x10;
    if (morse_len >= 6) {
      morse_len = 6;
      mask = 0x20;
    }
    //
    for (i = 0; i < morse_len; i++) {
      startCW();
      if ((m & mask) > 0x00) { // Dash
        vTaskDelay(WPM_dot_delay);
        vTaskDelay(WPM_dot_delay);
        vTaskDelay(WPM_dot_delay);
      } else { // Dot
        vTaskDelay(WPM_dot_delay);
      }
      stopCW();
      // Dot-wait between played dot/dash
      vTaskDelay(WPM_dot_delay);
      mask = mask >> 1;
    } //end for(i=0...
    // Dash-wait between characters
    vTaskDelay(WPM_dot_delay);
    vTaskDelay(WPM_dot_delay);
    vTaskDelay(WPM_dot_delay);
    //
  } //if (rxd < 97...
}

void morseSendUDP (char rxd) {
  //
  if (remote_ip_valid == true) {
    // Send UDP Packet
    bufAsyncMsg[0] = rxd;
    bufAsyncMsg[1] = 0;
    AsyncUDPmsg.write(bufAsyncMsg, 1);
    udp.sendTo(AsyncUDPmsg, IP_REMOTE, UDP_PORT);
    AsyncUDPmsg.flush();
    //Serial.print(rxd);
    //Serial.print(' ');
    //Serial.println((int)rxd);
    //
    // In order to improve reliability,
    // after a few miliseconds we will send another UDP packet with repeated information
    // This packet should contain only "normally non playable characters" since we want to keep compatibility with CWDaemon !!!
    // 
    // We will need to send 2 Bytes and both Bytes will need to have values < 0d32 or >= 0d128
    // We will make the following encoding:
    // when first Byte = 0x0a   -->  next Byte which is repeated character is standard playable morse code + 128 ==> substract 128 from it's value to decode it 
    // when first Byte = 0x15   -->  next Byte which is repeated character is non-standard playable morse code  ==> no need to substract 128...
    //
    vTaskDelay(50);
    //
    //
    if (rxd <128) {
      bufAsyncMsg[0] = 0x0a;
      bufAsyncMsg[1] = rxd + 128;
    } else {
      bufAsyncMsg[0] = 0x15;
      bufAsyncMsg[1] = rxd;
    }
    AsyncUDPmsg.write(bufAsyncMsg, 2);
    udp.sendTo(AsyncUDPmsg, IP_REMOTE, UDP_PORT);
    AsyncUDPmsg.flush();
  }  // end remote_ip_valid    
}

void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}


// process replacement in html pages
String processor(const String& var) {
  if (var == "FRQ_INDEX") {
    return String(RotaryEncISR.cntVal);
  }
  if (var == "MYCALL") {
    return s_mycall_ascii_buf;
  }
  if (var == "SPEED") {
    return sspeed;
  }
  if (var == "NETWORKS" ) {
    return wifiNetworkList;
  }


  if (var == "PWR" ) {
    String rsp;
    String pwst;
    int n = PowerArrayMiliWatt_Size;
    for (int i = 0; i < n; ++i) {
      pwst = String(PowerArrayMiliWatt[i][0]);
      rsp += "<option value=\"" + String(i) + "\" ";
      if (RotaryEnc_OutPowerMiliWatt.cntVal == i)  rsp += "SELECTED";
      rsp += ">" + pwst + "</option>";
    }
    return rsp;
  }



  if (var == "APIKEY") {
    return apikey;
  }

  if (var == "DHCP") {

    String rsp = "";
    if (dhcp) rsp = "checked";

    return  rsp;
  }


  if (var == "LOCALIP") {
    return sIP;
  }
  if (var == "SUBNET") {
    return sSubnet;
  }

  if (var == "GATEWAY") {
    return sGateway;
  }

  if (var == "PDNS") {
    return sPrimaryDNS;
  }
  if (var == "SDNS") {
    return sSecondaryDNS;
  }

  return String();
}

void savePrefs() {
  //preferences.begin("my-app", false);    -- We assume this has been already called 
  preferences.putString("ssid", ssid);
  preferences.putString("password", password);
  preferences.putString("sRemote_ip", sRemote_ip);
  preferences.putString("apikey", apikey);
  preferences.putBool("dhcp", dhcp);
  preferences.putBool("con", con);
  preferences.getString("ip", sIP);
  preferences.getString("gateway", sGateway);
  preferences.getString("subnet", sSubnet);
  preferences.getString("pdns", sPrimaryDNS);
  preferences.getString("sdns", sSecondaryDNS);
  preferences.end();      // We can call preferences.end since after that we reboot
}

// Task SendMorse
void SendMorse_old_no_Queue ( void * parameter) {
  Serial.print("Send morse Task running on core ");
  Serial.println(xPortGetCoreID());
  for (;;) { //infinite loop
    if (message != "") {
      message.toUpperCase();
      int stri = message.length();
      for (int i = 0; i < stri; i++) {
        char c = message.charAt(i);
        //send(c);
        morseEncode(c);
        if (stri < message.length()) {
          stri = message.length();
        }
      }
      message = "";
    }
    vTaskDelay(20);
  }
}

// Task SendMorse
void SendMorse ( void * parameter) {
  Serial.print("Send morse Task running on core ");
  Serial.println(xPortGetCoreID());
  char cc;
  for (;;) { //infinite loop
    if (xQueueReceive(queue, (void *)&cc, 0) == pdTRUE) {
        //if (cc >= 128) {
         // if c >=128 we are sending undefined morse characted which is encoded by Encode2 routine 
        // morseEncode2(cc);
        //} else { 
         // Encoding of regular Morse character
         morseEncode(cc);
        //}
    }
    vTaskDelay(15);
  }
}

// Send all characters in message String to Queue
void messageQueueSend() {
  message.toUpperCase();
  int stri = message.length();
  for (int i = 0; i < stri; i++) {
    char c = message.charAt(i);
    xQueueSend(queue, &c, 1000);
  }
}


uint8_t wpm_delay_and_paddle_check (uint32_t delay_ms, uint8_t squeezedFlag, uint8_t return_val_when_released) {
  uint8_t return_val = 0;
  // delay WPM_dot_delay with checking if paddles released
  delay(3);  
  for(uint32_t d=3;d<delay_ms;d++) {
    delay(1);  
    if (RotaryEnc_KeyerType.cntVal == 2) {  // if Iambic-B keyer is configured
      keyerVal = digitalRead(KEYER_DASH) << 1 | digitalRead(KEYER_DOT);
      if (keyerVal==0x03) { return_val = return_val_when_released;}  // if during dot/dash playing both keyers released then play additional dash
    }
  }
  return squeezedFlag ? return_val : 0;
}

#define PADS_NOT_SQUEEZED 0
#define PADS_SQUEEZED 1

void loop()
{
  //-----------------------------------------
  if (keyerEnabled == true) {
  // -- Straight keyer
  if (RotaryEnc_KeyerType.cntVal == 1) {     //  1 = Straight
    keyerVal = digitalRead(KEYER_DOT);
    // Keyer pressed
    if (keyerVal == 0)  {
      if (keyerCWstarted == 0) {
        udp_sending_enabled = true;
        startCW();
      }
      keyerCWstarted = 1;
      delay(10);
    }
    // Keyer released
    if ((keyerVal == 1) && (keyerCWstarted == 1)){
      keyerCWstarted = 0;
      stopCW();
      delay(10);
    }
  } else {
    // -- Iambic keywer
    keyerVal   = digitalRead(KEYER_DASH) << 1 | digitalRead(KEYER_DOT);
    switch(keyerVal) {
      case 0x02:   // DOT
        udp_sending_enabled = true;
        startCW();
        //delay(WPM_dot_delay);
        // here we set iambicBfinishFlag to 0 by passing PADS_NOT_SQUEEZED to function
        iambicBfinishFlag = wpm_delay_and_paddle_check(WPM_dot_delay, PADS_NOT_SQUEEZED, 0); // delay WPM_dot_delay with checking if paddles released
        stopCW();
        iambicBfinishFlag = wpm_delay_and_paddle_check(WPM_dot_delay, PADS_NOT_SQUEEZED, 0);
        lastPlayedPad = DOT;
      break;
      case 0x01:   // DASH
        udp_sending_enabled = true;
        startCW();
        // here we set iambicBfinishFlag to 0 by passing PADS_NOT_SQUEEZED to function
        iambicBfinishFlag = wpm_delay_and_paddle_check(WPM_dash_delay, PADS_NOT_SQUEEZED, 0);
        stopCW();
        iambicBfinishFlag = wpm_delay_and_paddle_check(WPM_dot_delay, PADS_NOT_SQUEEZED, 0);  
        lastPlayedPad = DASH;
      break;
      case 0x00:   // SQUEEZED = BOTH Paddles pressed
        udp_sending_enabled = true;
        startCW();
        if (lastPlayedPad == DOT) {
          iambicBfinishFlag  = wpm_delay_and_paddle_check(WPM_dash_delay, PADS_SQUEEZED, 1);
          stopCW();
          iambicBfinishFlag += wpm_delay_and_paddle_check(WPM_dot_delay, PADS_SQUEEZED, 1);
          lastPlayedPad = DASH;
          // here iambicBfinishFlag will have max 4
        } else {
          iambicBfinishFlag = wpm_delay_and_paddle_check(WPM_dot_delay, PADS_SQUEEZED, 10);
          stopCW();
          iambicBfinishFlag += wpm_delay_and_paddle_check(WPM_dot_delay, PADS_SQUEEZED, 10);
          lastPlayedPad = DOT;
          // here iambicBfinishFlag will have max 20
        }
      break;
      case 0x03:    // No paddle pressed = no action
      break;
      default:
      break;
    }
    //
    // For Iambic-B keyer play finishing dot or dash if keys released 
    if (iambicBfinishFlag > 0) {
      udp_sending_enabled = true;
      startCW();
      delay(WPM_dot_delay);
      if (iambicBfinishFlag >= 10) {  // play DOT
        delay(WPM_dot_delay);
        delay(WPM_dot_delay);
      }
      stopCW();
      delay(WPM_dot_delay);
      iambicBfinishFlag=0;
    }
  }  // end else iambickeyer
  } // end keyerEnabled
  //
   // Process timeout for display items other than main screen
   if (program_state != S_RUN_RUN) {
     if (timeout_cnt > 6000) {
       display_valuefield_begin();
       display.print("Timeout...");
       display_display();
       delay(600);
       timeout_cnt=0;
       RotaryEncPush(&RotaryEnc_FreqWord);
       program_state = S_RUN;
     }
   }

  //
  // Morse decoding
	if (keyerEventTimeBufLen > 0) {
    timeout_cnt=0;
		tOn = keyerEventTimeBuf[keyerEventTimeBufPtrRd];
		// Determine duration of DOT; with sort of filtering mechanism
		if ((tOn & 0x10000) > 0) {  // Valid on-time
      tOnDot  = tOn     & 0xFFFF;
      tOnDash = tOnLast & 0xFFFF;
		  if (tOnDot > tOnDash) { tOnDot = tOnDash; tOnDash = tOn & 0xFFFF; }  // Exchange so that tOnDot contains smaller value (DOT?)
		  tOnTmp  = tOnDot;
		  tOnTmp += tOnDot;
		  tOnTmp += tOnDot;  // 3 * tOnDot should now give duration of DASH
		  tOnTmp1 = tOnTmp;
		  if (tOnTmp < tOnDash) { tOnTmp = tOnDash; tOnDash = tOnTmp1; }  // Exchange so that tOnTmp is always > tOnDash
      tOnTmp1 = tOnTmp - tOnDash;
		  if (tOnTmp1 < (tOnDot>>1)) {   // Difference of DASH - (3*DOT) is less than DOT-duration/2
		    tOnDotTreshold2 = tOnDotTreshold1;
		    tOnDotTreshold1 = tOnDot;
		    tOnDotTreshold  = (tOnDotTreshold1 + tOnDotTreshold2) >> 1; // average of last 2 dots
		    tOnPauseTreshold = tOnDotTreshold + tOnDotTreshold;
		    tOnDotTreshold  = tOnDotTreshold + (tOnDotTreshold >> 1);   // 1.5x DOT
		  }
		  tOnLast = tOn;
      //Serial.print("KeyerTime, DotTreshold: ");
      //Serial.print(tOn, HEX);
      //Serial.print(", ");
      //Serial.println(tOnDotTreshold, HEX);
		} // end if ((tOn...
		//
		if ((tOn & 0x10000) > 0) {  // Valid on-time
			if ((tOn & 0xFFFF) > tOnDotTreshold) {               // Dash
			  //rxd = '-';
			  decodeIndexK += decodeOffsetK;
			  lastPlayedK = LAST_WAS_DASH;
			} else {
			  //rxd = '.';                                       // Dot
			  decodeIndexK -= decodeOffsetK;
			  lastPlayedK = LAST_WAS_DOT;
			}
      decodeOffsetK >>= 1;
			decodeLenK++;
      spaceOkK = 0;
		}
		//
	  if (lastPlayedK == LAST_WAS_NOTHING) {
			if ((tOn == 0xFFFF) && (spaceOkK == 1)) {
			   //if (ignoreSpaceChar == 0) {
        cw_decoded_buf[cw_decoded_buf_ptr] = ' ';
        for(uint8_t bb=1; bb<=16; bb++) {
          uint8_t cw_decoded_buf_ptr2;
          cw_decoded_buf_ptr2 = (cw_decoded_buf_ptr + bb) % 16;
          cw_decoded_buf_ordered[bb-1] = cw_decoded_buf[cw_decoded_buf_ptr2];
        }
        cw_decoded_buf_ordered[16] = 0; // null
        cw_decoded_buf_ptr = (cw_decoded_buf_ptr + 1) % 16;
        display_decodedmorse_field_begin();
        display.print(cw_decoded_buf_ordered);
        display_display();
	      spaceOkK = 2;
			}
		}
		//
		if ((tOn == 0xFFFF) && (spaceOkK == 0)) {
			  if (decodeLenK <= 7) {
			    decodedChar = Morse_decode_table[decodeIndexK];
          if (decodedChar > 127) {
            decodedCharShown = 168; // inverted ?
          } else {
            decodedCharShown = decodedChar;
          }
          
			  } else {
  			  decodedChar = ' ';  // It is better to send just space/delay if invalid character was played...
          decodedCharShown = decodedChar;
			  }

        if ((remote_ip_valid == true) && (udp_sending_enabled == true)) {
          // Send UDP Packet
          morseSendUDP(decodedChar);
        }
        //
        cw_decoded_buf[cw_decoded_buf_ptr] = decodedCharShown;
        for(uint8_t bb=1; bb<=16; bb++) {
          uint8_t cw_decoded_buf_ptr2;
          cw_decoded_buf_ptr2 = (cw_decoded_buf_ptr + bb) % 16;
          cw_decoded_buf_ordered[bb-1] = cw_decoded_buf[cw_decoded_buf_ptr2];
        }
        cw_decoded_buf_ordered[16] = 0; // null
        cw_decoded_buf_ptr = (cw_decoded_buf_ptr + 1) % 16;
        display_decodedmorse_field_begin();
        display.print(cw_decoded_buf_ordered);
        display_display();
//  			EADOG_Putchar ((uint8_t*)NewBitmap, rxd);
			  //if (menuState == 0) { updateDispRxBuf(rxd); }
		    spaceOkK++;
			  lastPlayedK = LAST_WAS_NOTHING;
			  keyerTimeCnt = 0; //tricky? - we need to create space...
			  decodeOffsetK = 32;
			  decodeIndexK  = 64;
			  decodeLenK    = 0;
		}

    keyerEventTimeBufLen--;
    keyerEventTimeBufPtrRd++;

	  } // keyerEventTimeBufLen...
  //
  //
  // Main FSM
  //-----------------------------------------
  switch (program_state) {
    //--------------------------------
    case S_RUN:
      timeout_cnt = 0;
      keyerEnabled = true;
      display_valuefield_clear();
      program_state = S_RUN_RUN;
      break;
    //--------------------------------
    case S_RUN_RUN:
      timeout_cnt = 0;
      // Update display with Frequency info when it has changed
      if (RotaryEncISR.cntVal != RotaryEncISR.cntValOld) {
        limitRotaryEncISR_values();
        display_mainfield_begin(23);
        format_freq(RegWordToFreq(RotaryEncISR.cntVal), freq_ascii_buf, false);
        display.print(freq_ascii_buf);
        display_display();
        //JUsetRfFrequency(RegWordToFreq(RotaryEncISR.cntVal), RotaryEnc_OffsetHz.cntVal);
        display_status_bar();
        SendParamsUDPtoRemoteIP(RotaryEncISR.cntVal, RotaryEnc_KeyerSpeedWPM.cntVal, RotaryEnc_OutPowerMiliWatt.cntVal);
      }
      RotaryEncISR.cntValOld = RotaryEncISR.cntVal;
      // This has to be at very end since with RotaryEncPush we are making RotaryEncISR.cntValOld different from RotaryEncISR.cntVal

      //
      #ifdef USE_PADS_INSTEAD_OF_ROTARY_ENC
        // On TTG0_T3_BOARD we jump into Freq/change with RIGHT button
        #ifdef TTGO_T3_BOARD
          pushBtnValR = digitalRead(TTGO_BOARD_PUSH_RIGHT);
          if (pushBtnValR == PUSH_BTN_PRESSED) {
            delay(200);while(digitalRead(TTGO_BOARD_PUSH_RIGHT)==0){};
            keyerEnabled = false;
            program_state = S_CHANGE_FREQ_WITH_PADS;
            display_valuefield_begin();
            display.print("Change Freq...");
            display_display();
            vTaskDelay(400);
            RotaryEncISR.cntValOld = RotaryEncISR.cntVal + 1;   // Force display update
          }
          ReadPushBtnVal();
          if (pushBtnVal == PUSH_BTN_PRESSED) {
            WAIT_Push_Btn_Release(200);
            keyerEnabled = false;
            program_state = S_TOP_MENU_ITEMS;
            RotaryEncPop(&RotaryEnc_FreqWord);
            RotaryEncPush(&RotaryEnc_MenuSelection);
          }
        // On SX1281_QO100_TX board we jump into freq-change with normal pushbutton
        #else
          ReadPushBtnVal();
          if (pushBtnVal == PUSH_BTN_PRESSED) {
            WAIT_Push_Btn_Release(200);
            keyerEnabled = false;
            program_state = S_CHANGE_FREQ_WITH_PADS;
            display_valuefield_begin();
            display.print("Change Freq...");
            display_display();
            vTaskDelay(300);
            RotaryEncISR.cntValOld = RotaryEncISR.cntVal + 1;   // Force display update
          }
        #endif
      // else when using standard rotary encoder
      #else
          ReadPushBtnVal();
          if (pushBtnVal == PUSH_BTN_PRESSED) {
            WAIT_Push_Btn_Release(200);
            program_state = S_TOP_MENU_ITEMS;
            RotaryEncPop(&RotaryEnc_FreqWord);
            RotaryEncPush(&RotaryEnc_MenuSelection);
          }
      #endif
      //
      break;
    //--------------------------------
    #ifdef USE_PADS_INSTEAD_OF_ROTARY_ENC
    case S_CHANGE_FREQ_WITH_PADS:
      // 
      pushBtnValL = digitalRead(TTGO_BOARD_PUSH_LEFT);
      if ((pushBtnValL == PUSH_BTN_PRESSED) && (sentCnt<10)) {
        //delay(200);while(digitalRead(TTGO_BOARD_PUSH_LEFT)==0){};
        //vTaskDelay(400);
        morseSendUDP(eee_message_buf[0]);
        vTaskDelay(10);
        sentCnt++;
      }      
      // Update display with Frequency info when it has changed      
      if (RotaryEncISR.cntVal != RotaryEncISR.cntValOld) {
        limitRotaryEncISR_values();
        display_mainfield_begin(23);
        display.setTextColor(MY_GREEN);
        format_freq(RegWordToFreq(RotaryEncISR.cntVal), freq_ascii_buf, true);
        display.print(freq_ascii_buf);
        display_display();
        //display_valuefield_begin();
        //display.print("Change Freq...");
        //display_display();
        timeout_cnt = 0;
        //JUsetRfFrequency(RegWordToFreq(RotaryEncISR.cntVal), RotaryEnc_OffsetHz.cntVal);
//        display_status_bar();
        SendParamsUDPtoRemoteIP(RotaryEncISR.cntVal, RotaryEnc_KeyerSpeedWPM.cntVal, RotaryEnc_OutPowerMiliWatt.cntVal);
        RotaryEncPop(&RotaryEnc_FreqWord);  // Store in RotaryEnc_FreqWord, this is needed for case when Timeout occurs... 
        sentCnt=0;
      }

      RotaryEncISR.cntValOld = RotaryEncISR.cntVal;
      // This has to be at very end since with RotaryEncPush we are making RotaryEncISR.cntValOld different from RotaryEncISR.cntVal
      #ifdef TTGO_T3_BOARD
        pushBtnValR = digitalRead(TTGO_BOARD_PUSH_RIGHT);
        if (pushBtnValR == PUSH_BTN_PRESSED) {
          delay(200);while(digitalRead(TTGO_BOARD_PUSH_RIGHT)==0){};
          program_state = S_RUN;
          RotaryEncISR.cntValOld = RotaryEncISR.cntVal + 1; // force display update
          //RotaryEncPop(&RotaryEnc_FreqWord);
          //RotaryEncPush(&RotaryEnc_MenuSelection);
        }
      #else
        ReadPushBtnVal();
        if (pushBtnVal == PUSH_BTN_PRESSED) {
          WAIT_Push_Btn_Release(200);
          program_state = S_TOP_MENU_ITEMS;
          RotaryEncISR.cntValOld = RotaryEncISR.cntVal + 1; // force display update
          RotaryEncPop(&RotaryEnc_FreqWord);
          RotaryEncPush(&RotaryEnc_MenuSelection);
        }
      #endif
      break;
      #endif   // USE_PADS_INSTEAD_OF_ROTARY_ENC
    //--------------------------------
    case S_TOP_MENU_ITEMS:
      // Wrap aroud
      menuIndex = RotaryEncISR.cntVal % (sizeof(TopMenuArray) / sizeof(TopMenuArray[0]));
      // Update display if there is change
      if (RotaryEncISR.cntVal != RotaryEncISR.cntValOld) {
        timeout_cnt = 0;
        display_mainfield_begin(10);
        display.print(TopMenuArray[menuIndex]);
        display_display();
      }
      RotaryEncISR.cntValOld = RotaryEncISR.cntVal;
      // Decide into which menu item to go based on selection
      ReadPushBtnVal();
      if (pushBtnVal == PUSH_BTN_PRESSED) {
        WAIT_Push_Btn_Release(200);
        RotaryEncPop(&RotaryEnc_MenuSelection);
        switch (menuIndex) {
          // -----------------------------
          case 0:
            program_state = S_RUN;
            RotaryEncPush(&RotaryEnc_FreqWord); // makes RotaryEncISR.cntValOld different from RotaryEncISR.cntVal  ==> forces display update
            break;
          // -----------------------------
          case 1:
            cq_running = false;
            program_state = S_RUN_CQ_BLOCKING;    //S_SEND_CQ_UDP;
            display_valuefield_begin();
            display.print("CQ...");
            display_display();
            break;
          // -----------------------------
          case 2:
            program_state = S_SET_SPEED_WPM;
            RotaryEncPush(&RotaryEnc_KeyerSpeedWPM); // makes RotaryEncISR.cntValOld different from RotaryEncISR.cntVal  ==> forces display update
            break;
          // -----------------------------
          case 3:
            program_state = S_SET_OUTPUT_POWER;
            RotaryEncPush(&RotaryEnc_OutPowerMiliWatt); // makes RotaryEncISR.cntValOld different from RotaryEncISR.cntVal  ==> forces display update
            break;
          // -----------------------------
          case 4:
            program_state = S_SAVE_SPEED_AND_POWER;
            break;
          // -----------------------------
          case 5:
            program_state = S_SET_KEYER_TYPE;
            RotaryEncPush(&RotaryEnc_KeyerType); // makes RotaryEncISR.cntValOld different from RotaryEncISR.cntVal  ==> forces display update
            break;
          // -----------------------------
          case 6:
            program_state = S_SET_BUZZER_FREQ;
            RotaryEncPush(&RotaryEnc_BuzzerFreq); // makes RotaryEncISR.cntValOld different from RotaryEncISR.cntVal  ==> forces display update
            break;
          // -----------------------------
          case 7:
            program_state  = S_SET_TEXT_GENERIC;
            set_text_state = S_SET_MY_CALL;
            s_general_ascii_buf = s_mycall_ascii_buf.substring(0);
            RotaryEncPush(&RotaryEnc_TextInput_Char_Index);
            RotaryEncISR.cntVal = s_general_ascii_buf[0];
            general_ascii_buf_index = 0;
            break;
          // -----------------------------
          case 8:
            program_state  = S_SET_TEXT_GENERIC;
            set_text_state = S_SET_WIFI_SSID;
            //s_general_ascii_buf = s_wifi_ssid_ascii_buf.substring(0);
            s_general_ascii_buf = ssid.substring(0);
            RotaryEncPush(&RotaryEnc_TextInput_Char_Index);
            RotaryEncISR.cntVal = s_general_ascii_buf[0];
            general_ascii_buf_index = 0;
            break;
          // -----------------------------
          case 9:
            program_state  = S_SET_TEXT_GENERIC;
            set_text_state = S_SET_WIFI_PWD;
            //s_general_ascii_buf = s_wifi_pwd_ascii_buf.substring(0);
            s_general_ascii_buf = password.substring(0);
            RotaryEncPush(&RotaryEnc_TextInput_Char_Index);
            RotaryEncISR.cntVal = s_general_ascii_buf[0];
            general_ascii_buf_index = 0;
            break;
          // -----------------------------
          case 10:
            program_state  = S_SET_TEXT_GENERIC;
            set_text_state = S_SET_sRemote_ip;
            s_general_ascii_buf = sRemote_ip.substring(0);
            RotaryEncPush(&RotaryEnc_TextInput_Char_Index);
            RotaryEncISR.cntVal = s_general_ascii_buf[0];
            general_ascii_buf_index = 0;
            break;
          // -----------------------------
          case 11:
            program_state = S_SET_DISP_ORIENTATION;
            RotaryEncPush(&RotaryEnc_DispOrientation); // makes RotaryEncISR.cntValOld different from RotaryEncISR.cntVal  ==> forces display update
            break;
          // -----------------------------
          case 12:
            program_state = S_WIFI_RECONNECT;
            break;
          // -----------------------------
          default:
            program_state = S_RUN;
            break;
        }
      }
      break;
    //--------------------------------
    case S_SEND_CQ_UDP:
        if (remote_ip_valid == true) {
          // Send UDP Packet
         cq_message = cq_message_buf;
         cq_message += s_mycall_ascii_buf;
         cq_message += cq_message;
         cq_message += cq_message_end_buf;
         memcpy(bufAsyncMsg, cq_message.c_str(), cq_message.length());
         AsyncUDPmsg.write(bufAsyncMsg, cq_message.length());
         udp.sendTo(AsyncUDPmsg, IP_REMOTE, UDP_PORT);
         AsyncUDPmsg.flush();
        }
        display_valuefield_begin();
        display.print("CQ sent via UDP...");
        delay(2000);
        RotaryEncPush(&RotaryEnc_FreqWord);
        program_state = S_RUN;

      break;
    //--------------------------------
    case S_RUN_CQ_SENDMORSE_TASK:
      if (cq_running == false) {
      message = " ";
      message += cq_message_buf;
      message += s_mycall_ascii_buf;
      message += message;
      message += cq_message_end_buf;
      messageQueueSend();
      cq_running = true;
      }
      //
      ReadPushBtnVal();
      if (pushBtnVal == PUSH_BTN_PRESSED) {
        cq_running = false;
        xQueueReset( queue );
        WAIT_Push_Btn_Release(200);
        RotaryEncPush(&RotaryEnc_FreqWord);
        program_state = S_RUN;
      }
      break;
    //--------------------------------
    case S_RUN_CQ_BLOCKING:
      udp_sending_enabled = false;  // we will send UDP from morseEncode function to make the UDP sending smoother (the SendMorse task introduces slight delay)
      stop = 0;
      for (int j = 0; (j < 3) && !stop; j++) {
        // CQ
        for (int i = 0; (i < sizeof(cq_message_buf)) && !stop; i++) {
          morseSendUDP(cq_message_buf[i]);  // Send UDP upfront, then play
          morseEncode (cq_message_buf[i]);
          timeout_cnt = 0; // do not allow to timeout this operation
          ReadPushBtnVal();
          if (pushBtnVal == PUSH_BTN_PRESSED) {
            stop++;
          }
        }
        // My Call
        for (int i = 0; (i < s_mycall_ascii_buf.length()) && !stop; i++) {
          morseSendUDP(s_mycall_ascii_buf[i]);
          morseEncode (s_mycall_ascii_buf[i]);
          timeout_cnt = 0; // do not allow to timeout this operation
          ReadPushBtnVal();
          if (pushBtnVal == PUSH_BTN_PRESSED) {
            stop++;
          }
        }
        //
        morseSendUDP(' ');
        morseEncode (' ');
        // My Call
        for (int i = 0; (i < s_mycall_ascii_buf.length()) && !stop; i++) {
          morseSendUDP(s_mycall_ascii_buf[i]);
          morseEncode (s_mycall_ascii_buf[i]);
          timeout_cnt = 0; // do not allow to timeout this operation
          ReadPushBtnVal();
          if (pushBtnVal == PUSH_BTN_PRESSED) {
            stop++;
          }
        }
      } // j
      // +K
      for (int i = 0; (i < sizeof(cq_message_end_buf)) && !stop; i++) {
        morseSendUDP(cq_message_end_buf[i]);
        morseEncode (cq_message_end_buf[i]);
        timeout_cnt = 0; // do not allow to timeout this operation
        ReadPushBtnVal();
        if (pushBtnVal == PUSH_BTN_PRESSED) {
          stop++;
        }
      }
      //
      WAIT_Push_Btn_Release(200);
      //udp_sending_enabled = true;
      RotaryEncPush(&RotaryEnc_FreqWord);
      program_state = S_RUN;
      display_display();
      break;
    //--------------------------------
    case S_SET_SPEED_WPM:
      if (RotaryEncISR.cntVal != RotaryEncISR.cntValOld) {
        timeout_cnt = 0;
        limitRotaryEncISR_values();
        display_valuefield_begin();
        display.print(RotaryEncISR.cntVal);
        display_display();
        Calc_WPM_dot_delay(RotaryEncISR.cntVal);  // this will set the WPM_dot_delay variable
      }
      RotaryEncISR.cntValOld = RotaryEncISR.cntVal;
      ReadPushBtnVal();
      if (pushBtnVal == PUSH_BTN_PRESSED) {
        WAIT_Push_Btn_Release(200);
        SendParamsUDPtoRemoteIP(RotaryEnc_FreqWord.cntVal, RotaryEncISR.cntVal, RotaryEnc_OutPowerMiliWatt.cntVal);
        speed = RotaryEncISR.cntVal;
        sspeed = String(speed);
        //preferences.putInt("KeyerWPM", RotaryEncISR.cntVal);    -- saving in S_SAVE_SPEED_AND_POWER state
        RotaryEncPop(&RotaryEnc_KeyerSpeedWPM);
        RotaryEncPush(&RotaryEnc_FreqWord);
        program_state = S_RUN;
      }
      break;
    //--------------------------------
    case S_SET_KEYER_TYPE:
      if (RotaryEncISR.cntVal != RotaryEncISR.cntValOld) {
        RotaryEncISR.cntVal = RotaryEncISR.cntVal % 3;  // limit values to 0-2 / 0 = Iambic-A, 1 = Straight, 2 = Iambic-B
        timeout_cnt = 0;
        display_valuefield_begin();
        switch(RotaryEncISR.cntVal) {
          case 0:
            display.print("Iambic A  ");
          break;
          case 1:
            display.print("Straight  ");
          break;
          case 2:
            display.print("Iambic B  ");
          break;
          default:
          break;
        }
        display_display();
      }
      RotaryEncISR.cntValOld = RotaryEncISR.cntVal;
      //
      ReadPushBtnVal();
      if (pushBtnVal == PUSH_BTN_PRESSED) {
        WAIT_Push_Btn_Release(200);
        preferences.putInt("KeyerType", RotaryEncISR.cntVal);
        RotaryEncPop(&RotaryEnc_KeyerType);
        RotaryEncPush(&RotaryEnc_FreqWord);
        program_state = S_RUN;
      }
      break;
    //--------------------------------
    case S_SET_OUTPUT_POWER:
      if (RotaryEncISR.cntVal != RotaryEncISR.cntValOld) {
        timeout_cnt = 0;
        limitRotaryEncISR_values();
        display_valuefield_begin();
        //RotaryEncISR.cntVal = RotaryEncISR.cntVal % (sizeof(PowerArrayMiliWatt) / sizeof(uint32_t)); // safe
        RotaryEncISR.cntVal = RotaryEncISR.cntVal % PowerArrayMiliWatt_Size;
        display.print(PowerArrayMiliWatt[RotaryEncISR.cntVal][0]);
        display.print(" mW    ");
        display_display();
        // We do not set output power locally
        //LT.setTxParams(PowerArrayMiliWatt[RotaryEncISR.cntVal][1], RADIO_RAMP_10_US);
        // instead we send UDP packet
        SendParamsUDPtoRemoteIP(RotaryEnc_FreqWord.cntVal, RotaryEnc_KeyerSpeedWPM.cntVal, RotaryEncISR.cntVal);
      }
      RotaryEncISR.cntValOld = RotaryEncISR.cntVal;
      ReadPushBtnVal();
      if (pushBtnVal == PUSH_BTN_PRESSED) {
        WAIT_Push_Btn_Release(200);
        //preferences.putInt("OutPower", RotaryEncISR.cntVal);    -- saving in S_SAVE_SPEED_AND_POWER state
        // instead we send UDP packet
        SendParamsUDPtoRemoteIP(RotaryEnc_FreqWord.cntVal, RotaryEnc_KeyerSpeedWPM.cntVal, RotaryEncISR.cntVal);
        RotaryEncPop(&RotaryEnc_OutPowerMiliWatt);
        RotaryEncPush(&RotaryEnc_FreqWord);
        program_state = S_RUN;
      }
      break;
    //--------------------------------
    case S_SAVE_SPEED_AND_POWER:
      preferences.putInt("KeyerWPM", RotaryEnc_KeyerSpeedWPM.cntVal);
      preferences.putInt("OutPower", RotaryEnc_OutPowerMiliWatt.cntVal);
      display_valuefield_begin();
      display.print("WPM & Power saved.");
      display_display();
      delay(1000);
      RotaryEncPush(&RotaryEnc_FreqWord);
      program_state = S_RUN;
      break;
    //--------------------------------
    case S_SET_BUZZER_FREQ:
      if (RotaryEncISR.cntVal != RotaryEncISR.cntValOld) {
        timeout_cnt = 0;
        limitRotaryEncISR_values();
        display_valuefield_begin();
        display.print(RotaryEncISR.cntVal);
        display_display();
        // Short beep with new tone value
        ledcWriteTone(3, RotaryEncISR.cntVal);
        delay(100);
        ledcWriteTone(3, 0);
      }
      RotaryEncISR.cntValOld = RotaryEncISR.cntVal;
      //
      ReadPushBtnVal();
      if (pushBtnVal == PUSH_BTN_PRESSED) {
        WAIT_Push_Btn_Release(200);
        preferences.putInt("BuzzerFreq", RotaryEncISR.cntVal);
        RotaryEncPop(&RotaryEnc_BuzzerFreq);
        RotaryEncPush(&RotaryEnc_FreqWord);
        program_state = S_RUN;
      }
      break;
    //--------------------------------
    case S_SET_TEXT_GENERIC:
      // Reset timeout_cnt when there is activity with encoder
      if (RotaryEncISR.cntVal != RotaryEncISR.cntValOld) { timeout_cnt = 0; }
      limitRotaryEncISR_values();
      // Blinking effect on selected character
      if (loopCnt % 2) {
        s_general_ascii_buf[general_ascii_buf_index] = RotaryEncISR.cntVal;
      } else {
        s_general_ascii_buf[general_ascii_buf_index] = ' ';
      }
      display_valuefield_begin2(s_general_ascii_buf);
      // Set it back to valid character after displaying
      s_general_ascii_buf[general_ascii_buf_index] = RotaryEncISR.cntVal;
      RotaryEncISR.cntValOld = RotaryEncISR.cntVal;
      delay(100);
      display_valuefield_clear2(s_general_ascii_buf);
      //
      // With pushbutton pressed either go to next character of finish if selected character was decimal 127 = ⌂ (looks like house icon with roof)
      ReadPushBtnVal();
      if (pushBtnVal == PUSH_BTN_PRESSED) {
        timeout_cnt = 0;
        WAIT_Push_Btn_Release(200);
        // 127 = End of string
        if (RotaryEncISR.cntVal == 127) {
          s_general_ascii_buf[general_ascii_buf_index] = 0;  // Terminate string with null
          switch (set_text_state) {
            //---------------------
            case S_SET_MY_CALL:
              preferences.putString("MyCall", s_general_ascii_buf);
              s_mycall_ascii_buf = s_general_ascii_buf.substring(0);
              break;
            //---------------------
            case S_SET_WIFI_SSID:
              preferences.putString("ssid", s_general_ascii_buf);
              //s_wifi_ssid_ascii_buf = s_general_ascii_buf.substring(0);
              ssid = s_general_ascii_buf.substring(0);
              break;
            //---------------------
            case S_SET_WIFI_PWD:
              preferences.putString("password", s_general_ascii_buf);
              //s_wifi_pwd_ascii_buf = s_general_ascii_buf.substring(0);
              password = s_general_ascii_buf.substring(0);
              break;
            //---------------------
            case S_SET_sRemote_ip:
              preferences.putString("sRemote_ip", s_general_ascii_buf);
              sRemote_ip = s_general_ascii_buf.substring(0);
              remote_ip_valid = IP_REMOTE.fromString(sRemote_ip);
              if (remote_ip_valid == false) {
                display_valuefield_begin();
                display.print("Invalid Remote IP !");
                display_display();
                delay(4000);
              }
              break;
            //---------------------
            default:
              break;
          }
          RotaryEncPush(&RotaryEnc_FreqWord);
          program_state = S_RUN;
        } else {
          general_ascii_buf_index++;
          if (general_ascii_buf_index >= s_general_ascii_buf.length()) {
            s_general_ascii_buf += s_general_ascii_buf[general_ascii_buf_index-1];  // ### Add last character
            RotaryEncISR.cntVal = s_general_ascii_buf[general_ascii_buf_index];
          } else {
            RotaryEncISR.cntVal = s_general_ascii_buf[general_ascii_buf_index];
          }
          RotaryEncISR.cntValOld = RotaryEncISR.cntVal + 1;   // Force display update in next cycle
        }
      }
      break;
    //--------------------------------
    case S_SET_DISP_ORIENTATION:
      if (RotaryEncISR.cntVal != RotaryEncISR.cntValOld) {
        timeout_cnt = 0;
        display_valuefield_begin();
        display.print(RotaryEncISR.cntVal % 2 ?  "Upside Dwn" : "Normal    ");
        display_display();
      }
      RotaryEncISR.cntValOld = RotaryEncISR.cntVal;
      //
      ReadPushBtnVal();
      if (pushBtnVal == PUSH_BTN_PRESSED) {
        WAIT_Push_Btn_Release(200);
        preferences.putInt("DispOrientation", RotaryEncISR.cntVal);
        delay(100);
        ESP.restart();  // just restart the board
      }
      break;
    //--------------------------------
    case S_WIFI_RECONNECT:
      ESP.restart();  // just restart the board
      break;
    //--------------------------------
    default:
      break;
  } // switch program_state

  loopCnt++;

}  // loop


void IRAM_ATTR onTimer() {
  //portENTER_CRITICAL_ISR(&timerMux);
  //interruptCounter++;
  //digitalWrite(LED_PIN,LEDtoggle);
  //LEDtoggle = !LEDtoggle;
  //portEXIT_CRITICAL_ISR(&timerMux);

  uint32_t incr_val;
  //
  timeout_cnt++;
  //
  if (ISR_cnt != 255) ISR_cnt++;
  //
  //keyerVal   = digitalRead(KEYER_DASH) << 1 | digitalRead(KEYER_DOT);
  //
  #ifdef USE_PADS_INSTEAD_OF_ROTARY_ENC
  // Process pressing of the pads
  if (keyerEnabled == false) {
  // Keyer pressed DOT
  if (digitalRead(KEYER_DOT) == 0) {
    if (ISR_cnt == 255) {
      RotaryEncISR.cntVal -= RotaryEncISR.cntIncr;
      ISR_cnt = 255 - 100;   // 40 * 2.5msec = 100msec
    }
    if (ISR_cnt == 255-60) {
      if (digitalRead(KEYER_DOT) == 0) {
        ISR_cnt = 255;   // accelerate...
      }
    }
  }
  // Keyer pressed DASH
  if (digitalRead(KEYER_DASH) == 0) {
    if (ISR_cnt == 255) {
      RotaryEncISR.cntVal += RotaryEncISR.cntIncr;
      ISR_cnt = 255 - 100;   // 40 * 2.5msec = 100msec
    }
    if (ISR_cnt == 255-60) {
      if (digitalRead(KEYER_DASH) == 0) {
        ISR_cnt = 255;   // accelerate...
      }
    }
  }
  } // end keyerEnabled
  #else
  // SW debounce
  rotaryA_Val = (rotaryA_Val << 1 | (uint8_t)digitalRead(ROTARY_ENC_A)) & 0x0F;
  //
  // Detecting edge --> 1110
  if (rotaryA_Val == 0x0E) {
    rotaryB_Val = digitalRead(ROTARY_ENC_B);
    // Rotation speedup
    (ISR_cnt <= 12) ? cntIncrISR = RotaryEncISR.cntIncr << 4 : cntIncrISR = RotaryEncISR.cntIncr;
    ISR_cnt = 0;
    //
    if (rotaryB_Val == 0) {
      RotaryEncISR.cntVal -= cntIncrISR;
    } else {
      RotaryEncISR.cntVal += cntIncrISR;
    }
  }
  #endif
  //
  // Morse decoding
  // The decoding is based on duration of CwToneOn (which means that key is pressed)
  //
		if (CwToneOn == 0) {  // Key OFF
      //
			if (keyerStateLast == KEYER_ON) {           // Transition from ON to OFF  -->
				keyerEventTime = keyerTimeCnt | 0x10000;  // Mask for valid on-time
				keyerEventTimeBuf[keyerEventTimeBufPtr++] = keyerEventTime;
				keyerEventTimeBufLen++;
				keyerTimeCnt = 0;
			}
			if ((keyerStateLast == KEYER_OFF) && (keyerTimeCnt == tOnPauseTreshold)) {
				keyerEventTime = 0xFFFF;    // End of character --> we can decode
				keyerEventTimeBuf[keyerEventTimeBufPtr++] = keyerEventTime;
				keyerEventTimeBufLen++;
			}
			keyerStateLast = KEYER_OFF;
		} else {	// Key ON
      //
			if (keyerStateLast == KEYER_OFF) {  // Transition from OFF to ON  --> we counted pause
				keyerEventTime = keyerTimeCnt & 0xFFFF;    // Pause-time has values up to 0xFFFE
				keyerEventTimeBuf[keyerEventTimeBufPtr++] = keyerEventTime;
				keyerEventTimeBufLen++;
				keyerTimeCnt = 0;
			}
			keyerStateLast = KEYER_ON;
		}
		//
		//(keyerTimeCnt == 126) ? keyerTimeCnt = keyerTimeCnt : keyerTimeCnt++;
    if (keyerTimeCnt != 0xFFFE) {
      keyerTimeCnt++;
    }

}




void setup() {

  // Setup pin as output for indicator LED or keying output (via Open-collector tranzistor)
  pinMode(LED1, OUTPUT);    
  // Configure BUZZER functionalities.
  ledcSetup(3, 8000, 8);   //PWM Channel, Freq, Resolution
  /// Attach BUZZER pin.
  ledcAttachPin(BUZZER, 3);  // Pin, Channel

  // Mount SPIFFS (filesystem, the HTML files must be flashed too by other utility)
  if (!SPIFFS.begin()) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    while (1) {
      delay(20);
      ledcWriteTone(3, 784);  // tone g
      delay(20);
      ledcWriteTone(3, 0);
    }
  }
  
  pinMode(ROTARY_ENC_A,    INPUT_PULLUP);
  pinMode(ROTARY_ENC_B,    INPUT_PULLUP);
  pinMode(ROTARY_ENC_PUSH, INPUT_PULLUP);
  pinMode(KEYER_DOT,       INPUT_PULLUP);
  pinMode(KEYER_DASH,      INPUT_PULLUP);

#ifdef USE_PADS_INSTEAD_OF_ROTARY_ENC
  pinMode(TTGO_BOARD_PUSH_LEFT, INPUT_PULLUP);
  pinMode(TTGO_BOARD_PUSH_RIGHT, INPUT_PULLUP);
#endif

  // Timer for ISR which is processing rotary encoder events
  timer = timerBegin(0, 80, true);
  timerAttachInterrupt(timer, &onTimer, true);
  timerAlarmWrite(timer, TIMER_PERIOD_USEC, true);  // 2500 = 2.5 msec
  timerAlarmEnable(timer);

  RotaryEnc_FreqWord.cntVal  = FreqToRegWord(Frequency);
  RotaryEnc_FreqWord.cntMin  = FreqToRegWord(2400000000);
  RotaryEnc_FreqWord.cntMax  = FreqToRegWord(2400500000);
  RotaryEnc_FreqWord.cntIncr = 1;
  RotaryEnc_FreqWord.cntValOld = RotaryEnc_FreqWord.cntVal + 1;   // MAke it different in order to update the display and eventually send out UDP packet

  // With MenuSelection we start counter at high value around 1 milion so that we can count up/down
  // The weird formula below using ...sizeof(TopMenuArray)... will just make sure that initial menu is at index 1 = "CQ..."
  RotaryEnc_MenuSelection    = { int(1000000/(sizeof(TopMenuArray) / sizeof(TopMenuArray[0]))) * (sizeof(TopMenuArray) / sizeof(TopMenuArray[0])) + 1, 0, 2000000, 1, 0}; 
  //
  RotaryEnc_KeyerSpeedWPM    = {20, 10, 30, 1, 0};
  RotaryEnc_KeyerType        = {1000000, 0, 2000000, 1, 0};   // We will implement modulo
  RotaryEnc_OutPowerMiliWatt = {PowerArrayMiliWatt_Size - 1, 0, PowerArrayMiliWatt_Size - 1, 1, 0};
  RotaryEnc_BuzzerFreq       = {600, 0, 2000, 100, 0};
  RotaryEnc_TextInput_Char_Index  = {65, 33, 127, 1, 66};
  RotaryEnc_DispOrientation  = {1000000, 0, 2000000, 1, 0};   // We will implement modulo


  // Get configuration values stored in EEPROM/FLASH
  preferences.begin("my-app", false);   // false = RW mode
  // Get the counter value, if the key does not exist, return a default value of XY
  // Note: Key name is limited to 15 chars.
  RotaryEnc_KeyerSpeedWPM.cntVal    = preferences.getInt("KeyerWPM", 20);
  speed = RotaryEnc_KeyerSpeedWPM.cntVal;
  sspeed = String(speed);
  Calc_WPM_dot_delay(RotaryEnc_KeyerSpeedWPM.cntVal);
  RotaryEnc_KeyerType.cntVal        = preferences.getInt("KeyerType", 0);
  RotaryEnc_BuzzerFreq.cntVal       = preferences.getInt("BuzzerFreq", 600);
  RotaryEnc_OutPowerMiliWatt.cntVal = preferences.getInt("OutPower", PowerArrayMiliWatt_Size - 1); // Max output power
  RotaryEnc_DispOrientation.cntVal  = preferences.getInt("DispOrientation", 0);

  s_mycall_ascii_buf                  = preferences.getString("MyCall", "MyCall???");
  //s_wifi_ssid_ascii_buf               = preferences.getString("ssid", "SSID??");
  //s_wifi_pwd_ascii_buf                = preferences.getString("password",  "PWD???");

  dhcp = preferences.getBool("dhcp", 1);
  con  = preferences.getBool("con", 0);
  ssid = preferences.getString("ssid", "WiFi SSID???");
  password = preferences.getString("password", "password???");
  sRemote_ip = preferences.getString("sRemote_ip", "192.168.1.100");
  remote_ip_valid = IP_REMOTE.fromString(sRemote_ip);
  apikey = preferences.getString("apikey","1111");
  sIP        = preferences.getString("ip", "192.168.1.200");
  sGateway   = preferences.getString("gateway", "192.168.1.1");
  sSubnet    = preferences.getString("subnet", "255.255.255.0");
  sPrimaryDNS = preferences.getString("pdns", "8.8.8.8");
  sSecondaryDNS = preferences.getString("sdns", "8.8.4.4");

  if (remote_ip_valid == false) {
    display_valuefield_begin();
    display.print("Invalid Remote IP !");
    display_display();
    delay(4000);
  }

  //preferences.end();   -- do not call prefs.end since we assume writing to NV memory later in application

  RotaryEncPush(&RotaryEnc_FreqWord);

#ifdef TTGO_T3_BOARD
  display.init();
  (RotaryEnc_DispOrientation.cntVal % 2) ? display.setRotation(1) : display.setRotation(3);
  display.fillScreen(TFT_BLACK);
#else
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  display.clearDisplay();
#endif
  // Initial splash screen
  display.fillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, MY_BLACK); // x, y, width, height
  display.setTextSize(MY_TEXT_SIZE);
  display.setTextColor(MY_WHITE);
  display.setCursor(1*MY_SCALE, 10*MY_SCALE);
  display.println("    CW UDP Keyer    ");
  display.setCursor(7*MY_SCALE, 20*MY_SCALE);
  display.println("by OK1CDJ & OM2JU");
  display.setCursor(13*MY_SCALE, 35*MY_SCALE);
  display.println("http://hamshop.cz");
  display.setCursor(10*MY_SCALE, 50*MY_SCALE);
  display.println("Starting WiFi...");
  display.drawFastVLine(0, 0, SCREEN_HEIGHT, MY_WHITE);
  display.drawFastVLine(SCREEN_WIDTH - 1, 0, SCREEN_HEIGHT, MY_WHITE);
  display.drawFastHLine(0, 0, SCREEN_WIDTH, MY_WHITE);
  display.drawFastHLine(0, SCREEN_HEIGHT-1, SCREEN_WIDTH, MY_WHITE);
  display_display();
  delay(500); 


  for(uint8_t bb=0; bb<16; bb++) {
    cw_decoded_buf[bb] = ' ';
  }
 // We send first character with configured speed so that Morse decoding adapts to actual speed; it will display invalid info
 // The character should contain dots and dashes too
  morseEncode('R');
 
  Serial.begin(115200);
  Serial.println();
  Serial.print(F(__TIME__));
  Serial.print(F(" "));
  Serial.println(F(__DATE__));
  Serial.println();
  Serial.println(F(Program_Version));
  Serial.println();
  Serial.println();

  //SPI.begin(SCK, MISO, MOSI, NSS);
  //Serial.println(F("SPI OK..."));

  //
  queue = xQueueCreate( 512, sizeof( char ) );
  xTaskCreatePinnedToCore(SendMorse, "Task1", 10000, NULL, 1, NULL,  0);

  
  
  // Try connect to WIFI
  if (!wifiConfigRequired)
  {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), password.c_str());
    if (WiFi.waitForConnectResult() != WL_CONNECTED) {
      Serial.printf("WiFi Failed!\n");
      WiFi.disconnect();
      wifiConfigRequired = true;
    }
    if (!wifiConfigRequired) {
      Serial.print("WIFI IP Address: ");
      IP = WiFi.localIP();
      Serial.println(IP);
    }
  }

// not connected and make AP
if (wifiConfigRequired) {

    wifiConfigRequired = true;
    //Serial.println("Start AP");
    Serial.printf("WiFi is not connected or configured. Starting AP mode\n");
    //ssid_ap = "CW_UDP_KEYER_AP";   // ### Already defined
    WiFi.mode(WIFI_AP);  
    WiFi.softAP(ssid_ap.c_str());
    delay(100);
    WiFi.softAPConfig(AP_IP, AP_gateway, AP_subnet);
    delay(100);
    IP = WiFi.softAPIP();
    wifiSoftAP = true;
    Serial.printf("SSID: %s, IP: ", ssid_ap.c_str());
    Serial.println(IP);
  }


   server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {

    if ((request->hasParam(PARAM_APIKEY) && request->getParam(PARAM_APIKEY)->value() == apikey) || wifiConfigRequired) {
      // Freq. Index
      if (request->hasParam(PARAM_FRQ_INDEX)) {
        sfreq = request->getParam(PARAM_FRQ_INDEX)->value();
        RotaryEncISR.cntVal = sfreq.toInt();
        Serial.print("Freq changed: ");
        Serial.println(sfreq);
        RotaryEncISR.cntValOld++;   // Change Old value in order to force display update
        program_state = S_RUN;
      }
      // Speed WPM
      if (request->hasParam(PARAM_SPEED)) {
        sspeed = request->getParam(PARAM_SPEED)->value();
        speed = sspeed.toInt();
        RotaryEnc_KeyerSpeedWPM.cntVal = speed;
        Calc_WPM_dot_delay (speed);
        program_state = S_RUN;
        RotaryEncISR.cntValOld++;   // Change Old value in order to force display update
      }
      // Output Power
      if (request->hasParam(PARAM_PWR)) {
        spwr = request->getParam(PARAM_PWR)->value();
        Serial.print("Power changed: ");
        Serial.println(spwr);
        RotaryEnc_OutPowerMiliWatt.cntVal = spwr.toInt();
        //LT.setTxParams(PowerArrayMiliWatt[RotaryEnc_OutPowerMiliWatt.cntVal][1], RADIO_RAMP_10_US);
        RotaryEncISR.cntValOld++;   // Change Old value in order to force display update
        program_state = S_RUN;
      }
      // Message
      if (request->hasParam(PARAM_MESSAGE)) {
        message = " ";
        message += request->getParam(PARAM_MESSAGE)->value();
        messageQueueSend();
      }
      // Break
      if (request->hasParam(PARAM_CMD_B)) {
        scmd = request->getParam(PARAM_CMD_B)->value();
        if (scmd.charAt(0) == 'B') {
          xQueueReset( queue );
          stopCW();
        }
      }
      // CQ
      if (request->hasParam(PARAM_CMD_C)) {
        scmd = request->getParam(PARAM_CMD_C)->value();
        if (scmd.charAt(0) == 'C') {
          message = " ";
          message += cq_message_buf;
          message += s_mycall_ascii_buf;
          message += message;
          message += cq_message_end_buf;
          messageQueueSend();
        }
      }
      // DOTS
      if (request->hasParam(PARAM_CMD_D)) {
        scmd = request->getParam(PARAM_CMD_D)->value();
        if (scmd.charAt(0) == 'D') {
          message = " ";
          message += eee_message_buf;
          messageQueueSend();
        }
      }
      // Tune
      if (request->hasParam(PARAM_CMD_T)) {
        scmd = request->getParam(PARAM_CMD_T)->value();
        if (scmd.charAt(0) == 'T') {
          startCW();
          delay(3000);
          stopCW();
        }
      }      

      request->send(SPIFFS, "/index.html", String(), false,   processor);

    }
    else {
      request->send(401, "text/plain", "Unauthorized");
    }
  });


  server.on("/update", HTTP_GET, [](AsyncWebServerRequest * request) {

    if ((request->hasParam(PARAM_APIKEY) && request->getParam(PARAM_APIKEY)->value() == apikey) || wifiConfigRequired) {

      request->send(SPIFFS, "/update.html", String(), false,   processor);
    }
    else {
      request->send(401, "text/plain", "Unauthorized");
    }
  });


  server.on("/cfg", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    if ((request->hasParam(PARAM_APIKEY) && request->getParam(PARAM_APIKEY)->value() == apikey) || wifiConfigRequired) {

      request->send(SPIFFS, "/cfg.html", String(), false,   processor);
    } else request->send(401, "text/plain", "Unauthorized");
  });
  server.on("/cfg-save", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    if (request->hasParam("ssid") && request->hasParam("password") && request->hasParam("apikey")) {
      ssid = request->getParam(PARAM_SSID)->value();
      if(request->getParam(PARAM_password)->value()!=NULL) password = request->getParam(PARAM_password)->value();
      apikey = request->getParam(PARAM_APIKEY)->value();

      if(request->hasParam("dhcp")) dhcp = true; else dhcp=false;  
      if(request->hasParam("localip")) sIP = request->getParam(PARAM_LOCALIP)->value();
      if(request->hasParam("gateway")) sGateway   = request->getParam(PARAM_GATEWAY)->value();
      if(request->hasParam("subnet")) sSubnet    = request->getParam(PARAM_SUBNET)->value();
      if(request->hasParam("pdns")) sPrimaryDNS = request->getParam(PARAM_PDNS)->value();
      if(request->hasParam("sdns")) sSecondaryDNS = request->getParam(PARAM_SDNS)->value();
      if(request->hasParam("con")) con = true; else con=false;
      
    // http://192.168.1.118/cfg-save?apikey=1111&dhcp=on&ssid=TP-Link&password=
    // http://192.168.1.118/cfg-save?apikey=1111&localip=192.168.1.200&subnet=255.255.255.0&gateway=192.168.1.1&pdns=8.8.8.8&sdns=8.8.4.4&ssid=TP-Link&password=
      request->send(200, "text/plain", "Config saved - SSID:" + ssid + " APIKEY: " + apikey + " restart in 5 seconds");
      savePrefs();
      delay(2000);
      ESP.restart();
      //request->redirect("/");
    }
    else
      request->send(400, "text/plain", "Missing required parameters");
  });


    // Send a GET request to <IP>/get?message=<message>
  /*
  server.on("/sendmorse", HTTP_GET, [] (AsyncWebServerRequest * request) {
    if (request->hasParam(PARAM_APIKEY) && request->getParam(PARAM_APIKEY)->value() == apikey) {
      if (request->hasParam(PARAM_MESSAGE)) {
        message = request->getParam(PARAM_MESSAGE)->value();
      }

      if (request->hasParam(PARAM_SPEED)) {
        sspeed = request->getParam(PARAM_SPEED)->value();
        speed = sspeed.toInt();
        RotaryEnc_KeyerSpeedWPM.cntVal = speed;
        //update_speed();
        Calc_WPM_dot_delay (speed);
        display_status_bar();
      }
      request->send(200, "text/plain", "OK");
    } else request->send(401, "text/plain", "Unauthorized");
  });
  */

  server.on("/js/bootstrap.bundle.min.js", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/js/bootstrap.bundle.min.js", "text/javascript");
  });

  server.on("/js/bootstrap4-toggle.min.js", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/js/bootstrap4-toggle.min.js", "text/css");
  });


  server.on("/js/jquery.mask.min.js", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/js/jquery.mask.min.js", "text/css");
  });

  server.on("/js/jquery-3.5.1.min.js", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/js/jquery-3.5.1.min.js", "text/css");
  });

  server.on("/css/bootstrap.min.css", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/css/bootstrap.min.css", "text/css");
  });


  server.on("/css/bootstrap4-toggle.min.css", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/css/bootstrap4-toggle.min.css", "text/css");
  });


  server.onNotFound(notFound);

  server.begin();

  if (udp.listen(UDP_PORT)) {
    Serial.print("UDP running at IP: ");
    Serial.print(IP);
    Serial.println(" port: 6789");
    udp.onPacket([](AsyncUDPPacket packet) {
      //Serial.print("Type of UDP datagram: ");
      //Serial.println(packet.isBroadcast() ? "Broadcast" : packet.isMulticast() ? "Multicast" : "Unicast");
      //Serial.print(", Sender: ");
      //Serial.print(packet.remoteIP());
      //Serial.print(":");
      //Serial.print(packet.remotePort());
      //Serial.print(", Receiver: ");
      //Serial.print(packet.localIP());
      //Serial.print(":");
      //Serial.println(packet.localPort());
      //Serial.print(", Message lenght: ");
      //Serial.print(packet.length());
      //Serial.print(", Payload: ");
      //Serial.write(packet.data(), packet.length());
      //Serial.println();
      String udpdataString = (const char*)packet.data();
      udpdataString = udpdataString.substring(0, packet.length());

      char bb[2];
      int command = (int)packet.data()[1] - 48;
      if (packet.data()[0] == 27) {

        switch (command ) {
          case 0: //break cw
            xQueueReset( queue );
            break;
          case 2:  // set speed
            bb[0] = packet.data()[2];
            bb[1] = packet.data()[3];
            speed = atoi(bb);
            sspeed = String(speed);
            Calc_WPM_dot_delay (speed);
            //Serial.print("UDP Speed: ");
            //Serial.print(bb);
            //Serial.print(", ");
            //Serial.println(speed);
            break;

          default:
            break;
        }
      } else {    // Transmit received char
        message = udpdataString;
        messageQueueSend();
        //Serial.print("UDP TX: ");
        //Serial.println(udpdataString);
      }

    });
  }


}

// JU: Modified function to set frequency since definition of FREQ_STEP was not precise enough (198.364 - should be 52e6/2**18=198.3642578125)
#define FREQ_STEP_JU 198.3642578125


void JUsetRfFrequency(uint32_t frequency, int32_t offset)
{

  // first we call original function so that private variables savedFrequency and savedOffset are updated
  //LT.setRfFrequency(frequency, offset);

  FreqWordNoOffset = FreqToRegWord(frequency);
  frequency = frequency + offset;
  uint8_t buffer[3];
  FreqWord = FreqToRegWord(frequency);
  buffer[0] = ( uint8_t )( ( FreqWord >> 16 ) & 0xFF );
  buffer[1] = ( uint8_t )( ( FreqWord >> 8 ) & 0xFF );
  buffer[2] = ( uint8_t )( FreqWord & 0xFF );
  //LT.writeCommand(RADIO_SET_RFFREQUENCY, buffer, 3);
}


// Get RegWord from frequency
uint32_t FreqToRegWord(uint32_t frequency) {
  return ( uint32_t )( (double) frequency / (double)FREQ_STEP_JU);
}

// Get frequency from RegWord
uint32_t RegWordToFreq(uint32_t freqword) {
  return (uint32_t)((double)freqword * (double)FREQ_STEP_JU);
}

// Will add decimal point as separator between thousands, milions...
// Will add decimal point as separator between thousands, milions...
void format_freq(uint32_t n, char *out, bool params_set_by_udp_pkt)
{
  int c;
  char buf[20];
  char *p;
  if (params_set_by_udp_pkt == true) {
    sprintf(buf, "*%u", n);
  } else {
    sprintf(buf, " %u", n);
  }
  c = 2 - strlen(buf) % 3;
  for (p = buf; *p != 0; p++) {
    *out++ = *p;
    if (c == 1) {
      *out++ = '.';
    }
    c = (c + 1) % 3;
  }
  *--out = 0;
}
