# CW_UDP_KEYER
CW Keyer with Morse decoding, displaying the decoded code on the fly and sending the code to remote IP via UDP port 6789
- Runs on the same hardware as SX1281_QO100_TX
- If TTGO_T3_BOARD directive is defined it can run on TTGO T7 v1.4 Mini32 board with ESP32 and color OLED display
- Remote IP is configurable,  must be entered in standard form e.g. 192.168.1.100. No sanity check is done yet
- In addition, specific UDP packet is sent to remote IP upon frequency or WPM or PWR change
- WEB Interface
- 
