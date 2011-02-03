/*! @file NUSensorsData.h
    @brief Declaration of a sensor data storage class to store sensor data in a platform independent way
    @author Jason Kulk
 
    @class NUSensorsData
    @brief A sensor class to store sensor data in a platform independent way
 
    @author Jason Kulk
 
  Copyright (c) 2009 Jason Kulk
 
    This file is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This file is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with NUbot.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef NUSENSORSDATA_H
#define NUSENSORSDATA_H

#include "sensor_t.h"
#include "Infrastructure/NUData.h"

#include <vector>
#include <string>
#include "Tools/Math/Matrix.h"
#include "Tools/FileFormats/TimestampedData.h"
using namespace std;

class NUSensorsData: public NUData, TimestampedData
{
public:
    // button ids
    static id_t MainButton;
    static id_t SecondaryButton;
    static id_t AllButtons;
public:
    NUSensorsData();
    ~NUSensorsData();
    
    // Get methods for a single joint at a time
    bool getJointPosition(id_t jointid, float& position);
    bool getJointVelocity(id_t jointid, float& velocity);
    bool getJointAcceleration(id_t jointid, float& acceleration);
    bool getJointTarget(id_t jointid, float& target);
    bool getJointStiffness(id_t jointid, float& stiffness);
    bool getJointCurrent(id_t jointid, float& current);
    bool getJointTorque(id_t jointid, float& torque);
    bool getJointTemperature(id_t jointid, float& temperature);
    
    // Get methods for a limb of joints (the limb can also be body and all)
    int getNumberOfJoints(id_t partid);
    bool getJointPositions(id_t bodypart, vector<float>& positions);
    bool getJointVelocities(id_t bodypart, vector<float>& velocities);
    bool getJointAccelerations(id_t bodypart, vector<float>& accelerations);
    bool getJointTargets(id_t bodypart, vector<float>& targets);
    bool getJointStiffnesses(id_t bodypart, vector<float>& stiffnesses);
    bool getJointCurrents(id_t bodypart, vector<float>& currents);
    bool getJointTorques(id_t bodypart, vector<float>& torques);
    bool getJointTemperatures(id_t bodypart, vector<float>& temperatures);
    bool getJointNames(id_t bodypart, vector<string>& names);
    
    // Get methods for soft proprioception
    bool getLeftLegTransform(Matrix& value);
    bool getRightLegTransform(Matrix& value);
    bool getSupportLegTransform(Matrix& value);
    bool getCameraTransform(Matrix& value);
    bool getCameraToGroundTransform(Matrix& value);

    bool getOdometry(float& time, vector<float>& values);
    bool getCameraHeight(float& height);
    
    // Get methods for the other sensors
    bool getAccelerometerValues(vector<float>& values);
    bool getGyroValues(vector<float>& values);
    bool getGyroOffsetValues(vector<float>& values);
    bool getGyroFilteredValues(vector<float>& values);
    bool getOrientation(vector<float>& values);
    bool getOrientationHardware(vector<float>& values);
    bool getHorizon(vector<float>& values);
    bool getButtonTriggers(vector<float>& values);
    bool getZMP(vector<float>& values);
    bool getFalling(vector<float>& values);
    bool getFallen(vector<float>& values);
    bool getDistanceLeftValues(vector<float>& values);
    bool getDistanceRightValues(vector<float>& values);
    bool getBatteryValues(vector<float>& values);
    bool getGPSValues(vector<float>& values);
    bool getCompassValues(vector<float>& values);
    
    // Get methods for foot pressure sensors
    bool getFootSoleValues(id_t footid, vector<float>& values);
    bool getFootBumperValues(id_t footid, vector<float>& values);
    bool getFootCoP(id_t footid, float& x, float& y);
    bool getFootForce(id_t footid, float& force);
    bool getFootContact(id_t footid, bool& contact);
    bool getFootSupport(id_t footid, bool& support);
    bool getButtonValues(id_t buttonid, vector<float>& values);
    
    // Common sub-get methods
    bool isFalling();
    bool isFallen();
    bool isOnGround();
    bool isIncapacitated();
    bool footImpact(id_t footid, float& time);
    
    // Motion sensor get methods
    bool getMotionFallActive(bool& active);
    bool getMotionGetupActive(bool& active);
    bool getMotionWalkActive(bool& active);
    bool getMotionKickActive(bool& active);
    bool getMotionSaveActive(bool& active);
    bool getMotionScriptActive(bool& active);
    bool getMotionWalkSpeed(vector<float>& speed);
    bool getMotionWalkMaxSpeed(vector<float>& speed);
    bool getMotionHeadCompletionTime(double& time);
    
    void setAvailableJoints(const vector<string>& joints);
    
    // Set methods for joints
    void setJointPositions(double time, const vector<float>& data, bool iscalculated = false);
    void setJointVelocities(double time, const vector<float>& data, bool iscalculated = false);
    void setJointAccelerations(double time, const vector<float>& data, bool iscalculated = false);
    void setJointTargets(double time, const vector<float>& data, bool iscalculated = false);
    void setJointStiffnesses(double time, const vector<float>& data, bool iscalculated = false);
    void setJointCurrents(double time, const vector<float>& data, bool iscalculated = false);
    void setJointTorques(double time, const vector<float>& data, bool iscalculated = false);
    void setJointTemperatures(double time, const vector<float>& data, bool iscalculated = false);
    
    // Set methods for other sensors
    void setBalanceAccelerometer(double time, const vector<float>& data, bool iscalculated = false);
    void setBalanceGyro(double time, const vector<float>& data, bool iscalculated = false);
    void setBalanceOrientationHardware(double time, const vector<float>& data, bool iscalculated = false);
    void setDistanceLeftValues(double time, const vector<float>& data, bool iscalculated = false);
    void setDistanceRightValues(double time, const vector<float>& data, bool iscalculated = false);
    void setFootSoleValues(double time, const vector<float>& data, bool iscalculated = false);
    void setFootBumperValues(double time, const vector<float>& data, bool iscalculated = false);
    void setButtonValues(double time, const vector<float>& data, bool iscalculated = false);
    void setBatteryValues(double time, const vector<float>& data, bool iscalculated = false);
    void setGPSValues(double time, const vector<float>& data, bool iscalculated = false);
    void setCompassValues(double time, const vector<float>& data, bool iscalculated = false);
    
    // Set methods for motion 'sensors'
    void setMotionFallActive(double time, bool active);
    void setMotionGetupActive(double time, bool active);
    void setMotionWalkActive(double time, bool active);
    void setMotionKickActive(double time, bool active);
    void setMotionSaveActive(double time, bool active);
    void setMotionScriptActive(double time, bool active);
    void setMotionWalkSpeed(double time, vector<float>& speed);
    void setMotionWalkMaxSpeed(double time, vector<float>& speed);
    void setMotionHeadCompletionTime(double time, double completiontime);
    
    void summaryTo(ostream& output) const;
    void csvTo(ostream& output);
    
    friend ostream& operator<< (ostream& output, const NUSensorsData& p_sensor);
    friend istream& operator>> (istream& input, NUSensorsData& p_sensor);
    
    int size() const;
    double GetTimestamp() const {return CurrentTime;};
private:
    void addSensor(sensor_t*& p_sensor, string sensorname, sensor_t::sensor_id_t sensorid);
    void addSoftSensor(sensor_t*& p_sensor, string sensorname, sensor_t::sensor_id_t sensorid);
    
    bool getJointData(sensor_t* p_sensor, id_t jointid, float& data);
    bool getJointsData(sensor_t* p_sensor, id_t bodypartid, vector<float>& data);
    
    void setData(sensor_t* p_sensor, double time, const vector<float>& data, bool iscalculated = false);
    
    void updateNamedSensorPointer(sensor_t* p_sensor);
public:
    double CurrentTime;                         //!< stores the most recent time sensors were updated in milliseconds
    // NAMED SENSORS
    // Proprioception Sensors:
    sensor_t* JointPositions;                   //!< stores the joint position sensors (in radians)
    sensor_t* JointVelocities;                  //!< stores the joint velocity sensors (in rad/s)
    sensor_t* JointAccelerations;               //!< stores the joint acceleration sensors (in rad/s/s)
    sensor_t* JointTargets;                     //!< stores the joint position targets (in radians)
    sensor_t* JointStiffnesses;                 //!< stores the joint stiffness values (as a percent)
    sensor_t* JointCurrents;                    //!< stores the joint motor current sensors (in A)
    sensor_t* JointTorques;                     //!< stores the joint torques (in Nm)
    sensor_t* JointTemperatures;                //!< stores the joint temperatures (in degrees C)
    
    sensor_t* LeftLegTransform;                 //!< stores the transform matrix from origin to the left leg
    sensor_t* RightLegTransform;                //!< stores the transform matrix from origin to the right leg
    sensor_t* SupportLegTransform;              //!< stores the transform matrix from origin to the support leg
    sensor_t* CameraTransform;                  //!< stores the transform matrix from origin to the camera
    sensor_t* CameraToGroundTransform;          //!< stores the transform matrix from the camera to the ground

    sensor_t* Odometry;                         //!< stores the movement in the [x (cm), y (cm),yaw (rad)] calculated from proprioception
    sensor_t* CameraHeight;                     //!< stores the height of the camera from the ground (cm) calculate from proprioception
    
    // Balance Sensors:
    sensor_t* BalanceAccelerometer;             //!< stores the sensor measurements for the linear acceleration of the torso in cm/s/s
    sensor_t* BalanceGyro;                      //!< stores the sensor measurements for the radial velocities of the torso in rad/s
    sensor_t* BalanceGyroOffset;                //!< stores the offsets for gyros
    sensor_t* BalanceOrientation;               //!< stores the robot's measured orientation (roll, pitch, yaw) rad
    sensor_t* BalanceOrientationHardware;       //!< stores the robot's hardware [roll, pitch, yaw] radians
    sensor_t* BalanceHorizon;                   //!< stores the Horizon line (A,B,C) Ax+ By +C =0
    sensor_t* BalanceZMP;                       //!< stores the robot's measured ZMP (x,y)
    sensor_t* BalanceFalling;                   //!< stores whether the robot is falling (sum, left, right, forward, backward)
    sensor_t* BalanceFallen;                    //!< stores whether the robot has fallen (sum, left, right, forward, backward)
    
    // Distance Sensors:
    sensor_t* DistanceLeftValues;                   //!< stores the ultrasonic left distance to obstacle measurements in cm
    sensor_t* DistanceRightValues;                   //!< stores the ultrasonic left distance to obstacle measurements in cm
    
    // Foot Pressure Sensors:
    sensor_t* FootSoleValues;                   //!< stores the foot force in Newtons
    sensor_t* FootBumperValues;                 //!< stores the foot bumper values; 0 for off, 1 for pressed
    sensor_t* FootCoP;                          //!< stores the foot centre of pressure as [lx, ly, rx, ry, x, y]
    sensor_t* FootForce;                        //!< stores the force on each of the feet in Newtons  as [l, r, both]
    sensor_t* FootContact;                      //!< stores whether each foot has contact with something (presumably the ground)
    sensor_t* FootSupport;                      //!< stores the whether each foot is supporting the robot as [l, r, both]
    sensor_t* FootImpact;                       //!< detects the time at which each foot last impacted with the ground
    
    // Buttons Sensors:
    sensor_t* ButtonValues;                     //!< stores the button values; 0 for unpressed, 1 for pressed
    sensor_t* ButtonTriggers;                   //!< stores the time since that last edge trigger for the button.

    // Battery Sensors:
    sensor_t* BatteryValues;                    //!< stores the battery values in Volts, Amperes and Watts
    
    // Motion Sensors:
    sensor_t* MotionFallActive;                 //!< stores whether the fall protection is currently active
    sensor_t* MotionGetupActive;                //!< stores whether the getup is currently active
    sensor_t* MotionWalkActive;                 //!< stores whether the walk is currently active
    sensor_t* MotionKickActive;                 //!< stores whether the kick is currently active
    sensor_t* MotionSaveActive;                 //!< stores whether the save is currently active
    sensor_t* MotionScriptActive;               //!< stores whether the script engine is active
    sensor_t* MotionWalkSpeed;                  //!< stores the current speeds [cm/s cm/s rad/s] of the walk engine
    sensor_t* MotionWalkMaxSpeed;               //!< stores the current maximum speeds of the walk engine
    sensor_t* MotionHeadCompletionTime;         //!< stores the completion time of the last head movement
    
    // GPS Sensors
    sensor_t* GPS;                              //!< stores the gps position of the robot
    sensor_t* Compass;                          //!< stores the bearing of the robot
    
private:
    vector<sensor_t*> m_sensors;                //!< a vector of all of the sensors
    vector<id_t> m_head_ids;              //!< a vector of id_t (index into sensor_t Joint*->Data) for each head joint
    vector<id_t> m_larm_ids;              //!< a vector of id_t (index into sensor_t Joint*->Data) for each left arm joint
    vector<id_t> m_rarm_ids;              //!< a vector of id_t (index into sensor_t Joint*->Data) for each right arm joint
    vector<id_t> m_torso_ids;             //!< a vector of id_t (index into sensor_t Joint*->Data) for each torso joint
    vector<id_t> m_lleg_ids;              //!< a vector of id_t (index into sensor_t Joint*->Data) for each left leg joint
    vector<id_t> m_rleg_ids;              //!< a vector of id_t (index into sensor_t Joint*->Data) for each right leg joint
    vector<id_t> m_body_ids;
    vector<id_t> m_all_joint_ids;
    vector<string> m_joint_names;
    int m_num_head_joints;
    int m_num_arm_joints;
    int m_num_torso_joints;
    int m_num_leg_joints;
    int m_num_body_joints;
    int m_num_joints;
};  

#endif

