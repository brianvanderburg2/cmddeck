/*
   Small Program to Simulate a Numpad using a 2.4" TFT Touchscreen
   Program does not act as an USB HID Device, for reference purposes only
   Tested on Arduino UNO Only and 0x9341
   By William Tavares

   Note:
   This version is coplete with styling and numbers,
   if you want the smaller version get the "numpad-layout" program
   from my Github https://github.com/williamtavares/Arduino-Uno-NumPad

   Open the Serial Monitor to see the program running
   Enjoy!

   Modificatons made by Brian Vanderburg II using Elegoo Mega2560 R3
   and Elegoo TFT LCD/Touchscreen.
*/

// Includes
///////////
#include <Elegoo_GFX.h>
#include <TouchScreen.h>
#include <Elegoo_TFTLCD.h>
#include <SPI.h>
#include <SD.h>
#include <string.h>

// Various Defines
//////////////////

#define YP A3  // must be an analog pin, use "An" notation!
#define XM A2  // must be an analog pin, use "An" notation!
#define YM 9   // can be a digital pin
#define XP 8   // can be a digital pin

//SPI Communication
#define LCD_CS A3
#define LCD_CD A2
#define LCD_WR A1
#define LCD_RD A0

// optional
#define LCD_RESET A4

//SD Card 
#define SD_CS 10 

// calibration mins and max for raw data when touching edges of screen
#define TS_LEFT 920
#define TS_BOTTOM 110
#define TS_RIGHT 85
#define TS_TOP 920

#define MINPRESSURE 300
#define MAXPRESSURE 1000

//Color Definitons
#define BLACK     0x0000
#define BLUE      0x001F
#define GREY      0xCE79
#define LIGHTGREY 0xDEDB
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define BROWN   0xA145
#define PURPLE  0x801F
#define WHITE   0xFFFF

// Print debug info to screen for touch coordinates
//#define DEBUG_TOUCH

//Size of key containers 70px
#define BOXSIZE 60

// Global Variables
///////////////////

// The screen
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 364);
Elegoo_TFTLCD tft(LCD_CS, LCD_CD, LCD_WR, LCD_RD, LCD_RESET);


//Space between squares
double padding = 4;
double edge = padding/2;

double info = padding + BOXSIZE*2/3 + padding;

double C1 = edge;
double C2 = C1 + BOXSIZE + padding;
double C3 = C2 + BOXSIZE + padding;
double C4 = C3 + BOXSIZE + padding;
double C5 = C4 + BOXSIZE + padding;

double R1 = info + edge;
double R2 = R1 + BOXSIZE + padding;
double R3 = R2 + BOXSIZE + padding;
double R4 = R3 + BOXSIZE + padding;
double R5 = R4 + BOXSIZE + padding;

int col[] = {C1,C2,C3,C4,C5}; 
int row[] = {R1,R2,R3};

// Rows and columns of buttons
#define COLS 5
#define ROWS 3

struct Button {
  char filename[25] = "";
  bool modified = false;
  int x = 0;
  int y = 0;
  int w = 1;
  int h = 1;
};

struct Buttons {
    Button buttons[ROWS*COLS];
    int count = 0;
    bool modified = false;
    bool layoutChanged = true;
};

Buttons theButtons;

// Messages
char msgs[3][10] = {"", "", ""};
bool msgsModified = true;


// Helper functions
///////////////////

void strbufcpy(char* dest, const char *src, size_t n) {
  strncpy(dest, src, n);
  dest[n - 1] = '\0';
}

void (*resetFunc)(void) = 0; // Reset function at address 0

// Read commands into a buffer
//////////////////////////////

#define CMDSIZE 100
char* retrieveCmd() {
  static char accum[CMDSIZE];
  static char* next = 0;
  static int len = 0;

  if(Serial.available() || next) {
    // Handle previously found data
    if(next) {
      strcpy(accum, next);
      len = strlen(accum);
      next = 0;
    }
    
    int bytesRead = Serial.readBytes(accum + len, CMDSIZE - len - 1);
    len += bytesRead;
    accum[len] = '\0';

    // Find terminator, if buffer full, mark terminator at end
    char* term = strchr(accum, '\n');
    if(len >= CMDSIZE - 1 && !term)
      term = accum + CMDSIZE - 1;

    if(term) {
      *term = '\0';
      if(term > accum && *(term - 1) == '\r')
        *(term - 1) = '\0';

      if(term < accum + CMDSIZE - 1)
        next = term + 1;
      else
        next = accum + len; // To big, just point to end of buffer so next call copy empty string to the front and reset len
        
      return accum;
    }
  }

  return 0;
}

// Read the touch screen
////////////////////////
bool pressed = 0;
unsigned long pressedtime = 0;
bool lastpressed = 0;

int X, Y, Z;

#ifdef DEBUG_TOUCH
int PX, PY, PZ;
#endif

void retrieveTouch()
{
  digitalWrite(13, HIGH);
  TSPoint p = ts.getPoint();
  digitalWrite(13, LOW);

  //If sharing pins, you'll need to fix the directions of the touchscreen pins
  pinMode(XM, OUTPUT);
  pinMode(YP, OUTPUT);

  // on my tft the numbers are reversed so this is used instead of the above
  Y = map(p.x, TS_TOP, TS_BOTTOM, 0, tft.height()); //
  X = map(p.y, TS_LEFT, TS_RIGHT, 0, tft.width());
  Z = p.z;

  lastpressed = pressed;
  
  pressed = (p.z > MINPRESSURE && p.z < MAXPRESSURE) ? true : false;
  if(pressed) {
    pressedtime = millis();
  }
  else if(millis() - pressedtime < 150) {
    pressed = true;
  }

#ifdef DEBUG_TOUCH
  if(Z > MINPRESSURE && Z < MAXPRESSURE) {
    PX = p.x;
    PY = p.y;
    PZ = p.z;
  }
#endif
}



//int button = 0;

// Drawing functions
////////////////////

void drawBoxes(int sel = -1, bool clear=false) {
  int x, y, w, h;
  int button;
  if(clear) {
    // TODO: clear button area
  }
  for(button = 0; button < theButtons.count; button++) {
    
    x = col[theButtons.buttons[button].x];
    y = row[theButtons.buttons[button].y];
    w = theButtons.buttons[button].w;
    h = theButtons.buttons[button].h;
    
    tft.drawRect(x, y, BOXSIZE * w + padding * (w - 1), BOXSIZE * h + padding * (h - 1), button == sel ? RED : WHITE);
  }

  tft.drawRect(edge,edge,tft.width()-padding,info-edge ,WHITE);
}

void bmp(bool force=false){
  int button;
  if(force || theButtons.modified || theButtons.layoutChanged) {
    if(theButtons.layoutChanged) {
      drawBoxes(-1, true);
      theButtons.layoutChanged = false;
    }
    for(button = 0; button < theButtons.count; button++) {
      if(force || theButtons.buttons[button].modified) {
        bmpDraw(theButtons.buttons[button].filename,col[theButtons.buttons[button].x]+1,row[theButtons.buttons[button].y]+1);
        theButtons.buttons[button].modified = false;
      }
    }
    theButtons.modified = false;
  }
}

void updateInfo(){
#ifdef DEBUG_TOUCH
  char buf[10];
  if ((Z > MINPRESSURE && Z < MAXPRESSURE) || msgsModified) {
#else
  if (msgsModified) {
#endif

    msgsModified = false;
    
    tft.fillRect(edge,edge,tft.width()-padding,info-edge ,BLACK);
    tft.drawRect(edge,edge,tft.width()-padding,info-edge ,WHITE);
    tft.setTextSize(2);
    
    tft.setTextColor(WHITE);
    
    tft.setCursor(padding*2 ,20);
#ifdef DEBUG_TOUCH
    itoa(PX, buf, 10);
    tft.println(buf);
#else
    tft.println(msgs[0]);
#endif

    tft.setCursor((tft.width()*2)/5+(BOXSIZE*1/4),20);
#ifdef DEBUG_TOUCH
    itoa(PY, buf, 10);
    tft.println(buf);
#else
    tft.println(msgs[1]);
#endif

    tft.setCursor((tft.width()*4)/5,20);
#ifdef DEBUG_TOUCH
    itoa(PZ, buf, 10);
    tft.println(buf);
#else
    tft.println(msgs[2]);
#endif

  }
}

void chooseButton(){
  int x, y, w, h;
  int button;
  if (pressed == 1 and lastpressed == 0) {
    for(button = 0; button < theButtons.count; button++) {
      x = col[theButtons.buttons[button].x];
      y = row[theButtons.buttons[button].y];
      w = theButtons.buttons[button].w;
      h = theButtons.buttons[button].h;
      
      if(X > x && X < x + BOXSIZE * w + padding * (w - 1) && Y > y && Y < y + BOXSIZE * h + padding * (h - 1)) {
        drawBoxes(button);
        Serial.println(button+1);
        break;
      }
    }
  }
}




// Braw BMP from SD card
////////////////////////

// This function opens a Windows Bitmap (BMP) file and
// displays it at the given coordinates.  It's sped up
// by reading many pixels worth of data at a time
// (rather than pixel by pixel).  Increasing the buffer
// size takes more of the Arduino's precious RAM but
// makes loading a little faster.  20 pixels seems a
// good balance.

#define BUFFPIXEL 60

void bmpDraw(char *filename, int x, int y) {

  File     bmpFile;
  int      bmpWidth, bmpHeight;   // W+H in pixels
  uint8_t  bmpDepth;              // Bit depth (currently must be 24)
  uint32_t bmpImageoffset;        // Start of image data in file
  uint32_t rowSize;               // Not always = bmpWidth; may have padding
  uint8_t  sdbuffer[3*BUFFPIXEL]; // pixel in buffer (R+G+B per pixel)
  uint16_t lcdbuffer[BUFFPIXEL];  // pixel out buffer (16-bit per pixel)
  uint8_t  buffidx = sizeof(sdbuffer); // Current position in sdbuffer
  boolean  goodBmp = false;       // Set to true on valid header parse
  boolean  flip    = true;        // BMP is stored bottom-to-top
  int      w, h, row, col;
  uint8_t  r, g, b;
  uint32_t pos = 0, startTime = millis();
  uint8_t  lcdidx = 0;
  boolean  first = true;

  if((x >= tft.width()) || (y >= tft.height())) return;

  //Serial.println();
  //Serial.print(F("Loading image '"));
  //Serial.print(filename);
  //Serial.println('\'');
  // Open requested file on SD card
  if ((bmpFile = SD.open(filename)) == NULL) {
    //Serial.println(F("File not found"));
    return;
  }

  // Parse BMP header
  if(read16(bmpFile) == 0x4D42) { // BMP signature
    //Serial.println(F("File size: ")); 
    read32(bmpFile);
    (void)read32(bmpFile); // Read & ignore creator bytes
    bmpImageoffset = read32(bmpFile); // Start of image data
    //Serial.print(F("Image Offset: ")); 
    (bmpImageoffset, DEC);
    // Read DIB header
    //Serial.print(F("Header size: ")); 
    read32(bmpFile);
    bmpWidth  = read32(bmpFile);
    bmpHeight = read32(bmpFile);
    if(read16(bmpFile) == 1) { // # planes -- must be '1'
      bmpDepth = read16(bmpFile); // bits per pixel
      //Serial.print(F("Bit Depth: ")); Serial.println(bmpDepth);
      if((bmpDepth == 24) && (read32(bmpFile) == 0)) { // 0 = uncompressed

        goodBmp = true; // Supported BMP format -- proceed!
        //Serial.print(F("Image size: "));
        //Serial.print(bmpWidth);
        //Serial.print('x');
        //Serial.println(bmpHeight);

        // BMP rows are padded (if needed) to 4-byte boundary
        rowSize = (bmpWidth * 3 + 3) & ~3;

        // If bmpHeight is negative, image is in top-down order.
        // This is not canon but has been observed in the wild.
        if(bmpHeight < 0) {
          bmpHeight = -bmpHeight;
          flip      = false;
        }

        // Crop area to be loaded
        w = bmpWidth;
        h = bmpHeight;
        if((x+w-1) >= tft.width())  w = tft.width()  - x;
        if((y+h-1) >= tft.height()) h = tft.height() - y;

        // Set TFT address window to clipped image bounds
        tft.setAddrWindow(x, y, x+w-1, y+h-1);

        for (row=0; row<h; row++) { // For each scanline...
          // Seek to start of scan line.  It might seem labor-
          // intensive to be doing this on every line, but this
          // method covers a lot of gritty details like cropping
          // and scanline padding.  Also, the seek only takes
          // place if the file position actually needs to change
          // (avoids a lot of cluster math in SD library).
          if(flip) // Bitmap is stored bottom-to-top order (normal BMP)
            pos = bmpImageoffset + (bmpHeight - 1 - row) * rowSize;
          else     // Bitmap is stored top-to-bottom
            pos = bmpImageoffset + row * rowSize;
          if(bmpFile.position() != pos) { // Need seek?
            bmpFile.seek(pos);
            buffidx = sizeof(sdbuffer); // Force buffer reload
          }

          for (col=0; col<w; col++) { // For each column...
            // Time to read more pixel data?
            if (buffidx >= sizeof(sdbuffer)) { // Indeed
              // Push LCD buffer to the display first
              if(lcdidx > 0) {
                tft.pushColors(lcdbuffer, lcdidx, first);
                lcdidx = 0;
                first  = false;
              }
              bmpFile.read(sdbuffer, sizeof(sdbuffer));
              buffidx = 0; // Set index to beginning
            }

            // Convert pixel from BMP to TFT format
            b = sdbuffer[buffidx++];
            g = sdbuffer[buffidx++];
            r = sdbuffer[buffidx++];
            lcdbuffer[lcdidx++] = tft.color565(r,g,b);
          } // end pixel
        } // end scanline
        // Write any remaining data to LCD
        if(lcdidx > 0) {
          tft.pushColors(lcdbuffer, lcdidx, first);
        } 
        //Serial.print(F("Loaded in "));
        //Serial.print(millis() - startTime);
        //Serial.println(" ms");
      } // end goodBmp
    }
  }

  bmpFile.close();
  if(!goodBmp) {
    Serial.print(filename);
    Serial.println(F(" BMP format not recognized."));
  }
}

// These read 16- and 32-bit types from the SD card file.
// BMP data is stored little-endian, Arduino is little-endian too.
// May need to reverse subscript order if porting elsewhere.

uint16_t read16(File f) {
  uint16_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read(); // MSB
  return result;
}

uint32_t read32(File f) {
  uint32_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read();
  ((uint8_t *)&result)[2] = f.read();
  ((uint8_t *)&result)[3] = f.read(); // MSB
  return result;
}

// Commands handling
////////////////////

bool freeze = false;

bool processButtonCmd(char* cmd) {
  char *start = 0;
  int button;

  if(cmd[1] == ' ' && cmd[0] >= '1' && cmd[0] <= '9') {
    button = cmd[0] - '1';
    start = cmd + 2;
  }
  else if(cmd[2] == ' ' && cmd[0] == '1' && cmd[1] >= '0' && cmd[1] <='5') {
    button = cmd[1] - '0' + 9;
    start = cmd + 3;
  }
  else {
    return false;
  }

  // Set button filename and set modified stated
  strbufcpy(theButtons.buttons[button].filename, start, 25);
  theButtons.buttons[button].modified = true;
  theButtons.modified = true;
  return true;
}

bool processAddButton(char* cmd) {
  if(theButtons.count == 15) {
    return false; // Can't add any more buttons
  }

  // Remainder of command is string of XYWH
  if(strlen(cmd) != 4) {
    return false; // Incorrect number of parameters
  }

  int idx = theButtons.count;
  int x = theButtons.buttons[idx].x = constrain(cmd[0] - '1', 0, 4);
  int y = theButtons.buttons[idx].y = constrain(cmd[1] - '1', 0, 2);
  theButtons.buttons[idx].w = constrain(cmd[2] - '0', 1, 5 - x);
  theButtons.buttons[idx].h = constrain(cmd[3] - '0', 1, 3 - y);
  theButtons.buttons[idx].modified = true;

  theButtons.count++;
  theButtons.layoutChanged = true;
  theButtons.modified = true;
  return true;
}

void processCmd(char* cmd) {
  if(cmd == 0) return;

  bool result = true;

  // TODO: better command parser that doesn't have to strcmp every string

  if(strcmp(cmd, "freeze") == 0) {
    freeze = true;
  }
  else if(strcmp(cmd, "unfreeze") == 0) {
    freeze = false;
  }
  else if(strncmp(cmd, "msg1 ", 4) == 0) {
    strbufcpy(msgs[0], cmd + 4, 10);
    msgsModified = true;
  }
  else if(strncmp(cmd, "msg2 ", 4) == 0) {
    strbufcpy(msgs[1], cmd + 4, 10);
    msgsModified = true;
  }
  else if(strncmp(cmd, "msg3 ", 4) == 0) {
    strbufcpy(msgs[2],cmd + 4, 10);
    msgsModified = true;
  }
  else if(strncmp(cmd, "button", 6) == 0) {
    result = processButtonCmd(cmd + 6);
  }
  else if(strncmp(cmd, "addbutton ", 10) == 0) {
    result = processAddButton(cmd + 10); 
  }
  else if(strcmp(cmd, "clearbuttons") == 0) {
    theButtons.count = 0;
    theButtons.modified = true;
  }
  else if(strcmp(cmd, "reset") == 0) {
    resetFunc();
  }
  else {
    result = false;
  }

  if(result) {
    Serial.println("OK");
  }
  else {
    Serial.println("NOK");
  }
}

// Main program code/loops
//////////////////////////

void setup() {
  // Set up defualt buttons
  for(int button = 0; button < 15; button++) {
    theButtons.buttons[button].x = button % 5;
    theButtons.buttons[button].y = button / 5;
    theButtons.buttons[button].w = theButtons.buttons[button].h = 1;
  }
  theButtons.count = 15;


  Serial.begin(9600);
  Serial.setTimeout(10); // This speeds command response time up a bit.

  tft.reset();
  uint16_t identifier = 0x9341; // Need to manually set this for some reason.
  
  tft.begin(identifier);
  tft.setRotation(1); //Rotate 90 degrees

  //Background color
  tft.fillScreen(BLACK);

  Serial.print(F("Initializing SD card..."));
  if (!SD.begin(SD_CS)) {
    Serial.println(F("failed!"));
    strcpy(msgs[0], "NO SD");
  }
  else {
    Serial.println(F("OK!"));
  }
  
  //Background color
  tft.fillScreen(BLACK);

  drawBoxes();
  bmp(true);
}

void loop() {
  processCmd(retrieveCmd());

  if(!freeze) {
    retrieveTouch();
    chooseButton();
    updateInfo();
    bmp(false);
  }
}

