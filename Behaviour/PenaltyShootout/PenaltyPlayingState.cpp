/*! @file PlayingState.h
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

#include "PenaltyPlayingState.h"
#include "PenaltyPlaying/PenaltyChaseState.h"
#include "PenaltyPlaying/PenaltyPositioningState.h"
#include "PenaltyPlaying/PenaltyBallIsLostState.h"
#include "PenaltyPlaying/PenaltyImLostState.h"

#include "PenaltyShootoutProvider.h"

#include "Behaviour/GameInformation.h"
#include "Behaviour/TeamInformation.h"
#include "Vision/FieldObjects/FieldObjects.h"
#include "NUPlatform/NUActionators/NUActionatorsData.h"
#include "NUPlatform/NUActionators/NUSounds.h"

PenaltyPlayingState::PenaltyPlayingState(PenaltyShootoutProvider* provider) : PenaltyShootoutFSMState(provider)
{
    m_chase_state = new PenaltyChaseState(this);
    m_positioning_state = new PenaltyPositioningState(this);
    m_ball_is_lost_state = new PenaltyBallIsLostState(this);
    m_im_lost_state = new PenaltyImLostState(this);
    
    m_state = m_chase_state;
    
    m_chase_led_indices.push_back(3);
    m_chase_led_indices.push_back(4);
    m_led_on = vector<vector<float> >(1, vector<float>(3,1.0f));
    m_led_off = vector<vector<float> >(1, vector<float>(3,0.0f));
    m_led_red = m_led_off;
    m_led_red[0][0] = 1;
    m_led_green = m_led_off;
    m_led_green[0][1] = 1;
    m_led_yellow = m_led_off;
    m_led_yellow[0][0] = 1;
    m_led_yellow[0][1] = 1;
}

PenaltyPlayingState::~PenaltyPlayingState()
{
    delete m_chase_state;
    delete m_positioning_state;
    delete m_ball_is_lost_state;
    delete m_im_lost_state;
}

void PenaltyPlayingState::doStateCommons()
{
    if (m_provider->stateChanged())
    {   // play a sound when we enter the playing state, turn the kick off light off
        m_actions->addSound(m_actions->CurrentTime, NUSounds::PLAYING);
        m_actions->addLeds(NUActionatorsData::RightFootLeds, m_actions->CurrentTime, 0, 0, 0);
    }
    // In playing the chest led should be green
    m_actions->addLeds(NUActionatorsData::ChestLeds, m_actions->CurrentTime, 0.1, 1, 0.1);
    
    // set the right eye leds to indicate which state we are in
    if (m_state == m_chase_state)
        m_actions->addLeds(NUActionatorsData::RightEyeLeds, m_chase_led_indices, m_actions->CurrentTime, m_led_red);
    else if (m_state == m_positioning_state)
        m_actions->addLeds(NUActionatorsData::RightEyeLeds, m_chase_led_indices, m_actions->CurrentTime, m_led_green);
    else
        m_actions->addLeds(NUActionatorsData::RightEyeLeds, m_chase_led_indices, m_actions->CurrentTime, m_led_off);
}

BehaviourFSMState* PenaltyPlayingState::nextStateCommons()
{   // do state transitions in playing state machine
    if (m_field_objects->mobileFieldObjects[FieldObjects::FO_BALL].lost())
        return m_ball_is_lost_state;
    else if (m_team_info->amIClosestToBall())
        return m_chase_state;
    else if (m_field_objects->self.lost())
        return m_im_lost_state;
    else
        return m_positioning_state;
}

BehaviourFSMState* PenaltyPlayingState::nextState()
{   // the playing state machine can never modify the game state machine
    return this;
}




