#include <FastLED.h>

#include <LiquidCrystal.h>
#include <LiquidCrystal_I2C.h>

#define LED_PIN 15
#define NUM_LEDS 256
#define BRIGHTNESS 64
#define LED_TYPE WS2812B
#define COLOR_ORDER GRB

#define ROTATE_BUTTON 32
#define START_BUTTON 12
#define HARDDROP_BUTTON 33

CRGB leds[NUM_LEDS];

#define JOYX_PIN 27
#define JOYX_MIN 0
#define JOYX_MAX 4095
#define JOYX_DEADZONE 250

#define DEBOUNCE_DELAY 100
#define DOUBLE_CLICK_TIME 500

//LiquidCrystal lcd(8, 9, 4, 5, 6, 7);
int lcdColumns = 16;
int lcdRows = 2;
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);  
int joyX = 0;
//int joyXSmooth = 0;
int pixelX = 0;
int pixelY = 0;
bool blocked[16][16];

const int tetrisPieces[7][4][4] = {
  // I piece
  {
    {0, 0, 0, 0},
    {1, 1, 1, 1},
    {0, 0, 0, 0},
    {0, 0, 0, 0}
  },
  // J piece
  {
    {1, 0, 0,0},
    {1, 1, 1,0},
    {0, 0, 0,0},
    {0, 0, 0,0}
  },
  // L piece
  {
    {0, 0, 1,0},
    {1, 1, 1,0},
    {0, 0, 0,0},
    {0, 0, 0,0}
  },
  // O piece
  {
    {1, 1,0,0},
    {1, 1,0,0},
    {0, 0, 0,0},
    {0, 0, 0,0}
  },
  // S piece
  {
    {0, 1, 1,0},
    {1, 1, 0,0},
    {0, 0, 0,0},
    {0, 0, 0,0}
  },
  // T piece
  {
    {0, 1, 0,0},
    {1, 1, 1,0},
    {0, 0, 0,0},
    {0, 0, 0,0}
  },
   // Z piece
   {
     {1,1 ,0,0},
     {0 ,1 ,1,0},
     {0 ,0 ,0,0},
     {0, 0, 0,0}
   }
};

// Function to generate a random Tetris piece
void generateRandomTetrisPiece(int piece[4][4]) {
   int randomIndex = random(7);
   memcpy(piece,tetrisPieces[randomIndex],sizeof(tetrisPieces[randomIndex]));
}

// Function to rotate a Tetris piece
volatile bool rotateButtonPressed = false;
volatile bool hardDropButtonPressed = false;

volatile bool startButtonPressed = false;
volatile unsigned long lastButtonPressTime = 0;
volatile bool doubleClicked = false;

unsigned long lastRotateButtonPressTime = 0;
unsigned long lastHardDropButtonPressTime = 0;
unsigned long lastStartButtonPressTime = 0;

int ROTATEDEBOUNCE_DELAY = 200;
void onRotateButtonPress() {
  if (millis() - lastRotateButtonPressTime > ROTATEDEBOUNCE_DELAY) {
    rotateButtonPressed = true;
    lastRotateButtonPressTime = millis();
  }
}
int DROPDEBOUNCE_DELAY = 500;
void onHardDropButtonPress(){
  if (millis() - lastHardDropButtonPressTime > DROPDEBOUNCE_DELAY) {
    hardDropButtonPressed = true;
    lastHardDropButtonPressTime = millis();
  }
}

void onStartButtonPress() {
  unsigned long currentTime = millis();
  if (currentTime - lastButtonPressTime < DEBOUNCE_DELAY) {
    return;
  }
  if (currentTime - lastButtonPressTime < DOUBLE_CLICK_TIME) {
    doubleClicked = true;
  } else {
    startButtonPressed = true;
  }
  lastButtonPressTime = currentTime;
}
void setup() {
    delay(3000);
    Serial.begin(9600);
    FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
    FastLED.setBrightness(BRIGHTNESS);
    pinMode(JOYX_PIN, INPUT);
    for(int i = 0; i < 16; i++){
      for(int j = 0; j < 16; j++){
          blocked[i][j] = false;        
      }
    }
    pinMode(ROTATE_BUTTON, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(ROTATE_BUTTON), onRotateButtonPress, FALLING);  
    pinMode(HARDDROP_BUTTON, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(HARDDROP_BUTTON), onHardDropButtonPress, FALLING);  
    pinMode(START_BUTTON, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(START_BUTTON), onStartButtonPress, FALLING);
    lcd.init();
    lcd.backlight();
}
int matrix[16][16] = {0};
int score = 0;
void loop() {
  while (!startButtonPressed) {
    delay(10);
  }
  startButtonPressed = false;
  // Generate a new random Tetris piece
  while(true){
    if (doubleClicked) {
      doubleClicked = false;
      break;
    }
    int currentTetrisPiece[4][4];
    generateRandomTetrisPiece(currentTetrisPiece);

    // Calculate the initial position of the Tetris piece
    int pieceWidth = sizeof(currentTetrisPiece[0]) / sizeof(currentTetrisPiece[0][0]);
    int pieceHeight = sizeof(currentTetrisPiece) / sizeof(currentTetrisPiece[0]);
    int currentX = (16 - pieceWidth) / 2;
    int currentY = 0;
    CRGB pieceColors[7] = {CRGB::Red, CRGB::Green, CRGB::Blue, CRGB::Yellow, CRGB::Magenta, CRGB::Cyan, CRGB::White};
    int max_x = 0;
    /*for (int y = 0; y < pieceHeight; y++) {
      for (int x = 0; x < pieceWidth; x++) {
        if (currentTetrisPiece[y][x] == 1 && x > max_x) {
          max_x = x;
        }
      }
    }
    if (currentX + max_x >= 16) {
      currentX = 16 - max_x - 1;
    }*/
    while(true){
    // Drop the Tetris piece from the top of the screen
      // Read the joystick value
      joyX = analogRead(JOYX_PIN);
      currentX += clamp(joyX);
      if (currentX < 0) currentX = 0; //edge
      if (currentX > 16 - pieceWidth +1) currentX = 16 - pieceWidth+1;
    
      int pieceIndex = 0;
      for (int i = 0; i < 7; i++) {
        if (memcmp(currentTetrisPiece, tetrisPieces[i], sizeof(currentTetrisPiece)) == 0) {
          pieceIndex = i;
          break;
        }
        // Check for rotated pieces
        int rotatedPiece[4][4];
        for (int y = 0; y < pieceHeight; y++) {
          for (int x = 0; x < pieceWidth; x++) {
            rotatedPiece[x][pieceHeight - y - 1] = tetrisPieces[i][y][x];
          }
        }
        if (memcmp(currentTetrisPiece, rotatedPiece, sizeof(rotatedPiece)) == 0) {
          pieceIndex = i;
          break;
        }
      }
      CRGB pieceColor = pieceColors[pieceIndex];  
      if (rotateButtonPressed) {
      // Rotate the current Tetris piece
        int rotatedPiece[4][4];
        for (int y = 0; y < pieceHeight; y++) {
          for (int x = 0; x < pieceWidth; x++) {
            rotatedPiece[x][pieceHeight - y - 1] = currentTetrisPiece[y][x];
          }
        }
        // Copy the rotated piece back to the currentTetrisPiece array
        if (!checkCollision(rotatedPiece, currentX, currentY)) {
          memcpy(currentTetrisPiece, rotatedPiece, sizeof(rotatedPiece));
        }
        rotateButtonPressed = false;
      }
      if (hardDropButtonPressed) {
      // Move the piece down until it collides with the bottom or another piece
        while (true) {
          bool collision = false;
          for (int y = 0; y < pieceHeight; y++) {
            for (int x = 0; x < pieceWidth; x++) {
              if (currentTetrisPiece[y][x] == 1) {
                if (currentY + y + 1 >= 16 || matrix[currentY + y + 1][currentX + x]!=0) {
                  collision = true;
                  break;
                }
              }
            }
            if (collision) break;
          }

          if (collision) {
            // Stop the current Tetris piece and update the matrix
            for (int y = 0; y < pieceHeight; y++) {
              for (int x = 0; x < pieceWidth; x++) {
                if (currentTetrisPiece[y][x] == 1) {
                  matrix[currentY + y][currentX + x] = pieceIndex + 1;
                }
              }
            }
            hardDropButtonPressed = false;
            break;
          }
          currentY++;
      }
      hardDropButtonPressed = false;
      break;
    }
    FastLED.clear();
    for (int y = 0; y < 16; y++) {
      for (int x = 0; x < 16; x++) {
        if (matrix[y][x]!=0) {
          int index = getIndex(x,y);
          leds[index] = pieceColors[matrix[y][x] - 1];
        }
      }
    }

      // Draw the current Tetris piece at its current position on the LED matrix
      for (int y = 0; y < pieceHeight; y++) {
        for (int x = 0; x < pieceWidth; x++) {
          if (currentTetrisPiece[y][x] == 1) {
            int index = getIndex(currentX+x,currentY+y);
            leds[index] = pieceColors[pieceIndex];
          }
        }
      }
      FastLED.show();

      delay(300);
      currentY++;
      // Move the piece down by one row

      // TODO: Add code to check for collision and stop the piece if necessary
      bool collision = false;
      for (int y = 0; y < pieceHeight; y++) {
        for (int x = 0; x < pieceWidth; x++) {
          if (currentTetrisPiece[y][x] == 1) {
            if (currentY + y + 1 >= 16 || matrix[currentY + y + 1][currentX + x]!=0) {
              collision = true;
              break;
            }
          }
        }
        if (collision) break;
      }


    if (collision) {
      // Stop the current Tetris piece and update the matrix
      for (int y = 0; y < pieceHeight; y++) {
        for (int x = 0; x < pieceWidth; x++) {
          if (currentTetrisPiece[y][x] ==1){
            matrix[currentY +y][currentX +x]=pieceIndex + 1;
          }
        }
      }

      // Check for filled rows and clear them
      for(int y=0;y<16;y++){
        bool rowFilled=true;
        for(int x=0;x<16;x++){
          if(matrix[y][x] == 0){
            rowFilled=false;
            break;
          }
        }
        if(rowFilled){
          // Clear row
          for(int x=0;x<16;x++){
            matrix[y][x]= 0;
          }
          // Move all rows above down by one
          for(int y2=y-1;y2>=0;y2--){
            for(int x=0;x<16;x++){
              matrix[y2+1][x]=matrix[y2][x];
            }
          }
          // Clear top row
          for(int x=0;x<16;x++){
            matrix[0][x]= 0;
          }
          score+=100;
        }        
      }
      lcd.setCursor(0, 0);
      lcd.print("Score: ");
      lcd.print(score);

      break;
    }
  }
  bool gameOver=false;
  for(int x=0;x<16;x++){
    if(matrix[1][x]!=0){
      gameOver=true;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("GAME OVER :(");
      break;
    }
  }

  if (gameOver) {
    score = 0;
    FastLED.clear();
    FastLED.show();

    // Clear the matrix
    for (int y = 0; y < 16; y++) {
      for (int x = 0; x < 16; x++) {
        matrix[y][x] = false;
      }
    }

    // Wait for button press to restart game
    while (!startButtonPressed && !doubleClicked) {
      delay(10);
    }

    startButtonPressed = false;
    doubleClicked = false;

    break;
  }
  }



}

bool checkCollision(int currentTetrisPiece[4][4], int currentX, int currentY) {
  int pieceWidth = sizeof(currentTetrisPiece[0]) / sizeof(currentTetrisPiece[0][0]);
  int pieceHeight = sizeof(currentTetrisPiece) / sizeof(currentTetrisPiece[0]);

  // Check for collision with sides
  if (currentX < 0 || currentX + pieceWidth > 16) {
    
    return true;
  }

  // Check for collision with bottom or another piece
  for (int y = 0; y < pieceHeight; y++) {
    for (int x = 0; x < pieceWidth; x++) {
      if (currentTetrisPiece[y][x] == 1) {
        if (currentY + y >= 16 || matrix[currentY + y][currentX + x]!=0) {
        return true;
        }
      }
    }
  }
  return false;
}

int getIndex(int row, int col) {
    if (row % 2 == 0) {
        return row * 16 + col;
    } else {
        return row * 16 + (15 - col);
    }
}
int clamp(int readX){
  if(readX >= 2047.0 - JOYX_DEADZONE && readX <= 2047.0 + JOYX_DEADZONE){
    return 0;
  }
  if(readX / 4095.0 > .5){
    return 1;
  }
  if(readX / 4095.0 < .5){
    return -1;
  }
}
