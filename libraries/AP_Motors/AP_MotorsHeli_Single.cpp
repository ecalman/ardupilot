// -*- tab-width: 4; Mode: C++; c-basic-offset: 4; indent-tabs-mode: nil -*-
/*
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <AP_HAL/AP_HAL.h>
#include <RC_Channel/RC_Channel.h>
#include "AP_MotorsHeli_Single.h"
#include <GCS_MAVLink/GCS.h>

extern const AP_HAL::HAL& hal;

const AP_Param::GroupInfo AP_MotorsHeli_Single::var_info[] = {
    AP_NESTEDGROUPINFO(AP_MotorsHeli, 0),
    
    // @Param: SV1_POS
    // @DisplayName: Servo 1 Position
    // @Description: Angular location of swash servo #1
    // @Range: -180 180
    // @Units: Degrees
    // @User: Standard
    // @Increment: 1
    AP_GROUPINFO("SV1_POS", 1, AP_MotorsHeli_Single, _servo1_pos, AP_MOTORS_HELI_SINGLE_SERVO1_POS),

    // @Param: SV2_POS
    // @DisplayName: Servo 2 Position
    // @Description: Angular location of swash servo #2
    // @Range: -180 180
    // @Units: Degrees
    // @User: Standard
    // @Increment: 1
    AP_GROUPINFO("SV2_POS", 2, AP_MotorsHeli_Single, _servo2_pos, AP_MOTORS_HELI_SINGLE_SERVO2_POS),

    // @Param: SV3_POS
    // @DisplayName: Servo 3 Position
    // @Description: Angular location of swash servo #3
    // @Range: -180 180
    // @Units: Degrees
    // @User: Standard
    // @Increment: 1
    AP_GROUPINFO("SV3_POS", 3, AP_MotorsHeli_Single, _servo3_pos, AP_MOTORS_HELI_SINGLE_SERVO3_POS),
  
    // @Param: TAIL_TYPE
    // @DisplayName: Tail Type
    // @Description: Tail type selection.  Simpler yaw controller used if external gyro is selected
    // @Values: 0:Servo only,1:Servo with ExtGyro,2:DirectDrive VarPitch,3:DirectDrive FixedPitch
    // @User: Standard
    AP_GROUPINFO("TAIL_TYPE", 4, AP_MotorsHeli_Single, _tail_type, AP_MOTORS_HELI_SINGLE_TAILTYPE_SERVO),

    // @Param: SWASH_TYPE
    // @DisplayName: Swash Type
    // @Description: Swash Type Setting - either 3-servo CCPM or H1 Mechanical Mixing
    // @Values: 0:3-Servo CCPM, 1:H1 Mechanical Mixing
    // @User: Standard
    AP_GROUPINFO("SWASH_TYPE", 5, AP_MotorsHeli_Single, _swash_type, AP_MOTORS_HELI_SINGLE_SWASH_CCPM),

    // @Param: GYR_GAIN
    // @DisplayName: External Gyro Gain
    // @Description: PWM sent to external gyro on ch7 when tail type is Servo w/ ExtGyro
    // @Range: 0 1000
    // @Units: PWM
    // @Increment: 1
    // @User: Standard
    AP_GROUPINFO("GYR_GAIN", 6, AP_MotorsHeli_Single, _ext_gyro_gain_std, AP_MOTORS_HELI_SINGLE_EXT_GYRO_GAIN),

    // @Param: PHANG
    // @DisplayName: Swashplate Phase Angle Compensation
    // @Description: Phase angle correction for rotor head.  If pitching the swash forward induces a roll, this can be correct the problem
    // @Range: -90 90
    // @Units: Degrees
    // @User: Advanced
    // @Increment: 1
    AP_GROUPINFO("PHANG", 7, AP_MotorsHeli_Single, _phase_angle, 0),

    // @Param: COLYAW
    // @DisplayName: Collective-Yaw Mixing
    // @Description: Feed-forward compensation to automatically add rudder input when collective pitch is increased. Can be positive or negative depending on mechanics.
    // @Range: -10 10
    // @Increment: 0.1
    AP_GROUPINFO("COLYAW", 8,  AP_MotorsHeli_Single, _collective_yaw_effect, 0),

    // @Param: FLYBAR_MODE
    // @DisplayName: Flybar Mode Selector
    // @Description: Flybar present or not.  Affects attitude controller used during ACRO flight mode
    // @Values: 0:NoFlybar 1:Flybar
    // @User: Standard
    AP_GROUPINFO("FLYBAR_MODE", 9, AP_MotorsHeli_Single, _flybar_mode, AP_MOTORS_HELI_NOFLYBAR),
  
    // @Param: TAIL_SPEED
    // @DisplayName: Direct Drive VarPitch Tail ESC speed
    // @Description: Direct Drive VarPitch Tail ESC speed.  Only used when TailType is DirectDrive VarPitch
    // @Range: 0 1000
    // @Units: PWM
    // @Increment: 1
    // @User: Standard
    AP_GROUPINFO("TAIL_SPEED", 10, AP_MotorsHeli_Single, _direct_drive_tailspeed, AP_MOTORS_HELI_SINGLE_DDVPT_SPEED_DEFAULT),

    // @Param: GYR_GAIN_ACRO
    // @DisplayName: External Gyro Gain for ACRO
    // @Description: PWM sent to external gyro on ch7 when tail type is Servo w/ ExtGyro. A value of zero means to use H_GYR_GAIN
    // @Range: 0 1000
    // @Units: PWM
    // @Increment: 1
    // @User: Standard
    AP_GROUPINFO("GYR_GAIN_ACRO", 11, AP_MotorsHeli_Single,  _ext_gyro_gain_acro, 0),
    
    AP_GROUPEND
};

// set update rate to motors - a value in hertz
void AP_MotorsHeli_Single::set_update_rate( uint16_t speed_hz )
{
    // record requested speed
    _speed_hz = speed_hz;

    // setup fast channels
    uint32_t mask = 
        1U << AP_MOTORS_MOT_1 |
        1U << AP_MOTORS_MOT_2 |
        1U << AP_MOTORS_MOT_3 |
        1U << AP_MOTORS_MOT_4;
    hal.rcout->set_freq(mask, _speed_hz);
}

// enable - starts allowing signals to be sent to motors and servos
void AP_MotorsHeli_Single::enable()
{
    // enable output channels
    hal.rcout->enable_ch(AP_MOTORS_MOT_1);    // swash servo 1
    hal.rcout->enable_ch(AP_MOTORS_MOT_2);    // swash servo 2
    hal.rcout->enable_ch(AP_MOTORS_MOT_3);    // swash servo 3
    hal.rcout->enable_ch(AP_MOTORS_MOT_4);    // yaw
    hal.rcout->enable_ch(AP_MOTORS_HELI_SINGLE_AUX);                                 // output for gyro gain or direct drive variable pitch tail motor
    hal.rcout->enable_ch(AP_MOTORS_HELI_SINGLE_RSC);                                 // output for main rotor esc
}

// init_outputs - initialise Servo/PWM ranges and endpoints
void AP_MotorsHeli_Single::init_outputs()
{
    // reset swash servo range and endpoints
    reset_swash_servo (_swash_servo_1);
    reset_swash_servo (_swash_servo_2);
    reset_swash_servo (_swash_servo_3);

    _yaw_servo.set_angle(4500);

    // set main rotor servo range
    // tail rotor servo use range as set in vehicle code for rc7
    _main_rotor.init_servo();
}

// output_test - spin a motor at the pwm value specified
//  motor_seq is the motor's sequence number from 1 to the number of motors on the frame
//  pwm value is an actual pwm value that will be output, normally in the range of 1000 ~ 2000
void AP_MotorsHeli_Single::output_test(uint8_t motor_seq, int16_t pwm)
{
    // exit immediately if not armed
    if (!armed()) {
        return;
    }

    // output to motors and servos
    switch (motor_seq) {
        case 1:
            // swash servo 1
            rc_write(AP_MOTORS_MOT_1, pwm);
            break;
        case 2:
            // swash servo 2
            rc_write(AP_MOTORS_MOT_2, pwm);
            break;
        case 3:
            // swash servo 3
            rc_write(AP_MOTORS_MOT_3, pwm);
            break;
        case 4:
            // external gyro & tail servo
            if (_tail_type == AP_MOTORS_HELI_SINGLE_TAILTYPE_SERVO_EXTGYRO) {
                if (_acro_tail && _ext_gyro_gain_acro > 0) {
                    write_aux(_ext_gyro_gain_acro);
                } else {
                    write_aux(_ext_gyro_gain_std);
                }
            }
            rc_write(AP_MOTORS_MOT_4, pwm);
            break;
        case 5:
            // main rotor
            rc_write(AP_MOTORS_HELI_SINGLE_RSC, pwm);
            break;
        default:
            // do nothing
            break;
    }
}

// set_desired_rotor_speed
void AP_MotorsHeli_Single::set_desired_rotor_speed(int16_t desired_speed)
{
    _main_rotor.set_desired_speed(desired_speed);

    // always send desired speed to tail rotor control, will do nothing if not DDVPT not enabled
    _tail_rotor.set_desired_speed(_direct_drive_tailspeed);
}

// calculate_scalars - recalculates various scalers used.
void AP_MotorsHeli_Single::calculate_armed_scalars()
{
    _main_rotor.set_ramp_time(_rsc_ramp_time);
    _main_rotor.set_runup_time(_rsc_runup_time);
    _main_rotor.set_critical_speed(_rsc_critical);
    _main_rotor.set_idle_output(_rsc_idle_output);
    _main_rotor.set_power_output_range(_rsc_power_low, _rsc_power_high);
    _main_rotor.recalc_scalers();
}


// calculate_scalars - recalculates various scalers used.
void AP_MotorsHeli_Single::calculate_scalars()
{
    // range check collective min, max and mid
    if( _collective_min >= _collective_max ) {
        _collective_min = AP_MOTORS_HELI_COLLECTIVE_MIN;
        _collective_max = AP_MOTORS_HELI_COLLECTIVE_MAX;
    }
    _collective_mid = constrain_int16(_collective_mid, _collective_min, _collective_max);

    // calculate collective mid point as a number from 0 to 1000
    _collective_mid_pwm = ((float)(_collective_mid-_collective_min))/((float)(_collective_max-_collective_min))*1000.0f;

    // calculate maximum collective pitch range from full positive pitch to zero pitch
    _collective_range = 1000 - _collective_mid_pwm;

    // determine roll, pitch and collective input scaling
    _roll_scaler = (float)_cyclic_max/4500.0f;
    _pitch_scaler = (float)_cyclic_max/4500.0f;
    _collective_scalar = ((float)(_collective_max-_collective_min))/1000.0f;

    // calculate factors based on swash type and servo position
    calculate_roll_pitch_collective_factors();

    // send setpoints to main rotor controller and trigger recalculation of scalars
    _main_rotor.set_control_mode(static_cast<RotorControlMode>(_rsc_mode.get()));
    calculate_armed_scalars();
    
    // send setpoints to tail rotor controller and trigger recalculation of scalars
    if (_tail_type == AP_MOTORS_HELI_SINGLE_TAILTYPE_DIRECTDRIVE_VARPITCH) {
        _tail_rotor.set_control_mode(ROTOR_CONTROL_MODE_SPEED_SETPOINT);
        _tail_rotor.set_ramp_time(AP_MOTORS_HELI_SINGLE_DDVPT_RAMP_TIME);
        _tail_rotor.set_runup_time(AP_MOTORS_HELI_SINGLE_DDVPT_RUNUP_TIME);
        _tail_rotor.set_critical_speed(_rsc_critical);
        _tail_rotor.set_idle_output(_rsc_idle_output);
    } else {
        _tail_rotor.set_control_mode(ROTOR_CONTROL_MODE_DISABLED);
        _tail_rotor.set_ramp_time(0);
        _tail_rotor.set_runup_time(0);
        _tail_rotor.set_critical_speed(0);
        _tail_rotor.set_idle_output(0);
    }
    _tail_rotor.recalc_scalers();
}

// calculate_roll_pitch_collective_factors - calculate factors based on swash type and servo position
void AP_MotorsHeli_Single::calculate_roll_pitch_collective_factors()
{
    if (_swash_type == AP_MOTORS_HELI_SINGLE_SWASH_CCPM) {                     //CCPM Swashplate, perform control mixing

        // roll factors
        _rollFactor[CH_1] = cosf(radians(_servo1_pos + 90 - (_phase_angle + _delta_phase_angle)));
        _rollFactor[CH_2] = cosf(radians(_servo2_pos + 90 - (_phase_angle + _delta_phase_angle)));
        _rollFactor[CH_3] = cosf(radians(_servo3_pos + 90 - (_phase_angle + _delta_phase_angle)));

        // pitch factors
        _pitchFactor[CH_1] = cosf(radians(_servo1_pos - (_phase_angle + _delta_phase_angle)));
        _pitchFactor[CH_2] = cosf(radians(_servo2_pos - (_phase_angle + _delta_phase_angle)));
        _pitchFactor[CH_3] = cosf(radians(_servo3_pos - (_phase_angle + _delta_phase_angle)));

        // collective factors
        _collectiveFactor[CH_1] = 1;
        _collectiveFactor[CH_2] = 1;
        _collectiveFactor[CH_3] = 1;

    }else{              //H1 Swashplate, keep servo outputs seperated

        // roll factors
        _rollFactor[CH_1] = 1;
        _rollFactor[CH_2] = 0;
        _rollFactor[CH_3] = 0;

        // pitch factors
        _pitchFactor[CH_1] = 0;
        _pitchFactor[CH_2] = 1;
        _pitchFactor[CH_3] = 0;

        // collective factors
        _collectiveFactor[CH_1] = 0;
        _collectiveFactor[CH_2] = 0;
        _collectiveFactor[CH_3] = 1;
    }
}

// get_motor_mask - returns a bitmask of which outputs are being used for motors or servos (1 means being used)
//  this can be used to ensure other pwm outputs (i.e. for servos) do not conflict
uint16_t AP_MotorsHeli_Single::get_motor_mask()
{
    // heli uses channels 1,2,3,4,7 and 8
    return (1U << 0 | 1U << 1 | 1U << 2 | 1U << 3 | 1U << AP_MOTORS_HELI_SINGLE_AUX | 1U << AP_MOTORS_HELI_SINGLE_RSC);
}

// update_motor_controls - sends commands to motor controllers
void AP_MotorsHeli_Single::update_motor_control(RotorControlState state)
{
    // Send state update to motors
    _tail_rotor.output(state);
    _main_rotor.output(state);

    if (state == ROTOR_CONTROL_STOP){
        // set engine run enable aux output to not run position to kill engine when disarmed
        RC_Channel_aux::set_radio_to_min(RC_Channel_aux::k_engine_run_enable);
    } else {
        // else if armed, set engine run enable output to run position
        RC_Channel_aux::set_radio_to_max(RC_Channel_aux::k_engine_run_enable);
    }

    // Check if both rotors are run-up, tail rotor controller always returns true if not enabled
    _heliflags.rotor_runup_complete = ( _main_rotor.is_runup_complete() && _tail_rotor.is_runup_complete() );
}

// set_delta_phase_angle for setting variable phase angle compensation and force
// recalculation of collective factors
void AP_MotorsHeli_Single::set_delta_phase_angle(int16_t angle)
{
    angle = constrain_int16(angle, -90, 90);
    _delta_phase_angle = angle;
    calculate_roll_pitch_collective_factors();
}

//
// move_actuators - moves swash plate and tail rotor
//                 - expected ranges:
//                       roll : -4500 ~ 4500
//                       pitch: -4500 ~ 4500
//                       collective: 0 ~ 1000
//                       yaw:   -4500 ~ 4500
//
void AP_MotorsHeli_Single::move_actuators(int16_t roll_out, int16_t pitch_out, int16_t coll_in, int16_t yaw_out)
{
    int16_t yaw_offset = 0;
    int16_t coll_out_scaled;

    // initialize limits flag
    limit.roll_pitch = false;
    limit.yaw = false;
    limit.throttle_lower = false;
    limit.throttle_upper = false;

    // rescale roll_out and pitch_out into the min and max ranges to provide linear motion
    // across the input range instead of stopping when the input hits the constrain value
    // these calculations are based on an assumption of the user specified cyclic_max
    // coming into this equation at 4500 or less, and based on the original assumption of the
    // total _servo_x.servo_out range being -4500 to 4500.

    float total_out = pythagorous2((float)pitch_out, (float)roll_out);

    if (total_out > _cyclic_max) {
        float ratio = (float)_cyclic_max / total_out;
        roll_out *= ratio;
        pitch_out *= ratio;
        limit.roll_pitch = true;
    }

    // constrain collective input
    _collective_out = coll_in;
    if (_collective_out <= 0) {
        _collective_out = 0;
        limit.throttle_lower = true;
    }
    if (_collective_out >= 1000) {
        _collective_out = 1000;
        limit.throttle_upper = true;
    }

    // ensure not below landed/landing collective
    if (_heliflags.landing_collective && _collective_out < _land_collective_min) {
        _collective_out = _land_collective_min;
        limit.throttle_lower = true;
    }

    // scale collective pitch
    coll_out_scaled = _collective_out * _collective_scalar + _collective_min - 1000;

    // if servo output not in manual mode, process pre-compensation factors
    if (_servo_mode == SERVO_CONTROL_MODE_AUTOMATED) {
        // rudder feed forward based on collective
        // the feed-forward is not required when the motor is stopped or at idle, and thus not creating torque
        // also not required if we are using external gyro
        if ((_main_rotor.get_control_output() > _rsc_idle_output) && _tail_type != AP_MOTORS_HELI_SINGLE_TAILTYPE_SERVO_EXTGYRO) {
            // sanity check collective_yaw_effect
            _collective_yaw_effect = constrain_float(_collective_yaw_effect, -AP_MOTORS_HELI_SINGLE_COLYAW_RANGE, AP_MOTORS_HELI_SINGLE_COLYAW_RANGE);
            yaw_offset = _collective_yaw_effect * abs(_collective_out - _collective_mid_pwm);
        }
    } else {
        yaw_offset = 0;  
    }

    // feed power estimate into main rotor controller
    // ToDo: include tail rotor power?
    // ToDo: add main rotor cyclic power?
    _main_rotor_power = ((float)(abs(_collective_out - _collective_mid_pwm)) / _collective_range);
    _main_rotor.set_motor_load(_main_rotor_power);

    // swashplate servos
    _swash_servo_1.servo_out = (_rollFactor[CH_1] * roll_out + _pitchFactor[CH_1] * pitch_out)/10 + _collectiveFactor[CH_1] * coll_out_scaled + (_swash_servo_1.radio_trim-1500);
    _swash_servo_2.servo_out = (_rollFactor[CH_2] * roll_out + _pitchFactor[CH_2] * pitch_out)/10 + _collectiveFactor[CH_2] * coll_out_scaled + (_swash_servo_2.radio_trim-1500);
    if (_swash_type == AP_MOTORS_HELI_SINGLE_SWASH_H1) {
        _swash_servo_1.servo_out += 500;
        _swash_servo_2.servo_out += 500;
    }
    _swash_servo_3.servo_out = (_rollFactor[CH_3] * roll_out + _pitchFactor[CH_3] * pitch_out)/10 + _collectiveFactor[CH_3] * coll_out_scaled + (_swash_servo_3.radio_trim-1500);

    // use servo_out to calculate pwm_out and radio_out
    _swash_servo_1.calc_pwm();
    _swash_servo_2.calc_pwm();
    _swash_servo_3.calc_pwm();

    hal.rcout->cork();

    // actually move the servos
    rc_write(AP_MOTORS_MOT_1, _swash_servo_1.radio_out);
    rc_write(AP_MOTORS_MOT_2, _swash_servo_2.radio_out);
    rc_write(AP_MOTORS_MOT_3, _swash_servo_3.radio_out);

    // update the yaw rate using the tail rotor/servo
    move_yaw(yaw_out + yaw_offset);

    hal.rcout->push();
}

// move_yaw
void AP_MotorsHeli_Single::move_yaw(int16_t yaw_out)
{
    _yaw_servo.servo_out = constrain_int16(yaw_out, -4500, 4500);

    if (_yaw_servo.servo_out != yaw_out) {
        limit.yaw = true;
    }

    _yaw_servo.calc_pwm();

    rc_write(AP_MOTORS_MOT_4, _yaw_servo.radio_out);

    if (_tail_type == AP_MOTORS_HELI_SINGLE_TAILTYPE_SERVO_EXTGYRO) {
        // output gain to exernal gyro
        if (_acro_tail && _ext_gyro_gain_acro > 0) {
            write_aux(_ext_gyro_gain_acro);
        } else {
            write_aux(_ext_gyro_gain_std);
        }
    } else if (_tail_type == AP_MOTORS_HELI_SINGLE_TAILTYPE_DIRECTDRIVE_FIXEDPITCH && _main_rotor.get_desired_speed() > 0) {
        // output yaw servo to tail rsc
        write_aux(_yaw_servo.servo_out);
    }
}

// write_aux - outputs pwm onto output aux channel (ch7)
// servo_out parameter is of the range 0 ~ 1000
void AP_MotorsHeli_Single::write_aux(int16_t servo_out)
{
    _servo_aux.servo_out = servo_out;
    _servo_aux.calc_pwm();
    rc_write(AP_MOTORS_HELI_SINGLE_AUX, _servo_aux.radio_out);
}

// servo_test - move servos through full range of movement
void AP_MotorsHeli_Single::servo_test()
{
    _servo_test_cycle_time += 1.0f / _loop_rate;

    if ((_servo_test_cycle_time >= 0.0f && _servo_test_cycle_time < 0.5f)||                                   // Tilt swash back
        (_servo_test_cycle_time >= 6.0f && _servo_test_cycle_time < 6.5f)){
        _pitch_test += (4500 / (_loop_rate/2));
        _oscillate_angle += 8 * M_PI_F / _loop_rate;
        _yaw_test = 2250 * sinf(_oscillate_angle);
    } else if ((_servo_test_cycle_time >= 0.5f && _servo_test_cycle_time < 4.5f)||                            // Roll swash around
               (_servo_test_cycle_time >= 6.5f && _servo_test_cycle_time < 10.5f)){
        _oscillate_angle += M_PI_F / (2 * _loop_rate);
        _roll_test = 4500 * sinf(_oscillate_angle);
        _pitch_test = 4500 * cosf(_oscillate_angle);
        _yaw_test = 4500 * sinf(_oscillate_angle);
    } else if ((_servo_test_cycle_time >= 4.5f && _servo_test_cycle_time < 5.0f)||                            // Return swash to level
               (_servo_test_cycle_time >= 10.5f && _servo_test_cycle_time < 11.0f)){
        _pitch_test -= (4500 / (_loop_rate/2));
        _oscillate_angle += 8 * M_PI_F / _loop_rate;
        _yaw_test = 2250 * sinf(_oscillate_angle);
    } else if (_servo_test_cycle_time >= 5.0f && _servo_test_cycle_time < 6.0f){                              // Raise swash to top
        _collective_test += (1000 / _loop_rate);
        _oscillate_angle += 2 * M_PI_F / _loop_rate;
        _yaw_test = 4500 * sinf(_oscillate_angle);
    } else if (_servo_test_cycle_time >= 11.0f && _servo_test_cycle_time < 12.0f){                            // Lower swash to bottom
        _collective_test -= (1000 / _loop_rate);
        _oscillate_angle += 2 * M_PI_F / _loop_rate;
        _yaw_test = 4500 * sinf(_oscillate_angle);
    } else {                                                                                                  // reset cycle
        _servo_test_cycle_time = 0.0f;
        _oscillate_angle = 0.0f;
        _collective_test = 0.0f;
        _roll_test = 0.0f;
        _pitch_test = 0.0f;
        _yaw_test = 0.0f;
        // decrement servo test cycle counter at the end of the cycle
        if (_servo_test_cycle_counter > 0){
            _servo_test_cycle_counter--;
        }
    }

    // over-ride servo commands to move servos through defined ranges
    _throttle_control_input = _collective_test;
    _roll_control_input = _roll_test;
    _pitch_control_input = _pitch_test;
    _yaw_control_input = _yaw_test;
}

// parameter_check - check if helicopter specific parameters are sensible
bool AP_MotorsHeli_Single::parameter_check(bool display_msg) const
{
    // returns false if Phase Angle is outside of range 
    if ((_phase_angle > 90) || (_phase_angle < -90)){
        if (display_msg) {
            GCS_MAVLINK::send_statustext_all(MAV_SEVERITY_CRITICAL, "PreArm: H_PHANG out of range");
        }
        return false;
    }

    // returns false if Acro External Gyro Gain is outside of range
    if ((_ext_gyro_gain_acro < 0) || (_ext_gyro_gain_acro > 1000)){
        if (display_msg) {
            GCS_MAVLINK::send_statustext_all(MAV_SEVERITY_CRITICAL, "PreArm: H_GYR_GAIN_ACRO out of range");
        }
        return false;
    }

    // returns false if Standard External Gyro Gain is outside of range
    if ((_ext_gyro_gain_std < 0) || (_ext_gyro_gain_std > 1000)){
        if (display_msg) {
            GCS_MAVLINK::send_statustext_all(MAV_SEVERITY_CRITICAL, "PreArm: H_GYR_GAIN out of range");
        }
        return false;
    }

    // check parent class parameters
    return AP_MotorsHeli::parameter_check(display_msg);
}
