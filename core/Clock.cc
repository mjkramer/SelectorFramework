#include "Clock.hh"

#include <stdexcept>

void Clock::registerWriter(Algorithm*)
{
  ++nWriters;
  if (nWriters > 1)
    throw std::runtime_error("Attempted to register multiple clock writers!");
}
