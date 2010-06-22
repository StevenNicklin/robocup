/*! @file BallIsLostState.h
    @brief Declaration of the chase ball soccer state
 
    @class BallIsLostState
    @brief The chase ball soccer state

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

#ifndef PENALTY_BALL_IS_LOST_SOCCER_FSM_STATE_H
#define PENALTY_BALL_IS_LOST_SOCCER_FSM_STATE_H

#include "../PenaltyShootoutFSMState.h"

class PenaltyBallIsLostPan;
class PenaltyBallIsLostSpin;
class PenaltyBallIsLostMove;

class PenaltyBallIsLostState : public PenaltyShootoutFSMState
{
public:
    PenaltyBallIsLostState(PenaltyShootoutFSMState* parent);
    ~PenaltyBallIsLostState();
    BehaviourFSMState* nextState();
private:
    void doStateCommons();
    BehaviourState* nextStateCommons();
protected:
    friend class PenaltyBallIsLostPan;
    BehaviourState* m_lost_pan;
    friend class PenaltyBallIsLostSpin;
    BehaviourState* m_lost_spin;
    friend class PenaltyBallIsLostMove;
    BehaviourState* m_lost_move;
};


#endif

