#include <FastLED.h>
#include "gate.h"
#include "pulse.h"

gate g;

void setup() {
  unsigned long t_start = millis();
  while (!Serial && t_start - millis() < 1000);
  //while (!Serial);
  Serial.begin(115200);
  Serial.println("resetting");

  // Setup the WS2812B LED Rope
  // Attach the SOL21 interrupt
  g.setup();

  // Briefly illuminate the visible portion and chevron positions
  // Note: This will draw 1.2 A briefly
  g.self_test();
}

void loop() {
  if (g.is_triggered())
  {
    // TODO: use attract/gameplay to control if dialing will succeed or fail
    // instead of checking if it's still triggered
    g.dial();

    if (g.is_triggered())
    {
      g.draw_event_horizon();

      // TODO: Verify theoretical maximum 38 minute wormhole open
      while (g.is_triggered()) {
        g.draw_gate_open();
      }
      g.draw_collapsing_gate();
    }
    else
    {
      g.draw_event_horizon();
      g.draw_collapsing_gate();
    }
  }
  else
  {
    g.draw_gate_closed();
  }
}
