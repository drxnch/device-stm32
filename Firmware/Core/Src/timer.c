void timerCheck() {
  if (currentState == STATE_TIMER_DONE && refresh_needed) {
        refresh_needed = 0;
        LD3_ON;
        DrawTextToScreen("Timer Done");
    }
}