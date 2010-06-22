/*! @file SoccerProvider.h
    @brief 
 
    @class SoccerProvider
    @brief 

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

#ifndef PENALTY_SHOOTOUT_PROVIDER_H
#define PENALTY_SHOOTOUT_PROVIDER_H

class Behaviour;
#include "Behaviour/BehaviourFSMProvider.h"

#include <vector>
using namespace std;

class PenaltyShootoutProvider : public BehaviourFSMProvider
{
public:
    PenaltyShootoutProvider(Behaviour* manager);
    virtual ~PenaltyShootoutProvider();
    
protected:
    void doBehaviourCommons();
    BehaviourState* nextStateCommons();
public:
    BehaviourState* m_initial;
    BehaviourState* m_set;
    BehaviourState* m_playing;
    BehaviourState* m_finished;
    BehaviourState* m_penalised;
    BehaviourState* m_substitute;
    BehaviourState* m_requires_substitution;
    
    vector<int> m_yellow_goal_led_indices;
    vector<int> m_blue_goal_led_indices;
    vector<int> m_lost_led_indices;
    vector<vector<float> > m_led_on;
    vector<vector<float> > m_led_off;
    vector<vector<float> > m_led_red;
    vector<vector<float> > m_led_green;
    vector<vector<float> > m_led_yellow;
    vector<vector<float> > m_led_blue;
};


#endif

