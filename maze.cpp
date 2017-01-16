/*  Names: Duncan Follis, Kevin de Haan
    Section: EA1
    Includes code and libraries from Ualberta CS department
    Verbal descriptions of common algorithms: http://www.astrolog.org/labyrnth/algrithm.htm

    I would strongly reccommend collapsing all functions before viewing.
    
    The reason things are not in separate .cpps with headers is that our maze
    is a global variable and made coordinating .h files very difficult. If we
    were to redo the project, this would be one of the things to reexamine.
*/

#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library
#include <string.h> // for memcpy
#include <stdlib.h> // for RNG

// standard U of A library settings, assuming Atmel Mega SPI pins
#define SD_CS    5  // Chip select line for SD card
#define TFT_CS   6  // Chip select line for TFT display
#define TFT_DC   7  // Data/command line for TFT
#define TFT_RST  8  // Reset line for TFT (or connect to +5V)

#define JOY_SEL 9 // Joystick select pin
#define JOY_VERT_ANALOG 0 // Vertical in pin
#define JOY_HORIZ_ANALOG 1 // Horizontal in pin

#define JOY_DEADZONE 128 // Movement threshold
#define JOY_CENTRE 512 // Analog joystick midpoint

#define TFT_WIDTH 128 // Screen dimensions
#define TFT_HEIGHT 160

#define MENU_SPACING 12
#define MAZE_DELAY 8 // 8 is good

#define MENU_LENGTH 5 // number of entries on the main screen

#define END_J 20 // describes where the exit/end of the maze is
#define END_I 20

/* USEFUL FOR DEBUGGING
for(int j = 1; j < 21; j++){
  for(int i = 1; i < 21; i++){
    Serial.print(g_maze[j][i].tile); Serial.print(' ');
  }
  Serial.println();
} */


Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);


typedef struct{
  int16_t tile;
  int8_t rightWall;
  int8_t downWall;
  int8_t col;
  int8_t row;
} mazeStruct;

// main struct describing the maze
mazeStruct g_maze[21][21];

////////////////////////////////////////////// Main Menu utilities

// holds information about menu names
typedef struct {
  char mazeType[11]; // null terminated
  char solverType[11];
} menuStruct;

// one of two global arrays, holds names
menuStruct menuInfo[MENU_LENGTH];

// used in the menu
typedef struct {
  int8_t horiz;
  int8_t vert;
  int8_t change;
} moveStruct;

// used in the stayright solver
typedef struct {
  int8_t j;
  int8_t i;
  int16_t step;
  int8_t prevDir;
} mazeLoc;

// used by the main menu to pass information
typedef struct {
  int8_t mazeChoice;
  int8_t solverChoice;
} choiceStruct;

// Fills the array that describes the main menu
void fillMenuInfo(){

  char name1[] = "Prim      ";
  char name2[] = "Backtrack ";
  char name3[] = "Shuffle   ";
  char name4[] = "No Squares";
  char name5[] = "Previous  ";
  memcpy(menuInfo[0].mazeType, name1, 11);
  memcpy(menuInfo[1].mazeType, name2, 11);
  memcpy(menuInfo[2].mazeType, name3, 11);
  memcpy(menuInfo[3].mazeType, name4, 11);
  memcpy(menuInfo[4].mazeType, name5, 11);

  char name7[] = "Stay Right";
  char name8[] = "Deadend   ";
  char name9[] = "Recursive ";
  char name10[] = "Breadth   ";
  char name11[] = "Depth     ";
  memcpy(menuInfo[0].solverType, name7, 11);
  memcpy(menuInfo[1].solverType, name8, 11);
  memcpy(menuInfo[2].solverType, name9, 11);
  memcpy(menuInfo[3].solverType, name10, 11);
  memcpy(menuInfo[4].solverType, name11, 11);

}

// Basic setup for main menu
void setupScreen(){
  tft.fillScreen(0);

  tft.setTextWrap(false);
  tft.setTextColor(0xFFFF, 0x0000); // white characters on black background

  tft.setCursor(0, 0); // where the characters will be displayed
  tft.print("Maze:");
  tft.setCursor(64, 0); // where the characters will be displayed
  tft.print("Solver:");


  for(int i = 0; i < MENU_LENGTH; i++){
    tft.setCursor(0, 14 + (i*MENU_SPACING));
    tft.print(menuInfo[i].mazeType);
    tft.print("\n");
  }

  for(int i = 0; i < MENU_LENGTH; i++){
    tft.setCursor(64, 14 + (i*MENU_SPACING));
    tft.print(menuInfo[i].solverType);
    tft.print("\n");
  }

}

// Gets user input for the menu
moveStruct menuRead(){
  int16_t vert = analogRead(JOY_VERT_ANALOG);
  int16_t horiz = analogRead(JOY_HORIZ_ANALOG);
  moveStruct menuMovement;
  menuMovement.change = 0;
  menuMovement.vert = 0;
  menuMovement.horiz = 0;

  if (abs(vert - JOY_CENTRE) > JOY_DEADZONE) {
    menuMovement.change = 1;
    if((vert - JOY_CENTRE) > 0){
      menuMovement.vert = 1;
    } else {
      menuMovement.vert = -1;
    }
  }
  if (abs(horiz - JOY_CENTRE) > JOY_DEADZONE) {
    menuMovement.change = 1;
    if((horiz - JOY_CENTRE) > 0){
      menuMovement.horiz = 1;
    } else {
      menuMovement.horiz = -1;
    }
  }
  return menuMovement;
}

// Changes the highlighted menu entry
void switchMenu(int8_t m_new, int8_t m_prev, int8_t n_new, int8_t n_prev){

  if(n_prev == 0){
    tft.setCursor(0, 14+(MENU_SPACING*m_prev));
    tft.setTextColor(0xFFFF, 0x0000); // white characters on black background
    tft.print(menuInfo[m_prev].mazeType);
  } else {
    tft.setCursor(64, 14+(MENU_SPACING*m_prev));
    tft.setTextColor(0xFFFF, 0x0000); // white characters on black background
    tft.print(menuInfo[m_prev].solverType);
  }
  if(n_new == 0){
    //tft.fillRect(0, 14+(MENU_SPACING*m_new), 60, 8, 0x0000);
    tft.setCursor(0, 14+(MENU_SPACING*m_new));
    tft.setTextColor(0x0000, 0xFfFF); // black characters on white background
    tft.print(menuInfo[m_new].mazeType);
  } else {
    //tft.fillRect(64, 14+(MENU_SPACING*m_new), 60, 8, 0x0000);
    tft.setCursor(64, 14+(MENU_SPACING*m_new));
    tft.setTextColor(0x0000, 0xFfFF); // black characters on white background
    tft.print(menuInfo[m_new].solverType);
  }

}

// Changes the selected menu entry
void underScore(int8_t column, int8_t row){
  for(int8_t i = 0; i<MENU_LENGTH; i++){
    tft.fillRect((0 + 64*column), (22 + MENU_SPACING*i), 60, 2, 0x0000);
  }
  tft.fillRect((0 + 64*column), (22 + MENU_SPACING*row), 60, 2, 0x3CC3);
}

// Main function for the primary menu
choiceStruct startScreen(){
  setupScreen();
  moveStruct menuMovement;
  choiceStruct menuChoice;
  bool stopLoop = false;
  int8_t m_select, n_select, m_prev, n_prev = 0;
  menuChoice.mazeChoice = -1;
  menuChoice.solverChoice = -1;
  int8_t solverChoice = -1;
  switchMenu(m_select, m_prev, n_select, n_prev);

  while(!stopLoop){
    menuMovement = menuRead();
    if (menuMovement.change == 1){
      m_prev = m_select;
      n_prev = n_select;
      m_select += menuMovement.vert;
      n_select += menuMovement.horiz;
      m_select = constrain(m_select, 0, MENU_LENGTH-1);
      n_select = constrain(n_select, 0, 1);
      switchMenu(m_select, m_prev, n_select, n_prev);
      delay(100);
    }
    if(digitalRead(JOY_SEL) == LOW){
      if(n_select == 0){
        menuChoice.mazeChoice = m_select;
        underScore(0, menuChoice.mazeChoice);
      } else {
        menuChoice.solverChoice = m_select;
        underScore(1, menuChoice.solverChoice);
      }
    }
    if((menuChoice.mazeChoice != -1) && (menuChoice.solverChoice != -1)){
      stopLoop = true;
    }
  }
  delay(500);
  return menuChoice;
}

///////////////////////////////////////////////////////////// mazes and solvers


//////// General Tools

// Quickly draws outline of the maze
void drawOutline(){
  tft.drawRect(4, 4, 121, 121, 0xFFFF);
}

// Returns 1 or 0
bool randomBool(){
  return (rand() % 2 == 1);
}

// Draws edges of a tile
void drawEdges(int8_t j, int8_t i){
  if(g_maze[j][i].rightWall == 1){
    tft.fillRect(4+(i*6), -2+(j*6), 1, 7, 0xFFFF);
  } else {
    tft.fillRect(4+(i*6), -1+(j*6), 1, 5, 0x0000);
  }

  if(g_maze[j][i].downWall == 1){
    tft.fillRect(-2+(i*6), 4+(j*6), 7, 1, 0xFFFF);
  } else {
    tft.fillRect(-1+(i*6), 4+(j*6), 5, 1, 0x0000);
  }

  if(g_maze[j][i-1].rightWall == 1){
    tft.fillRect(-2+(i*6), -2+(j*6), 1, 7, 0xFFFF);
  } else {
    tft.fillRect(-2+(i*6), -1+(j*6), 1, 5, 0x0000);
  }

  if(g_maze[j-1][i].downWall == 1){
    tft.fillRect(-2+(i*6), -2+(j*6), 7, 1, 0xFFFF);
  } else {
    tft.fillRect(-1+(i*6), -2+(j*6), 5, 1, 0x0000);
  }



  delay(MAZE_DELAY);
}

// Quickly draws a grid
void gridFill(){
  for(int8_t i = 0; i < 19; i++){
    tft.fillRect(10+(i*6), 4, 1, 120, 0xFFFF);
  }
  for(int8_t i = 0; i < 19; i++){
    tft.fillRect(4, 10+(i*6), 120, 1, 0xFFFF);
  }
}

// Looks to see if a tile has three walls around it
bool checkDeadEnd(int8_t j, int8_t i){
  int8_t numWalls = 0;
  if(g_maze[j][i].rightWall > 0){
    numWalls++;
  }
  if(g_maze[j-1][i].downWall > 0){
    numWalls++;
  }
  if(g_maze[j][i-1].rightWall > 0){
    numWalls++;
  }
  if(g_maze[j][i].downWall > 0){
    numWalls++;
  }
  if(numWalls >= 3){
    return true;
  } else {
    return false;
  }
}

// Deletes temporary data created for some functions
void clearTempWalls(){
  for(int8_t j = 1; j < 21; j++){
    for(int8_t i = 1; i < 21; i++){
      if(g_maze[j][i].rightWall == 2){
        g_maze[j][i].rightWall = 0;
      }
      if(g_maze[j][i].downWall == 2){
        g_maze[j][i].downWall = 0;
      }
    }
  }
}

// Sets all tiles of the maze to 0, meaning empty
void emptyMazeTiles(){
  for(int8_t i = 0; i<21; i++){
    for(int8_t j = 0; j<21; j++){
      g_maze[i][j].tile = 0;
    }
  }
}

// Marks all tiles of a given value (used by depth and breadth)
void markPath(int16_t tileNum){
  for(int8_t j = 1; j < 21; j++){
    for(int8_t i = 1; i < 21; i++){
      if(g_maze[j][i].tile == tileNum){
        tft.fillRect((6*i), (6*j), 3, 3, 0xFFFF);
      }
    }
  }
}

// Displays steps of solving algorithms
void printMazeLoc(mazeLoc currentLoc){
  int16_t tileR = constrain(abs(currentLoc.step-484)-129, 0, 255);
  int16_t tileG = constrain(-abs(currentLoc.step-612)+255, 0, 255);
  int16_t tileB = constrain(-abs(currentLoc.step-356)+255, 0, 255);
  uint16_t tileColour = tft.Color565(tileR, tileG, tileB);
  tft.fillRect(-1+(6*currentLoc.i), -1+(6*currentLoc.j), 5, 5, tileColour);
  delay(MAZE_DELAY*4);


}

// Checks if current position is the end of the maze
bool checkFinish(mazeLoc currentLoc){
  if((currentLoc.i == END_I) && (currentLoc.j == END_J)){
    return true;
  } else {
    return false;
  }
}

// Builds borders
void fullWallsSetup(){

    for(int8_t j = 1; j < 21; j++){
      g_maze[j][0].rightWall = 1;
    }
    for(int8_t i = 1; i < 21; i++){
      g_maze[0][i].downWall = 1;
    }

}

// Used by most solvers to diplay where they have checked
void fillTile(int8_t j, int8_t i, int16_t step){
  int16_t tileR = constrain(abs(step-484)-129, 0, 255);
  int16_t tileG = constrain(-abs(step-612)+255, 0, 255);
  int16_t tileB = constrain(-abs(step-356)+255, 0, 255);
  uint16_t tileColour = tft.Color565(tileR, tileG, tileB);
  tft.fillRect(-1+(6*i), -1+(6*j), 5, 5, tileColour);
  delay(MAZE_DELAY*4);
}

// Sees if there is an empty tile with a path to it adjacent
bool emptyCheck(int8_t j, int8_t i){
  if((g_maze[j][i].rightWall == 0) && (g_maze[j][i+1].tile == 0)){
    return true;
  }
  if((g_maze[j][i-1].rightWall == 0) && (g_maze[j][i-1].tile == 0)){
    return true;
  }
  if((g_maze[j][i].downWall == 0) && (g_maze[j+1][i].tile == 0)){
    return true;
  }
  if((g_maze[j-1][i].downWall == 0) && (g_maze[j-1][i].tile == 0)){
    return true;
  }
  return false;
}

// Displays steps taken, allows returning to the main menu. This runs after every solver
void mazeMenu(int16_t steps){
  tft.setTextColor(0x0000, 0xFFFF); // black characters on white background
  tft.setCursor(32, 144);
  tft.print("Re-select");
  tft.setTextColor(0xFFFF, 0x0000); // white characters on black background
  tft.setCursor(8, 130);
  tft.print("Steps Taken: "); tft.print(steps);
  while(true){
    if(!digitalRead(JOY_SEL)){
      break;
    }
  }
}

///////////////////////////////////////////////// Generators

//////// Prim Generator


// Fills walls and sets each tile to have a different value
void buildPrimMaze(){
  for(int8_t j = 0; j < 21; j++){
    for(int8_t i = 0; i< 21; i++){
      g_maze[j][i].rightWall = 1;
      g_maze[j][i].downWall = 1;
      g_maze[j][i].tile = -1;
    }
  }
  for(int8_t j = 1; j < 21; j++){
    for(int8_t i = 1; i< 21; i++){
      g_maze[j][i].tile = (20*(j-1) + i-1);
    }
  }
}

// Returns 1 if same tile value
int8_t primCheckDown(int8_t j, int8_t i){
  if(g_maze[j][i].tile == g_maze[j+1][i].tile){
    return 1;
  } else {
    return 0;
  }
}

// Returns 1 if same tile value
int8_t primCheckRight(int8_t j, int8_t i){
  if(g_maze[j][i].tile == g_maze[j][i+1].tile){
    return 1;
  } else {
    return 0;
  }
}

// Checks to see if any more walls can be filled
int8_t primCheckContinue(int16_t seed){
  for(int8_t j = 1; j < 21; j++){
    for(int8_t i = 1; i < 21; i++){
      if(g_maze[j][i].tile != seed){
        return 1;
      }
    }
  }
  return 0;
}

// Main prim function
void primMaze(){
  srand(millis());
  drawOutline();
  gridFill();
  buildPrimMaze();
  int16_t seed = (rand() % 399);
  Serial.print("seed: "); Serial.println(seed);
  while(primCheckContinue(seed)){
    for(int8_t j = 1; j < 21; j++){
      for(int8_t i = 1; i < 21; i++){
        if(g_maze[j][i].tile == seed){
          // ensuring !(0 | 1)
          int8_t up = -1;
          int8_t right = -1;
          int8_t down = -1;
          int8_t left = -1;

          if(j > 1){
            if(g_maze[j][i].tile == seed){
              up = primCheckDown(j-1,i);
            }
          }
          if(j < 20){
            if(g_maze[j][i].tile == seed){
              down = primCheckDown(j,i);
            }
          }
          if(i > 1){
            if(g_maze[j][i].tile == seed){
              left = primCheckRight(j, i-1);
            }
          }
          if(i < 20){
            if(g_maze[j][i].tile == seed){
              right = primCheckRight(j, i);
            }
          }

          if((up == 0) && randomBool()){
            g_maze[j-1][i].downWall = 0;
            g_maze[j-1][i].tile = seed;
            drawEdges(j-1, i);
          }
          if((right == 0) && randomBool()){
            g_maze[j][i].rightWall = 0;
            g_maze[j][i+1].tile = seed;
            drawEdges(j, i);
          }
          if((down == 0) && randomBool()){
            g_maze[j][i].downWall = 0;
            g_maze[j+1][i].tile = seed;
            drawEdges(j, i);
          }
          if((left == 0) && randomBool()){
            g_maze[j][i-1].rightWall = 0;
            g_maze[j][i-1].tile = seed;
            drawEdges(j, i-1);
          }
        }
      }
    }
  }
}


//////// Eller Generator (NOT ACTUALLY ELLER'S ALGORITHM, called shuffle in menu)

// Initial setup for the maze
void buildEllersMaze(){
  for(int8_t j = 0; j < 21; j++){
    for(int8_t i = 0; i< 21; i++){
      g_maze[j][i].rightWall = 1;
      g_maze[j][i].downWall = 1;
      g_maze[j][i].tile = -1;
    }
  }
  for(int8_t j = 1; j < 21; j++){
    for(int8_t i = 1; i< 21; i++){
      g_maze[j][i].tile = (20*(j-1) + i-1);
    }
  }
}

// Randomly decides to join horizonal segments
void Ellerscombside(int8_t j) {
  for(int i = 1; i < 20; i++){
    if (g_maze[j][i].tile != g_maze[j][i+1].tile){
      if (randomBool()) {
        g_maze[j][i+1].tile = g_maze[j][i].tile;
        g_maze[j][i].rightWall = 0;
        drawEdges(j, i);
      }
      else {

      }
    }
  }

}

// Forces connections randomly down
void EllerForceDown(int8_t j){
  for(int i = 1; i < 20; i++){
    if(g_maze[j][i].tile != g_maze[j][i+1].tile){
      g_maze[j][i].downWall = 0;
      drawEdges(j, i);
    } else if (randomBool() && randomBool()){
      g_maze[j][i].downWall = 0;
      drawEdges(j, i);
    }
  }
  if(g_maze[j][20].tile != g_maze[j][19].tile){
    g_maze[j][20].downWall = 0;
    drawEdges(j, 20);

  }
}

// Forces connections up, required to absolutely guarantee completeness
void EllerForceUp(int8_t j){
  for(int i = 20; i > 1; i--){
    if(g_maze[j][i].tile != g_maze[j][i-1].tile){
      g_maze[j-1][i].downWall = 0;
      drawEdges(j-1, i);
    }
  }
}

// Primary maze function
void EllersMaze(){
  srand(millis());
  drawOutline();
  gridFill();
  buildEllersMaze();
  for(int8_t j = 1; j < 21; j++){
    Ellerscombside(j);
  }
  for(int8_t j = 1; j < 20; j++){
    EllerForceDown(j);
  }
  for(int8_t j = 20; j > 1; j--){
    EllerForceUp(j);
  }



}

//////// Duplicate Generator

// Retains the same maze as last created;
// Doesn't overwrite maze in memory and redraws it on the screen
void sameMaze(){
  drawOutline();
  for(int8_t j = 1; j < 21; j++){
    for(int8_t i = 1; i < 21; i++){
      if(g_maze[j][i].rightWall == 1){
        tft.fillRect(4+(i*6), -2+(j*6), 1, 7, 0xFFFF);
      }
      if(g_maze[j][i].downWall == 1){
        tft.fillRect(-2+(i*6), 4+(j*6), 7, 1, 0xFFFF);
      }
    }

  }

}


//////// NoSquare Generator

// Sets up initial conditions for NoSquare maze
void buildNosquareMaze(){
  for(int8_t j = 0; j < 21; j++){
    for(int8_t i = 0; i< 21; i++){
      g_maze[j][i].rightWall = 0;
      g_maze[j][i].downWall = 0;
      g_maze[j][i].tile = 0;
    }
  }
  for(int8_t j = 1; j < 21; j++){
    g_maze[j][0].rightWall = 1;
    g_maze[j][20].rightWall = 1;
  }
  for(int8_t i = 1; i< 21; i++){
    g_maze[0][i].downWall = 1;
    g_maze[20][i].downWall = 1;
  }

}

// Similar to primcheck, but for NoSquare
bool nsCheckRight(int8_t j, int8_t i){
  if(!checkDeadEnd(j, i) && !checkDeadEnd(j, i+1)){
    return 1;
  } else {
    return 0;
  }
}

// See above
bool nsCheckDown(int8_t j, int8_t i){
  if(!checkDeadEnd(j, i) && !checkDeadEnd(j+1, i)){
    return 1;
  } else {
    return 0;
  }
}

// NoSquare building function
void fillNosquareMaze(){
  for(int8_t j = 1; j < 21; j++){
    for(int8_t i = 1; i < 21; i++){
      if(randomBool() && nsCheckRight(j, i)){
        g_maze[j][i].rightWall = 1;
        if(checkDeadEnd(j, i) || checkDeadEnd(j, i+1)){
          g_maze[j][i].rightWall = 2;
        }
      }
      if(randomBool() && nsCheckDown(j, i)){
        g_maze[j][i].downWall = 1;
        if(checkDeadEnd(j, i) || checkDeadEnd(j+1, i)){
          g_maze[j][i].downWall = 2;
        }
      }
      drawEdges(j, i);
    }
  }
}

// Main NoSquare function
void nosquareMaze(){
  drawOutline();
  buildNosquareMaze();
  for(int8_t i = 0; i < 2; i++){
    fillNosquareMaze();
  }
  clearTempWalls();
}


//////// BackTrack Generator

// Used to check if squares in the cardinal directions are valid for expansion
bool backRightcheck(int8_t j, int8_t i){
  if(g_maze[j][i+1].tile == 1){
    return true;
  }
  return false;
}

bool backLeftcheck(int8_t j, int8_t i){
  if(g_maze[j][i-1].tile == 1){
    return true;
  }
  return false;
}

bool backDowncheck(int8_t j, int8_t i){
  if(g_maze[j+1][i].tile == 1){
    return true;
  }
  return false;
}

bool backUpcheck(int8_t j, int8_t i){
  if(g_maze[j-1][i].tile == 1){
    return true;
  }
  return false;
}

// Initial maze condition setup
void buildBackMaze(){
  for(int8_t i = 0; i<21; i++){
    for(int8_t j = 0; j<21; j++){
      if(!(j == 0 || i == 0)){
        g_maze[i][j].tile = 1;
      }
      g_maze[j][i].rightWall = 1;
      g_maze[j][i].downWall = 1;
      g_maze[j][i].col = j;
      g_maze[j][i].row = i;
    }
  }
}

// Where the maze is actually created; uses a stack to emulate recursion
void backFastRecurse(){
  mazeStruct* stack[400];
  stack[0] = &g_maze[1][1];
  int16_t stackSize = 1;
  while(stackSize > 0){
    int16_t count = 0;
    mazeStruct *top = stack[stackSize-1];
    int8_t i = (*top).row;
    int8_t j = (*top).col;
    g_maze[j][i].tile = 0;
    if(backLeftcheck(j, i)){
      if(randomBool()){
        g_maze[j][i-1].rightWall = 0;
        drawEdges(j, i);
        stack[stackSize] = &g_maze[j][i-1];
        stackSize++;
        count++;
        continue;
      }
    } else {
      count++;
    }
    if(backUpcheck(j, i)){
      if(randomBool()){
        g_maze[j-1][i].downWall = 0;
        drawEdges(j-1, i);
        stack[stackSize] = &g_maze[j-1][i];
        stackSize++;
        count++;
        continue;
      }
    } else {
      count++;
    }
    if(backRightcheck(j, i)){
      if(randomBool()){
        g_maze[j][i].rightWall = 0;
        drawEdges(j, i);
        stack[stackSize] = &g_maze[j][i+1];
        stackSize++;
        count++;
        continue;
      }
    } else {
      count++;
    }
    if(backDowncheck(j, i)){
      if(randomBool()){
        g_maze[j][i].downWall = 0;
        drawEdges(j, i);
        stack[stackSize] = &g_maze[j+1][i];
        stackSize++;
        count++;
        continue;
      }
    } else {
      count++;
    }



    if(count == 4){
      stackSize--;
    }
  }
}

// Main function
void backtrackMaze(){
  srand(millis());
  drawOutline();
  gridFill();
  buildBackMaze();
  backFastRecurse();

}

////////////////////////////////////////////////// Solvers

//////// Stayright Solver

// Primary solver function
mazeLoc stayRightCheck(mazeLoc currentLoc){
  int8_t rightGuess = (currentLoc.prevDir+3) % 4;
  while(true){
    if((rightGuess % 4) == 0){
      if(g_maze[currentLoc.j-1][currentLoc.i].downWall == 0){
        currentLoc.prevDir = 2;
        currentLoc.j -= 1;
        currentLoc.step += 1;
        printMazeLoc(currentLoc);
        return currentLoc;
      }
    }
    if((rightGuess % 4) == 1){
      if(g_maze[currentLoc.j][currentLoc.i].rightWall == 0){
        currentLoc.prevDir = 3;
        currentLoc.i += 1;
        currentLoc.step += 1;
        printMazeLoc(currentLoc);
        return currentLoc;
      }
    }
    if((rightGuess % 4) == 2){
      if(g_maze[currentLoc.j][currentLoc.i].downWall == 0){
        currentLoc.prevDir = 0;
        currentLoc.j += 1;
        currentLoc.step += 1;
        printMazeLoc(currentLoc);
        return currentLoc;
      }
    }
    if((rightGuess % 4) == 3){
      if(g_maze[currentLoc.j][currentLoc.i-1].rightWall == 0){
        currentLoc.prevDir = 1;
        currentLoc.i -= 1;
        currentLoc.step += 1;
        printMazeLoc(currentLoc);
        return currentLoc;
      }
    }
    rightGuess = ((rightGuess + 3) % 4); // cycles through possibilities

  }

}

// Main stayright function
int16_t stayRightSolve(){


  fullWallsSetup();
  emptyMazeTiles();
  mazeLoc currentLoc;
  currentLoc.j = 1;
  currentLoc.i = 1;
  currentLoc.step = 0;
  currentLoc.prevDir = 3;
  printMazeLoc(currentLoc);
  g_maze[currentLoc.j][currentLoc.i].tile = 1;

  mazeLoc prevLoc; // this forces a down-check first
  prevLoc.i = 0;
  prevLoc.j = 1;
  prevLoc.step = 3; // 0 = up 1 = right etc, direction of origin

  while(true){
    currentLoc = stayRightCheck(currentLoc);
    g_maze[currentLoc.j][currentLoc.i].tile = 1;
    if(checkFinish(currentLoc)){
      //markRightPath(1);
      return currentLoc.step;
    }

  }

}

//////// Deadend Solver

// Used to keep track of where it has been (creates temp walls)
void fillNewWall(int8_t j, int8_t i, int16_t step){
  if(g_maze[j-1][i].downWall == 0){
    g_maze[j-1][i].downWall = 2;
  }
  if(g_maze[j][i].downWall == 0){
    g_maze[j][i].downWall = 2;
  }
  if(g_maze[j][i-1].rightWall == 0){
    g_maze[j][i-1].rightWall = 2;
  }
  if(g_maze[j][i].rightWall == 0){
    g_maze[j][i].rightWall = 2;
  }
  g_maze[j][i].tile = 1;
  fillTile(j, i, step);


}

// Main deadend function
int16_t deadendSolve(){
  int16_t step = 0;
  fullWallsSetup();
  emptyMazeTiles();
  while(true){
    bool keepRunning = false;
    for(int8_t j = 1; j < 21; j++){
      for(int8_t i = 1; i < 21; i++){
        if((g_maze[j][i].tile == 0) && ((j+i != 2) && !((j == END_J)&&(i == END_I)))){ // leaves the start and end alone
          if(checkDeadEnd(j, i)){
            fillNewWall(j, i, step);
            step++;
            keepRunning = true;
          }
        }
      }
    }
    if(!keepRunning){
      clearTempWalls();
      return step;
    }
  }
}

//////// Depth Solver

// Checks validity of nearby tile
int8_t depthTileCheck(int8_t j, int8_t i, int16_t dist){
  if((g_maze[j][i].tile == 0) || (g_maze[j][i].tile > dist)){
    return 1;
  } else {
    return 0;
  }

}

// Marks surrounding tiles as checked
int16_t depthMarkSurrounding(int8_t j, int8_t i, int16_t step){
  int16_t dist = g_maze[j][i].tile;
  if(g_maze[j][i].rightWall == 0 && (depthTileCheck(j, i+1, dist))){
    g_maze[j][i+1].tile = dist+1;
    fillTile(j, i+1, step);
    step++;
  }
  if(g_maze[j][i-1].rightWall == 0 && (depthTileCheck(j, i-1, dist))){
    g_maze[j][i-1].tile = dist+1;
    fillTile(j, i-1, step);
    step++;
  }
  if(g_maze[j][i].downWall == 0 && (depthTileCheck(j+1, i, dist))){
    g_maze[j+1][i].tile = dist+1;
    fillTile(j+1, i, step);
    step++;
  }
  if(g_maze[j-1][i].downWall == 0 && (depthTileCheck(j-1, i, dist))){
    g_maze[j-1][i].tile = dist+1;
    fillTile(j-1, i, step);
    step++;
  }
  return step;
}

// Backtracks from the end to the beginning based on which tile is on the shortest path
int8_t depthPathCheck(int8_t j, int8_t i){
  int8_t dir = -1;
  int16_t min = g_maze[j][i].tile;
  //int8_t mark = -1;
  if((g_maze[j-1][i].downWall == 0) && !depthTileCheck(j-1, i, min)){
    min = g_maze[j-1][i].tile;
    dir = 0;
  }
  if((g_maze[j][i].rightWall == 0) && !depthTileCheck(j, i+1, min)){
    min = g_maze[j][i+1].tile;
    dir = 1;
  }
  if((g_maze[j][i].downWall == 0) && !depthTileCheck(j+1, i, min)){
    min = g_maze[j+1][i].tile;
    dir = 2;
  }
  if((g_maze[j][i-1].rightWall == 0) && !depthTileCheck(j, i-1, min)){
    min = g_maze[j][i-1].tile;
    dir = 3;
  }
  return dir;
}

// Detects if the end has been reached
void depthPath(){
  int8_t j = END_J;
  int8_t i = END_I;
  while(true){
    g_maze[j][i].tile = 400;

    int8_t dir = depthPathCheck(j, i);
    if(dir == 0){
      j--;
    }
    if(dir == 1){
      i++;
    }
    if(dir == 2){
      j++;
    }
    if(dir == 3){
      i--;
    }

    if((j+i) == 2){
      g_maze[1][1].tile = 400;
      markPath(400);
      break;
    }
  }

}

// Main depth function
int16_t depthSolve(){
  emptyMazeTiles();

  g_maze[1][1].tile = 1;
  int16_t step = 0;
  fillTile(1, 1, step);
  while(true){
    for(int8_t j = 1; j < 21; j++){
      for(int8_t i = 1; i < 21; i++){
        if((g_maze[j][i].tile > 0) && emptyCheck(j, i)){ // if previously visited
          step = depthMarkSurrounding(j, i, step);
        }
        if(g_maze[END_J][END_I].tile != 0){
          //fillTile(20, 20, step);
          depthPath();
          return step;
        }
      }
    }
  }
}

//////// Breath Solver (resuses a great deal of depthsolve)

// Main breadth function
int16_t breadthSolve(){
  emptyMazeTiles();
  g_maze[1][1].tile = 1;
  int16_t step = 0;
  int16_t distance = 0;
  fillTile(1, 1, step);
  while(true){
    distance++;
    for(int8_t j = 1; j < 21; j++){
      for(int8_t i = 1; i < 21; i++){
        if(g_maze[j][i].tile == distance){
          if((g_maze[j][i].rightWall == 0) && (g_maze[j][i+1].tile == 0)){
            g_maze[j][i+1].tile = distance +1;
            fillTile(j, i+1, step);
            step++;
          }
          if((g_maze[j][i-1].rightWall == 0) && (g_maze[j][i-1].tile == 0)){
            g_maze[j][i-1].tile = distance +1;
            fillTile(j, i-1, step);
            step++;
          }
          if((g_maze[j][i].downWall == 0) && (g_maze[j+1][i].tile == 0)){
            g_maze[j+1][i].tile = distance +1;
            fillTile(j+1, i, step);
            step++;
          }
          if((g_maze[j-1][i].downWall == 0) && (g_maze[j-1][i].tile == 0)){
            g_maze[j-1][i].tile = distance + 1;
            fillTile(j-1, i, step);
            step++;
          }
        }
      }
    }
    if(g_maze[END_J][END_I].tile != 0){

      depthPath();
      return step;
    }
  }
}

//////// Recursive Solver

// This solver operates on the condition "am I on a path to the end" and recurses if not

// Struct for passing information
typedef struct{
  int8_t rightpath; // 0 == false, 1 == true, 2 == unsure
  int16_t step;
} recursiveStruct;

// These functions check cardinal directions for possible paths
bool recRightcheck(int8_t j, int8_t i){
  if((g_maze[j][i].rightWall == 0) && (g_maze[j][i+1].tile == 0)){
    return true;
  }
  return false;
}

bool recLeftcheck(int8_t j, int8_t i){
  if((g_maze[j][i-1].rightWall == 0) && (g_maze[j][i-1].tile == 0)){
    return true;
  }
  return false;
}

bool recDowncheck(int8_t j, int8_t i){
  if((g_maze[j][i].downWall == 0) && (g_maze[j+1][i].tile == 0)){
    return true;
  }
  return false;
}

bool recUpcheck(int8_t j, int8_t i){
  if((g_maze[j-1][i].downWall == 0) && (g_maze[j-1][i].tile == 0)){
    return true;
  }
  return false;
}

// Actual recursing function
recursiveStruct recursingSolver(int8_t j, int8_t i, recursiveStruct path){
  path.step++;
  g_maze[j][i].tile = 2;
  fillTile(j, i, path.step);
  if((j == END_J) && (i == END_I)){
    g_maze[j][i].tile = 3;
    path.rightpath = true;
    return path;
  }
  if(!emptyCheck(j, i)){
    g_maze[j][i].tile = 1;
    return path;
  }
  if(recRightcheck(j, i) && (path.rightpath != true)){
    path = recursingSolver(j, i+1, path);
  }
  if(recDowncheck(j, i) && (path.rightpath != true)){
    path = recursingSolver(j+1, i, path);
  }
  if(recLeftcheck(j, i) && (path.rightpath != true)){
    path = recursingSolver(j, i-1, path);
  }
  if(recUpcheck(j, i) && (path.rightpath != true)){
    path = recursingSolver(j-1, i, path);
  }
  if(path.rightpath == 1){
    g_maze[j][i].tile = 3;
  }

  return path;

}

// Sets initial conditions for recursion
int16_t recursiveSolve(){
  emptyMazeTiles();
  recursiveStruct path;
  path.rightpath = false; // unknown
  path.step = 0;
  path = recursingSolver(1, 1, path);
  markPath(3);
  return path.step;
}

////////////////////////// End of Solvers

// Setup for arduino
void setup(void) {
  init();
  Serial.begin(9600);

  pinMode(JOY_SEL, INPUT);
  digitalWrite(JOY_SEL, HIGH);

  tft.initR(INITR_BLACKTAB);
  tft.fillScreen(ST7735_BLACK);
  fillMenuInfo();

}

// Main function
int main(){
  setup();
  buildNosquareMaze();
  while(true){
    int16_t step = 0;
    choiceStruct menuChoice = startScreen();
    tft.fillScreen(ST7735_BLACK);
    switch(menuChoice.mazeChoice){
      case 0 :
        delay(50);
        primMaze();
        break;
      case 1 :
        delay(50);
        backtrackMaze();
        break;
      case 2 :
        delay(50);
        EllersMaze();
        break;
      case 3 :
        delay(50);
        nosquareMaze();
        break;
      case 4 :
        delay(50);
        sameMaze();
        break;
    }
    switch(menuChoice.solverChoice){
      case 0 :
        delay(50);
        step = stayRightSolve();
        mazeMenu(step);
        break;
      case 1 :
        delay(50);
        step = deadendSolve();
        mazeMenu(step);
        break;
      case 2 :
        delay(50);
        step = recursiveSolve();
        mazeMenu(step);
        break;
      case 3 :
        delay(50);
        step = breadthSolve();
        mazeMenu(step);
        break;
      case 4 :
        delay(50);
        step = depthSolve();
        mazeMenu(step);
        break;
    }


  }

}
