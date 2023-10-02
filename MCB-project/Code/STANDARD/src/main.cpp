// General library includes
#include <cmath>
#include <iostream>
#include <string>

// Drivers
#include "drivers_singleton.hpp"

// Taproot
#include "tap/algorithms/smooth_pid.hpp"
#include "tap/architecture/periodic_timer.hpp"
#include "tap/board/board.hpp"
#include "tap/motor/dji_motor.hpp"

// Our .h's
#include "DriveTrainController.h"
#include "TurretController.h"
#include "RobotController.h"

tap::arch::PeriodicMilliTimer sendDrivetrainTimeout(2);
tap::arch::PeriodicMilliTimer sendTurretTimeout(2);
tap::arch::PeriodicMilliTimer updateIMUTimeout(2);
src::Drivers *drivers;

bool KeyboardAndMouseEnabled = false;
bool doBeyblading = false;
double right_stick_vert, right_stick_horz, left_stick_vert, left_stick_horz = 0.0;
int16_t wheel_value = 0;
double angleOffset = 0.0;
bool isRightStickMid = false;
bool isLeftStickUp = false;
bool isLeftStickDown = false;
bool turretIndependent = false;
int rightSwitchValue = 1;
int leftSwitchValue = 0;

int main() {
    src::Drivers *drivers = src::DoNotUse_getDrivers();
    Board::initialize();
    drivers->can.initialize();
    drivers->remote.initialize();
    drivers->bmi088.initialize(500, 0.0, 0.0);
    drivers->bmi088.requestRecalibration();
    ThornBots::DriveTrainController *driveTrainController = new ThornBots::DriveTrainController(drivers);
    ThornBots::TurretController *turretController = new ThornBots::TurretController(drivers);
    ThornBots::RobotController *RobotController = new ThornBots::RobotController(drivers, driveTrainController, turretController);

    while (1) {
        
        modm::delay_us(10);
        drivers->canRxHandler.pollCanData();
        drivers->remote.read();  // Reading the remote before we check if it is connected yet or not.

        if (updateIMUTimeout.execute())
        {
            drivers->bmi088.periodicIMUUpdate();
            angleOffset =
                drivers->bmi088
                    .getYaw();  // TODO: Make this calculate the AngleOffset and not the raw angle.
        }                       // Stop reading from the IMU

        if (drivers->remote.isConnected()) {  // If the remote is On and connected do the following
             RobotController->update();
        } else {  // Remote not connected, so have everything turn off (Saftey features!)
            driveTrainController->stopMotors(sendDrivetrainTimeout.execute());
            turretController->stopMotors(sendTurretTimeout.execute());
        }
    }
    return 0;
}