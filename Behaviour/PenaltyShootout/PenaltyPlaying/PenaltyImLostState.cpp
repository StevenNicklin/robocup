/*! @file ImLostState.h
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

#include "PenaltyImLostState.h"
#include "PenaltyImLostStates.h"

#include "Behaviour/GameInformation.h"
#include "NUPlatform/NUActionators/NUActionatorsData.h"
#include "NUPlatform/NUActionators/NUSounds.h"

#include "debug.h"
#include "debugverbositybehaviour.h"
using namespace std;

PenaltyImLostState::PenaltyImLostState(PenaltyShootoutFSMState* parent) : PenaltyShootoutFSMState(parent)
{
    m_lost_pan = new PenaltyImLostPan(this);
    m_lost_spin = new PenaltyImLostSpin(this);
    
    m_state = m_lost_pan;
}

PenaltyImLostState::~PenaltyImLostState()
{
    delete m_lost_pan;
    delete m_lost_spin;
}

void PenaltyImLostState::doStateCommons()
{   // do behaviour that is common to all sub lost states
    #if DEBUG_BEHAVIOUR_VERBOSITY > 1
        debug << "ImLostState" << endl;
    #endif
}

BehaviourState* PenaltyImLostState::nextStateCommons()
{   // do state transitions in the lost state machine
    return m_state;
}

BehaviourFSMState* PenaltyImLostState::nextState()
{   // do state transitions in the playing state machine
    return this;
}


