/******************************************************************************

GeoCache Hunt Project (GeoCache.cpp)

This is skeleton code provided as a project development guideline only.  You
are not required to follow this coding structure.  You are free to implement
your project however you wish.

List Team Members Here:

1. Logano Gamba
2. Drew Ritter
3. Zixun Xiang
4.

NOTES:

You only have 32k of program space and 2k of data space.  You must
use your program and data space wisely and sparingly.  You must also be
very conscious to properly configure the digital pin usage of the boards,
else weird things will happen.

The Arduino GCC sprintf() does not support printing floats or doubles.  You should
consider using sprintf(), dtostrf(), strtok() and strtod() for message string
parsing and converting between floats and strings.

The GPS provides latitude and longitude in degrees minutes format (DDDMM.MMMM).
You will need convert it to Decimal Degrees format (DDD.DDDD).  The switch on the
GPS Shield must be set to the "Soft Serial" position, else you will not receive
any GPS messages.

*******************************************************************************

Following is the GPS Shield "GPRMC" Message Structure.  This message is received
once a second.  You must parse the message to obtain the parameters required for
the GeoCache project.  GPS provides coordinates in Degrees Minutes (DDDMM.MMMM).
The coordinates in the following GPRMC sample message, after converting to Decimal
Degrees format(DDD.DDDDDD) is latitude(23.118757) and longitude(120.274060).  By
the way, this coordinate is GlobaTop Technology in Tiawan, who designed and
manufactured the GPS Chip.

"$GPRMC,064951.000,A,2307.1256,N,12016.4438,E,0.03,165.48,260406,3.05,W,A*2C/r/n"

$GPRMC,         // GPRMC Message
064951.000,     // utc time hhmmss.sss
A,              // status A=data valid or V=data not valid
2307.1256,      // Latitude 2307.1256 (degrees minutes format dddmm.mmmm)
N,              // N/S Indicator N=north or S=south
12016.4438,     // Longitude 12016.4438 (degrees minutes format dddmm.mmmm)
E,              // E/W Indicator E=east or W=west
0.03,           // Speed over ground knots
165.48,         // Course over ground (decimal degrees format ddd.dd)
260406,         // date ddmmyy
3.05,           // Magnetic variation (decimal degrees format ddd.dd)
W,              // E=east or W=west
A               // Mode A=Autonomous D=differential E=Estimated
*2C             // checksum
/r/n            // return and newline

******************************************************************************/

// Required
#include "Arduino.h"

//Add
#include<math.h>
#include <string.h>
#include <stdio.h> 
#include <stdlib.h> 

/*
Configuration settings.

These defines makes it easy to enable/disable certain capabilities
during the development and debugging cycle of this project.  There
may not be sufficient room in the PROGRAM or DATA memory to enable
all these libraries at the same time.  You are only permitted to
have NEO_ON, GPS_ON and SDC_ON during the actual GeoCache Treasure
Hunt.
*/
#define NEO_ON 1		// NeoPixelShield
#define TRM_ON 1		// SerialTerminal
#define ONE_ON 0		// 1Sheeld
#define SDC_ON 1		// SecureDigital
#define GPS_ON 1		// GPSShield (off = simulated)

// define pin usage
#define NEO_TX	6		// NEO transmit
#define GPS_TX	7		// GPS transmit
#define GPS_RX	8		// GPS receive

#if ONE_ON
#define TERM Terminal
#elif TRM_ON
#define TERM Serial
#else
#define TERM ///
#endif

// GPS message buffer
#define GPS_RX_BUFSIZ	128
char cstr[GPS_RX_BUFSIZ];



// variables
uint8_t target = 0;
float distance = 0.0, heading = 0.0;
float currentlon, currentlat, currentheading;
uint8_t Button = 3;

#if GPS_ON
#include "SoftwareSerial.h"
SoftwareSerial gps(GPS_RX, GPS_TX);
#endif
char dmLat[11], dmLon[11], bearing[7], dirNS = 0, dirEW = 0;
float latitude, longitude;
#define GPSMESSAGETIME 250

#if NEO_ON
#include <Adafruit_NeoPixel.h>
Adafruit_NeoPixel strip = Adafruit_NeoPixel(40, NEO_TX, NEO_GRB + NEO_KHZ800);
#endif

#if ONE_ON
#define CUSTOM_SETTINGS
#define INCLUDE_TERMINAL_SHIELD
#include <OneSheeld.h>
#endif

#if SDC_ON
#include <SD.h>
bool cardEnabled = true;
SDLib::File MyFile;
#endif

/*
Following is a Decimal Degrees formatted waypoint for the large tree
in the parking lot just outside the front entrance of FS3B-116.
*/
#define GEOLAT0 28.594532
#define GEOLON0 -81.304437

#if GPS_ON
/*
These are GPS command messages (only a few are used).
*/
#define PMTK_AWAKE "$PMTK010,002*2D"
#define PMTK_STANDBY "$PMTK161,0*28"
#define PMTK_Q_RELEASE "$PMTK605*31"
#define PMTK_ENABLE_WAAS "$PMTK301,2*2E"
#define PMTK_ENABLE_SBAS "$PMTK313,1*2E"
#define PMTK_CMD_HOT_START "$PMTK101*32"
#define PMTK_CMD_WARM_START "$PMTK102*31"
#define PMTK_CMD_COLD_START "$PMTK103*30"
#define PMTK_CMD_FULL_COLD_START "$PMTK104*37"
#define PMTK_SET_BAUD_9600 "$PMTK251,9600*17"
#define PMTK_SET_BAUD_57600 "$PMTK251,57600*2C"
#define PMTK_SET_NMEA_UPDATE_1HZ  "$PMTK220,1000*1F"
#define PMTK_SET_NMEA_UPDATE_5HZ  "$PMTK220,200*2C"
#define PMTK_API_SET_FIX_CTL_1HZ  "$PMTK300,1000,0,0,0,0*1C"
#define PMTK_API_SET_FIX_CTL_5HZ  "$PMTK300,200,0,0,0,0*2F"
#define PMTK_SET_NMEA_OUTPUT_RMC "$PMTK314,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*29"
#define PMTK_SET_NMEA_OUTPUT_GGA "$PMTK314,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*29"
#define PMTK_SET_NMEA_OUTPUT_RMCGGA "$PMTK314,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*28"
#define PMTK_SET_NMEA_OUTPUT_OFF "$PMTK314,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*28"

#endif // GPS_ON

/*************************************************
**** GEO FUNCTIONS - BEGIN ***********************
*************************************************/

/**************************************************
Convert Degrees Minutes (DDMM.MMMM) into Decimal Degrees (DDD.DDDD)

float degMin2DecDeg(char *cind, char *ccor)

Input:
cind = string char pointer containing the GPRMC latitude(N/S) or longitude (E/W) indicator
ccor = string char pointer containing the GPRMC latitude or longitude DDDMM.MMMM coordinate

Return:
Decimal degrees coordinate.

**************************************************/
float degMin2DecDeg(char *cind, char *ccor) {
	float degrees = 0.0;

	// add code here
	int d = (int)atof(ccor) / 100;
	float mm = atof(ccor) - d * 100;
	float dd;

	dd = mm / 60;
	degrees = d + dd;

	if (cind == "S" || cind == "W")
		degrees = -degrees;

	return(degrees);
}

/**************************************************
Calculate Great Circle Distance between to coordinates using
Haversine formula.

float calcDistance(float flat1, float flon1, float flat2, float flon2)

EARTH_RADIUS_FEET = 3959.00 radius miles * 5280 feet per mile

Input:
flat1, flon1 = first latitude and longitude coordinate in decimal degrees
flat2, flon2 = second latitude and longitude coordinate in decimal degrees

Return:
distance in feet (3959 earth radius in miles * 5280 feet per mile)
**************************************************/
float calcDistance(float flat1, float flon1, float flat2, float flon2) {
	float distance = 0.0;

	// add code here



	//////float rLAT = radians(flat1);
	//////float rLAT2 = radians(flat2);

	//////float latitude = radians(flat2 - flat1);
	//////float lontude = radians(flon2 - flon1);


	//////float VAR1 = pow(sin(latitude / 2.0f), 2) + cos(rLAT)*cos(rLAT2) * (pow(sin(lontude / 2.0f), 2));
	//////float VAR2 = 2.0f * atan2(sqrt(VAR1), sqrt(1 - VAR1));

	//////distance = 3959 * 5280 * VAR2;


	flat1 = radians(flat1);
	flon1 = radians(flon1);
	flat2 = radians(flat2);
	flon2 = radians(flon2);

	float radLatitudeDiff = flat2 - flat1;
	float radLongitudeDiff = flon2 - flon1;

	float a = sin(radLatitudeDiff / 2.00) * sin(radLatitudeDiff / 2.00) + cos(flat1) * cos(flat2) * sin(radLongitudeDiff / 2.00) * sin(radLongitudeDiff / 2.00);
	float c = 2.00 * atan2(sqrt(a), sqrt(1.00 - a));
	distance = c * 3959.00 * 5280.00;

	return(distance);
}

/**************************************************
Convert GPS bearing into float bearing

float GPS2floatbearing(char *b)

Input:
b = string char pointer containing the GPS bearing

Return:
Decimal degrees coordinate.
**************************************************/
float GPS2floatbearing(char *b) {
	float bearing = 0.0;
	Serial.println(b);
	// add code here
	bearing = atof(b);

	return(bearing);
}

/**************************************************
Calculate Great Circle Bearing between two coordinates

float calcBearing(float flat1, float flon1, float flat2, float flon2, float originalbearing)

Input:
flat1, flon1 = first latitude and longitude coordinate in decimal degrees
flat2, flon2 = second latitude and longitude coordinate in decimal degrees

Return:
angle in degrees from magnetic north
**************************************************/
float calcBearing(float flat1, float flon1, float flat2, float flon2, float courseoverground) {
	float bearing = 0.0;

	// add code here

	flat1 = radians(flat1);
	flon1 = radians(flon1);
	flat2 = radians(flat2);
	flon2 = radians(flon2);

	float y = sin(flon2 - flon1 ) * cos(flat2);
	float x = cos(flat1) * sin(flat2 ) - sin(flat1) * cos(flat2 ) * cos(flon2- flon1);
	bearing = atan2(y, x) * RAD_TO_DEG;

	bearing = bearing - courseoverground;
	//change to 0-360:
	if (bearing < 0.0) {
		bearing += 360.0f;
	}

	if (bearing >= 360.0)
		bearing -= 360.0f;

	return(bearing);
}

/*************************************************
**** GEO FUNCTIONS - END**************************
*************************************************/

bool parseGPS() {
	String finder(cstr);
	uint16_t index = -1, found;
	uint8_t i = 0;
	// Skip GPRMC and UTC Time...
	for (i = 0; i < 2; i++) {
		index = finder.indexOf(",", index + 1);
	}
	if (cstr[index + 1] != 'A') {
		return false;
	}

	index += 3;
	found = finder.indexOf(",", index + 1);
	if (found == -1) {
		return false;
	}

	memcpy(dmLat, cstr + index, found - index);
	index = found + 1;
	dirNS = cstr[index++];
	found = finder.indexOf(",", ++index);
	if (found == -1) {
		return false;
	}

	memcpy(dmLon, cstr + index, found - index);
	index = found + 1;
	found = -1;
	dirEW = cstr[index];
	index += 2;
	found = finder.indexOf(",", index);
	if (found == -1) {
		return false;
	}

	index = found + 1;
	found = finder.indexOf(",", index);
	if (found == -1) {
		return false;
	}

	memcpy(bearing, cstr + index, found - index);
	Serial.println("parsed successfully");
	return true;
}


#if NEO_ON
/*
Sets target number, heading and distance on NeoPixel Display
*/
void setNeoPixel(float heading, float distance) {

	SetDirection(heading);
	ClearCompass();

	SetFlagNeo();
	SetDistanceToFlag(distance);
	SetDisNeo();

	strip.show();
}
#pragma region Compass Neo Pixel
int index = 0;
/*Takes in a value between -180 & +180.
will then tell the compass to display a direction.
The directions float must be calculated before passing it into this function.
directions acts as if 0 degress is the face.*/
void SetDirection(float directions)
{
	// oox
	// o o
	// ooo
	if (directions >= 22.5 && directions <= 67.5)
	{
		strip.setPixelColor(2, 0, 255, 0);
		index = 1;
	}
	// ooo
	// o x
	// ooo
	else if (directions >= 67.5 && directions <= 112.5)
	{
		strip.setPixelColor(10, 0, 255, 0);
		index = 2;
	}
	// ooo
	// o o
	// oox
	else if (directions >= 112.5 && directions <= 157.5)
	{
		strip.setPixelColor(18, 0, 255, 0);
		index = 3;
	}
	// ooo
	// o o 
	// oxo
	else if (directions >= 157.5 && directions <= 202.5)
	{
		strip.setPixelColor(17, 0, 255, 0);
		index = 4;
	}
	// ooo
	// o o
	// xoo
	else if (directions >= 202.5  && directions <= 247.5)
	{
		strip.setPixelColor(16, 0, 255, 0);
		index = 5;
	}
	// ooo
	// x o
	// ooo
	else if (directions >= 247.5  && directions <= 292.5)
	{
		strip.setPixelColor(8, 0, 255, 0);
		index = 6;
	}
	// xoo
	// o o
	// ooo
	else if (directions >= 292.5 && directions <= 337.5)
	{
		strip.setPixelColor(0, 0, 255, 0);
		index = 7;
	}
	// oxo
	// o o
	// ooo
	else if (directions >= 337.5 || directions <= 22.5)
	{
		strip.setPixelColor(1, 0, 255, 0);
		index = 0;
	}
}
int IndexArray[8] = { 1,2,10,18,17,16,8,0 };
/*Clears the compass for everything except the index value for the IndexArray*/
void ClearCompass()
{
	for (int i = 0; i < 8; i++)
	{
		if (i != index)
			strip.setPixelColor(IndexArray[i], 0, 0, 0);
	}
}
#pragma endregion
#pragma region TargetFlagsNeoPixel
struct  FLAGS
{
	float lat;
	float lon;
};
//FLAGS KTarget;
//KTarget.lat = GEOLAT0;
//KTarget.lon = GEOLON0;
FLAGS flagsdata[4]; //you can add flags here
//flagsdata[0] = KTarget;
int FlagIndex = 0;
int Flagss[4] = { 4,5,6,7 };
/*
Increments the FlagIndex
Handles rolling over.
*/
void IncrementFlagIndex()
{
	FlagIndex = (FlagIndex + 1) % 4;
}
/*
Based on the FlagIndex the function the neopixel index is set to be bright green
while the rest of the pixels are set to black AKA off.
*/
void SetFlagNeo()
{
	for (int i = 0; i < 4; i++)
	{
		if (i == FlagIndex)
			strip.setPixelColor(Flagss[i], 0, 255, 0);
		else
		{
			strip.setPixelColor(Flagss[i], 0, 0, 0);
		}
	}
}
#pragma endregion
#pragma region DistanceNeoLight

float DistanceToFlag = 0;
/*Sets the DistanceToFlag equal to kek*/
void SetDistanceToFlag(float kek)
{
	DistanceToFlag = kek;
}
/*Sets The Color of the Distance to a corrosponding Distance
1000+ feet		 :Red
500+  feet		 :Orange
250+  feet		 :Yellow
75+   feet		 :Blue
40+   feet		 :Green
10+   feet		 :Purple
9-0, -1, -2 ...   feet      :White
*/
void SetDisNeo()
{
	//pixel 22 on the strip is the distance 
	if (DistanceToFlag >= 1000)
		strip.setPixelColor(22, 255, 0, 0);
	else if (DistanceToFlag >= 500)
		strip.setPixelColor(22, 255, 69, 0);
	else if (DistanceToFlag >= 250)
		strip.setPixelColor(22, 255, 215, 0);
	else if (DistanceToFlag >= 75)
		strip.setPixelColor(22, 0, 0, 255);
	else if (DistanceToFlag >= 40)
		strip.setPixelColor(22, 0, 255, 0);
	else if (DistanceToFlag >= 10)
		strip.setPixelColor(22, 241, 0, 241);
	else if(DistanceToFlag >=0)
		strip.setPixelColor(22, 255, 255, 255);
	else
	{
		strip.setPixelColor(22, 0, 0, 0);
	}
}
#pragma endregion

#endif	// NEO_ON

#if GPS_ON
/*
Get valid GPS message. This function returns ONLY once a second.

void getGPSMessage(void)

Side affects:
Message is placed in global "cstr" string buffer.

Input:
none

Return:
none

*/
void getGPSMessage(void) {
	uint8_t x = 0, y = 0, isum = 0;

	memset(cstr, 0, sizeof(cstr));

	// get nmea string
	while (true) {
		if (gps.peek() != -1) {
			cstr[x] = gps.read();

			// if multiple inline messages, then restart
			if ((x != 0) && (cstr[x] == '$')) {
				x = 0;
				cstr[x] = '$';
			}

			// if complete message
			if ((cstr[0] == '$') && (cstr[x++] == '\n')) {
				// nul terminate string before /r/n
				cstr[x - 2] = 0;

				// if checksum not found
				if (cstr[x - 5] != '*') {
					x = 0;
					continue;
				}

				// convert hex checksum to binary
				isum = strtol(&cstr[x - 4], NULL, 16);

				// reverse checksum
				for (y = 1; y < (x - 5); y++) isum ^= cstr[y];

				// if invalid checksum
				if (isum != 0) {
					x = 0;
					continue;
				}

				// else valid message

				// Status code check
				if (cstr[18] != 'A') {
					x = 0;
					continue;
				}
				break;
			}
		}
	}
}

#else
/*
Get simulated GPS message once a second.

This is the same message and coordinates as described at the top of this
file.  You could edit these coordinates to point to the tree out front (GEOLAT0,
GEOLON0) to test your distance and direction calculations.  Just note that the
tree coordinates are in Decimal Degrees format, and the message coordinates are
in Degrees Minutes format.

void getGPSMessage(void)

Side affects:
Static GPRMC message is placed in global "cstr" string buffer.

Input:
none

Return:
none

*/
void getGPSMessage(void) {
	static unsigned long gpsTime = 0;

	// simulate waiting for message
	while (gpsTime > millis()) delay(100);

	// do this once a second
	gpsTime = millis() + 1000;

	memcpy(cstr, "$GPRMC,064951.000,A,2307.1256,N,12016.4438,E,0.03,165.48,260406,3.05,W,A*2C", sizeof(cstr));

	TERM.println("CSTR: ");
	TERM.println(cstr);

	return;
}

#endif	// GPS_ON

void setup(void) {

	strip.setBrightness(50);
#if TRM_ON
	// init Terminal interface
	Serial.begin(9600);
#endif	

#if ONE_ON
	// init OneShield Shield
	OneSheeld.begin();
#endif

#if NEO_ON
	// init NeoPixel Shield

	pinMode(6, OUTPUT);

	FLAGS KTarget;
	KTarget.lat = GEOLAT0;
	KTarget.lon = GEOLON0;
	flagsdata[0] = KTarget;


	//FLAGS KTarget;
	//KTarget.lat = 28.59627;
	//KTarget.lon = -81.30653;
	//flagsdata[0] = KTarget;
	//FLAGS KTarget2;
	//KTarget.lat = 28.59641;
	//KTarget.lon = -81.30081;
	//flagsdata[1] = KTarget;
	//FLAGS KTarget3;
	//KTarget.lat = 28.59208;
	//KTarget.lon = -81.30407;
	//flagsdata[2] = KTarget;
	//FLAGS KTarget4;
	//KTarget.lat = 28.59505;
	//KTarget.lon = -81.30677;
	//flagsdata[3] = KTarget;
#endif	

#if SDC_ON
	/*
	Initialize the SecureDigitalCard and open a numbered sequenced file
	name "MyMapNN.txt" for storing your coordinates, where NN is the
	sequential number of the file.  The filename can not be more than 8
	chars in length (excluding the ".txt").
	*/
	if (!SD.begin()) {
		cardEnabled = false;
		Serial.println("SD Disabled.");
	}
	else {
		for (uint8_t i = 0; i < 100; i++) {
			char file[12] = "\0";
			Serial.print(i);
			sprintf(file, "MyFile%i.txt", i);
			if (SD.exists(file)) {
				if (i == 99) {
					cardEnabled = false;
				}
				continue;
			}
			else {
				TERM.print(file);
				TERM.println(" successfully opened.");
				MyFile = SD.open(file, FILE_WRITE);
				if (!MyFile) {
					TERM.println("Plot twist! it failed!");
					cardEnabled = false;
				}
				break;
			}
		}
	}
#endif

#if GPS_ON
	// enable GPS sending GPRMC message

	gps.begin(9600);
	gps.println(PMTK_SET_NMEA_UPDATE_1HZ);
	gps.println(PMTK_API_SET_FIX_CTL_1HZ);
	gps.println(PMTK_SET_NMEA_OUTPUT_RMC);

#endif		
	memset(dmLat, 0, 11);
	memset(dmLon, 0, 11);
	memset(bearing, 0, 7);
	// init target button here
	pinMode(Button, INPUT_PULLUP);
}

//debounce funtion for button
bool debounce(int pin)
{
	for (int i = 0; i < 5000; i++)
	{
		if (digitalRead(pin) == HIGH)
			return false;
	}
	return true;
}
bool PreviousButtonState = false;
void loop(void) {
	// if button pressed, set new target

	if (debounce(Button))
	{
		if (PreviousButtonState == false)
		{
			IncrementFlagIndex();

		}
		PreviousButtonState = true;
	}
	else
	{
		PreviousButtonState = false;
	}
	// returns with message once a second
	getGPSMessage();
	// if GPRMC message (3rd letter = R)

	while (cstr[3] == 'R') {
		// parse message parameters
		if (!parseGPS()) {
			break;
		}

		currentheading = GPS2floatbearing(bearing);
		currentlat = degMin2DecDeg(&dirNS, dmLat);
		currentlon = degMin2DecDeg(&dirEW, dmLon);
		// calculated destination heading
		heading = calcBearing(currentlat, currentlon, flagsdata[FlagIndex].lat, flagsdata[FlagIndex].lon, currentheading);
		// calculated destination distance
		distance = calcDistance(currentlat, currentlon, flagsdata[FlagIndex].lat, flagsdata[FlagIndex].lon);
		SetDistanceToFlag(distance);



#if SDC_ON

		// write current position to SecureDigital then flush
		if (cardEnabled) {
			MyFile.print(currentlon);
			MyFile.print(",");
			MyFile.print(currentlat);
			MyFile.print(",");
			MyFile.println(distance);
			MyFile.flush();
		}
#endif

		break;
	}

#if NEO_ON

	// set NeoPixel target display
	setNeoPixel(heading, distance);
#endif		


#if TRM_ON
	// print debug information to Serial Terminal


	Serial.print("Calc Lat:");
	Serial.println(currentlat, 6);
	Serial.print("Calc Lon:");
	Serial.println(currentlon, 6);
	Serial.print("Calc Bearing:");
	Serial.println(bearing);
	Serial.print("Calc Heading:");
	Serial.println(heading, 6);
	Serial.print("Calculated distance: ");
	Serial.println(distance, 6);
	Serial.print("Calculated bearing: ");
	Serial.println(heading, 6);
#endif		

#if ONE_ON
	// print debug information to OneSheeld Terminal
	if (serialEventRun) serialEventRun();
#endif
}