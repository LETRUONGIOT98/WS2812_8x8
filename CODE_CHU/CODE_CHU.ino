
#define BRIGHTNESS 30
#define DEFAULT_BRIGHTNESS 10
#define MAX_APDS_VAL 1000
#define MAT_PIN 6

//////////////--------------------------------------------------------------------|
/// PUBLIC ///                                                                    |
//////////////--------------------------------------------------------------------|

Panel::Panel(unsigned pin, unsigned width, unsigned height, Style_enum style, IniSide_enum iniSide, unsigned matrixRotation, neoPixelType stripParams) 
  :  _pin(pin), _numLeds(width*height), _width(width), _height(height), _layoutStyle(style), _iniSide(iniSide), 
    _stripParams(stripParams), _rotation(0), _x_ref(0), _y_ref(0)
{

    rotateMatrix(matrixRotation);
    
    _autoBrightnessMin = 10;
    _autoBrightnessMax = 255;
    _autoBrightnessMode = false;

    _cData = (uint32_t*)malloc(_numLeds*sizeof(uint32_t));
    memset(_cData, _numLeds*sizeof(uint32_t), 0);
    
    _strip = new Adafruit_NeoPixel(_numLeds, _pin, _stripParams); 
}


Panel::Panel(unsigned pin, unsigned width, unsigned height, Style_enum style, IniSide_enum iniSide, unsigned matrixRotation)
  : Panel(pin, width, height, style, iniSide, matrixRotation, NEO_GRB + NEO_KHZ800) {}

Panel::Panel(unsigned width, unsigned height, Style_enum style, IniSide_enum iniSide, unsigned matrixRotation)
  : Panel(MAT_PIN, width, height, style, iniSide, matrixRotation){}

Panel::~Panel(){
    free(_cData);
    delete _strip;  
}

void Panel::rebuild(unsigned pin, unsigned width, unsigned height, Style_enum style, IniSide_enum iniSide, int matrixRotation, neoPixelType stripParams){
    //clear
    free(_cData);
    delete _strip; 

    _width = width;
    _height = height;
    _numLeds = width*height;
  
    _iniSide = iniSide;
    _layoutStyle = style;
    _stripParams = stripParams;
    rotateMatrix(matrixRotation);
   
    _cData = (uint32_t*)malloc(_numLeds*sizeof(uint32_t));
    memset(_cData, _numLeds*sizeof(uint32_t), 0);
    
    _strip = new Adafruit_NeoPixel(_numLeds, _pin, _stripParams); 
}

void Panel::rebuild(unsigned pin, unsigned width, unsigned height, Style_enum style, IniSide_enum iniSide, int matrixRotation){
    rebuild(pin, width, height, style, iniSide, matrixRotation, _stripParams);
}

void Panel::rebuild(unsigned width, unsigned height, Style_enum style, IniSide_enum iniSide, int matrixRotation){
    rebuild(_pin, width, height, style, iniSide, matrixRotation, _stripParams);
}

void Panel::rebuild(unsigned width, unsigned height){
    rebuild(_pin, width, height, _layoutStyle, _iniSide, _matrixRotation, _stripParams);
}

void Panel::testLayout(){
  for(unsigned i=0; i<_numLeds; ++i){
    _strip->setPixelColor(i, _strip->Color(random(0, 255), random(0, 255), random(0, 255)));
    show();
    delay(100);
  }
}

void Panel::rotateMatrix(int deg){
  deg %= 360;

  if(deg < 0)
    deg = 360 -deg;

  _matrixRotation = deg;
}

bool Panel::begin(){
    _strip->begin();
    
    if (!APDS.begin())
      return false;  

    return true;
}

void Panel::setBrightness(int lvl){
    _strip->setBrightness(lvl);
}

void Panel::setAutoBrightness(int _min, int _max, bool enable){
    _autoBrightnessMin = _min;
    _autoBrightnessMax = _max;
    _autoBrightnessMode = enable;
}

bool Panel::canShow(){
    return _strip->canShow();
}

void Panel::show(){
    if(_autoBrightnessMode)
          if(APDS.colorAvailable()){
                int r, g, b;
                APDS.readColor(r, g, b);  
                setBrightness((r+g+b)/MAX_APDS_VAL*(_autoBrightnessMax-_autoBrightnessMin) + _autoBrightnessMin);
          }
  
    _strip->show();
}

bool Panel::setPixel(int x, int y, uint32_t color){
    if(calcTrans(x, y))
      return -1;
    
    unsigned pos = XY(x, y);

    _cData[pos] = color;
    _strip->setPixelColor(pos, color);
    return true;
}
bool Panel::setPixel(int x, int y){
    return setPixel(x, y, _fillColor);
}

void Panel::setPixel(uint32_t color){
    for(int i=0; i<_numLeds; i++)
        _strip->setPixelColor(i, color);
}

uint32_t Panel::getPixel(int x, int y) const{ 

    if(calcTrans(x, y))
      return -1;
      
    return _cData[XY(x, y)];
}

void Panel::fill(uint32_t color){
    _fillColor = color;
}

void Panel::clear(){
    setPixel(_strip->Color(0, 0, 0));
}

void Panel::line(int x0, int y0, int x1, int y1){
    line(x0, y0, x1, y1, _fillColor);
}

void Panel::line(int x0, int y0, int x1, int y1, uint32_t color){ 
    orderPoints(x0, y0, x1, y1);

    // Draw
    if(x0 == x1)
        for (int y = y0; y <= y1; ++y)
            setPixel(x0, y, color);   
    else if(y0 == y1)
        for (int x = x0; x <= x1; ++x)
          setPixel(x, y0, color);
    else {
        int deltaX = abs(x1-x0), deltaY = (y1-y0); 
  
        if(deltaX == deltaY)
          for (int i = 0; i < deltaY; ++i)
            setPixel(x0+i, y0+i, color);
    }
}

void Panel::rect(int x0, int y0, unsigned width, unsigned height){
  rect(x0, y0, width, height, _fillColor);
}

void Panel::rect(int x0, int y0, unsigned width, unsigned height, uint32_t color){
    for(int y = y0; y<(y0+height); ++y)
        for(int x = x0; x<(x0+width); ++x)
            setPixel(x, y, color);        
}

void Panel::image(uint32_t *img, unsigned width, unsigned height, int posX, int posY){
    for(unsigned x=0; x<width; ++x)
        for(unsigned y=0; y<height; ++y)
            setPixel(posX+x, posY+y, *((img + y*width) + x));
}

void Panel::image(Img_t img, int posX, int posY){
    for(unsigned x=0; x<img.width; ++x)
        for(unsigned y=0; y<img.height; ++y)
            setPixel(posX+x, posY+y, *((img.mat + y*img.width) + x));
}

void Panel::pushMatrix(){
  popX = _x_ref;
  popY = _y_ref;
  popRot = _rotation; 
  popMatrixRot = _matrixRotation;
}

void Panel::popMatrix(){
  _x_ref = popX;
  _y_ref = popY;
  _rotation = popRot;
  _matrixRotation = popMatrixRot;
}


void Panel::translate(int x, int y){
  _x_ref += x;
  _y_ref += y;
}

void Panel::rotate(int deg){
  deg %= 360;

  _rotation += deg;

  if(_rotation < 0)
    _rotation = 360 - _rotation;
}





///////////////-------------------------------------------------------------------|
/// PRIVATE ///                                                                   |
///////////////-------------------------------------------------------------------|


inline unsigned Panel::XY(unsigned x, unsigned y) const{
    unsigned i;

    //calc matrix rotation
    if(_matrixRotation == 90){
        unsigned tx = x;
        x = _width-1-y;
        y = tx;
    }
    else if(_matrixRotation == 180){
        x = _width-1-x;
        y = _height-1-y;
    }
    else if(_matrixRotation == 270){
        unsigned tx = x;
        x = y;
        y = _height-1-tx;
    }

    if(_iniSide == RIGHT)
        x = _width-1 - x;
            
    switch(_layoutStyle){
        case LINE:
            i = (y * _width) + x;
            break;
        case SERPENTINE:
            if( y & 0x01) {
              // Odd rows run backwards
              unsigned revX = (_width - 1) - x;
              i = (y * _width) + revX;
            } else {
              // Even rows run forwards
              i = (y * _width) + x;
            }
            break;
    }
      
  return i;
}

inline bool Panel::calcTrans(int &x, int &y) const{
    
    //calc dot rotation  
    if (_rotation == 90){
        int tx = x;
    
        x = y;
        y = -tx;
    }
    else if (_rotation == 180){
        x = -x;
        y = -y;
    }
    else if (_rotation == 270){
      int tx = x;
  
      x = -y;
      y = tx;
    }  
    
    //calc translation
    x += _x_ref;
    y += _y_ref;

    //ensure range
    if( x >= _width  || x < 0) return -1;
    if( y >= _height || y < 0) return -1;

    return 0;
}

inline void Panel::orderPoints(int &x0, int &y0, int &x1, int &y1) const{
    //make sure p1 is greater/equal than p0 -> (x1 >= x0 && y1 >= y0) 
    if(x1 < x0 || y1 < y0){
        int tx = x0, ty = y0;
    
        x0 = x1;
        y0 = y1;
    
        x1 = tx;
        y1 = ty;
    }
}
#ifndef PANEL_H
#define PANEL_H




typedef enum Style_enum{
  SERPENTINE,
  LINE
};

typedef enum IniSide_enum{
    LEFT,
    RIGHT 
};

typedef struct {
  const uint32_t *mat;
  const uint8_t width, height;  
} Img_t;

 
class Panel{
    private:
      //params
      neoPixelType _stripParams;
      unsigned  _pin;
      unsigned  _width, _height, _numLeds;
      Style_enum _layoutStyle;
      IniSide_enum _iniSide;
      
      // transformations
      int _x_ref, _y_ref; 
      unsigned _matrixRotation, _rotation;
      int popX, popY, popRot, popMatrixRot;
      
      //containers
      Adafruit_NeoPixel *_strip;
      uint32_t *_cData;

      int _autoBrightnessMin, _autoBrightnessMax;
      bool _autoBrightnessMode;
      
      uint32_t _fillColor;

      
      unsigned XY( unsigned x, unsigned y) const;
      bool calcTrans(int &x, int &y) const;
      void orderPoints(int &x0, int &y0, int &x1, int &y1) const;

      
    public:

    Panel(unsigned pin, unsigned width, unsigned height, Style_enum style, IniSide_enum iniSide, unsigned matrixRotation, neoPixelType stripParams);   
    Panel(unsigned pin, unsigned width, unsigned height, Style_enum style, IniSide_enum iniSide, unsigned matrixRotation); 
    Panel(unsigned width, unsigned height, Style_enum style, IniSide_enum iniSide, unsigned matrixRotation);     

    ~Panel();

    void rebuild(unsigned pin, unsigned width, unsigned height, Style_enum style, IniSide_enum iniSide, int matrixRotation, neoPixelType stripParams);
    void rebuild(unsigned pin, unsigned width, unsigned height, Style_enum style, IniSide_enum iniSide, int matrixRotation);
    void rebuild(unsigned width, unsigned height, Style_enum style, IniSide_enum iniSide, int matrixRotation);
    void rebuild(unsigned width, unsigned height);

    void rotateMatrix(int deg);
    void testLayout();
    
    bool begin();
    void setBrightness(int lvl);
    void setAutoBrightness(int _min, int _max, bool enable);
    bool canShow();
    void show();

    bool setPixel(int x, int y, uint32_t color);
    bool setPixel(int x, int y);
    void setPixel(uint32_t color);
    uint32_t getPixel(int x, int y) const;
    void fill(uint32_t color);
    void clear();
    void line(int x0, int y0, int x1, int y1);
    void line(int x0, int y0, int x1, int y1, uint32_t color);
    void rect(int x0, int y0, unsigned width, unsigned height);
    void rect(int x0, int y0, unsigned width, unsigned height, uint32_t color);
    void image(Img_t img, int posX, int posY);
    void image(uint32_t *img, unsigned width, unsigned height, int posX, int posY);

    void pushMatrix();
    void popMatrix();
    void translate(int x, int y);
    void rotate(int deg);

        unsigned getWidth(){ return _width; }
        unsigned getHeight(){ return _height; }
        unsigned getRotation(){ return _rotation; }
        unsigned getMatrixRotation(){ return _matrixRotation; }
        uint32_t getFillColor(){ return _fillColor; }
        int getXTranslation(){ return _x_ref; }
        int getYTranslation(){ return _y_ref; }
        Style_enum getLayoutStyle(){ return _layoutStyle; }
        IniSide_enum getIniSide(){ return _iniSide; }


    static uint32_t color(uint8_t r, uint8_t g, uint8_t b){
        return Adafruit_NeoPixel::gamma32(Adafruit_NeoPixel::Color(r, g, b));
    }
    static uint32_t colorHSV(uint16_t hue, uint8_t sat, uint8_t val){
        return Adafruit_NeoPixel::ColorHSV(hue, sat, val);
    }
    static uint32_t colorUncorrected(uint8_t r, uint8_t g, uint8_t b){
        return Adafruit_NeoPixel::Color(r, g, b);
    }


      
};   

#endif
#include <Arduino.h>
#include <Wire.h>

enum {
  GESTURE_NONE = -1,
  GESTURE_UP = 0,
  GESTURE_DOWN = 1,
  GESTURE_LEFT = 2,
  GESTURE_RIGHT = 3
};

class APDS9960 {
public:
  APDS9960(TwoWire &wire, int intPin);
  virtual ~APDS9960();

  bool begin();
  void end();

  int gestureAvailable();
  int readGesture();

  int colorAvailable();
  bool readColor(int& r, int& g, int& b);
  bool readColor(int& r, int& g, int& b, int& c);

  int proximityAvailable();
  int readProximity();

  void setGestureSensitivity(uint8_t sensitivity);

  void setInterruptPin(int pin);

  bool setLEDBoost(uint8_t boost);

private:
  bool setGestureIntEnable(bool en);
  bool setGestureMode(bool en);
  int gestureFIFOAvailable();
  int handleGesture();

  bool enablePower();
  bool disablePower();
  bool enableColor();
  bool disableColor();
  bool enableProximity();
  bool disableProximity();
  bool enableWait();
  bool disableWait();
  bool enableGesture();
  bool disableGesture();

private:
  TwoWire& _wire;
  int _intPin;

  bool _gestureEnabled;
  bool _proximityEnabled;
  bool _colorEnabled;
  bool _gestureIn;
  int _gestureDirectionX;
  int _gestureDirectionY;
  int _gestureDirInX;
  int _gestureDirInY;
  int _gestureSensitivity;
  int _detectedGesture;

  bool write(uint8_t val);
  bool write(uint8_t reg, uint8_t val);
  bool read(uint8_t reg, uint8_t *val);
  size_t readBlock(uint8_t reg, uint8_t *val, unsigned int len);

private:
#define REG(name, addr) \
  bool get##name(uint8_t *val) { return read(addr,  val); } \
  bool set##name(uint8_t val)  { return write(addr, val); } \
  size_t read##name(uint8_t *val, uint8_t len) { return readBlock(addr, val, len); }
  REG(ENABLE,     0x80)
  REG(ATIME,      0x81)
  REG(WTIME,      0x83)
  REG(AILTL,      0x84)
  REG(AILTH,      0x85)
  REG(AIHTL,      0x86)
  REG(AIHTH,      0x87)
  REG(PILT,       0x89)
  REG(PIHT,       0x8B)
  REG(PERS,       0x8C)
  REG(CONFIG1,    0x8D)
  REG(PPULSE,     0x8E)
  REG(CONTROL,    0x8F)
  REG(CONFIG2,    0x90)
  REG(ID,         0x92)
  REG(STATUS,     0x93)
  REG(CDATAL,     0x94)
  REG(CDATAH,     0x95)
  REG(RDATAL,     0x96)
  REG(RDATAH,     0x97)
  REG(GDATAL,     0x98)
  REG(GDATAH,     0x99)
  REG(BDATAL,     0x9A)
  REG(BDATAH,     0x9B)
  REG(PDATA,      0x9C)
  REG(POFFSET_UR, 0x9D)
  REG(POFFSET_DL, 0x9E)
  REG(CONFIG3,    0x9F)
  REG(GPENTH,     0xA0)
  REG(GEXTH,      0xA1)
  REG(GCONF1,     0xA2)
  REG(GCONF2,     0xA3)
  REG(GOFFSET_U,  0xA4)
  REG(GOFFSET_D,  0xA5)
  REG(GPULSE,     0xA6)
  REG(GOFFSET_L,  0xA7)
  REG(GOFFSET_R,  0xA9)
  REG(GCONF3,     0xAA)
  REG(GCONF4,     0xAB)
  REG(GFLVL,      0xAE)
  REG(GSTATUS,    0xAF)
  REG(IFORCE,     0xE4)
  REG(PICLEAR,    0xE5)
  REG(CICLEAR,    0xE6)
  REG(AICLEAR,    0xE7)
  REG(GFIFO_U,    0xFC)
  REG(GFIFO_D,    0xFD)
  REG(GFIFO_L,    0xFE)
  REG(GFIFO_R,    0xFF)
};

extern APDS9960 APDS;

APDS9960::APDS9960(TwoWire& wire, int intPin) :
  _wire(wire),
  _intPin(intPin),
  _gestureEnabled(false),
  _proximityEnabled(false),
  _colorEnabled(false),
  _gestureIn(false),
  _gestureDirectionX(0),
  _gestureDirectionY(0),
  _gestureDirInX(0),
  _gestureDirInY(0),
  _gestureSensitivity(20),
  _detectedGesture(GESTURE_NONE)
{
}

APDS9960::~APDS9960()
{
}

bool APDS9960::begin() {
  _wire.begin();
    
  // Check ID register
  uint8_t id;
  if (!getID(&id)) return false;
  if (id!=0xAB) return false;
    
  // Disable everything
  if (!setENABLE(0x00)) return false;
  if (!setWTIME(0xFF)) return false;
  if (!setGPULSE(0x8F)) return false; // 16us, 16 pulses // default is: 0x40 = 8us, 1 pulse
  if (!setPPULSE(0x8F)) return false; // 16us, 16 pulses // default is: 0x40 = 8us, 1 pulse
  if (!setGestureIntEnable(true)) return false;
  if (!setGestureMode(true)) return false;
  if (!enablePower()) return false;
  if (!enableWait()) return false;
  // set ADC integration time to 10 ms
  if (!setATIME(256 - (10 / 2.78))) return false;
  // set ADC gain 4x (0x00 => 1x, 0x01 => 4x, 0x02 => 16x, 0x03 => 64x)
  if (!setCONTROL(0x02)) return false;
  delay(10);
  // enable power
  if (!enablePower()) return false;

  if (_intPin > -1) {
    pinMode(_intPin, INPUT);
  }

  return true;
}

void APDS9960::end() {
  // Disable everything
  setENABLE(0x00);

  _gestureEnabled = false;

  _wire.end();
}

// Sets the LED current boost value:
// 0=100%, 1=150%, 2=200%, 3=300%
bool APDS9960::setLEDBoost(uint8_t boost) {
  uint8_t r;
  if (!getCONFIG2(&r)) return false;
  r &= 0b11001111;
  r |= (boost << 4) & 0b00110000;
  return setCONFIG2(r);
}

void APDS9960::setGestureSensitivity(uint8_t sensitivity) {
  if (sensitivity > 100) sensitivity = 100;
  _gestureSensitivity = 100 - sensitivity;
}

void APDS9960::setInterruptPin(int pin) {
  _intPin = pin;
}

bool APDS9960::setGestureIntEnable(bool en) {
    uint8_t r;
    if (!getGCONF4(&r)) return false;
    if (en) {
      r |= 0b00000010;
    } else {
      r &= 0b11111101;
    }
    return setGCONF4(r);
}

bool APDS9960::setGestureMode(bool en)
{
    uint8_t r;
    if (!getGCONF4(&r)) return false;
    if (en) {
      r |= 0b00000001;
    } else {
      r &= 0b11111110;
    }
    return setGCONF4(r);
}

bool APDS9960::enablePower() {
  uint8_t r;
  if (!getENABLE(&r)) return false;
  if ((r & 0b00000001) != 0) return true;
  r |= 0b00000001;
  return setENABLE(r);
}

bool APDS9960::disablePower() {
  uint8_t r;
  if (!getENABLE(&r)) return false;
  if ((r & 0b00000001) == 0) return true;
  r &= 0b11111110;
  return setENABLE(r);
}

bool APDS9960::enableColor() {
  uint8_t r;
  if (!getENABLE(&r)) return false;
  if ((r & 0b00000010) != 0) {
    _colorEnabled = true;
    return true;
  }
  r |= 0b00000010;
  bool res = setENABLE(r);
  _colorEnabled = res;
  return res;
}

bool APDS9960::disableColor() {
  uint8_t r;
  if (!getENABLE(&r)) return false;
  if ((r & 0b00000010) == 0) {
    _colorEnabled = false;
    return true;
  }
  r &= 0b11111101;
  bool res = setENABLE(r);
  _colorEnabled = !res; // (res == true) if successfully disabled
  return res;
}

bool APDS9960::enableProximity() {
  uint8_t r;
  if (!getENABLE(&r)) return false;
  if ((r & 0b00000100) != 0) {
    _proximityEnabled = true;
    return true;
  }
  r |= 0b00000100;
  bool res = setENABLE(r);
  _proximityEnabled = res;
  return res;
}

bool APDS9960::disableProximity() {
  uint8_t r;
  if (!getENABLE(&r)) return false;
  if ((r & 0b00000100) == 0) {
    _proximityEnabled = false;
    return true;
  }
  r &= 0b11111011;
  bool res = setENABLE(r);
  _proximityEnabled = !res; // (res == true) if successfully disabled
  return res;
}

bool APDS9960::enableWait() {
  uint8_t r;
  if (!getENABLE(&r)) return false;
  if ((r & 0b00001000) != 0) return true;
  r |= 0b00001000;
  return setENABLE(r);
}

bool APDS9960::disableWait() {
  uint8_t r;
  if (!getENABLE(&r)) return false;
  if ((r & 0b00001000) == 0) return true;
  r &= 0b11110111;
  return setENABLE(r);
}

bool APDS9960::enableGesture() {
  uint8_t r;
  if (!getENABLE(&r)) return false;
  if ((r & 0b01000000) != 0) {
    _gestureEnabled = true;
    return true;
  }
  r |= 0b01000000;
  bool res = setENABLE(r);
  _gestureEnabled = res;
  return res;
}

bool APDS9960::disableGesture() {
  uint8_t r;
  if (!getENABLE(&r)) return false;
  if ((r & 0b01000000) == 0) {
    _gestureEnabled = false;
    return true;
  }
  r &= 0b10111111;
  bool res = setENABLE(r);
  _gestureEnabled = !res; // (res == true) if successfully disabled
  return res;
}

#define APDS9960_ADDR 0x39

bool APDS9960::write(uint8_t val) {
  _wire.beginTransmission(APDS9960_ADDR);
  _wire.write(val);
  return _wire.endTransmission() == 0;
}

bool APDS9960::write(uint8_t reg, uint8_t val) {
  _wire.beginTransmission(APDS9960_ADDR);
  _wire.write(reg);
  _wire.write(val);
  return _wire.endTransmission() == 0;
}

bool APDS9960::read(uint8_t reg, uint8_t *val) {
  if (!write(reg)) return false;
  _wire.requestFrom(APDS9960_ADDR, 1);
  if (!_wire.available()) return false;
  *val = _wire.read();
  return true;
}

size_t APDS9960::readBlock(uint8_t reg, uint8_t *val, unsigned int len) {
    size_t i = 0;
    if (!write(reg)) return 0;
    _wire.requestFrom((uint8_t)APDS9960_ADDR, len);
    while (_wire.available()) {
      if (i == len) return 0;
      val[i++] = _wire.read();
    }
    return i;
}

int APDS9960::gestureFIFOAvailable() {
  uint8_t r;
  if (!getGSTATUS(&r)) return -1;
  if ((r & 0x01) == 0x00) return -2;
  if (!getGFLVL(&r)) return -3;
  return r;
}

int APDS9960::handleGesture() {
  const int gestureThreshold = 30;
  while (true) {
    int available = gestureFIFOAvailable();
    if (available <= 0) return 0;

    uint8_t fifo_data[128];
    uint8_t bytes_read = readGFIFO_U(fifo_data, available * 4);
    if (bytes_read == 0) return 0;

    for (int i = 0; i+3 < bytes_read; i+=4) {
      uint8_t u,d,l,r;
      u = fifo_data[i];
      d = fifo_data[i+1];
      l = fifo_data[i+2];
      r = fifo_data[i+3];
      // Serial.print(u);
      // Serial.print(",");
      // Serial.print(d);
      // Serial.print(",");
      // Serial.print(l);
      // Serial.print(",");
      // Serial.println(r);

      if (u<gestureThreshold && d<gestureThreshold && l<gestureThreshold && r<gestureThreshold) {
        _gestureIn = true;
        if (_gestureDirInX != 0 || _gestureDirInY != 0) {
          int totalX = _gestureDirInX - _gestureDirectionX;
          int totalY = _gestureDirInY - _gestureDirectionY;
          // Serial.print("OUT ");
          // Serial.print(totalX);
          // Serial.print(",");
          // Serial.println(totalY);
          if (totalX < -_gestureSensitivity) { _detectedGesture = GESTURE_LEFT; }
          if (totalX > _gestureSensitivity) { _detectedGesture = GESTURE_RIGHT; }
          if (totalY < -_gestureSensitivity) { _detectedGesture = GESTURE_DOWN; }
          if (totalY > _gestureSensitivity) { _detectedGesture = GESTURE_UP; }
          _gestureDirectionX = 0;
          _gestureDirectionY = 0;
          _gestureDirInX = 0;
          _gestureDirInY = 0;
        }
        continue;
      }

      _gestureDirectionX = r - l;
      _gestureDirectionY = u - d;
      if (_gestureIn) {
        _gestureIn = false;
        _gestureDirInX = _gestureDirectionX;
        _gestureDirInY = _gestureDirectionY;
        // Serial.print("IN ");
        // Serial.print(_gestureDirInX);
        // Serial.print(",");
        // Serial.print(_gestureDirInY);
        // Serial.print(" ");
      }
    }
  }
}

int APDS9960::gestureAvailable() {
  if (!_gestureEnabled) enableGesture();

  if (_intPin > -1) {
    if (digitalRead(_intPin) != LOW) {
      return 0;
    }
  } else if (gestureFIFOAvailable() <= 0) {
    return 0;
  }

  handleGesture();
  if (_proximityEnabled) {
    setGestureMode(false);
  }
  return (_detectedGesture == GESTURE_NONE) ? 0 : 1;
}

int APDS9960::readGesture() {
  int gesture = _detectedGesture;

  _detectedGesture = GESTURE_NONE;

  return gesture;
}

int APDS9960::colorAvailable() {
  uint8_t r;

  enableColor();

  if (!getSTATUS(&r)) {
    return 0;
  }

  if (r & 0b00000001) {
    return 1;
  }

  return 0;
}

bool APDS9960::readColor(int& r, int& g, int& b) {
  int c;

  return readColor(r, g, b, c);
}

bool APDS9960::readColor(int& r, int& g, int& b, int& c) {
  uint16_t colors[4];

  if (!readCDATAL((uint8_t *)colors, sizeof(colors))) {
    r = -1;
    g = -1;
    b = -1;
    c = -1;

    return false;
  }

  c = colors[0];
  r = colors[1];
  g = colors[2];
  b = colors[3];

  disableColor();

  return true;
}

int APDS9960::proximityAvailable() {
  uint8_t r;

  enableProximity();

  if (!getSTATUS(&r)) {
    return 0;
  }

  if (r & 0b00000010) {
    return 1;
  }

  return 0;
}

int APDS9960::readProximity() {
  uint8_t r;

  if (!getPDATA(&r)) {
    return -1;
  }

  disableProximity();

  return (255 - r);
}

#if defined(APDS9960_INT_PIN)
APDS9960 APDS(APDS9960_WIRE_INSTANCE, APDS9960_INT_PIN);
#elif defined(ARDUINO_ARDUINO_NANO33BLE)
APDS9960 APDS(Wire1, PIN_INT_APDS);
#else
APDS9960 APDS(Wire, -1);
#endif
#ifndef HEADER_Panel
#define HEADER_Panel
/*
htop - Panel.h
(C) 2004-2011 Hisham H. Muhammad
Released under the GNU GPL, see the COPYING file
in the source distribution for its full text.
*/

//#link curses
 

typedef struct Panel_ Panel;

typedef enum HandlerResult_ {
   HANDLED     = 0x01,
   IGNORED     = 0x02,
   BREAK_LOOP  = 0x04,
   REDRAW      = 0x08,
   RESCAN      = 0x10,
   SYNTH_KEY   = 0x20,
} HandlerResult;

#define EVENT_SET_SELECTED -1

#define EVENT_HEADER_CLICK(x_) (-10000 + x_)
#define EVENT_IS_HEADER_CLICK(ev_) (ev_ >= -10000 && ev_ <= -9000)
#define EVENT_HEADER_CLICK_GET_X(ev_) (ev_ + 10000)

typedef HandlerResult(*Panel_EventHandler)(Panel*, int);

typedef struct PanelClass_ {
   const ObjectClass super;
   const Panel_EventHandler eventHandler;
} PanelClass;

#define As_Panel(this_)                ((PanelClass*)((this_)->super.klass))
#define Panel_eventHandlerFn(this_)    As_Panel(this_)->eventHandler
#define Panel_eventHandler(this_, ev_) As_Panel(this_)->eventHandler((Panel*)(this_), ev_)

struct Panel_ {
   Object super;
   int x, y, w, h;
   WINDOW* window;
   Vector* items;
   int selected;
   int oldSelected;
   int selectedLen;
   void* eventHandlerState;
   int scrollV;
   short scrollH;
   bool needsRedraw;
   FunctionBar* currentBar;
   FunctionBar* defaultBar;
   RichString header;
   int selectionColor;
};

#define Panel_setDefaultBar(this_) do{ (this_)->currentBar = (this_)->defaultBar; }while(0)


#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif
#ifndef MAX
#define MAX(a,b) ((a)>(b)?(a):(b))
#endif

#define KEY_CTRL(l) ((l)-'A'+1)

extern PanelClass Panel_class;

Panel* Panel_new(int x, int y, int w, int h, bool owner, ObjectClass* type, FunctionBar* fuBar);

void Panel_delete(Object* cast);

void Panel_init(Panel* this, int x, int y, int w, int h, ObjectClass* type, bool owner, FunctionBar* fuBar);

void Panel_done(Panel* this);

void Panel_setSelectionColor(Panel* this, int color);

RichString* Panel_getHeader(Panel* this);

extern void Panel_setHeader(Panel* this, const char* header);

void Panel_move(Panel* this, int x, int y);

void Panel_resize(Panel* this, int w, int h);

void Panel_prune(Panel* this);

void Panel_add(Panel* this, Object* o);

void Panel_insert(Panel* this, int i, Object* o);

void Panel_set(Panel* this, int i, Object* o);

Object* Panel_get(Panel* this, int i);

Object* Panel_remove(Panel* this, int i);

Object* Panel_getSelected(Panel* this);

void Panel_moveSelectedUp(Panel* this);

void Panel_moveSelectedDown(Panel* this);

int Panel_getSelectedIndex(Panel* this);

int Panel_size(Panel* this);

void Panel_setSelected(Panel* this, int selected);

void Panel_draw(Panel* this, bool focus);

bool Panel_onKey(Panel* this, int key);

HandlerResult Panel_selectByTyping(Panel* this, int ch);

#endif

#define W 16
#define H 16
#define LED_PIN 6

Panel panel(LED_PIN, W, H, SERPENTINE, RIGHT, 0);

#define rgb(R,G,B) panel.color(R,G,B)
#define G rgb(255, 0, 0)
#define R rgb(0, 255, 0)
#define B rgb(0, 0, 255)
#define Z rgb(0, 0, 0)
void setup(){
  panel.setBrightness(30);
}
void loop(){
   panel.image(house, 0, 0);
  panel.show();
}
