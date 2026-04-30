#ifndef SEEDER_MOTOR_H
#define SEEDER_MOTOR_H

class SeederMotor {
public:
  SeederMotor();

  void begin();
  void rotateSteps(int steps);
  void release();

private:
  int stepIndex;

  void setStep(int a, int b, int c, int d);
};

#endif