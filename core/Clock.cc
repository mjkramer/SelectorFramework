#pragma once

#include "Kernel.cc"
#include "Util.cc"

class Clock : public Tool {
public:
  void update(Time t);
  Time current() const { return current_; }

private:
  Time current_;
};

void Clock::update(Time t)
{
  if (t > current_)
    current_ = t;
}
