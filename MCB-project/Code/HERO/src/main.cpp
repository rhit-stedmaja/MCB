#include "tap/algorithms/smooth_pid.hpp"
#include "tap/board/board.hpp"
#include "drivers_singleton.hpp"
#include <cmath>
#include <iostream>
#include "tap/architecture/periodic_timer.hpp"
static tap::algorithms::SmoothPidConfig pid_conf_dt = { 20, 0, 0, 0, 8000, 1, 0, 1, 0, 0, 0 };
tap::algorithms::SmoothPid pidController = tap::algorithms::SmoothPid(pid_conf_dt);
tap::arch::PeriodicMilliTimer sendMotorTimeout(2);
src::Drivers *drivers;
int indexerDesiredRPM = 0;
int indexerMaxRPM = 1000;
int indexerStepSpeed = 10;
bool alreadyChanged = false;

int main() {
    src::Drivers *drivers = src::DoNotUse_getDrivers();
    Board::initialize();
    drivers->can.initialize();
    drivers->remote.initialize();
    tap::motor::DjiMotor indexer = tap::motor::DjiMotor(src::DoNotUse_getDrivers(), tap::motor::MotorId::MOTOR7, tap::can::CanBus::CAN_BUS2, true, "ID1", 0, 0);
    tap::motor::DjiMotor flywheel_one = tap::motor::DjiMotor(src::DoNotUse_getDrivers(), tap::motor::MotorId::MOTOR5, tap::can::CanBus::CAN_BUS2, true, "ID1", 0, 0);
    tap::motor::DjiMotor flywheel_two = tap::motor::DjiMotor(src::DoNotUse_getDrivers(), tap::motor::MotorId::MOTOR8, tap::can::CanBus::CAN_BUS2, true, "ID1", 0, 0);
    indexer.initialize();
    flywheel_one.initialize();
    flywheel_two.initialize();

    while (1) {
        drivers->canRxHandler.pollCanData();
        drivers->remote.read(); //Reading the remote before we check if it is connected yet or not.

        if(drivers->remote.isConnected()) { //Doing stuff for the remote every loop   
            if(drivers->remote.getSwitch(drivers->remote.Switch::LEFT_SWITCH) == drivers->remote.SwitchState::MID) { 
                //Do Nothing
                alreadyChanged = false;
      	    } else if((drivers->remote.getSwitch(drivers->remote.Switch::LEFT_SWITCH) == drivers->remote.SwitchState::UP)) { 
                //Increment the Indexer Speed
                if(!alreadyChanged) {
                    if(indexerDesiredRPM<indexerMaxRPM) {
                        indexerDesiredRPM+=indexerStepSpeed;
                    }
                    alreadyChanged = true;
                }
            } else if(drivers->remote.getSwitch(drivers->remote.Switch::LEFT_SWITCH) == drivers->remote.SwitchState::DOWN) { 
                //Decrement the Indexer
                if(!alreadyChanged) {
                    if(-indexerDesiredRPM<indexerMaxRPM) {
                        indexerDesiredRPM-=indexerStepSpeed;
                    }
                    alreadyChanged = true;
                }
            }
            if(sendMotorTimeout.execute()) {
                pidController.runControllerDerivateError(indexerDesiredRPM - indexer.getShaftRPM(), 1);
                indexer.setDesiredOutput(static_cast<int32_t>(pidController.getOutput()));
            }
        } else { //Remote not connected, so have everything turn off (Saftey features!)
            
        }
        drivers->djiMotorTxHandler.encodeAndSendCanData(); //Processes these motor speed changes into can signal

    }
    return 0;
}