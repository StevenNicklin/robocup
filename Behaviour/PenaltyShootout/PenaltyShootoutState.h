/*! @file PenaltyShootoutState.h
    @brief Declaration of an abstract behaviour state class for other states to inherit from
 
    @class PenaltyShootoutState
    @brief Declaration of an abstract behaviour state class for other states to inherit from

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

#ifndef PENALTY_SHOOTOUT_STATE_H
#define PENALTY_SHOOTOUT_STATE_H

class PenaltyShootoutProvider;
class PenaltyShootoutFSMState;
#include "Behaviour/BehaviourState.h"

class PenaltyShootoutState : public BehaviourState
{
public:
    virtual ~PenaltyShootoutState() {};
protected:
    PenaltyShootoutState(PenaltyShootoutProvider* provider) {m_provider = provider; m_parent = 0;};
    PenaltyShootoutState(PenaltyShootoutFSMState* parent) {m_parent = parent; m_provider = 0;};
    PenaltyShootoutProvider* m_provider;
    PenaltyShootoutFSMState* m_parent;
};


#endif

