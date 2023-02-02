//#define Program_Version "CW UDP Keyer v1.0"   // 2021-11-03
//#define Program_Version "CW UDP Keyer v1.1"   // 2022-07-26
#define Program_Version "CW UDP Keyer v1.2"   // 2023-02-02 JU, Added support for Iambic-B keying. Based on request of Markus DL6YYM


#define LED1 12                                 //on board LED, high for on

#define BUZZER 33                               //pin for buzzer, set to -1 if not used 
#define VCCPOWER 17    //JU-not needed by module          //pin controls power to external devices

// Pin definitions
#define ROTARY_ENC_A     26    // Rotary encoder contact A 
#define ROTARY_ENC_B     27    // Rotary encoder contact B 
#define ROTARY_ENC_PUSH  25    // Rotary encoder pushbutton
//
//


//*******  Setup LoRa Parameters Here ! ***************

//LoRa Modem Parameters
//const uint32_t Frequency = 2445000000;          //frequency of transmissions
const uint32_t Frequency = 2400020000;            //frequency of transmissions
const int32_t Offset = 0;                         //offset frequency for calibration purposes  

// OM2JU
const uint8_t  Morse_Coding_table [] = {
0x00,   // Space = do not play anything just intercharacter delay
0xEB,   // !	-.-.-- => len = 110 char=101011 => 11101011 = EB
0xD2,   // "
0x00,   // # ?
0x00,   // $ ?
0x00,   // % ?
0x00,   // & ?
0xDE,   // '
0xB6,   // (
0xED,   // )
0x00,   // * ?
0xAA,   // +	.-.-. => len=101 char=01010     => 10101010 = AA
0xF3,   // ,	--..-- => len = 110 char=110011 => 11110011 = F3
0xE1,   // -
0xD5,   // .
0xB2,   // /
0xBF,   // 0
0xAF,   // 1
0xA7,   // 2
0xA3,   // 3
0xA1,   // 4
0xA0,   // 5
0xB0,   // 6
0xB8,   // 7
0xBC,   // 8
0xBE,   // 9
0xF8,   // :
0xEA,   // ;
0x00,   // < ?
0xB1,   // =
0x00,   // > ?
0xCC,   // ?
0xDA,   // @
0x48,   // A
0x90,   // B
0x94,   // C
0x70,   // D
0x20,   // E
0x84,   // F
0x78,   // G
0x80,   // H
0x40,   // I
0x8E,   // J
0x74,   // K
0x88,   // L
0x58,   // M
0x50,   // N
0x7C,   // O
0x8C,   // P
0x9A,   // Q
0x68,   // R
0x60,   // S
0x30,   // T
0x64,   // U
0x82,   // V
0x6C,   // W
0x92,   // X
0x96,   // Y
0x98,   // Z
0x00,   // [ ?
0x00,   // \ ?
0x00,   // ] ?
0x00,   // ^ ?
0xCD,   // _
0x00    // ` ?
};


//-----------------------------------------------------------------------------
const uint8_t  Morse_decode_table [] = {
0x80,	// ......  	0 no valid morse code, len = 7
0x81,	// ......  	1 no valid morse code, len = 6
'5',	// .....    	2 '5'
0x83,	// .....-  	3 no valid morse code, len = 6
'H',	// ....     	4 'H'
0x85,	// ....-.  	5 no valid morse code, len = 6
'4',	// ....-    	6 '4'
0x87,	// ....--  	7 no valid morse code, len = 6
'S',	// ...      	8 'S'
0x89,	// ...-..  	9 no valid morse code, len = 6
0x8a,	// ...-.   	10 no valid morse code, len = 5
0x8b,	// ...-.-  	11 no valid morse code, len = 6
'V',	// ...-     	12 'V'
0x8d,	// ...--.  	13 no valid morse code, len = 6
'3',	// ...--    	14 '3'
0x8f,	// ...---  	15 no valid morse code, len = 6
'I',	// ..       	16 'I'
0x91,	// ..-...  	17 no valid morse code, len = 6
0x92,	// ..-..   	18 no valid morse code, len = 5
0x93,	// ..-..-  	19 no valid morse code, len = 6
'F',	// ..-.     	20 'F'
0x95,	// ..-.-.  	21 no valid morse code, len = 6
0x96,	// ..-.-   	22 no valid morse code, len = 5
0x97,	// ..-.--  	23 no valid morse code, len = 6
'U',	// ..-      	24 'U'
'?',	// ..--..   	25 '?'
0x9a,	// ..--.   	26 no valid morse code, len = 5
'_',	// ..--.-   	27 '_'
0x9c,	// ..--    	28 no valid morse code, len = 4
0x9d,	// ..---.  	29 no valid morse code, len = 6
'2',	// ..---    	30 '2'
0x9f,	// ..----  	31 no valid morse code, len = 6
'E',	// .        	32 'E'
0xa1,	// .-....  	33 no valid morse code, len = 6
'&',	// .-...    	34 '&'
0xa3,	// .-...-  	35 no valid morse code, len = 6
'L',	// .-..     	36 'L'
'"',	// .-..-.   	37 '"'
0xa6,	// .-..-   	38 no valid morse code, len = 5
0xa7,	// .-..--  	39 no valid morse code, len = 6
'R',	// .-.      	40 'R'
0xa9,	// .-.-..  	41 no valid morse code, len = 6
'+',	// .-.-.    	42 '+'
'.',	// .-.-.-   	43 '.'
0xac,	// .-.-    	44 no valid morse code, len = 4
0xad,	// .-.--.  	45 no valid morse code, len = 6
0xae,	// .-.--   	46 no valid morse code, len = 5
0xaf,	// .-.---  	47 no valid morse code, len = 6
'A',	// .-       	48 'A'
0xb1,	// .--...  	49 no valid morse code, len = 6
0xb2,	// .--..   	50 no valid morse code, len = 5
0xb3,	// .--..-  	51 no valid morse code, len = 6
'P',	// .--.     	52 'P'
'@',	// .--.-.   	53 '@'
0xb6,	// .--.-   	54 no valid morse code, len = 5
0xb7,	// .--.--  	55 no valid morse code, len = 6
'W',	// .--      	56 'W'
0xb9,	// .---..  	57 no valid morse code, len = 6
0xba,	// .---.   	58 no valid morse code, len = 5
0xbb,	// .---.-  	59 no valid morse code, len = 6
'J',	// .---     	60 'J'
'\'',	// .----.   	61 '''
'1',	// .----    	62 '1'
0xbf,	// .-----  	63 no valid morse code, len = 6
0xc0,	//         	64 no valid morse code, len = 0
0xc1,	// -.....  	65 no valid morse code, len = 6
'6',	// -....    	66 '6'
'-',	// -....-   	67 '-'
'B',	// -...     	68 'B'
0xc5,	// -...-.  	69 no valid morse code, len = 6
'=',	// -...-    	70 '='
0xc7,	// -...--  	71 no valid morse code, len = 6
'D',	// -..      	72 'D'
0xc9,	// -..-..  	73 no valid morse code, len = 6
'/',	// -..-.    	74 '/'
0xcb,	// -..-.-  	75 no valid morse code, len = 6
'X',	// -..-     	76 'X'
0xcd,	// -..--.  	77 no valid morse code, len = 6
0xce,	// -..--   	78 no valid morse code, len = 5
0xcf,	// -..---  	79 no valid morse code, len = 6
'N',	// -.       	80 'N'
0xd1,	// -.-...  	81 no valid morse code, len = 6
0xd2,	// -.-..   	82 no valid morse code, len = 5
0xd3,	// -.-..-  	83 no valid morse code, len = 6
'C',	// -.-.     	84 'C'
';',	// -.-.-.   	85 ';'
0xd6,	// -.-.-   	86 no valid morse code, len = 5
'!',	// -.-.--   	87 '!'
'K',	// -.-      	88 'K'
0xd9,	// -.--..  	89 no valid morse code, len = 6
'(',	// -.--.    	90 '('
')',	// -.--.-   	91 ')'
'Y',	// -.--     	92 'Y'
0xdd,	// -.---.  	93 no valid morse code, len = 6
0xde,	// -.---   	94 no valid morse code, len = 5
0xdf,	// -.----  	95 no valid morse code, len = 6
'T',	// -        	96 'T'
0xe1,	// --....  	97 no valid morse code, len = 6
'7',	// --...    	98 '7'
0xe3,	// --...-  	99 no valid morse code, len = 6
'Z',	// --..     	100 'Z'
0xe5,	// --..-.  	101 no valid morse code, len = 6
0xe6,	// --..-   	102 no valid morse code, len = 5
',',	// --..--   	103 ','
'G',	// --.      	104 'G'
0xe9,	// --.-..  	105 no valid morse code, len = 6
0xea,	// --.-.   	106 no valid morse code, len = 5
0xeb,	// --.-.-  	107 no valid morse code, len = 6
'Q',	// --.-     	108 'Q'
0xed,	// --.--.  	109 no valid morse code, len = 6
0xee,	// --.--   	110 no valid morse code, len = 5
0xef,	// --.---  	111 no valid morse code, len = 6
'M',	// --       	112 'M'
':',	// ---...   	113 ':'
'8',	// ---..    	114 '8'
0xf3,	// ---..-  	115 no valid morse code, len = 6
0xf4,	// ---.    	116 no valid morse code, len = 4
0xf5,	// ---.-.  	117 no valid morse code, len = 6
0xf6,	// ---.-   	118 no valid morse code, len = 5
0xf7,	// ---.--  	119 no valid morse code, len = 6
'O',	// ---      	120 'O'
0xf9,	// ----..  	121 no valid morse code, len = 6
'9',	// ----.    	122 '9'
0xfb,	// ----.-  	123 no valid morse code, len = 6
0xfc,	// ----    	124 no valid morse code, len = 4
0xfd,	// -----.  	125 no valid morse code, len = 6
'0',	// -----    	126 '0'
0xff	// ------  	127 no valid morse code, len = 6
};
