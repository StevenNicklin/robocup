/*! @file NUMotion.cpp
    @brief Implementation of motion class

    @author Jason Kulk
 
 Copyright (c) 2009, 2010 Jason Kulk
 
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
#include "NUMotion.h"
#ifdef USE_HEAD
    #include "NUHead.h"
#endif
#ifdef USE_WALK
    #include "NUWalk.h"
#endif
#ifdef USE_KICK
    #include "NUKick.h"
#endif
#if defined(USE_BLOCK) or defined(USE_SAVE)
    #include "NUSave.h"
#endif
#ifdef USE_SCRIPT
    #include "Script.h"
#endif

#include "Behaviour/Jobs.h"
#include "FallProtection.h"
#include "Getup.h"
#include "Tools/MotionScript.h"
#include "Tools/Math/General.h"

#include "NUPlatform/NUPlatform.h"
#include "NUPlatform/NUSensors/NUSensorsData.h"
#include "NUPlatform/NUActionators/NUActionatorsData.h"

#include "debug.h"
#include "debugverbositynumotion.h"

#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics.hpp>

/*! @brief Constructor for motion module
 */
NUMotion::NUMotion(NUSensorsData* data, NUActionatorsData* actions)
{
    #if DEBUG_NUMOTION_VERBOSITY > 4
        debug << "NUMotion::NUMotion" << endl;
    #endif
    m_current_time = 0;
    m_previous_time = 0;
    m_killed = true;
    m_last_kill_time = m_current_time - 10000;
    
    m_data = data;
    m_actions = actions;
    
    m_current_head_provider = 0; 
    m_current_arm_provider = 0; 
    m_current_leg_provider = 0;
    
    m_next_head_provider = 0;
    m_next_arm_provider = 0;
    m_next_leg_provider = 0;
    
    #ifdef USE_HEAD
        m_head = new NUHead(m_data, m_actions);
        m_current_head_provider = m_head;
    #endif
    
    #if defined(USE_WALK)
        m_walk = NUWalk::getWalkEngine(m_data, m_actions);
        m_current_arm_provider = m_walk;
        m_current_leg_provider = m_walk;
        m_getup = new Getup(m_walk, m_data, m_actions);
        m_fall_protection = new FallProtection(m_walk, m_data, m_actions);
        #if defined(USE_KICK)
            m_kick = new NUKick(m_walk, m_data, m_actions);
        #endif
        #if defined(USE_BLOCK) or defined(USE_SAVE)
            m_save = new NUSave(m_walk, m_data, m_actions);
        #endif
        #if defined(USE_SCRIPT)
            m_script = new Script(m_walk, m_data, m_actions);
        #endif
    #else
        m_getup = new Getup(NULL, m_data, m_actions);
        m_fall_protection = new FallProtection(NULL, m_data, m_actions);
        #if defined(USE_KICK)
            m_kick = new NUKick(NULL, m_data, m_actions);
        #endif
        #if defined(USE_BLOCK) or defined(USE_SAVE)
            m_save = new NUSave(NULL, m_data, m_actions);
        #endif
        #if defined(USE_SCRIPT)
            m_script = new Script(NULL, m_data, m_actions);
        #endif
    #endif
    m_next_head_provider = m_current_head_provider;
    m_next_arm_provider = m_current_arm_provider;
    m_next_leg_provider = m_current_leg_provider;
}

/*! @brief Destructor for motion module
 */
NUMotion::~NUMotion()
{
    if (m_fall_protection != NULL)
        delete m_fall_protection;
    
    if (m_getup != NULL)
        delete m_getup;                   
    #ifdef USE_HEAD
        if (m_head != NULL)
            delete m_head;
    #endif
    #ifdef USE_WALK
        if (m_walk != NULL)
            delete m_walk;                
    #endif
    #ifdef USE_KICK
        if (m_kick != NULL)
            delete m_kick;                   
    #endif
    #if defined(USE_BLOCK) or defined(USE_SAVE)
        delete m_save;
    #endif
    #ifdef USE_SCRIPT
        delete m_script;
    #endif
}

/*! @brief Freezes all motion modules
 */
void NUMotion::stop()
{
    m_killed = true;
    m_last_kill_time = m_current_time;
    m_fall_protection->stop();
    m_getup->stop();
    #ifdef USE_HEAD
        m_head->stop();
    #endif
    #ifdef USE_WALK
        m_walk->stop();                
    #endif
    #ifdef USE_KICK
        m_kick->stop();                   
    #endif
    #if defined(USE_BLOCK) or defined(USE_SAVE)
        m_save->stop();
    #endif
}

/*! @brief Adds actions to bring the robot to rest quickly, and go into a safe-for-robot pose
 */
void NUMotion::kill()
{
    m_killed = true;
    m_last_kill_time = m_current_time;
    m_fall_protection->kill();
    m_getup->kill();
    #ifdef USE_HEAD
        m_head->kill();
    #endif
    #ifdef USE_WALK
        m_walk->kill();                
    #endif
    #ifdef USE_KICK
        m_kick->kill();                   
    #endif
    #if defined(USE_BLOCK) or defined(USE_SAVE)
        m_save->kill();
    #endif
    
    float safelegpositions[] = {0, -0.85, -0.15, 2.16, 0, -1.22};
    float safelarmpositions[] = {0, 1.41, -1.1, -0.65};
    float saferarmpositions[] = {0, 1.41, 1.1, 0.65};
    vector<float> legpositions(safelegpositions, safelegpositions + sizeof(safelegpositions)/sizeof(*safelegpositions));
    vector<float> larmpositions(safelarmpositions, safelarmpositions + sizeof(safelarmpositions)/sizeof(*safelarmpositions));
    vector<float> rarmpositions(saferarmpositions, saferarmpositions + sizeof(saferarmpositions)/sizeof(*saferarmpositions));
    vector<float> legvelocities(legpositions.size(), 1.0);
    vector<float> armvelocities(larmpositions.size(), 1.0);
    
    // check if there is a reason it is not safe or possible to go into the crouch position
    if (m_actions == NULL)
        return;
    else if (m_data != NULL)
    {
        // check the orientation
        vector<float> orientation;
        if (m_data->getOrientation(orientation))
            if (fabs(orientation[0]) > 0.5 or fabs(orientation[1]) > 0.5)
            {
                m_actions->addJointPositions(NUActionatorsData::LeftLegJoints, nusystem->getTime(), legpositions, legvelocities, 0);
                m_actions->addJointPositions(NUActionatorsData::RightLegJoints, nusystem->getTime(), legpositions, legvelocities, 0);
                m_actions->addJointPositions(NUActionatorsData::LeftArmJoints, nusystem->getTime(), larmpositions, armvelocities, 0);
                m_actions->addJointPositions(NUActionatorsData::RightArmJoints, nusystem->getTime(), rarmpositions, armvelocities, 0);
                return;
            }
        // check the feet are on the ground
        if (not m_data->isOnGround())
        {
            m_actions->addJointPositions(NUActionatorsData::LeftLegJoints, nusystem->getTime(), legpositions, legvelocities, 0);
            m_actions->addJointPositions(NUActionatorsData::RightLegJoints, nusystem->getTime(), legpositions, legvelocities, 0);
            m_actions->addJointPositions(NUActionatorsData::LeftArmJoints, nusystem->getTime(), larmpositions, armvelocities, 0);
            m_actions->addJointPositions(NUActionatorsData::RightArmJoints, nusystem->getTime(), rarmpositions, armvelocities, 0);
            return;
        }
        
        // check the stiffness is on
        vector<float> leftstiffnesses, rightstiffnesses;
        if (m_data->getJointStiffnesses(NUSensorsData::LeftLegJoints, leftstiffnesses) and m_data->getJointStiffnesses(NUSensorsData::RightLegJoints, rightstiffnesses))
            if (mathGeneral::allZeros(leftstiffnesses) and mathGeneral::allZeros(rightstiffnesses))
                return;
    }
    
    // go into safe mode
    m_actions->addJointPositions(NUActionatorsData::LeftLegJoints, nusystem->getTime() + 1500, legpositions, legvelocities, 65);
    m_actions->addJointPositions(NUActionatorsData::RightLegJoints, nusystem->getTime() + 1500, legpositions, legvelocities, 65);
    m_actions->addJointPositions(NUActionatorsData::LeftArmJoints, nusystem->getTime() + 750, larmpositions, armvelocities, 30);
    m_actions->addJointPositions(NUActionatorsData::RightArmJoints, nusystem->getTime() + 750, rarmpositions, armvelocities, 30);
    
    m_actions->addJointPositions(NUActionatorsData::LeftLegJoints, nusystem->getTime() + 2000, legpositions, legvelocities, 0);
    m_actions->addJointPositions(NUActionatorsData::RightLegJoints, nusystem->getTime() + 2000, legpositions, legvelocities, 0);
    m_actions->addJointPositions(NUActionatorsData::LeftArmJoints, nusystem->getTime() + 2000, larmpositions, armvelocities, 0);
    m_actions->addJointPositions(NUActionatorsData::RightArmJoints, nusystem->getTime() + 2000, rarmpositions, armvelocities, 0);
}

/*! @brief Calls kill on each of the active motion providers */
void NUMotion::killActiveProviders()
{
    if (m_current_head_provider)
        m_current_head_provider->kill();
    if (m_current_arm_provider)
        m_current_arm_provider->kill();
    if (m_current_leg_provider)
        m_current_leg_provider->kill();
}

/*! @brief Calls stop on each of the active motion providers */
void NUMotion::stopActiveProviders()
{
    if (m_current_head_provider)
        m_current_head_provider->stop();
    if (m_current_arm_provider)
        m_current_arm_provider->stop();
    if (m_current_leg_provider)
        m_current_leg_provider->stop();
}

/*! @brief Process new sensor data, and produce actionator commands.
 
    This function basically calls all of the process functions of the submodules of motion. I have it setup
    so that the process functions are only called when they are allowed to be executed.
 
    @param data a pointer to the most recent sensor data storage class
    @param actions a pointer to the actionators data storage class. This variable will be filled
                   with the desired actions by NUMotion provided the NUActionatorsData instance
                   has been initialised by NUActionators.
 */
void NUMotion::process(NUSensorsData* data, NUActionatorsData* actions)
{
    #if DEBUG_NUMOTION_VERBOSITY > 2
        debug << "NUMotion::process(" << data << ", " << actions << ")" << endl;
    #endif
    if (data == NULL || actions == NULL)
        return;
    m_data = data;
    m_actions = actions;
    m_current_time = m_data->CurrentTime;
    updateMotionSensors();
    
    if (m_killed)
        return;
    else if (m_fall_protection->enabled() and m_data->isFalling())
    {
        if (m_current_leg_provider != m_fall_protection)
            killActiveProviders();          // fast hard kill on active providers if falling
        
        m_next_head_provider = m_fall_protection;
        m_next_arm_provider = m_fall_protection;
        m_next_leg_provider = m_fall_protection;
    }
    else if (m_getup->enabled() and m_data->isFallen())
    {
        if (m_current_leg_provider != m_getup)
            killActiveProviders();          // slow soft stop on active providers if fallen
        
        m_next_head_provider = m_getup;
        m_next_arm_provider = m_getup;
        m_next_leg_provider = m_getup;
    }
    
    #if DEBUG_NUMOTION_VERBOSITY > 0
        debug << "NUMotion::CurrentHeadProvider: ";
        if (m_current_head_provider)
            debug << m_current_head_provider->getName();
        else
            debug << "None";
        debug << " nextHeadProvider: ";
        if (m_next_head_provider)
            debug << m_next_head_provider->getName();
        else
            debug << "None";
        debug << endl;
    #endif
    #if DEBUG_NUMOTION_VERBOSITY > 0
        debug << "NUMotion::CurrentArmProvider: ";
        if (m_current_arm_provider)
            debug << m_current_arm_provider->getName();
        else
            debug << "None";
        debug << " nextArmProvider: ";
        if (m_next_arm_provider)
            debug << m_next_arm_provider->getName();
        else
            debug << "None";
        debug << endl;
    #endif
    #if DEBUG_NUMOTION_VERBOSITY > 0
        debug << "NUMotion::CurrentLegProvider: ";
        if (m_current_leg_provider)
            debug << m_current_leg_provider->getName();
        else
            debug << "None";
        debug << " nextLegProvider: ";
        if (m_next_leg_provider)
            debug << m_next_leg_provider->getName();
        else
            debug << "None";
        debug << endl;
    #endif
    
    if (m_current_head_provider != m_next_head_provider)
    {   // handle head provider transition
        if (m_current_head_provider and m_current_head_provider->isUsingHead())
            m_current_head_provider->stopHead();
        if (not m_current_head_provider->isUsingHead())
            m_current_head_provider = m_next_head_provider;
    }
    
    if (m_current_arm_provider != m_next_arm_provider)
    {   // handle arm provider transition
        if (m_current_arm_provider and m_current_arm_provider->isUsingArms())
            m_current_arm_provider->stopArms();
        if (not m_current_arm_provider->isUsingArms())
            m_current_arm_provider = m_next_arm_provider;
    }
    
    if (m_current_leg_provider != m_next_leg_provider)
    {   // handle leg provider transition
        if (m_current_leg_provider and m_current_leg_provider->isUsingLegs())
            m_current_leg_provider->stopLegs();
        if (not m_current_leg_provider->isUsingLegs())
            m_current_leg_provider = m_next_leg_provider;
    }
    
    if (isCurrentProvider(m_current_head_provider))
        m_current_head_provider->process(data, actions);
    
    if (isCurrentProvider(m_current_arm_provider) and m_current_arm_provider != m_current_head_provider)
        m_current_arm_provider->process(data, actions);
    
    if (isCurrentProvider(m_current_leg_provider) and m_current_leg_provider != m_current_arm_provider and m_current_leg_provider != m_current_head_provider)
        m_current_leg_provider->process(data, actions);
    
    m_previous_time = m_current_time;
}

/*! @brief Process the jobs. Jobs are deleted when they are completed, and more jobs can be added inside this function.
    
    @param jobs the current list of jobs
 */
void NUMotion::process(JobList* jobs)
{
#if DEBUG_NUMOTION_VERBOSITY > 4
    debug << "NUMotion::process(): Start" << endl;
#endif
    if (jobs == NULL or m_data == NULL or m_actions == NULL)
        return;
    
    list<Job*>::iterator it = jobs->motion_begin();     // the iterator over the motion jobs
    while (it != jobs->motion_end())
    {
        m_killed = false;
        NUMotionProvider* next_provider = 0;
        Job::job_id_t id = (*it)->getID();
        switch (id) 
        {
        #ifdef USE_WALK
            case Job::MOTION_WALK:
                next_provider = m_walk;
                m_walk->process(reinterpret_cast<WalkJob*> (*it), canProcessJobs(m_walk));
                break;
            case Job::MOTION_WALK_TO_POINT:
                next_provider = m_walk;
                m_walk->process(reinterpret_cast<WalkToPointJob*> (*it), canProcessJobs(m_walk));
                break;
            case Job::MOTION_WALK_PARAMETERS:
                next_provider = m_walk;
                m_walk->process(reinterpret_cast<WalkParametersJob*> (*it));
                break;
        #endif
        #ifdef USE_KICK
            case Job::MOTION_KICK:
                next_provider = m_kick;
                m_kick->process(reinterpret_cast<KickJob*> (*it));
                break;
        #endif
        #ifdef USE_HEAD
            case Job::MOTION_HEAD:
                next_provider = m_head;
                m_head->process(reinterpret_cast<HeadJob*> (*it), canProcessJobs(m_walk));
                break;
            case Job::MOTION_TRACK:
                next_provider = m_head;
                m_head->process(reinterpret_cast<HeadTrackJob*> (*it), canProcessJobs(m_walk));
                break;
            case Job::MOTION_PAN:
                next_provider = m_head;
                m_head->process(reinterpret_cast<HeadPanJob*> (*it), canProcessJobs(m_walk));
                break;
            case Job::MOTION_NOD:
                next_provider = m_head;
                m_head->process(reinterpret_cast<HeadNodJob*> (*it), canProcessJobs(m_walk));
                break;
        #endif
        #if defined(USE_BLOCK) or defined(USE_SAVE)
            case Job::MOTION_BLOCK:
                next_provider = m_save;
                m_save->process(reinterpret_cast<BlockJob*> (*it));
                break;
            case Job::MOTION_SAVE:
                next_provider = m_save;
                m_save->process(reinterpret_cast<SaveJob*> (*it));
                break;
        #endif
        #ifdef USE_SCRIPT
            case Job::MOTION_SCRIPT:
                next_provider = m_script;
                m_script->process(reinterpret_cast<ScriptJob*> (*it));
                break;
        #endif
            case Job::MOTION_KILL:
                process(reinterpret_cast<MotionKillJob*> (*it));
                break;
            case Job::MOTION_FREEZE:
                process(reinterpret_cast<MotionFreezeJob*> (*it));
                break;
            default:
                break;
        }
        it = jobs->removeMotionJob(it);
        setNextProviders(next_provider);
    }
    
    #if DEBUG_NUMOTION_VERBOSITY > 4
        debug << "NUMotion::process(): Finished" << endl;
    #endif
}

/*! @brief Sets the m_next_*_providers depending on which limbs next_provider requires */
void NUMotion::setNextProviders(NUMotionProvider* next_provider)
{
    if (next_provider and next_provider->isReady() and (m_current_time > m_last_kill_time + 2000))
    {
        if (next_provider->requiresHead())
            m_next_head_provider = next_provider;
        if (next_provider->requiresArms())
            m_next_arm_provider = next_provider;
        if (next_provider->requiresLegs())
            m_next_leg_provider = next_provider;
        
        #if DEBUG_NUMOTION_VERBOSITY > 0
            debug << "NUMotion::setNextProviders: ";
            if (m_next_head_provider)
                debug << m_next_head_provider->getName() << " ";
            if (m_next_arm_provider)
                debug << m_next_arm_provider->getName() << " ";
            if (m_next_leg_provider)
                debug << m_next_leg_provider->getName() << " ";
            debug << endl;
        #endif
    }
}

/*! @brief Checks whether provider is the current provider
    @param provider the provider you want to check is the current
    @return true if it is the current provider of ALL of the limbs it requires
 */
bool NUMotion::isCurrentProvider(NUMotionProvider* provider)
{
    if (not provider)
        return false;
    else
    {
        // check the provider is current on each limb it requires
        if (provider->requiresHead() and provider != m_current_head_provider)
            return false;
        if (provider->requiresArms() and provider != m_current_arm_provider)
            return false;
        if (provider->requiresLegs() and provider != m_current_leg_provider)
            return false;

        return true;
    }
}

bool NUMotion::isNextProviderReady(NUMotionProvider* provider)
{
    if (not provider)
        return true;
    else
    {
        if (provider == m_current_head_provider and m_current_head_provider != m_next_head_provider and m_next_head_provider->isReady())
            return true;
        if (provider == m_current_arm_provider and m_current_arm_provider != m_next_arm_provider and m_next_arm_provider->isReady())
            return true;
        if (provider == m_current_leg_provider and m_current_leg_provider != m_next_leg_provider and m_next_leg_provider->isReady())
            return true;

        return false;
    }
}
                                
bool NUMotion::canProcessJobs(NUMotionProvider* provider)
{
    return isCurrentProvider(provider) and not isNextProviderReady(provider);
}

/*! @brief Process a kill motion job */
void NUMotion::process(MotionKillJob* job)
{
    kill();
}

/*! @brief Process a freeze motion job */
void NUMotion::process(MotionFreezeJob* job)
{
    stop();
}

/*! @brief Updates the motion sensors in NUSensorsData */
void NUMotion::updateMotionSensors()
{
    m_data->setMotionFallActive(m_current_time, m_fall_protection->enabled() and m_data->isFalling());
    m_data->setMotionGetupActive(m_current_time, m_getup->isActive());
    #ifdef USE_KICK
        m_data->setMotionKickActive(m_current_time, m_kick->isActive());
    #endif
    #ifdef USE_SAVE
        m_data->setMotionSaveActive(m_current_time, m_save->isActive());
    #endif
    #ifdef USE_SCRIPT
        m_data->setMotionScriptActive(m_current_time, m_script->isActive());
    #endif
    #ifdef USE_WALK
        vector<float> speed;
        m_walk->getCurrentSpeed(speed);
        m_data->setMotionWalkSpeed(m_current_time, speed);
        m_walk->getMaximumSpeed(speed);
        m_data->setMotionWalkMaxSpeed(m_current_time, speed);
    #endif
    #ifdef USE_HEAD
        m_data->setMotionHeadCompletionTime(m_current_time, m_head->getCompletionTime());
    #endif
}

