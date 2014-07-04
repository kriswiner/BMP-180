//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//++ Useful Bubble Display Functions++
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void writeInteger(uint8_t dsply, int data)
{
  char string[10] = "";                             // define character array to hold the digits
  itoa(data, string);                               // get ascii character string representation of the integer to be displayed
  uint8_t length = strlen(string);                  // get the length of the string; number of digits in integer
  uint8_t blanks = 4 - length;                      // how many blanks do we have?

  if (length > 4) return;                           // if length greater than 4 digits we can't display on a four-digit display!

  for (uint8_t digit = 0; digit < blanks; digit++)  // scroll through each digit to determine what to write to the display
  {
      writeDigit(dsply, digit + 1, 11, 0);          // clear digit wherever there are blanks
  }

  for (uint8_t digit = 0; digit < 4; digit++)       // scroll through each digit to determine what to write to the display
  {
      char ch = string[digit];                      // get the ascii character of the next string segment

      if (ch == '-') {
      writeDigit(dsply, digit + 1 + blanks, 12, 0); // check if negative sign needed
      } 
      else {                                        // character must be a digit
      ch -= '0';                                    // convert it to an integer
      writeDigit(dsply, digit + 1 + blanks, ch, 0); // write it to the display; right justify the integer
      } 
  }
}

void writeFloat(uint8_t dsply, float data, uint8_t dp)
{
  char string[10] = "";  // define character array to hold the digits
  int datanew = 0;
  
  switch (dp)
  {
    case 0:
    datanew = (int )(1.*data);
    break;
 
    case 1:
    datanew = (int )(10.*data);
    break;

    case 2:
    datanew = (int )(100.*data);
    break;
 
    case 3:
    datanew = (int )(1000.*data);
    break;
   }
   
  
  itoa(datanew, string);                                    // get ascii character string representation of the integer to be displayed
  uint8_t length = strlen(string);                          // get the length of the string; number of digits in integer
  uint8_t blanks = 4 - length;                              // how many blanks do we have?

  if (length > 4) return;                                   // if length greater than 4 digits we can't display on a four-digit display!

// scroll through each digit to determine what to write to the display
for (uint8_t digit = 0; digit < blanks; digit++)            // first the blanks
  {
          if( (digit + 1) == (4 - dp) ) {                   // handle special case where blank coincides with decimal point
            writeDigit(dsply, digit + 1, 0, 0x80);          // add leading zero before decimal place
          }
          else {
            writeDigit(dsply, digit + 1, 11, 0x00);         // otherwise clear digit wherever there are blanks
          }
  }

  for (uint8_t digit = 0; digit < 4; digit++)               // now the characters to determine what to write to the display
  {
      char ch = string[digit];                              // get the ascii character of the next string segment

      if (ch == '-') {
        if((digit + 1 + blanks) == (4 - dp) ) {
          writeDigit(dsply, digit + 1 + blanks,  0, 0x80);  // check if negative sign needed, add a decimal point
          writeDigit(dsply, digit + 0 + blanks, 12, 0x00);  // add a leading zero
        }
        else {
          writeDigit(dsply, digit + 1 + blanks, 12, 0x00);  // check if negative sign needed, no decimal point
        }
        }
      else  {                                               // character must be a digit
        ch -= '0';                                          // convert it to an integer
        if((digit + 1 + blanks) == (4 - dp) ) {
          writeDigit(dsply, digit + 1 + blanks, ch, 0x80);  // write it to the display with decimal point; right justify the integer
        } 
        else {
          writeDigit(dsply, digit + 1 + blanks, ch, 0x00);  // write it to the display; right justify the integer
        } 
     }
  }
}
  

void writeDigit(uint8_t dsply, uint8_t digit, uint8_t data, uint8_t dp) 
{
if(dsply == 1) {
  digit = (digit - 1)*2 + 0; 
} 
if(dsply == 2) {
  digit = (digit - 1)*2 + 8 ;
}
if(dsply == 3) {
  digit = (digit - 1)*2 + 1;
}
if(dsply == 4) {
  digit = (digit - 1)*2 + 9;
}
writeByte(HT16K33_ADDRESS, digit, numberTable[data] | dp);
}


void clearDsplay(int dsply) 
{
  for(int i = 0; i < 8; i++)  {
  writeDigit(dsply, i, 11, 0);  // Clear display, 11 is blank in the numberTable above
  }
}


void initHT16K33()
{
  writeCommand(HT16K33_ADDRESS, HT16K33_ON);         // Turn on system oscillator
  writeCommand(HT16K33_ADDRESS, HT16K33_DISPLAYON);  // Display on
  writeCommand(HT16K33_ADDRESS, HT16K33_DIM);        // Set brightness

}


void blinkHT16K33(int time) 
{
  writeCommand(HT16K33_ADDRESS, HT16K33_BLINKON);  // Turn on blink
  delay(1000*time);
  writeCommand(HT16K33_ADDRESS, HT16K33_BLINKOFF);  // Turn on blink
}


 /* itoa:  convert n to characters in s */
 void itoa(int n, char s[])
 {
     int i, sign;
 
     if ((sign = n) < 0)  /* record sign */
         n = -n;          /* make n positive */
     i = 0;
     do {       /* generate digits in reverse order */
         s[i++] = n % 10 + '0';   /* get next digit */
     } while ((n /= 10) > 0);     /* delete it */
     if (sign < 0)
         s[i++] = '-';
     s[i] = '\0';
     reverse(s);
 } 
 
 /* reverse:  reverse string s in place */
 void reverse(char s[])
 {
     int i, j;
     char c;
 
     for (i = 0, j = strlen(s)-1; i<j; i++, j--) {
         c = s[i];
         s[i] = s[j];
         s[j] = c;
     }
 }

        // Wire.h read and write protocols
        void writeCommand(uint8_t address, uint8_t command)
  {
	Wire.beginTransmission(address);  // Initialize the Tx buffer
	Wire.write(command);              // Put command in Tx buffer
	Wire.endTransmission();           // Send the Tx buffer
}
