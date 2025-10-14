#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7796S_kbv.h>
#include <TFT_Touch.h>
#include <math.h>

// === TFT LCD (Display) Connections ===
#define TFT_CS   A5
#define TFT_DC   A3
#define TFT_RST  A4
#define TFT_LED  A0

// === Touch Screen Connections ===
#define t_CLK 3
#define t_CS  2
#define t_DIN 5
#define t_DO  4

// === Touchscreen calibration constants ===
#define HMIN 0
#define HMAX 3840
#define VMIN 0
#define VMAX 3840
#define XYSWAP 1
#define HRES 480
#define VRES 320

// === Game constants ===
#define NUM_ARROWS 4
#define ARROW_SIZE 40
#define ARROW_SPACING 100
#define DOT_SIZE 4
#define SWIPE_WINDOW 2000
#define MIN_SWIPE 25
#define MAX_POINTS 20
#define INTERPOLATE_STEP 25
#define MIN_TOTAL_DISTANCE 40
#define DIRECTION_RATIO 0.6

// === TFT and Touch objects ===
Adafruit_ST7796S_kbv tft(TFT_CS, TFT_DC, TFT_RST);
TFT_Touch touch(t_CS, t_CLK, t_DIN, t_DO);

// === Arrow directions ===
enum Direction {UP, DOWN, LEFT, RIGHT};
Direction arrows[NUM_ARROWS];
bool swiped[NUM_ARROWS];
int arrowX[NUM_ARROWS];
int arrowY[NUM_ARROWS];

// === Swipe tracking ===
struct Point { int x, y; };
Point swipePoints[MAX_POINTS];
int pointCount = 0;
bool touching = false;
unsigned long swipeStartTime = 0;
unsigned long lastTouchTime = 0;

// === Game state ===
int currentArrow = 0;
int score = 0;
int roundNumber = 1;
bool gameComplete = false;

void setup() {
  Serial.begin(115200);
  tft.begin();
  tft.setRotation(3);
  tft.fillScreen(ST7796S_BLACK);
  pinMode(TFT_LED, OUTPUT);
  analogWrite(TFT_LED, 255);

  touch.setCal(HMIN, HMAX, VMIN, VMAX, HRES, VRES, XYSWAP);
  touch.setRotation(1);

  randomSeed(analogRead(A1));
  generateArrows();
  drawArrows();
  drawScore();
}

void loop() {
  unsigned long now = millis();

  // Check if round is complete
  if (gameComplete) {
    delay(2000);
    gameComplete = false;
    roundNumber++;
    generateArrows();
    drawArrows();
    drawScore();
    return;
  }

  if (touch.Pressed()) {
    int x = touch.X();
    int y = touch.Y();
    if (x == 0 && y == 0) return;

    if (!touching) {
      touching = true;
      swipeStartTime = now;
      pointCount = 0;
    }

    // Add point if it's different enough from last point
    bool addPoint = true;
    if (pointCount > 0) {
      int dx = x - swipePoints[pointCount-1].x;
      int dy = y - swipePoints[pointCount-1].y;
      int distSq = dx*dx + dy*dy;
      if (distSq < 16) addPoint = false; // Skip if < 4 pixels away
    }

    if (addPoint && pointCount < MAX_POINTS) {
      swipePoints[pointCount++] = {x, y};
      tft.fillCircle(x, y, DOT_SIZE, ST7796S_RED);
    }
    
    lastTouchTime = now;

  } else if (touching && (now - lastTouchTime > 100)) {
    // Swipe ended (100ms after last touch)
    touching = false;

    if (pointCount < 2) { 
      pointCount = 0; 
      return; 
    }

    // Draw thicker blue line with better interpolation
    for (int i = 1; i < pointCount; i++) {
      drawThickLine(swipePoints[i-1], swipePoints[i], ST7796S_BLUE, 3);
    }

    // Calculate total swipe path and average direction
    int totalDeltaX = 0;
    int totalDeltaY = 0;
    float totalDistance = 0;
    
    for (int i = 1; i < pointCount; i++) {
      int dx = swipePoints[i].x - swipePoints[i-1].x;
      int dy = swipePoints[i].y - swipePoints[i-1].y;
      totalDeltaX += dx;
      totalDeltaY += dy;
      totalDistance += sqrt(dx*dx + dy*dy);
    }

    // Also use start-to-end for overall direction
    int xStart = swipePoints[0].x;
    int yStart = swipePoints[0].y;
    int xEnd = swipePoints[pointCount-1].x;
    int yEnd = swipePoints[pointCount-1].y;
    
    int directDeltaX = xEnd - xStart;
    int directDeltaY = yEnd - yStart;

    // Combine both methods (weighted average)
    float avgDeltaX = (totalDeltaX * 0.6 + directDeltaX * 0.4);
    float avgDeltaY = (totalDeltaY * 0.6 + directDeltaY * 0.4);

    Serial.print("Points: "); Serial.print(pointCount);
    Serial.print(" | Total dist: "); Serial.print(totalDistance);
    Serial.print(" | avgDX: "); Serial.print(avgDeltaX);
    Serial.print(" | avgDY: "); Serial.println(avgDeltaY);

    // Check if swipe is long enough
    if (totalDistance < MIN_TOTAL_DISTANCE) {
      Serial.println("Swipe too short");
      delay(1000);
      drawArrows();
      drawScore();
      pointCount = 0;
      return;
    }

    // Determine dominant direction with relaxed ratio
    bool isVertical = abs(avgDeltaY) > abs(avgDeltaX) * DIRECTION_RATIO;
    bool isHorizontal = abs(avgDeltaX) > abs(avgDeltaY) * DIRECTION_RATIO;

    // Check if swipe matches current arrow
    bool correctSwipe = false;

    if (currentArrow < NUM_ARROWS) {
      Direction targetDir = arrows[currentArrow];
      
      // More lenient direction checking
      if (targetDir == UP) {
        if (avgDeltaY < -MIN_SWIPE && isVertical) {
          correctSwipe = true;
          Serial.println("UP detected");
        }
      } else if (targetDir == DOWN) {
        if (avgDeltaY > MIN_SWIPE && isVertical) {
          correctSwipe = true;
          Serial.println("DOWN detected");
        }
      } else if (targetDir == LEFT) {
        if (avgDeltaX < -MIN_SWIPE && isHorizontal) {
          correctSwipe = true;
          Serial.println("LEFT detected");
        }
      } else if (targetDir == RIGHT) {
        if (avgDeltaX > MIN_SWIPE && isHorizontal) {
          correctSwipe = true;
          Serial.println("RIGHT detected");
        }
      }

      if (correctSwipe) {
        drawArrow(arrowX[currentArrow], arrowY[currentArrow], arrows[currentArrow], ST7796S_GREEN);
        swiped[currentArrow] = true;
        score++;
        currentArrow++;
        
        // Check if all arrows completed
        if (currentArrow >= NUM_ARROWS) {
          gameComplete = true;
        }
      } else {
        drawArrow(arrowX[currentArrow], arrowY[currentArrow], arrows[currentArrow], ST7796S_RED);
        Serial.println("Wrong direction!");
      }
    }

    delay(1000);
    drawArrows();
    drawScore();
    pointCount = 0;
  }
}

// Draw score display
void drawScore() {
  tft.fillRect(0, 0, 480, 30, ST7796S_BLACK);
  tft.setTextColor(ST7796S_WHITE);
  tft.setTextSize(2);
  tft.setCursor(10, 8);
  tft.print("Score: ");
  tft.print(score);
  tft.setCursor(200, 8);
  tft.print("Round: ");
  tft.print(roundNumber);
}

// Draw thick line between two points
void drawThickLine(Point p1, Point p2, uint16_t color, int thickness) {
  int dx = p2.x - p1.x;
  int dy = p2.y - p1.y;
  float distance = sqrt(dx*dx + dy*dy);
  int steps = max(1, (int)(distance / INTERPOLATE_STEP));
  float xStep = dx / (float)steps;
  float yStep = dy / (float)steps;
  
  for (int i = 0; i <= steps; i++) {
    int xi = p1.x + xStep * i;
    int yi = p1.y + yStep * i;
    tft.fillCircle(xi, yi, thickness, color);
  }
}

// Generate random arrows
void generateArrows() {
  for (int i = 0; i < NUM_ARROWS; i++) {
    arrows[i] = (Direction)random(0, 4);
    swiped[i] = false;
    arrowX[i] = 60 + i * ARROW_SPACING;
    arrowY[i] = VRES / 2;
  }
  currentArrow = 0;
}

// Draw all arrows
void drawArrows() {
  tft.fillRect(0, 35, 480, 285, ST7796S_BLACK);
  
  for (int i = 0; i < NUM_ARROWS; i++) {
    uint16_t color;
    
    if (swiped[i]) {
      color = ST7796S_GREEN;
    } else if (i == currentArrow) {
      color = ST7796S_CYAN; // Highlight current arrow
    } else {
      color = 0x7BEF; // Dim gray for waiting arrows
    }
    
    drawArrow(arrowX[i], arrowY[i], arrows[i], color);
    
    // Draw arrow number below
    tft.setTextColor(ST7796S_WHITE);
    tft.setTextSize(1);
    tft.setCursor(arrowX[i] - 8, arrowY[i] + 50);
    tft.print(i + 1);
  }
}

// Draw single arrow
void drawArrow(int x, int y, Direction dir, uint16_t color) {
  int s = ARROW_SIZE;
  switch(dir) {
    case UP:
      tft.fillTriangle(x, y - s, x - s/2, y + s/2, x + s/2, y + s/2, color);
      break;
    case DOWN:
      tft.fillTriangle(x, y + s, x - s/2, y - s/2, x + s/2, y - s/2, color);
      break;
    case LEFT:
      tft.fillTriangle(x - s, y, x + s/2, y - s/2, x + s/2, y + s/2, color);
      break;
    case RIGHT:
      tft.fillTriangle(x + s, y, x - s/2, y - s/2, x - s/2, y + s/2, color);
      break;
  }
}