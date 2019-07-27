/*
  Mimic Panel Controller / Train Emulator
  Forest Hill Station Mimic Panel (QLD Australia)
  Ben Hussey - May 2018
*/

#include <Bounce2.h>

// Originally written for low-trigger relays
// Swap these if using high-trigger relay board
static const int RELAY_OFF = HIGH;
static const int RELAY_ON = LOW;

static const int BUZZ = 15;  // Buzzer

// Class to manage each block of track
class Block
{
  public: int id;
  public: int nextBlock;
  int signalPin;
  long transitTime;
  long timeEntered;
  bool occupied;
  public: bool rtd;
  long leaving;

  public:
  Block(int pk, int next, int pin, long transit)
  {
    id = pk;
    nextBlock = next;
    signalPin = pin;
    transitTime = transit;
    occupied = false;
    rtd = false;
    leaving = 0;
    pinMode(signalPin, OUTPUT);
    digitalWrite(signalPin, RELAY_ON);
  }

  bool EnterBlock()
  // Train is waiting to enter block, return true if clear
  {
    unsigned long currentTime = millis();
    if (!occupied && !leaving) {
      occupied = true;
      digitalWrite(signalPin, RELAY_OFF);
      timeEntered = currentTime;
      Serial.print("ENTERED: ");
      Serial.println(id);
      return true;
    }
    return false;
  }

  void ExitBlock()
  // Train has entered another block
  // Check block is occupied before calling
  {
    unsigned long currentTime = millis();
    leaving = currentTime;
    rtd = false;
    timeEntered = 0;
    Serial.print("LEAVING: ");
    Serial.println(id);
  }

  void Update()
  {
    unsigned long currentTime = millis();

    // Set request to depart once train has transited block
    if (occupied && !rtd && !leaving && (currentTime - timeEntered) > transitTime) {
      rtd = true;
      Serial.print("WAITING: ");
      Serial.println(id);
    }

    // Clear block some time (5 sec) after entering next block
    if (occupied && leaving > 0 && (currentTime - 5000) > leaving) {
      occupied = false;
      timeEntered = 0;
      leaving = currentTime;
      digitalWrite(signalPin, RELAY_ON);
      Serial.print("CLEAR: ");
      Serial.println(id);
    }

    // Allow train to enter some time (2 sec) after block clear
    if (!occupied && leaving > 0 && (currentTime - 2000) > leaving) {
      leaving = 0;
    }
  }
};

// Setup routes
const int numEntryBlocks = 2;
int EntryBlocks[numEntryBlocks] = {11, 21};
long EntryTimes[numEntryBlocks];

const int numRoutes = 8;
Block routes[numRoutes] = {
  Block(11, 12, 12, 500),   // Up entry
  Block(12, 13, 2, 9000),   // Up approach
  Block(13, 14, 3, 10000),  // Up platform
  Block(14, 0, 6, 20000),   // Up away

  Block(21, 22, 14, 500),   // Down entry
  Block(22, 23, 7, 20000),   // Down approach
  Block(23, 24, 8, 5000),  // Down platform
  Block(24, 0, 11, 9000),  // Down away
};

// Setup buttons
const int numButtons = 2;
const int buttonPins[numButtons] = {
  16,                       // Up acknowledge button
  17,                       // Down acknowledge button
};
Bounce buttons[numButtons];

// Setup other output pins
const int numOutPins = 4;
const int outPins[numOutPins] = {
  4,                       // Up ER given
  5,                       // Up ER free
  9,                       // Down ER given
  10,                      // Down ER free
};

long stationTimes[3] = {3000, 10000, 5000};
long upStationTimer = 0;
int upStationState = 0;
long downStationTimer = 0;
int downStationState = 0;

void setup() {
  pinMode(BUZZ, OUTPUT);
  digitalWrite(BUZZ, HIGH);

  for (int i=0; i <= numOutPins - 1; i++){
    pinMode(outPins[i], OUTPUT);
    digitalWrite(outPins[i], RELAY_ON);
  }

  Serial.begin(9600);
  unsigned long currentTime = millis();

  for (int i=0; i <= numEntryBlocks - 1; i++){
    EntryTimes[i] = currentTime + random(1000, 20000);
  }

  for (int i=0; i <= numButtons - 1; i++){
    pinMode(buttonPins[i],INPUT_PULLUP);
    buttons[i] = Bounce();
    buttons[i].attach(buttonPins[i]);
    buttons[i].interval(5);
  }
}

int getBlock(int id) {
   for (int i=0; i <= numRoutes - 1; i++){
      if (routes[i].id == id) {
        return i;
      }
   }
}

bool tryPassToNextBlock(int i) {
  // Check we are not passing out of section
  if (routes[i].nextBlock > 0) {
    // Check the next block is not occupied
    int index = getBlock(routes[i].nextBlock);
    if (routes[index].EnterBlock()) {
      routes[i].ExitBlock();
      return true;
    }
  // Pass out of section
  } else {
    routes[i].ExitBlock();
    return true;
  }
  return false;
}

bool buzzState = false;
long buzzSeq[8] = {0, 0, 0, 0, 0, 0, 0, 0}; // Buzzer sequence
long buzzTimer = 0; // Buzzer timer

void loop() {
  unsigned long currentTime = millis();

  // Buzzer sequence control
  if (buzzTimer) {
    long buzzInc = 0;
    for (int i=0; i <= 7; i++){
        buzzInc = buzzInc + buzzSeq[i];
        if (currentTime < buzzTimer + buzzInc) {
          if ((i % 2) == 0 && !buzzState) {
            digitalWrite(BUZZ, LOW);
            Serial.println("BUZZ");
            buzzState = true;
          } else if ((i % 2) != 0 && buzzState) {
            digitalWrite(BUZZ, HIGH);
            buzzState = false;
          }
          break;
        }
    }
    if (currentTime > (buzzTimer + buzzInc)) {
      buzzTimer = 0;
      buzzState = false;
      digitalWrite(BUZZ, HIGH);
    }
  }

  // Train generation
  for (int i=0; i <= numEntryBlocks - 1; i++){
    if (EntryTimes[i] < currentTime) {
      int index = getBlock(EntryBlocks[i]);
      if (routes[index].EnterBlock()) {
        if (!buzzTimer) {
          // Double buzz on arrival in section
          long seq[8] = {200, 200, 200, 0, 0, 0, 0, 0};
          memcpy(buzzSeq, seq, 8*sizeof(long));
          buzzTimer = currentTime;
        };
      };
      EntryTimes[i] = currentTime + random(15000, 50000);
    }
  }

  // Main logic loop to pass trains between blocks
  for (int i=0; i <= numRoutes - 1; i++){
    routes[i].Update();
    // Check if train is waiting to leave block
    if (routes[i].rtd) {
      if (routes[i].id == 11) {
        // Entry block, wait for ack
        // Buzzer sounds once to indicate train entering block
        //
        if (!buttons[0].read()) {
          tryPassToNextBlock(i);
        }
      } else if (routes[i].id == 21) {
        // Entry block, wait for ack
        if (!buttons[1].read()) {
          tryPassToNextBlock(i);
        }
      } else if (routes[i].id == 13) {
        // Station block, wait for departure
        // Currently uses ER lamps to indicate train departure
        // Not entirely accurate but more visually interesting!
        if (!upStationState) {
          digitalWrite(outPins[0], RELAY_OFF);
          upStationTimer = currentTime;
          upStationState = 1;
          Serial.println("ERU Given");
        } if (upStationState == 1 && currentTime > upStationTimer + stationTimes[0]) {
          digitalWrite(outPins[1], RELAY_OFF);
          upStationState = 2;
          Serial.println("ERU Free");
        } else if (upStationState == 2 && currentTime > upStationTimer + stationTimes[1]) {
          if (tryPassToNextBlock(i)) {
            upStationTimer = currentTime + stationTimes[2];
            upStationState = 3;
          }
        }
      } else if (routes[i].id == 23) {
        // Station block, wait for departure
        if (!downStationState) {
          digitalWrite(outPins[2], RELAY_OFF);
          downStationTimer = currentTime;
          downStationState = 1;
          Serial.println("ERU Given");
        } if (downStationState == 1 && currentTime > downStationTimer + stationTimes[0]) {
          digitalWrite(outPins[3], RELAY_OFF);
          downStationState = 2;
          Serial.println("ERU Free");
        } else if (downStationState == 2 && currentTime > downStationTimer + stationTimes[1]) {
          if (tryPassToNextBlock(i)) {
            downStationTimer = currentTime + stationTimes[2];
            downStationState = 3;
          }
        }
      } else {
        tryPassToNextBlock(i);
      }
    }
  }

  if (upStationState == 3 && currentTime > upStationTimer) {
    digitalWrite(outPins[0], RELAY_ON);
    digitalWrite(outPins[1], RELAY_ON);
    upStationState = 0;
  }
  if (downStationState == 3 && currentTime > downStationTimer) {
    digitalWrite(outPins[2], RELAY_ON);
    digitalWrite(outPins[3], RELAY_ON);
    downStationState = 0;
  }

  for (int i=0; i <= numButtons - 1; i++){
    buttons[i].update();
  }
}
