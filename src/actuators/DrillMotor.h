#ifndef DRILL_MOTOR_H
#define DRILL_MOTOR_H

class DrillMotor {
public:
  DrillMotor();

  void begin();
  void on();
  void off();
  void setSpeed(int speed);
  int getSpeed();
  bool isRunning();

private:
  int speed;
  bool running;
};

#endif