#include "pulse.h"

pulse_sol21::pulse_sol21()
  : m_rising(0), m_falling(0)
{
}

pulse_sol21::pulse_sol21(unsigned long rising, unsigned long falling)
  : m_rising(rising), m_falling(falling)
{
}

unsigned long pulse_sol21::rising()
{
  return m_rising;
}

unsigned long pulse_sol21::falling()
{
  return m_falling;
}

unsigned long pulse_sol21::high_time()
{
  return m_falling - m_rising;
}

pulse_sol21::mode pulse_sol21::interpretation()
{
  if (high_time() > 40 && high_time() < 60)
  {
    return pulse_sol21::mode::gameplay;
  }
  else if (high_time() > 121 && high_time() < 141)
  {
    return pulse_sol21::mode::attract;
  }
  else
  {
    return pulse_sol21::mode::invalid;
  }
}
