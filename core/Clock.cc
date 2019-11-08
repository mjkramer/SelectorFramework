#pragma once

#include "Kernel.cc"
#include "Util.cc"

class Clock : public Tool {
public:
  void update(Time t);
  Time current() const { return current_; }
  bool atTheEnd() const { return atTheEnd_; }
  void signalTheEnd();

private:
  Time current_;
  bool atTheEnd_ = false;
};

void Clock::update(Time t)
{
  if (t > current_)
    current_ = t;
}

void Clock::signalTheEnd()
{
  atTheEnd_ = true;
}
