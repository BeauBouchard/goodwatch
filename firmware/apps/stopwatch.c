/*! \file stopwatch.c
  
  \brief Stopwatch application.
   
  This is a simple stop watch, which begins counting as the + key is
  hit and clears the count when the 0 key is pressed.  Because the
  RTC is busy with the real time, we count in quarter seconds as the
  display is updated.

  We do not count time or store the count when in other applications.
  It would be nice to change that, so that something could be timed in
  the background while the user is doing other things.

*/

#include "api.h"
#include "stopwatch.h"

static int counting=0;

/* It's a crying shame, but you'll break the code if you increase this
   count to a long, because repeated divisions in rendering will take
   longer than the ~8k cycles we have per interrupt when running at
   32kHz and the stopwatch will run too slow.

   To work around this, we maintain the count of hours separately from
   the general second count.
 */
static uint16_t count, hour;
static uint8_t hourhex;
static uint8_t min, minhex;
static uint8_t sec, sechex;


//! Renders the low bits of the count in quarter seconds.
static const char subs[]={0, 0x25, 0x50, 0x75};

//! Entry to the stopwatch app.
void stopwatch_init(){
  //Zero the count.
  count=hour=min=sec=0;
  
  //Start off without counting.
  counting=0;

  //Draw these once, rather than every frame.
  lcd_cleardigit(5); //Space
  lcd_cleardigit(2); //Space

  //Force a draw on startup.
  stopwatch_draw(1);
}

//! Exit form the stopwatch app.
int stopwatch_exit(){
  //Give up without a fight when the mode button is pressed.
  return 0;
}

//! A button has been pressed for the stopwatch.
int stopwatch_keypress(char ch){
  /* For now, we support only two buttons.  + stops and starts the
     count, while 0 resets the counter.
   */
  
  switch(ch){
    case '+':
      counting=!counting;
      break;
    case '0':
      //Zero the count.
      count=hour=min=sec=0;
      break;
  }
  
  /* Stopwatch uses rendering frequency to count time, so we don't
     redraw after a keypress when we are counting. */
  return !counting;
}


//! Draw the stopwatch app and handle its input.
void stopwatch_draw(int forced){
  uint8_t subhex;
  
  /* The stopwatch is special in that it never times out.  Be very
     careful when doing this, because a minor bug might kill the
     battery.
   */
  app_cleartimer();

  //If we aren't counting and there's not been a keypress, don't
  //bother drawing.
  if(!forced && !counting)
    return;
  
  //Increment the count if we're counting.
  if(counting)
    count++;

  //Update the subhex field.
  subhex=subs[count&3];

  //Handle the second rollover every forth tick.
  if((count&3)==3){
    sec++;
    sechex=int2bcd(sec);
  }

  //Handle the minute rollover every 60 seconds.
  if(sec>=60){
    sec-=60;
    sechex=int2bcd(sec);
    min++;
    minhex=int2bcd(min);
  }
  
  //Handle hour rollover every 60 minutes
  if(min>=60){
    min-=60;
    minhex=int2bcd(min);
    hour++;
    hourhex=int2bcd(hour);
  }
  
  
  //Blink the colon once a second.
  setcolon((count>>1)&1);
  
  //We either draw hhmmss or mmssSS.
  if(hour && !subhex){
    lcd_digit(1,sechex>>4);
    lcd_digit(0,sechex&0xF);
    
    //Draw minutes and hours
    if(!sec){
      //Draw minutes
      lcd_digit(4,minhex>>4);
      lcd_digit(3,minhex&0xF);
      
      //Draw hours
      lcd_digit(7,hourhex>>4);
      lcd_digit(6,hourhex&0xF);
    }
  }else if(!hour){
    //Draw the subsecond first.
    lcd_digit(1,subhex>>4);
    lcd_digit(0,subhex&0xF);
    
    //Only draw the rest if the subseconds have changed.
    if(!subhex || count==1){
      lcd_digit(4,sechex>>4);
      lcd_digit(3,sechex&0xF);
      
      //Update minutes if the seconds are zero.
      if(!sec){
	lcd_digit(7,minhex>>4);
	lcd_digit(6,minhex&0xF);
      }
    }
  }
}
