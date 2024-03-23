struct Button {
  int pin;
  bool state;
  bool changed;

  void read () {
    bool s = digitalRead(pin)==0;
    if (s!=state) {
      changed = true;
      state = s;
    }
  }

  bool hasChanged () {
    if (changed) {
      changed = false;
      return true;
    }
    return false;
  }
};