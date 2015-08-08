
#include <avr/io.h>
#include <stdlib.h>
#include <avr/eeprom.h>
#include <stdint.h>

#define CELLS_X 25
#define CELLS_Y 25


unsigned long old_generation[CELLS_X];
volatile unsigned long d[25];
unsigned char InputRegOld;
unsigned int refreshNum = 10;
unsigned char frame = 0;

unsigned int x_pos = 0;
unsigned int y_pos = 0;


unsigned char get_cell(unsigned long from[],  signed char x, signed char y)
{
  if(x < 0)
     x = 24;

  if(x > 24)
     x = 0;

  if(y < 0)
     y = 24;

  if(y > 24)
     y = 0;

  return ((from[x] & ( (unsigned long) 1 << y)) > 0);
}

static void set_cell(unsigned long to[], signed char x, signed char y, bool value)
{
  if(value)
    to[x] |= (unsigned long) 1 <<  y;
  else
    to[x] &= ~( (unsigned long) 1 << y);

  return;
}

// copies from[] into d[] (the framebuffer) for display
static void display(unsigned long from[])
{
  for(unsigned char x = 0; x < CELLS_X; x++)
  {
    unsigned long longtemp = 0;
    for(unsigned char y = 0; y < CELLS_Y; y++)
    {
      if(get_cell(from,x,y))
        longtemp |= (unsigned long) 1 << y;
    }

    d[x] = longtemp;
  }

  return;
}

void SPI_TX(char cData)
{
  SPDR = cData; //Start Transmission
  while (!(SPSR & _BV(SPIF))); //Wait for transmission complete:
}

void light_leds()
{
  unsigned int k = 0;
  while (k < refreshNum)  // k must be at least 1
  {
    unsigned int j = 0;
    while (j < 25)
    {
      if (j == 0)
          PORTD = 160;
      else if (j < 16)
        PORTD = j;
      else
        PORTD = (j - 15) << 4;

      unsigned char out1, out2, out3, out4;
      unsigned long dtemp;

      dtemp = d[j];
      out4 = dtemp & 255U;
      dtemp >>= 8;
      out3 = dtemp & 255U;
      dtemp >>= 8;
      out2 = dtemp & 255U;
      dtemp >>= 8;
      out1 = dtemp & 255U;

      SPI_TX(out1);
      SPI_TX(out2);
      SPI_TX(out3);

      PORTD = 0;  // Turn displays off

      SPI_TX(out4);

      PORTB |= _BV(1);    //Latch Pulse
      PORTB &= ~( _BV(1));

      j++;
    }

    k++;
  }
}

// Runs when sketch starts
void setup()
{
  srand(eeprom_read_word((uint16_t *) 2));

  for(unsigned char temp = 0; temp != 255; temp++)
  {
    TCNT0 = rand();
  }

  eeprom_write_word((uint16_t *) 2,rand());

  PORTD = 0U;    // All B Input
  DDRD = 255U;   // All D Output

  PORTB = 1;     // Pull up on ("OFF/SELECT" button)
  PORTC = 255U;  // Pull-ups on C

  DDRB = 254U;   // B0 is an input ("OFF/SELECT" button)
  DDRC = 0;      // All inputs

  // SET MOSI, SCK Output, all other SPI as input:

  DDRB = ( 1 << 3 ) | (1 << 5) | (1 << 2) | (1 << 1);

  //ENABLE SPI, MASTER, CLOCK RATE fck/4:   //TEMPORARY:: 1/128
  SPCR = (1 << SPE) | ( 1 << MSTR );//| ( 1 << SPR0 );//| ( 1 << SPR1 ) | ( 1 << CPOL );

  SPI_TX(0);
  SPI_TX(0);
  SPI_TX(0);

  PORTB |= _BV(1);    // Latch Pulse
  PORTB &= ~( _BV(1));

  InputRegOld = (PINC & 31) | ((PINB & 1) << 5);
}

// Runs over and over
void loop()
{
  unsigned char temp = 0;
  unsigned char out1, out2, out3, out4;

  signed char x=0, y=0;

  display(old_generation);
  light_leds();

  for (x = 0; x < CELLS_X; x++)
  {
    for (y = 0; y < CELLS_Y; y++)
    {
      unsigned int x_coord = x_pos + x;
      unsigned int y_coord = y_pos + y;

      unsigned int k = ((x_coord ^ ~y_coord) & ((x_coord - 350) >> 3));
      k = k * k;
      k = ((k >> 12) & 1);

      set_cell(old_generation, x, y, k >= 1);
    }

    light_leds();
  }

  x_pos += 32;

  if (frame % 15 == 0)
    y_pos += 32;

  frame++;

  light_leds();
  light_leds();
}
