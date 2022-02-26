#include <FastLED.h>
#include "gate.h"
#include "pulse.h"

#define NUM_LEDS  96
#define FIRST_LED 12
#define LAST_LED  86
#define GATE_LEDS ((LAST_LED-FIRST_LED)+1)
static_assert(1 <= FIRST_LED && FIRST_LED <= NUM_LEDS, "First visible LED must be defined 1 <= FIRST_LED <= NUM_LEDS");
static_assert(1 <= LAST_LED  && LAST_LED  <= NUM_LEDS, "Last visible LED must be defined 1 <= LAST_LED <= NUM_LEDS");
static_assert(FIRST_LED <= LAST_LED, "Gate LEDs must be defined FIRST_LED <= LAST_LED");

#define DATA_PIN 3
#define TRIGGER_PIN 4
//#define TEST_PIN 7

#define NUM_CHEVRONS 7
#define NUM_CHEVRONS_TO_LOCK (sizeof(gate::chevron_sequence)/sizeof(chevron_sequence[0]))

CRGB leds[NUM_LEDS];

volatile unsigned long gate::pulseRisingEdge = millis();
bool gate::got_pulseRisingEdge = false;
volatile unsigned long gate::pulseFallingEdge = millis();
volatile bool gate::newPulseAvailable = false;

const uint8_t gate::led_chevron_position[] = {
  (uint8_t)((FIRST_LED - 1) + (1 / 14.0) * (GATE_LEDS - 1)),
  (uint8_t)((FIRST_LED - 1) + (3 / 14.0) * (GATE_LEDS - 1)),
  (uint8_t)((FIRST_LED - 1) + (5 / 14.0) * (GATE_LEDS - 1)),
  (uint8_t)((FIRST_LED - 1) + (7 / 14.0) * (GATE_LEDS - 1)),
  (uint8_t)((FIRST_LED - 1) + (9 / 14.0) * (GATE_LEDS - 1)),
  (uint8_t)((FIRST_LED - 1) + (11 / 14.0) * (GATE_LEDS - 1)),
  (uint8_t)((FIRST_LED - 1) + (13 / 14.0) * (GATE_LEDS - 1)),
};

const uint8_t gate::chevron_sequence[] = {4, 5, 6, 0, 1, 2, 3};

static void fadeall(uint8_t n)
{
  for (int i = 0; i < NUM_LEDS; i++)
  {
    leds[i].nscale8(n);
  }
}

gate::gate()
  : dial_position(0), spin_direction(direction::CLOCKWISE), next_chevron_to_lock(0)
{
}

void gate::setup()
{
  LEDS.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);
  LEDS.setBrightness(255);

  pinMode(TRIGGER_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(TRIGGER_PIN), gate::isr, CHANGE);

#if defined(TEST_PIN)
  pinMode(TEST_PIN, INPUT_PULLUP);
#endif

  // Print chevron locations spaced between 0 and NUM_LEDS
  Serial.print("Chevrons: ");
  for (int i = 1; i < NUM_CHEVRONS; i++) {
    Serial.print(get_led_chevron(i));
    Serial.print(" ");
  }
  Serial.print("\n");
}

void gate::isr()
{
  // TRIGGER_PIN is connected by opto-isolator to SOL21 (open drain, ground)
  // SOL21 is asserted normally, eg. ring is normally illuminated
  // During attract mode the effect is 262ms period (50% duty cycle)
  // During game-play the effect is 100ms period (50% duty cycle)

  if (digitalRead(TRIGGER_PIN) == HIGH)
  {
    // set newPulseAvailable = false?
    pulseRisingEdge = millis();
    got_pulseRisingEdge = true;
  }
  else if (! got_pulseRisingEdge)
  {
    // Ignore spurious falling edge
  }
  else
  {
    pulseFallingEdge = millis();
    pulse_sol21 pulse(pulseRisingEdge, pulseFallingEdge);

    switch (pulse.interpretation())
    {
    case pulse_sol21::mode::gameplay:
    case pulse_sol21::mode::attract:
      newPulseAvailable = true;
      Serial.print("sol21 pulse width ");
      Serial.print(pulse.high_time());
      Serial.print(" ms (Begin: ");
      Serial.print(pulse.rising());
      Serial.print(" End: ");
      Serial.print(pulse.falling());
      Serial.print(")\n");
      break;

    case pulse_sol21::mode::invalid:
      Serial.print("Ignoring invalid pulse: \n");
      Serial.print(pulse.high_time());
      Serial.print(" ms\n");
      break;

    default:
      Serial.print("Ignoring unrecognized pulse type\n");
      break;
    }

    got_pulseRisingEdge = false;
  }
}

bool gate::is_triggered()
{
#if defined(TEST_PIN)
  // If button held, the gate is triggered
  if (digitalRead(TEST_PIN) == LOW)
  {
    return true;
  }
#endif

  // Avoid overflow by clearing the buffer after 3 seconds of no pulses
  // This function must be called at least once every 70 minutes to work properly
  if (!pulses.isEmpty() && (millis() - pulses.first().falling() > 3000))
  {
    Serial.print(millis());
    Serial.print(" - ");
    Serial.print(pulses.first().falling());
    Serial.print(" = ");
    Serial.print(millis() - pulses.first().falling());
    Serial.print(" elapsed since pulse\n");
    Serial.print("Clearing pulses at due to 3s timer expiration\n");
    pulses.clear();
  }

  // Record any new pulse
  if (newPulseAvailable)
  {
    pulse_sol21 pulse(pulseRisingEdge, pulseFallingEdge);

    //noInterrupts();
    pulses.push(pulse);
    newPulseAvailable = false;
    pulseRisingEdge = millis();   // clear for safety?
    pulseFallingEdge = millis();  // clear for safety?
    //interrupts();
  }

  // During attract mode the effect is 262ms period (50% duty cycle)
  // During game-play the effect is 100ms period (50% duty cycle)
  //
  // Debounce, require:
  //  - 3 pulses
  //  - TODO: they're all valid and same interpretation
  //  - it's still pulsing (least-recent is within 262ms * 4 = 1048ms)
  //
  // TODO: return interpretation so caller can react differently
  //
  if (pulses.isFull() && (millis() - pulses.last().falling() < 1048))
  {
    return true;
  }
  else
  {
    return false;
  }
}

uint8_t gate::get_led_chevron(uint8_t n)
{
  return led_chevron_position[n];
}

void gate::self_test()
{
  // Dim everything
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB::Black;
  }

  // Illuminate the gate
  for (int i = FIRST_LED - 1; i < LAST_LED - 1; i++) {
    leds[i] = CRGB::White;
  }

  // Illuminate the chevrons
  for (int i = 0; i < NUM_CHEVRONS_TO_LOCK; i++) {
    const uint8_t led_of_chevron = led_chevron_position[i];
    leds[led_of_chevron] = CRGB::Red;
  }

  FastLED.show();
  delay(1000);
}

void gate::dial()
{
  next_chevron_to_lock = 0;
  dial_position = 0;
  spin_direction = direction::CLOCKWISE;

  // Debug info
  Serial.print("Dialing Abydos\n");
  //Serial.print("Next chevron to lock: ");
  //Serial.print(next_chevron_to_lock);
  //Serial.print("\n");

  for (int i = 0; i < NUM_CHEVRONS_TO_LOCK; i++)
  {
    // Debug info
    //Serial.print("Hunting chevron ");
    //Serial.print(chevron_sequence[next_chevron_to_lock]);
    //Serial.print(" at LED ");
    //Serial.print(get_led_chevron(next_chevron_to_lock));
    //Serial.print("\n");

    do
    {
      // just ensure we're processing 
      is_triggered();

      // Show the current gate location
      draw_current_dialing_status();

      // Rotate the gate
      move_dial_position();
    } while (dial_is_on_chevron() == false);

    if (i < NUM_CHEVRONS_TO_LOCK - 1)
    {
      Serial.print("Chevron ");
      Serial.print(i + 1);
      Serial.print(" encoded\n");
    }
    else if (is_triggered())
    {
      Serial.print("Chevron ");
      Serial.print(i + 1);
      Serial.print(" is locked\n");
    }
    else
    {
      Serial.print("Chevron ");
      Serial.print(i + 1);
      Serial.print(" will not engage\n");
    }

    lock_chevron();
    change_dial_spin_direction();
  }
}

void gate::draw_event_horizon()
{
  uint32_t t_start = millis();
  do
  {
    // just ensure we're processing 
    is_triggered();
    
    Serial.print("Sparkling: ");
    for (int i = 0; i < 1; i++)
    {
      uint8_t led_to_sparkle = random8(GATE_LEDS - 1) + FIRST_LED - 1;
      Serial.print(led_to_sparkle);
      Serial.print(" ");
      leds[led_to_sparkle] = CRGB::White;
    }
    Serial.print("\n");

    // Draw any locked chevrons
    for (int i = 0; i < next_chevron_to_lock; i++)
    {
      const uint8_t chevron = chevron_sequence[i];
      const uint8_t led_of_chevron = led_chevron_position[chevron];
      leds[led_of_chevron + 1] = CRGB::Yellow;
      leds[led_of_chevron] = CRGB::Red;
      leds[led_of_chevron - 1] = CRGB::Yellow;
    }

    FastLED.show();
    delay(5);

    // now that we've shown the leds, fade the event horizon
    fadeall(250);
  } while (millis() - t_start < 1500);
}

void gate::draw_collapsing_gate()
{
  // TODO: Refactor this to a fixed time and
  unsigned long duration = 1250;
  unsigned long t_start = millis();

  const long on_time[] = {
    map(1, 0, 14, 0, duration),
    map(3, 0, 14, 0, duration),
    map(5, 0, 14, 0, duration),
    map(7, 0, 14, 0, duration),
    map(9, 0, 14, 0, duration),
    map(11, 0, 14, 0, duration),
    map(13, 0, 14, 0, duration),
  };

  do
  {
    // Draw any locked chevrons
    for (int i = 0; i < NUM_CHEVRONS_TO_LOCK; i++)
    {
      const uint8_t chevron = chevron_sequence[i];
      const uint8_t led_of_chevron = led_chevron_position[chevron];

      if (millis() - t_start < on_time[i])
      {
        leds[led_of_chevron + 1] = CRGB::Yellow;
        leds[led_of_chevron] = CRGB::Red;
        leds[led_of_chevron - 1] = CRGB::Yellow;
      }
      else
      {
        leds[led_of_chevron + 1] = CRGB::Black;
        leds[led_of_chevron] = CRGB::Black;
        leds[led_of_chevron - 1] = CRGB::Black;
      }
    }

    FastLED.show();
    fadeall(250);
    delay(10);
  } while (millis() - t_start < duration);
}

void gate::draw_gate_closed()
{
  // Dim everything
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB::Black;
  }

  // Illuminate the gate faintly
  for (int i = FIRST_LED - 1; i < LAST_LED - 1; i++) {
    leds[i] = CRGB::DarkSlateGray;
  }

  // Darken the chevrons
  for (int i = 0; i < NUM_CHEVRONS_TO_LOCK; i++) {
    const uint8_t led_of_chevron = led_chevron_position[i];
    leds[led_of_chevron + 1] = CRGB::Black;
    leds[led_of_chevron] = CRGB::Black;
    leds[led_of_chevron - 1] = CRGB::Black;
  }
  FastLED.show();
  delay(10);
}

void gate::draw_gate_open()
{
  //Serial.print("Sparkling: ");
  for (int i = 0; i < 1; i++)
  {
    uint8_t led_to_sparkle = random8(GATE_LEDS - 1) + FIRST_LED - 1;
    //Serial.print(led_to_sparkle);
    //Serial.print(" ");
    leds[led_to_sparkle] = CRGB::Blue;
  }
  //Serial.print("\n");

  // Draw any locked chevrons
  for (int i = 0; i < next_chevron_to_lock; i++)
  {
    const uint8_t chevron = chevron_sequence[i];
    const uint8_t led_of_chevron = led_chevron_position[chevron];
    leds[led_of_chevron + 1] = CRGB::Yellow;
    leds[led_of_chevron] = CRGB::Red;
    leds[led_of_chevron - 1] = CRGB::Yellow;
  }

  FastLED.show();
  delay(5);

  // now that we've shown the leds, fade the event horizon
  fadeall(250);
}

void gate::draw_current_dialing_status()
{
  // Dim everything
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB::Black;
  }

  // Illuminate the gate faintly
  for (int i = FIRST_LED - 1; i < LAST_LED - 1; i++) {
    leds[i] = CRGB::DarkSlateGray;
  }

  // Darken the chevrons
  for (int i = 0; i < NUM_CHEVRONS_TO_LOCK; i++) {
    const uint8_t led_of_chevron = led_chevron_position[i];
    leds[led_of_chevron + 1] = CRGB::Black;
    leds[led_of_chevron] = CRGB::Black;
    leds[led_of_chevron - 1] = CRGB::Black;
  }

  // Draw the current gate location
  if (next_chevron_to_lock < NUM_CHEVRONS_TO_LOCK)
  {
    static uint8_t previous_position = get_dial_position_led();
    leds[previous_position] = CRGB::DarkRed;
    leds[get_dial_position_led()] = CRGB::Red;
    previous_position = get_dial_position_led();
  }

  // Draw any locked chevrons
  for (int i = 0; i < next_chevron_to_lock; i++)
  {
    const uint8_t chevron = chevron_sequence[i];
    const uint8_t led_of_chevron = led_chevron_position[chevron];
    leds[led_of_chevron + 1] = CRGB::Yellow;
    leds[led_of_chevron] = CRGB::Red;
    leds[led_of_chevron - 1] = CRGB::Yellow;
  }

  FastLED.show();

  // now that we've shown the leds, reset the i'th led to black
  delay(8);
}

uint8_t gate::get_dial_position_led()
{
  return dial_position + FIRST_LED - 1;
}

void gate::move_dial_position()
{
  // this calculates the position 0..GATE_LEDS-1
  // callers should use get_dial_position_led() to include the offset

  if (spin_direction == direction::CLOCKWISE) {
    // rollover back to zero
    dial_position = (dial_position + 1) % GATE_LEDS;
  }
  else {
    // rollover back to NUM_LEDS
    dial_position = (dial_position + (GATE_LEDS - 1)) % GATE_LEDS;
  }
}

bool gate::dial_is_on_chevron()
{
  static uint8_t cnt = 0;

  // Bail if we've already locked all chevrons
  if (next_chevron_to_lock >= sizeof(chevron_sequence) / sizeof(chevron_sequence[0]))
  {
    return false;
  }

  // Force the gate to make at least half a rotation
  cnt++;
  if (cnt < GATE_LEDS / 2)
    return false;

  const uint8_t chevron = chevron_sequence[next_chevron_to_lock];
  const uint8_t led_of_chevron = led_chevron_position[chevron];

  if (get_dial_position_led() == led_of_chevron)
  {
    cnt = 0;
    return true;
  }

  return false;
}

void gate::change_dial_spin_direction()
{
  if (spin_direction == direction::CLOCKWISE)
    spin_direction = direction::COUNTERCLOCKWISE;
  else
    spin_direction = direction::CLOCKWISE;
}

void gate::lock_chevron()
{
  if (next_chevron_to_lock < NUM_CHEVRONS_TO_LOCK)
    next_chevron_to_lock++;
}
