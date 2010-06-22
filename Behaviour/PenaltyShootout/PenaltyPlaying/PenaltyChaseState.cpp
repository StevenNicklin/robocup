/*! @file ChaseState.h
    @brief Implementation of the ready soccer state

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

#include "PenaltyChaseState.h"
#include "PenaltyChaseStates.h"

#include "Behaviour/GameInformation.h"
#include "NUPlatform/NUActionators/NUActionatorsData.h"
#include "NUPlatform/NUActionators/NUSounds.h"

#include "debug.h"
#include "debugverbositybehaviour.h"

PenaltyChaseState::PenaltyChaseState(PenaltyShootoutFSMState* parent) : PenaltyShootoutFSMState(parent)
{
    m_go_to_ball = new PenaltyGoToBall(this);
    m_find_target = new PenaltyFindTarget(this);
    m_kick = new PenaltyKick(this);
    
    m_state = m_go_to_ball;
}

PenaltyChaseState::~PenaltyChaseState()
{
    delete m_go_to_ball;
    delete m_find_target;
    delete m_kick;
}

void PenaltyChaseState::doStateCommons()
{   // do behaviour that is common to all sub chase states
    #if DEBUG_BEHAVIOUR_VERBOSITY > 1
        debug << "ChaseState" << endl;
    #endif
}

BehaviourState* PenaltyChaseState::nextStateCommons()
{   // do state transitions in the chase state machine
    return m_state;
}

BehaviourFSMState* PenaltyChaseState::nextState()
{   // do state transitions in the playing state machine
    return this;
}


