/*! @file ChaseBallStates.h
    @brief Chase ball states

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

#ifndef KICKERSTATES_H
#define KICKERSTATES_H

#include "Behaviour/BehaviourState.h"
#include "KickerProvider.h"

#include "Behaviour/Jobs/JobList.h"
#include "NUPlatform/NUSensors/NUSensorsData.h"
#include "NUPlatform/NUActionators/NUActionatorsData.h"
#include "Vision/FieldObjects/FieldObjects.h"
#include "Behaviour/TeamInformation.h"

#include "Behaviour/Jobs/MotionJobs/KickJob.h"
#include "Behaviour/Jobs/MotionJobs/WalkJob.h"
#include "Behaviour/Jobs/MotionJobs/HeadJob.h"
#include "Behaviour/Jobs/MotionJobs/HeadTrackJob.h"
#include "Behaviour/Jobs/MotionJobs/HeadPanJob.h"
#include "Behaviour/Jobs/MotionJobs/HeadNodJob.h"
#include "Behaviour/Jobs/MotionJobs/MotionFreezeJob.h"
#include "NUPlatform/NUActionators/NUSounds.h"

#include "debug.h"

class KickerState : public BehaviourState
{
public:
    KickerState(KickerProvider* provider){m_provider = provider;};
protected:
    KickerProvider* m_provider;
};

// ----------------------------------------------------------------------------------------------------------------------- ChaseState
class WaitState : public KickerState
{
public:
    int m_initialMoveCounter;
    WaitState(KickerProvider* provider) : KickerState(provider) {m_initialMoveCounter = 0;};
    BehaviourState* nextState()
    {
        float ballXvel = m_provider->m_field_objects->mobileFieldObjects[FieldObjects::FO_BALL].velX();
        float ballYvel = m_provider->m_field_objects->mobileFieldObjects[FieldObjects::FO_BALL].velY();
        float ballVelocity = sqrt(ballXvel*ballXvel + ballYvel*ballYvel);
//        if ((m_provider->m_field_objects->mobileFieldObjects[FieldObjects::FO_BALL].TimeSeen() > 500) && (ballVelocity < 5.0f))
//        {
//            debug << "Wait -> Kick" << endl;
//            debug << "TimeSeen = " << m_provider->m_field_objects->mobileFieldObjects[FieldObjects::FO_BALL].TimeSeen() << " ballVelocity = " << ballVelocity << endl;
//            m_initialMoveCounter = 0;
//            return m_provider->m_kick_state;
//        }
//        else
            return m_provider->m_state;
    };
    
    void doState()
    {
        /*
        if(m_provider->m_field_objects->mobileFieldObjects[FieldObjects::FO_BALL].isObjectVisible())
        {
            HeadTrackJob* head = new HeadTrackJob(m_provider->m_field_objects->mobileFieldObjects[FieldObjects::FO_BALL]);
            m_provider->m_jobs->addMotionJob(head);
        }
        else if(m_provider->m_field_objects->mobileFieldObjects[FieldObjects::FO_BALL].TimeSinceLastSeen() > 250.0f)
        {
            m_jobs->addMotionJob(new HeadPanJob(HeadPanJob::Ball));
        }
        */
        if(m_initialMoveCounter < 10)
        {
            m_jobs->addMotionJob(new WalkJob(0.001,0,0));
            m_initialMoveCounter++;
        }
        else
        {
            m_jobs->addMotionJob(new WalkJob(0,0,0));
        }
    };
};

// ----------------------------------------------------------------------------------------------------------------------- PositonState
class KickState : public KickerState
{
public:
    bool m_kickActivePrev;
    KickState(KickerProvider* provider) : KickerState(provider) {m_kickActivePrev = false;};
    BehaviourState* nextState()
    {
        bool kickActive = false;
        m_provider->m_data->getMotionKickActive(kickActive);
        bool kickFinished = m_kickActivePrev && !kickActive;
        m_kickActivePrev = kickActive;
        bool ballvisible = m_provider->m_field_objects->mobileFieldObjects[FieldObjects::FO_BALL].TimeSinceLastSeen() < 500.0f;

        float ballXvel = m_provider->m_field_objects->mobileFieldObjects[FieldObjects::FO_BALL].velX();
        float ballYvel = m_provider->m_field_objects->mobileFieldObjects[FieldObjects::FO_BALL].velY();
        float ballVelocity = sqrt(ballXvel*ballXvel + ballYvel*ballYvel);

        if (kickFinished)
        {
            debug << "Kicking -> Waiting" << endl;
            debug << "kickFinished = " << kickFinished << " ballvisible = " << ballvisible << " ballVelocity = " << ballVelocity << endl;
            return m_provider->m_wait_state;
        }
        else
            return m_provider->m_state;
    };
    
    void doState()
    {
        vector<float> kickPos(2,0.0f);
        vector<float> targetPos(2,0.0f);
        float ballDistance = m_provider->m_field_objects->mobileFieldObjects[FieldObjects::FO_BALL].estimatedDistance();
        float ballBearing = m_provider->m_field_objects->mobileFieldObjects[FieldObjects::FO_BALL].estimatedBearing();
//        kickPos[0] = ballDistance * cos(ballBearing);
//        kickPos[1] = ballDistance * sin(ballBearing);
//        targetPos[0] = kickPos[0];
//        targetPos[1] = kickPos[1] + 1000;

        // FWD TEST
                const float kickAngle = 0.0f;//3.14 / 8.0f;
		const float kickDistance = 1000.0f;
        kickPos[0] = 15;
        kickPos[1] = -6;
        targetPos[0] = kickPos[0] + kickDistance * cos(kickAngle);
        targetPos[1] = kickPos[1] + kickDistance * sin(kickAngle);

        // RIGHT FOOT LEFT TEST
//        kickPos[0] = 8;
//        kickPos[1] = -10;
//        targetPos[0] = kickPos[0];
//        targetPos[1] = kickPos[1]+ 1000;

        if(m_provider->singleLeftBumperClick())
        {
            kickPos[0] = 1.0;
            kickPos[1] = 1.0;
            targetPos[0] = 1.0;
            targetPos[1] = 1.0;
            m_actions->addSound(m_actions->CurrentTime, NUSounds::SET);
        }
        else if(m_provider->singleRightBumperClick())
        {
            kickPos[0] = -1.0;
            kickPos[1] = -1.0;
            targetPos[0] = -1.0;
            targetPos[1] = -1.0;
            m_actions->addSound(m_actions->CurrentTime, NUSounds::SET);
        }

        KickJob* kick = new KickJob(0,kickPos,targetPos);
        m_provider->m_jobs->addMotionJob(kick);
/*
        if(m_provider->m_field_objects->mobileFieldObjects[FieldObjects::FO_BALL].isObjectVisible())
        {
            HeadTrackJob* head = new HeadTrackJob(m_provider->m_field_objects->mobileFieldObjects[FieldObjects::FO_BALL]);
            m_provider->m_jobs->addMotionJob(head);
        }
        else if(m_provider->m_field_objects->mobileFieldObjects[FieldObjects::FO_BALL].TimeSinceLastSeen() < 250.0f)
        {
            m_jobs->addMotionJob(new HeadPanJob(HeadPanJob::Ball));
        }
        */
    };
};

#endif

