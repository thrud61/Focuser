#include "tmc22xx.h"

//#define DEBUG 1

Motor::Motor()
{
  settings["fullsteps"] = 1;
  settings["steps"] = 1012;  // nema motor 1.8 deg
  settings["accel"] = true;
  settings["speed"] = 40;   // rpm
  settings["max_steps"] = settings["steps"] * 100; // we will wrap around at this point
  settings["position"] = 1;
  settings["limit"] = 3600;
  settings["invert"] = 0;
  settings["max_speed"] = 4;

  settings["D"] = 2;

  moving = false;

  // set them to output
  for (int pin : motor_pins)
  {
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_OUT);
  }

  // gpio 23 for end stop
  //    pinMode(23, INPUT);
  //    pullUpDnControl (23, PUD_UP);

  // set non-energised state
  halt();
}

bool Motor::is_moving()
{
  return moving;
}

void Motor::halt()
{
  if (debug)
    printf("Halt\n");
  moving = false;
  sleep_ms(100); // wait 100ms to give the move time to notice if its async
  for (int pin : motor_pins)
  {
    gpio_put(pin, 1);
  }
}

// take a single step
void Motor::step(int pos)
{
  // don't go past the limit
  if (position >= settings["limit"])
    return;

  if (debug == 2)
    printf("Step %d\n", pos);

  // check the end stop
  //    if (!digitalRead(23))
  //    {
  //        if (debug)
  //            printf("End Stop\n");
  //
  //        halt();
  //    }

  step_state = !step_state;
  gpio_put(pDir, direction < 0 ? 1 : 0);
  gpio_put(pStep, step_state);
}

void Motor::async_move(int new_pos)
{
  moving = true;
  target = new_pos;
  return async_move();
}
void Motor::async_move(void)
{
  // not like this on bare metal
  //    async_m = std::async(std::launch::async, [&](){move();});
}

void Motor::wait(void)
{
  //     async_m.wait();
}

int Motor::move(int new_pos)
{
  target = new_pos;
  return move();
}

// move 1 step, used when switching from half to full steps
// to sync to full step position
void Motor::nudge(void)
{
  int delay;

  if (debug)
    printf("Nudge\n");

  gpio_put(pEnable, 0);

  // do some stuff with the motor
  delay = (1000000 / ((settings["speed"] * (settings["steps"])) / 60));

  if (position > target)
    direction = -1;
  else
    direction = 1;

  // ensure we stay within the limits of the scope
  if ((position < settings["limit"]) && (position + direction >= 0))
  {
    step(position);
    sleep_us(delay);

    position += direction;
    position = position % settings["max_steps"];
  }

  halt();
}

// move to a specific position
int Motor::move(void)
{
  int slow = 0;
//  int delay;

  if (debug)
    printf("Move %d %d\n", position, target);

  gpio_put(pEnable, 0);

  // do some stuff with the motor
  delay = (1000000 / ((settings["speed"] * (settings["steps"] / settings["fullsteps"])) / 60)) / 2;

  if (debug)
    printf("delay %d, target, position\n", delay, target, position);

  if (settings["accel"])
    slow = 20000;

  if (position > target)
    direction = -settings["fullsteps"];
  else
    direction = settings["fullsteps"];

  // someone can stop this with moving set to false
  while ((target != position) && moving && (position + direction < settings["limit"]) && (position + direction >= 0))
  {
    if (moving)
    {
      position += direction;
      position = position % settings["max_steps"];

      if (settings["accel"])
      {
        if (abs(target - position) < 200)
        {
          slow += 100 * settings["fullsteps"];
        }
        else if (slow)
        {
          slow -= 100;
          if (slow < 0)
            slow = 0;
        }
      }
      step(position);
      sleep_us(delay);
      step(position);
      sleep_us(delay);
    }
  }
  halt();

  return position;
}
