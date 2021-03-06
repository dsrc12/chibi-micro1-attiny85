//Dice/On/Learn/Impulse for ATTiny by Natalie Freed and Jie Qi, 2014
//Part of a series of ATTiny paper electronics samples
//Debounce code from Arduino example by David Mellis
// GPLv3 license

#include <EEPROM.h>

//Pin connections
int buttonPin = 2;
int randomLED = 0;  //roll dice, blinks between 1 and 6 times
int onWhenPressedLED = 3; //is on when pressed
int memorizeLED = 1; //memorizes patterns
int impulseLED = 2; //blinks on and fades out when pressed


//Button
int buttonState = HIGH; //keeps track of whether the button is on or off
int previousButtonState = LOW;
long timeChanged = millis();
long timeStateChanged = millis();
int debounceTime = 50; //minimum number of milliseconds before a change in the state of the button is registered, to help catch
//any electrical noisiness caused by the button "bouncing" from as the two contacts come to a resting state (this is called "debouncing")

//Touch LED
boolean touching = false;
int touchVal = 0; //brightness of touch LED

//Dice LED
boolean blinking = false; //is it in the process of blinking randomly?
long blinkStart, blinkLength; //keep track of how long to blink
int blinkRate = 300; //how fast should the LED blink? (time on in milliseconds)
int blinkVal = 0; // brightness of blink LED

//Impulse LED
#define fadeRate 2000 // in milliseconds how fast the fade
boolean impulse = false; //is the impulse happening?
long latestFade = 0;
int fadeStep = 1; 
int fadeVal = 0;  // brightness of impulse LED

//Pattern-learning LED
boolean learning = false; //is it in the middle of listening to a new pattern for the "memorizing" LED?
boolean was_learning = false;
#define MAXLENGTH  0x80 // must be power of two for efficient maths
#define MAXLENGTH_MASK 0x7F  // mask for optimizing modular arithmetic
byte blinkTimes[MAXLENGTH]; //array that holds a blink pattern. Each element is a number in milliseconds to stay on or off, and it alternates between on times and off times
//int blinkTimes[] = { 2000, 100, 5000, 5000 }; //to test!
int m_index = 0; //current index for blink pattern array (where we are adding new numbers)
int m_index_end = 0; //end index for blink pattern array
long patternStartTime;
int listeningTimeout = 3800; //how long before we decide user is done entering pattern
int delayBetweenPatternRepeats = 1500;
int patternVal = 0; // brightness of pattern LED
int patternTime = 0; // duration of pattern step
boolean repeating = false; // is it turning ON during or OFF during the pattern?

boolean reset = false;

void setup()
{
  randomSeed(analogRead(1));

  pinMode(buttonPin, INPUT);

  pinMode(randomLED, OUTPUT);
  pinMode(memorizeLED, OUTPUT);
  pinMode(impulseLED, OUTPUT);
  pinMode(onWhenPressedLED, OUTPUT);
  
  for( int i = 0; i < MAXLENGTH; i++ ) {
    blinkTimes[i] = EEPROM.read( i );
  }
  m_index_end = EEPROM.read(MAXLENGTH + 1);
  
  if( m_index_end == 255 ) { // we're dealing with a virgin sticker
    for( int i = 0; i < MAXLENGTH; i++ ) {
      blinkTimes[i] = 0;
    }
    m_index_end = 0;
  }
}

void loop()
{
  //Read switch value
  int reading = analogRead(buttonPin);

  if (reading == 0)
  {
    reading = 0;
    reset = true;
  }  
  else if(reading < 600)
  {
    reading = 0;
  }
  else
  {
    reading = 1;
  }

  if(reading != previousButtonState)
  {
    timeChanged = millis();
  }

  //check for debounce
  if(millis() - timeChanged > debounceTime)
  { 

    // start reading phase (something is happening at the button) 
    if(reading != buttonState) 
    {  
      buttonState = reading;
      timeStateChanged = millis();
      if(buttonState == HIGH) //button NOT pressed (just let go)
      {
        
        touching = false;
        if(!blinking)// set up dice blinks
        {
          blinking = true;
          blinkLength = blinkRate * 2 * random(1, 7); //generates a random number between 1 and 6
 //         blinkLength = blinkRate * 2 * 6; // generated by fair dice roll, guaranteed random
          blinkStart = millis();
        }
        if(learning) //if recording, save not-touching time
        {
          patternTime = millis() - patternStartTime;
          blinkTimes[m_index] = byte(patternTime); // save low byte
          blinkTimes[m_index+1] = byte(patternTime >> 8); //save high byte

//          m_index = (m_index + 2) % (MAXLENGTH+1); //move index over 2
          m_index = (m_index + 2) & MAXLENGTH_MASK; //move index over 2
          patternStartTime = millis(); 
        }
      } 
      else //button IS pressed
      { 
        touching = true;
        if(!impulse)
        {
          fadeVal=250;
          impulse = true;
        }
        if(!learning) //if not already recording, clear pattern and begin record
        {
          digitalWrite(memorizeLED, LOW);
          learning = true;
          m_index = 0;
          m_index_end = 0;
          patternStartTime = millis();
          for( int i = 0; i < MAXLENGTH; i++ )
            blinkTimes[i] = 0;  
        }
        else //otherwise continue to record touching time
        {
          int patternTime = millis() - patternStartTime;
          blinkTimes[m_index] = byte(patternTime);
          blinkTimes[m_index+1] = byte(patternTime >> 8);
//          m_index = (m_index + 2) % (MAXLENGTH+1);
          m_index = (m_index + 2) & MAXLENGTH_MASK;
          patternStartTime = millis(); 
        }
      }
    }

    // if too much time passes after pressing switch, no longer reading
    else if(learning && reading == HIGH && millis() - timeChanged > listeningTimeout)
    {
      learning = false;
      blinkTimes[m_index] = byte(delayBetweenPatternRepeats);
      blinkTimes[m_index+1] = byte(delayBetweenPatternRepeats >> 8);
      m_index_end = m_index+2;

//      blinkTimes[m_index-2] = byte(delayBetweenPatternRepeats);
//      blinkTimes[m_index-1] = byte(delayBetweenPatternRepeats >> 8) + blinkTimes[m_index-1];
//      blinkTimes[m_index] = 1;
//      blinkTimes[m_index+1] = 0;
//      m_index_end = m_index + 2;

      m_index = 0;
      patternStartTime = millis();
      patternTime = int((blinkTimes[1] << 8) |  blinkTimes[0]);
    }
  }
#if 1  
  // memorize the blink times when we stop learning
  if( was_learning && !learning ) {
     for( int i = 0; i < MAXLENGTH; i++ ) {
       EEPROM.write(i, blinkTimes[i]);
     }
     EEPROM.write(MAXLENGTH + 1, m_index_end);

    // reset the pattern start time because we spent some time writing the EEPROM
     patternStartTime = millis();
     patternTime = int((blinkTimes[1] << 8) |  blinkTimes[0]);
  }
#endif
  was_learning = learning;
  
  //fade Touch LED
  //touchVal= transition(touching, touchVal, 2);
  //analogWrite(onWhenPressedLED, touchVal);
  
  if (touching)
  {
    digitalWrite(onWhenPressedLED, HIGH);
  }
  else
  {
    digitalWrite(onWhenPressedLED, LOW);
  }

  //blink the Dice LED
  if(blinking)
  {
    if(millis() - blinkStart < blinkLength)
    {
//      int blinkState = !( ((millis() - blinkStart) / blinkRate) % 2 );
      int blinkState = !( ((millis() - blinkStart) / blinkRate) & 0x1 );
      blinkVal= transition(blinkState, blinkVal, 1);
      analogWrite(randomLED, blinkVal);
    }
    else
    {
      blinking = false; //done random blinking
    } 
  } else {
    analogWrite(randomLED, 0); // turn off the randomLED when we're not blinking
  }

  //turn on impulse LED
  if(millis() - latestFade > fadeRate / 200) //fade one step in or out at user-specified rate
  { 
    fadeVal= transition(false, fadeVal, 2);
    latestFade = millis();
  }
  if (fadeVal <= 0) //when brightness is all the way down, stay down until next impulse triggered
  {
    impulse = false;
  }
  software_analogWrite(impulseLED, fadeVal);

  //play repeater LED
  if(!learning) //if not in the middle of learning a new pattern
  {
    //play current pattern
    if( (blinkTimes[m_index] != 0) && (m_index_end > 4)) //if there is a pattern to play
    {     

      patternTime = int((blinkTimes[m_index+1] << 8) |  blinkTimes[m_index]);

//      if(!((m_index%4)/2))
      if( ((m_index & 0x2)) )
      {
        repeating = false;
      }
      else
      {
        repeating = true;
      }

      patternVal = transition(repeating, patternVal, 2);
      analogWrite(memorizeLED, patternVal);

    }
    else
    {
      digitalWrite(memorizeLED, LOW);
    }

    if(millis() - patternStartTime > patternTime)
    {
//      m_index = (m_index + 2) % (m_index_end + 1); //next three lines of code are equivalent
      m_index = m_index + 2;
      if( m_index > m_index_end ) {
        m_index = 0;
      }
      patternStartTime = millis();
    }
  }

  previousButtonState = reading;
}

//one cycle of pwm for any output pin
void software_analogWrite(int pin, int brightness)
{
  if(brightness<=0)
  {
    digitalWrite(pin, LOW); 
    delayMicroseconds(255); // normalize loop times 
  } 
  else
    if(brightness>=255)
    {
      digitalWrite(pin, HIGH); 
      delayMicroseconds(255); // normalize loop times
    } 
    else
    {
      digitalWrite(pin, HIGH);   
      delayMicroseconds(brightness);  
      digitalWrite(pin, LOW); 
      delayMicroseconds(255-brightness);  
    }

}

//calculates brightness for smooth transitions
// state tells whether fading on (true) or fading off (false)
int transition(boolean state, int brightVal, int rate)
{
  if(state)
  {
    brightVal = brightVal + rate;
  }
  else
  {
    brightVal = brightVal - rate;
  }

  if(brightVal >=255)
  {
    brightVal = 255;
  }
  else if(brightVal <=0)
  {
    brightVal = 0;
  }

  return brightVal;
}






