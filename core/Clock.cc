#pragma once

#include "Kernel.cc"
#include "Util.cc"

#include <stdexcept>

class Clock : public Tool {
public:
  void update(Time t);
  Time current() const { return current_; }
  bool atTheEnd() const { return atTheEnd_; }
  void signalTheEnd();
  void registerWriter(Algorithm*);

private:
  Time current_;
  bool atTheEnd_ = false;
  unsigned nWriters = 0;
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

void Clock::registerWriter(Algorithm*)
{
  ++nWriters;
  if (nWriters > 1)
    throw std::runtime_error("Attempted to register multiple clock writers!");
}
