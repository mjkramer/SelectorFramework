#pragma once

#include "Kernel.hh"
#include "Util.hh"

class Clock : public Tool {
public:
  void update(Time t);
  Time current() const { return current_; }
  bool atTheEnd() const { return atTheEnd_; }
  void signalTheEnd() { atTheEnd_ = true; };
  void registerWriter(Algorithm*);

private:
  Time current_;
  bool atTheEnd_ = false;
  unsigned nWriters = 0;
};

inline
void Clock::update(Time t)
{
  if (t > current_)
    current_ = t;
}
