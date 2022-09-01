#include "Unit.h"

using namespace std::chrono;

/**
 * Valid triggers:
 * - Fixed time: `at 3:30pm`
 * - Interval time: `every 10 {seconds, minutes, hours, days, weeks, months} [at 10:00am]`
 * @param previous
 * @return
 */
time_point<system_clock> Unit::NextTrigger(time_point<system_clock> previous)
{

}
