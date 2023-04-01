// Focuser controller for telescopes, emulates the moonlite focuser protocol
// allowing use of the ascom drivers for the moonlite, saving having to write COM+ drivers.
// Can be used with the TMC22xx or with the UL2003 if its needed (github.com/thrud61/ul2003)
//
// Author James Wilson
// 1st November 2021
//

#include <FreeRTOS.h>
#include <queue.h>

#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/irq.h"


#include <cstring>
#include <string>

#include "tmc22xx.h"

#define UART_ID uart0
#define BAUD_RATE 9600
#define DATA_BITS 8
#define STOP_BITS 1
#define PARITY    UART_PARITY_NONE

// We are using pins 16 and 17 for UART0
#define UART_TX_PIN 16
#define UART_RX_PIN 17

// need a queue for each core/thread
QueueHandle_t xISRQueue = NULL;
QueueHandle_t xMoveQueue = NULL;

// create the motor object, in this case its using the tmc2208 and the 35PM48L01-05B stepper motor
// we can also use the less powerful 28BYJ-48 converted to bipolar https://ardufocus.com/howto/28byj-48-bipolar-hw-mod/
Motor motor;

// led state variable (sort of)
static int led = 1;

// function that implements the moonlite focuser protocol based on this doc https://indilib.org/media/kunena/attachments/1/HighResSteppermotor107.pdf
// allows me to use the standard ascom driver for the moonlite focuser https://focuser.com/media/Downloads/MoonLite_Software/Ascom/MoonLite%20DRO%20Setup.zip
// also the Moonlite Focuser Control from here https://focuser.com/media/Downloads/MoonLite_Software/NonAscom/MoonliteSingleFocuser_v1.4.zip
std::string do_moonlite(std::string &, Motor &);

// Core 0 loop process waiting for messages from the UART0 ISR
// calls the moonlite protocol handler and sends any out put to the UART tx
void MoonliteProcess(void)
{
  char buffer[20];
  static int led = 1;

  // lets flash the led at 1/10th of the actual call rate
  led++;
  gpio_put(PICO_DEFAULT_LED_PIN, (led / 10) & 1);

  // wait for a message from the uart isr
  if (xQueueReceive(xISRQueue, buffer, (TickType_t) 10) == pdTRUE)
  {

      // do protocol stuff, we only know moonlite
      std::string out = do_moonlite(buffer, motor);

      // do we have a response, then send it on
      if (out.size() > 0)
      {
        // Can we send it back?
        if (uart_is_writable(UART_ID))
        {
          uart_puts(UART_ID, out.c_str());
        }
      }
  }
}

// UART0 RX interrupt handler, don't need a TX isr
void on_uart_rx()
{
  // we are going to maintain these variable across all interrupts
  static int state = 1;
  static int b_pos = 0;
  static char buffer[20]; // does not need to be big, we only really need 9 bytes

  // really should be an error if there is nothing to read
  // but its a pico so we will just carry on
  while (uart_is_readable(UART_ID))
  {
    // fetch a single char from the uart
    uint8_t ch = uart_getc(UART_ID);
    BaseType_t xTaskWoken; // needed for calling the queue send, we don't care

    // pick out the input command :ccxxxx#
    // its a state machine only 2 states but it counts
    switch (state)
    {
      case 1: // looking for :, its our stx
        {
          // not : then get next char
          if (ch != ':')
            continue;

          // initialise the command buffer, move to next state
          b_pos = 0;
          memset(buffer, 0, 20);
          state = 3;
        }
        break;

      case 2: // looking for #, its our etx, or [setting=x] when we implement it
        {
          // we will keep everything thats not etx
          if (ch != '#')
          {
            // if we get stx then reset command buffer
            if (ch == ':')
            {
              b_pos = 0;
              memset(buffer, 0, 20);
              state = 2; // we are staying in this state
              continue;
            }
            buffer[b_pos++] = ch;
          }
          else
          {
            // got a complete command send message to the moonlite process core 0
            if ( xQueueSendFromISR( xISRQueue, ( void * ) buffer, &xTaskWoken) != pdPASS )
            {
              // send error to USB port, this is an example Serial.printf can be used for debugging
              //              Serial.printf ("failed message %s\n", buffer);
            }
            state = 1;
            b_pos = 0;
          }
          // something went wrong
          if (b_pos > 16)
            state = 1;
        }
        break;

    }
  }
}

// standard arduino run once for core 0
void setup()
{
  //  Serial.begin(115200);

  // couple of queues to communicate between cores and isr
  xISRQueue = xQueueCreate( 10, 10);
  xMoveQueue = xQueueCreate( 10, sizeof(int));

  gpio_init(PICO_DEFAULT_LED_PIN);
  gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
  gpio_put(PICO_DEFAULT_LED_PIN, 1);

  // Set up our UART with a basic baud rate.
  uart_init(UART_ID, 9600);

  // Set the TX and RX pins by using the function select on the GPIO
  // Set datasheet for more information on function select
  gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
  gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

  // Actually, we may want a different speed
  // The call will return the actual baud rate selected, which will be as close as
  // possible to that requested, which we will ignore. But we got it right first time so....
  //  int __unused actual = uart_set_baudrate(UART_ID, BAUD_RATE);

  // Set UART flow control CTS/RTS, we don't want these, so turn them off
  uart_set_hw_flow(UART_ID, false, false);

  // Set our data format
  uart_set_format(UART_ID, DATA_BITS, STOP_BITS, PARITY);

  // Turn on FIFO's - we want to do this character by character but we don't want to miss a char
  uart_set_fifo_enabled(UART_ID, true);

  // Set up a RX interrupt
  // We need to set up the handler first
  // Select correct interrupt for the UART we are using
  int UART_IRQ = UART_ID == uart0 ? UART0_IRQ : UART1_IRQ;

  // And set up and enable the interrupt handlers
  irq_set_exclusive_handler(UART_IRQ, on_uart_rx);
  irq_set_enabled(UART_IRQ, true);

  // Now enable the UART to send interrupts - RX only
  uart_set_irq_enables(UART_ID, true, false);

  // OK, all set up.
  // Lets run a loop and wait for RX interrupts
  // everything happens in the irq handler
}

// core 0 process
void loop()
{
  MoonliteProcess();
}

// core 1 used for moving the stepper asynchronously
void loop1()
{
  int buffer;

  // used to do async move, if we get a message we will try to move, don't care it we don't actually move
  if (xQueueReceive(xMoveQueue, &buffer, (TickType_t) 10) == pdTRUE)
  {
    if (motor.is_moving())
      motor.move();
  }
}

// Moonlite focuser protocol emulation
// worked out from https://indilib.org/media/kunena/attachments/1/HighResSteppermotor107.pdf
// mine ignores half steps cmd though it is implemented in the code, ignores temp calibration and other non-essential commands
std::string do_moonlite(char *buffer, Motor & motor)
{
  char out[10] = "";
  int v;
  unsigned int s, e;

  //
  // lets ignore the temp compensation stuff C + -
  // we can add later if its needed and we can work out
  // what it actually means

  // check the first char of the command
  // G = get
  // S = Set
  // F = Do something physical
  switch (buffer[0])
  {
    case 'G': // GET
      switch (buffer[1])
      {
        case 'B':
        case 'C':
          strcpy(out, "00#");
          break;
        case 'V':
          strcpy(out, "10#");
          break;
        case 'T':
          sprintf(out, "%04X#", int(analogReadTemp() * 2));
          break;
        case 'I':
          sprintf(out, "%02X#", motor.moving);
          break;
        case 'P':
          sprintf(out, "%04X#", motor.position / motor.settings["fullsteps"]);
          break;
        case 'N':
          sprintf(out, "%04X#", motor.target / motor.settings["fullsteps"]);
          break;
        case 'H':
          sprintf(out, "%s", motor.settings["fullsteps"] == 2 ? "FF#" : "FF#");
          break;
        case 'D':
          sprintf(out, "%02d#", motor.settings["D"]);
          break;
      }
      break;
    case 'S': // SET
      switch (buffer[1])
      {
        case 'F':
          /*
              // fix partial step on odd position
              if ((motor.position & 1) == 1)
                  motor.target = (motor.position/2)*2;

              if (motor.target != motor.position)
              {
                  printf("fix %d %d\n",motor.target, motor.position);
                  motor.moving = true;
                  motor.nudge();
              }
          */
          motor.settings["fullsteps"] = 1;
          break;
        case 'H': // not supporting half steps currently
          // motor.settings["fullsteps"]=1;
          break;
        case 'N':
          sscanf(buffer, "SN%X", &v);

          if (!motor.moving)
            motor.target = v * motor.settings["fullsteps"];
            
          break;
        case 'P':
          sscanf(buffer, "SP%X", &v);

          if (!motor.moving)
            motor.position = v * motor.settings["fullsteps"];
          break;
        case 'D':
          sscanf(buffer, "SD%d", &v);
          if (!motor.moving)
          {
            motor.settings["speed"] = (motor.settings["max_speed"] * 2) / v; // gives us 40 to 4 RPM on the shaft
            motor.settings["D"] = v;
          }
          break;

      }
      break;
    case 'F': // Do stuff
      switch (buffer[1])
      {
        case 'G':
          if (!motor.moving)
          {
            motor.moving = true;
            if ( xQueueSend( xMoveQueue, ( void * ) 1, (TickType_t) 10) != pdPASS )
            {
              //Serial.printf ("failed message %s\n", buffer);
            }
          }
          break;
        case 'Q':
          motor.halt();
          break;
      }
      break;
    default:
      break;
  }

  return std::string(out);
}
