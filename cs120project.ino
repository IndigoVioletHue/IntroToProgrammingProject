#include <AberLED.h>

#define obstacleArraySize 8 * 8  //8 obstacles per row, 9 rows (to allow for one to be off screen at all times.)
#define X_MAX 8
#define Y_MAX 8
#define DEBUG

short moveInterval = 250;
byte state = 0;  //0 is start, 1 is playing, 2 is lose.

byte redCount = 0; //7 reds is a loss.
const byte speedAdjust = 50;

unsigned long nextMoveTime = 0l;

struct obstacle {  //storing all of them as one type allows me to store every obstacle in an array
  signed short x, y;
  uint8_t color = 0;  //can be read when setting and colliding to determine outcome. 1 is green, 2 is red
};

int playerY = 5;  //color and y is constant, so no need to store any of them.

// about every 1 in 10 spaces needs an obstacle, so with a maximum of 63 locations (one has the player)
// i should be able to get away with 16 spaces in the array of all objects.
obstacle Obstacles[obstacleArraySize] = {};



void initialiseGame() {
  moveInterval = 250;
  state = 1;
  playerY = 5;
  redCount = 0;
  nextMoveTime = millis() + moveInterval;
  randomSeed(((millis() * 64400) / 2004) + (millis() / 1000) + 12); //arbitrary numbers, keeps it bounded below 1 billion so it never wraps around.
  generateObstacles();
}

/*
Generate obstacles offscreen for the initial loading. Give them a random color and Y position.
*/
void generateObstacles() {
  memset(&Obstacles, 0, sizeof(Obstacles));
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < random(2, 6); j++) {
      obstacle tmp = {};
      tmp.x = X_MAX + i;  //set initial to 8, then 9, then 10 etc..
      Obstacles[(i * 8) + j] = tmp;
#ifdef DEBUG
      Serial.println((i * 8) + j);
#endif
    }
  }
}

/*
Function to move every obstacle left. If an obstacle enters x = -1 after it is moved, it is moved to x = 8 (one move offscreen to the right). Any obstacles in position
8 are given a new random color and y position. There will be a random amount from 2 to 6 objects per row.
*/
void moveObstacles() {

  bool flags[8] = { false };
  //randomSeed(((millis() * 64400) / 20404) + (millis() / 1000) + 12); 
  for (int i = 0; i < obstacleArraySize; i++) {
    if ((i + 1) % 8 == 0) {                              //checks whether this is a new row or not
      for (int j = 0; j < 8; j++) { flags[j] = false; }  //resetting the flags for every row
    }
    obstacle *tmp = &Obstacles[i];  //we just need a pointer, dont want to load copies of every variable.
#ifdef DEBUG
    Serial.println(i);
    Serial.println(" : ");
      Serial.println(tmp->x);
#endif
    tmp->x--;  // move them first, as i can rearrange them before they are rendered
    if (tmp->x == -1) {
      tmp->x = 7;
      //continue;
      //stops the row from being moved prematurely. This was causing an error that
      //made an empty row appear.
    }
    if (tmp->x == 7) {
      do {
        tmp->y = random(0, 8);  //assign it to a random y value
      } while (flags[tmp->y]);  //continually assign new y values until an empty space is found.
      flags[tmp->y] = true;
      tmp->color = random(1, 17) <= 4 ? 1 : 2;
      //randoms' upper limit is exclusive, so the range 1-17 is 16 values. This means that it has a 25% chance of being 4 or less.
      // i could use 0,100 but i dont like dealing with that range of numbers.
    }
    if (tmp->x == 0 && tmp->y == playerY) {  //check if the obstacle is colliding with the player
      if (tmp->color == 1) {
        moveInterval += moveInterval < 225 ? speedAdjust / 2 : 0;  //slow down the game

      } else {
        moveInterval -= speedAdjust;  //speed up the game
        redCount++;
      }
      if (redCount >= 7) {  //end state
        state = 2;
      }
    }
#ifdef DEBUG
    //Serial.println("X modified!");
#endif
  }
}


//render all of the obstacles one by one, using pointers.
void render(obstacle *obst) {
  if (obst->y < 0 || obst->y > Y_MAX - 1) {
    return;  //skip anything thats trying to be rendered off-screen. saves an arbitrary amount of time.
  }
  AberLED.set(obst->x, obst->y, obst->color);  //using pointers as it reduces memory usage, allows for more obstacles on screen with less memory usage.
}

void checkButtons() {
  if (AberLED.getButtonDown(UP)) {
    playerY--;
  }
  if (AberLED.getButtonDown(DOWN)) {
    playerY++;
  }
  if (AberLED.getButtonDown(FIRE)) {
    initialiseGame();  //im using the fire button to reset the game whenever the gameover screen
  }
  // [TODO] Implement a much better method to do this.
  switch (playerY) {  //not the best way but whatever, works for now
    case -1:
      playerY = 0;
      break;
    case 8:
      playerY = 7;
      break;
  }
}


void setup() {
  AberLED.begin();
  Serial.begin(9600);
  initialiseGame();
  Serial.println("Setup");
}

void showSadFace() {  //the gameover screen. pressing the 'FIRE' button resets the game.
  unsigned short sadFace[8] = {
    0b0000000000000000,
    0b0011110000111100,
    0b0011110000111100,
    0b0000000000000000,
    0b0000111111110000,
    0b0011000000001100,
    0b0000000000000000,
    0b0000000000000000,
  };
  //the following is some code i had to figure out in a seperate file, essentially i built a library around the library.
  unsigned short *bufferPointer = AberLED.getBuffer();
  unsigned short *tmp = sadFace;
  for (int i = 0; i < 8; i++) {
    *(bufferPointer + i) = *(tmp + i);  //sets each row of the back buffer to my updated buffer
  }
  //end of quoted code
}

void loop() {
  AberLED.clear();
  if (state == 2) {  //gameover state
    showSadFace();
  } else if (state == 1) {  //gameplay state
    if (nextMoveTime != 0 && millis() >= nextMoveTime) {
      moveObstacles();
      nextMoveTime = millis() + moveInterval;
    }
    for (int i = 0; i < obstacleArraySize; i++) {
      render(&Obstacles[i]);
    }
    AberLED.set(0, playerY, 3);  //player wants to be drawn on top of everything else.
  }
  checkButtons();
  AberLED.swap();
}
