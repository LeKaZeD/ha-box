#include "buzzer.h"
#include "config.h"
#include "model/model.h"
#include <Arduino.h>

void buzzerBeep(const Model& model)
{
  if (!model.settings.sound_enabled) return;
  digitalWrite(PIN_BUZZER, HIGH);
  delay(BUZZER_BEEP_MS);
  digitalWrite(PIN_BUZZER, LOW);
}
