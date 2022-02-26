#pragma once

class pulse_sol21
{
  public:
    pulse_sol21();
    pulse_sol21(unsigned long rising, unsigned long falling);

    enum class mode { gameplay, attract, invalid };

    unsigned long rising();
    unsigned long falling();
    unsigned long high_time();
    mode interpretation();

  private:
    unsigned long m_rising;
    unsigned long m_falling;
};
