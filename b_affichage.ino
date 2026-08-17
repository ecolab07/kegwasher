// ============================================================
// Affichage
// ============================================================

// Printf-style wrapper for the LCD: formats into a 16-character
// left-justified string and prints it at the current cursor position.
// Use this when `fmt` (or any %s argument) lives in RAM.
void lcd_printf(const char *fmt, ...)
{
  char buf1[17];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf1, sizeof(buf1), fmt, args);
  va_end(args);

  char buf2[17];
  snprintf(buf2, sizeof(buf2), "%-16s", buf1);
  lcd.print(buf2);
}

// Same as lcd_printf(), but for a format string that lives in flash
// (PROGMEM) — e.g. the fixed headers on the interrupted-cycle screen,
// or the strings returned by reset_cause_label().
void lcd_printf_P(const char *fmt, ...)
{
  char buf1[17];
  va_list args;
  va_start(args, fmt);
  vsnprintf_P(buf1, sizeof(buf1), fmt, args);
  va_end(args);

  char buf2[17];
  snprintf(buf2, sizeof(buf2), "%-16s", buf1);
  lcd.print(buf2);
}

// Returns the current time in whole seconds.
unsigned long seconds()
{
  return millis() / 1000;
}

// Returns a human-readable label for the given valve configuration.
const char* resolve_label(unsigned int config)
{
  switch(config) {
    case CONFIG_DRAIN:           return "Vidange";
    case CONFIG_DRAIN_SANITIZER: return "Vidange desinf.";
    case CONFIG_DRAIN_CLEANER:   return "Vidange deter.";
    case CONFIG_FILL_SANITIZER:  return "Rempl. desinf.";
    case CONFIG_FILL_CLEANER:    return "Rempl. deter.";
    case CONFIG_RINCE:           return "Rincage";
    case CONFIG_RINCE_PURGE:     return "Purge air";
    case CONFIG_RINCE_PURGE_CO2: return "Purge CO2";
    case CONFIG_CLEAN:           return "Detergent";
    case CONFIG_CLEAN_PURGE:     return "Purge deter.";
    case CONFIG_SANITIZE:        return "Desinfectant";
    case CONFIG_SANITIZE_PURGE:  return "Purge desinf.";
    case CONFIG_CO2:             return "CO2";
    case CTRL_WATER:             return "Eau";
    case CTRL_CLEANER_IN:        return "Deter. IN";
    case CTRL_SANITIZER_IN:      return "Desinf. IN";
    case CTRL_AIR:               return "Air";
    case CTRL_CLEANER_OUT:       return "Deterg. OUT";
    case CTRL_SANITIZER_OUT:     return "Desinf. OUT";
    case CTRL_PUMP:              return "Pompe";
    case CONFIG_WARNING:         return "Cuves vides ??";
    case CONFIG_WAIT:            return "Attente...";
    default:                     return "";
  }
}
