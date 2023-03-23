// Class to drive the TMC22xx stepper driver chips, with very little tweaking
// can also be used with most stepper drivers, minor changes needed for the
// A4998 driver (or none if direction of travel is not important)
//
// Author James Wilson
// 5th October 2021
//

#include <unistd.h>
//#include <future>
#include <atomic>
#include <map>
#include <string>

#include "pico/stdlib.h"


class Motor {
public:
    int delay;
    int position = 1;
    int target = 0;
    int debug = 0;
    bool moving;
    std::map<std::string,int> settings;
//    std::future<void> async_m;


private:

    int pDir = 6;
    int pStep = 8;
    int pEnable = 5;
    
    int step_state = 0;
    int direction =0;

    int motor_pins[3] = {pDir,pStep,pEnable};

    void step(int pos);

public:
    Motor ();
    void halt();
    int move(int target);
    int move(void);
    void async_move(void);
    void nudge(void);
    void async_move(int);
    void wait(void);
    bool is_moving();
};
