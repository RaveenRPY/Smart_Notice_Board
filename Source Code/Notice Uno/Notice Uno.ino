#include <Adafruit_GFX.h>   // Core graphics library
#include <MCUFRIEND_kbv.h>  // Hardware-specific library
MCUFRIEND_kbv tft;

#include <SPI.h>
#include <SD.h>

#include <SoftwareSerial.h>

#include <Fonts/FreeMonoBold12pt7b.h>
#include <gfxfont.h>

#define LCD_CS A3  // Chip Select goes to Analog   3
#define LCD_CD A2  // Command/Data goes to Analog 2
#define LCD_WR A1  // LCD Write goes to Analog 1
#define LCD_RD A0  // LCD Read goes to Analog 0

#define BLACK 0x0000
#define BLUE 0x001F
#define RED 0xF800
#define GREEN 0x07E0
#define CYAN 0x07FF
#define MAGENTA 0xF81F
#define YELLOW 0xFFE0
#define WHITE 0xFFFF

#define SD_CS 10  //SD card pin on your shieldu

SoftwareSerial size(2, 3);

void setup() {

  Serial.begin(115200);
  size.begin(115200);

  pinMode(10, OUTPUT);
  digitalWrite(10, HIGH);

  tft.begin(38024);
  tft.setRotation(3);

  if (!SD.begin(SD_CS)) {
    progmemPrintln(PSTR("failed!"));
    return;
  }
  bmpDraw("Welcome.bmp", 0, 0);
  delay(1000);
}

String note;
int t;
String tSize;

void loop() {

  if (Serial.available()) {
    bmpDraw("Note2.bmp", 0, 0);
    note = Serial.readString();

    while (1) {
      if (Serial.available()) {
        tft.drawRect(25, 70, 425, 210, WHITE);
        tft.fillRect(25, 70, 425, 210, WHITE);
        note = Serial.readString();
        Serial.print(note);
      }
      const char *msg = note.c_str();
      
      showmsgXY(50, 100, 1, &FreeMonoBold12pt7b, msg);
    }
  }
}

#define BUFFPIXEL 20

int widchar, htchar;  //global variables

void printwrap(const char *msg, int x, int y) {
  int maxchars = (480 - x) / widchar;
  int len = strlen(msg), pos, i;
  char buf[maxchars + 1];
  while (len > 0) {
    pos = 0;
    for (i = 0; i < len && i < maxchars; i++) {
      char c = msg[i];
      if (c == '\n' || c == ' ') pos = i;
      if (c == '\n') break;
    }
    if (pos == 0) pos = i;    //did not find terminator
    if (i == len) pos = len;  //whole string fits
    strncpy(buf, msg, pos);   //copy the portion to print
    buf[pos] = '\0';          //terminate with NUL
    tft.print(buf);           //go use UTFT method
    msg += pos + 1;           //skip the space or linefeed
    len -= pos + 1;           //ditto
    y += htchar;              //perform linefeed
  }
}

void showmsgXY(int x, int y, int sz, const GFXfont *f, const char *msg) {
  int16_t x1, y1;
  uint16_t wid, ht;
  static uint16_t size[2] = { 0, 0 };
  tft.setFont(f);
  tft.setTextColor(BLACK);
  tft.setTextSize(sz);
  tft.setTextWrap(true);
  tft.getTextBounds(msg, x, y, &x1, &y1, &size[0], &size[1]);  //calc width of new string
  tft.setCursor((480 - size[0]) / 2, (320 - size[1]) / 2);

  if (size[0] < 410) {
    tft.setCursor((480 - size[0]) / 2, (320 - size[1]) / 2);
  }

  else {
  }

  tft.print(msg);
}

void bmpDraw(char *filename, int x, int y) {

  File bmpFile;
  int bmpWidth, bmpHeight;             //   W+H in pixels
  uint8_t bmpDepth;                    // Bit depth (currently must   be 24)
  uint32_t bmpImageoffset;             // Start of image data in file
  uint32_t rowSize;                    // Not always = bmpWidth; may have padding
  uint8_t sdbuffer[3 * BUFFPIXEL];     // pixel in buffer (R+G+B per pixel)
  uint16_t lcdbuffer[BUFFPIXEL];       // pixel   out buffer (16-bit per pixel)
  uint8_t buffidx = sizeof(sdbuffer);  // Current   position in sdbuffer
  boolean goodBmp = false;             // Set to true on valid   header parse
  boolean flip = true;                 // BMP is stored bottom-to-top
  int w, h, row, col;
  uint8_t r, g, b;
  uint32_t pos = 0, startTime = millis();
  uint8_t lcdidx = 0;
  boolean first = true;

  if ((x >= 480) || (y >= 320)) return;

  //Serial.println();
  progmemPrint(PSTR("Loading   image '"));

  // Open   requested file on SD card
  if ((bmpFile = SD.open(filename)) == NULL) {
    progmemPrintln(PSTR("File not found"));
    return;
  }

  //   Parse BMP header
  if (read16(bmpFile) == 0x4D42) {  // BMP signature
    progmemPrint(PSTR("File   size: "));
    read32(bmpFile);
    (void)read32(bmpFile);             // Read   & ignore creator bytes
    bmpImageoffset = read32(bmpFile);  // Start of image   data
    progmemPrint(PSTR("Image Offset: "));
    //Serial.println(bmpImageoffset, DEC);
    // Read DIB header
    progmemPrint(PSTR("Header size: "));
    read32(bmpFile);
    bmpWidth = read32(bmpFile);
    bmpHeight = read32(bmpFile);
    if (read16(bmpFile) == 1) {    // # planes -- must be '1'
      bmpDepth = read16(bmpFile);  // bits   per pixel
      progmemPrint(PSTR("Bit Depth: "));
      if ((bmpDepth == 24) && (read32(bmpFile) == 0)) {  // 0 = uncompressed

        goodBmp = true;  // Supported BMP format -- proceed!
        progmemPrint(PSTR("Image   size: "));

        // BMP rows are padded (if needed)   to 4-byte boundary
        rowSize = (bmpWidth * 3 + 3) & ~3;

        //   If bmpHeight is negative, image is in top-down order.
        // This is not   canon but has been observed in the wild.
        if (bmpHeight < 0) {
          bmpHeight = -bmpHeight;
          flip = false;
        }

        // Crop   area to be loaded
        w = bmpWidth;
        h = bmpHeight;
        if ((x + w - 1) >= 480) w = 480 - x;
        if ((y + h - 1) >= 320) h = (320) - y;

        // Set TFT address window to clipped image bounds
        tft.setAddrWindow(x, y, x + w - 1, y + h - 1);

        for (row = 0; row < h; row++) {  // For each scanline...
          // Seek to start of scan line.  It   might seem labor-
          // intensive to be doing this on every line, but   this
          // method covers a lot of gritty details like cropping
          //   and scanline padding.  Also, the seek only takes
          // place if the file   position actually needs to change
          // (avoids a lot of cluster math   in SD library).
          if (flip)  // Bitmap is stored bottom-to-top order (normal   BMP)
            pos = bmpImageoffset + (bmpHeight - 1 - row) * rowSize;
          else  // Bitmap is stored top-to-bottom
            pos = bmpImageoffset + row * rowSize;
          if (bmpFile.position() != pos) {  // Need seek?
            bmpFile.seek(pos);
            buffidx = sizeof(sdbuffer);  // Force   buffer reload
          }

          for (col = 0; col < w; col++) {  // For   each column...
            // Time to read more pixel data?
            if (buffidx >= sizeof(sdbuffer)) {  // Indeed
              // Push LCD buffer to   the display first
              if (lcdidx > 0) {
                tft.pushColors(lcdbuffer, lcdidx, first);
                lcdidx = 0;
                first = false;
              }
              bmpFile.read(sdbuffer, sizeof(sdbuffer));
              buffidx = 0;  // Set index to beginning
            }

            // Convert pixel   from BMP to TFT format
            b = sdbuffer[buffidx++];
            g = sdbuffer[buffidx++];
            r = sdbuffer[buffidx++];
            lcdbuffer[lcdidx++] = tft.color565(r, g, b);
          }  // end pixel
        }    // end scanline
             // Write any remaining data to LCD
        if (lcdidx > 0) {
          tft.pushColors(lcdbuffer, lcdidx, first);
        }
        progmemPrint(PSTR("Loaded in "));
      }  // end goodBmp
    }
  }

  bmpFile.close();
  if (!goodBmp) progmemPrintln(PSTR("BMP format not recognized."));
}

// These read   16- and 32-bit types from the SD card file.

uint16_t read16(File f) {
  uint16_t result;
  ((uint8_t *)&result)[0] = f.read();  // LSB
  ((uint8_t *)&result)[1] = f.read();  // MSB
  return result;
}

uint32_t read32(File f) {
  uint32_t result;
  ((uint8_t *)&result)[0] = f.read();  // LSB
  ((uint8_t *)&result)[1] = f.read();
  ((uint8_t *)&result)[2] = f.read();
  ((uint8_t *)&result)[3] = f.read();  //   MSB
  return result;
}

// Copy string from flash to serial port
//   Source string MUST be inside a PSTR() declaration!
void progmemPrint(const char *str) {
  char c;
  while (c = pgm_read_byte(str++))
    ;
}

//   Same as above, with trailing newline
void progmemPrintln(const char *str) {
  progmemPrint(str);
}