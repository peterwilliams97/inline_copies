#include <windows.h>
#include "timer.h"

double Timer::_get_absolute()
{
    LARGE_INTEGER time;
    QueryPerformanceCounter(&time);
    return time.QuadPart * _freq;
}

Timer::Timer() {
    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);
    _freq = 1.0 / freq.QuadPart;
    _time0 = _get_absolute();
}

double Timer::get()
{
    return _get_absolute() - _time0;
}