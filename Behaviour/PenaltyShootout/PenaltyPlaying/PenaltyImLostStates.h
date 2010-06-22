/*! @file ImLostStates.h
    @brief Declaration of the robot is lost states

    @author Jason Kulk
 
  Copyright (c) 2010 Jason Kulk
 
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

#ifndef PENALTY_IM_LOST_STATES_H
#define PENALTY_IM_LOST_STATES_H

#include "../PenaltyShootoutState.h"
#include "PenaltyImLostState.h"

#include "Behaviour/Jobs/JobList.h"
#include "NUPlatform/NUSensors/NUSensorsData.h"

#include "Behaviour/Jobs/MotionJobs/WalkJob.h"
#include "Behaviour/Jobs/MotionJobs/HeadPanJob.h"
#include "Behaviour/Jobs/MotionJobs/HeadNodJob.h"

#include "debug.h"
#include "debugverbositybehaviour.h"
using namespace std;

class PenaltyImLostSubState : public PenaltyShootoutState
{
public:
    PenaltyImLostSubState(PenaltyImLostState* parent) : PenaltyShootoutState(parent), m_parent_machine(parent) {};
    virtual ~PenaltyImLostSubState() {};
protected:
    PenaltyImLostState* m_parent_machine;
};

// ----------------------------------------------------------------------------------------------------------------------- ImLostPan
/*! @class ImLostPan
    In this state we stop and do a wide localisation pan. When the pan is completed we move into a spin state.
 */
class PenaltyImLostPan : public PenaltyImLostSubState
{
public:
    PenaltyImLostPan(PenaltyImLostState* parent) : PenaltyImLostSubState(parent) 
    {
        m_time_in_state = 0;
        m_previous_time = 0;
        m_pan_started = false;
        m_pan_end_time = 0;
    }
    ~PenaltyImLostPan() {};
protected:
    BehaviourState* nextState()
    {   // do state transitions in the lost state machine
        // we transition to the spin state when the pan is completed.
        if (m_pan_started and m_pan_end_time < m_data->CurrentTime and not m_parent_machine->stateChanged())
            return m_parent_machine->m_lost_spin;
        else
            return this;
    }
    void doState()
    {
        #if DEBUG_BEHAVIOUR_VERBOSITY > 1
            debug << "ImLostPan" << endl;
        #endif
        m_jobs->addMotionJob(new WalkJob(0, 0, 0));
        m_jobs->addMotionJob(new HeadPanJob(HeadPanJob::Localisation));
        
        // keep track of the time in this state
        if (m_parent_machine->stateChanged())
            reset();
        else
            m_time_in_state += m_data->CurrentTime - m_previous_time;
        m_previous_time = m_data->CurrentTime;
        
        // grab the pan end time
        if (not m_pan_started and m_time_in_state > 200)
        {
            if (m_data->getMotionHeadCompletionTime(m_pan_end_time))
                m_pan_started = true;
        }
    }
private:
    void reset()
    {
        m_time_in_state = 0;
        m_pan_started = false;
        m_pan_end_time = 0;
    }
    float m_time_in_state;
    double m_previous_time;
    bool m_pan_started;
    double m_pan_end_time;
};

// ----------------------------------------------------------------------------------------------------------------------- ImLostSpin
/*! @class ImLostSpin
    In this state we spin on the spot and do the nod. After 1.25 revolutions we go back to the pan state.
 */
class PenaltyImLostSpin : public PenaltyImLostSubState
{
public:
    PenaltyImLostSpin(PenaltyImLostState* parent) : PenaltyImLostSubState(parent), m_ROTATIONAL_SPEED(0.4)
    {
        m_time_in_state = 0;
        m_previous_time = 0;
    }
    ~PenaltyImLostSpin() {};
protected:
    BehaviourState* nextState()
    {   // do state transitions in the ball is lost state machine
        if (m_time_in_state > 1.25*1000*(6.28/m_ROTATIONAL_SPEED))
            return m_parent_machine->m_lost_pan;
        else
            return this;
    }
    void doState()
    {
        #if DEBUG_BEHAVIOUR_VERBOSITY > 1
            debug << "ImLostSpin" << endl;
        #endif
        if (m_parent_machine->stateChanged())
            m_time_in_state = 0;
        else
            m_time_in_state += m_data->CurrentTime - m_previous_time;
        m_previous_time = m_data->CurrentTime;
        
        m_jobs->addMotionJob(new WalkJob(0, 0, m_ROTATIONAL_SPEED));
        m_jobs->addMotionJob(new HeadNodJob(HeadNodJob::Localisation, m_ROTATIONAL_SPEED));
    }
private:
    const float m_ROTATIONAL_SPEED;
    float m_time_in_state;
    float m_previous_time;
};
#endif

