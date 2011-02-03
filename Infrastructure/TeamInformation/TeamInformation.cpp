/*! @file TeamInformation.cpp
    @brief Implementation of TeamInformation class

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

#include "TeamInformation.h"
#include "Infrastructure/NUBlackboard.h"
#include "Infrastructure/NUSensorsData/NUSensorsData.h"
#include "Infrastructure/NUActionatorsData/NUActionatorsData.h"
#include "Infrastructure/FieldObjects/FieldObjects.h"
#include "NUPlatform/NUPlatform.h"

#include <memory.h>

#include "debug.h"
#include "debugverbositynetwork.h"

TeamInformation::TeamInformation(int playernum, int teamnum) : m_TIMEOUT(2000)
{
    m_player_number = playernum;
    m_team_number = teamnum;
    m_data = Blackboard->Sensors;
    m_actions = Blackboard->Actions;
    m_objects = Blackboard->Objects;
    
    initTeamPacket();
    m_received_packets = vector<boost::circular_buffer<TeamPacket> >(13, boost::circular_buffer<TeamPacket>(3));
}


TeamInformation::~TeamInformation()
{
}

bool TeamInformation::amIClosestToBall()
{
    for (size_t i=0; i<m_received_packets.size(); i++)
    {
        if (not m_received_packets[i].empty())
        {
            if ((m_data->CurrentTime - m_received_packets[i].back().ReceivedTime < m_TIMEOUT) and (m_packet.TimeToBall > m_received_packets[i].back().TimeToBall))
                return false;
        }
    }
    return true;
}

/*! @brief Returns all of the shared balls in the TeamInformation
 */
vector<TeamPacket::SharedBall> TeamInformation::getSharedBalls()
{
    vector<TeamPacket::SharedBall> sharedballs;
    sharedballs.reserve(m_received_packets.size());
    for (size_t i=0; i<m_received_packets.size(); i++)
    {
        if (not m_received_packets[i].empty() and (m_data->CurrentTime - m_received_packets[i].back().ReceivedTime < m_TIMEOUT))
        {   // if there is a received packet that is not too old grab the shared ball
            sharedballs.push_back(m_received_packets[i].back().Ball);
        }
    }
    return sharedballs;
}

/*! @brief Initialises my team packet to send to my team mates
 */
void TeamInformation::initTeamPacket()
{   
    memcpy(m_packet.Header, TEAM_PACKET_STRUCT_HEADER, sizeof(m_packet.Header));
    m_packet.ID = 0;
    // we initialise everything that never changes here
    m_packet.PlayerNumber = static_cast<char>(m_player_number);
    m_packet.TeamNumber = static_cast<char>(m_team_number);
}

/*! @brief Updates my team packet with the latest information
 */
void TeamInformation::updateTeamPacket()
{
    if (m_data == NULL or m_objects == NULL)
        return;

    m_packet.ID = m_packet.ID + 1;
    m_packet.SentTime = m_data->CurrentTime;
    m_packet.TimeToBall = getTimeToBall();
    
    // ------------------------------ Update shared localisation information
    // update shared ball
    MobileObject& ball = m_objects->mobileFieldObjects[FieldObjects::FO_BALL];
    m_packet.Ball.TimeSinceLastSeen = ball.TimeSinceLastSeen();
    m_packet.Ball.X = ball.X();
    m_packet.Ball.Y = ball.Y();
    m_packet.Ball.SRXX = ball.srXX();
    m_packet.Ball.SRXY = ball.srXY();
    m_packet.Ball.SRYY = ball.srYY();
    
    // update self
    Self& self = m_objects->self;
    m_packet.Self.X = self.wmX();
    m_packet.Self.Y = self.wmY();
    m_packet.Self.Heading = self.Heading();
    m_packet.Self.SDX = self.sdX();
    m_packet.Self.SDY = self.sdY();
    m_packet.Self.SDHeading = self.sdHeading();
}

float TeamInformation::getTimeToBall()
{
    float time = 600;
    
    Self& self = m_objects->self;
    MobileObject& ball = m_objects->mobileFieldObjects[FieldObjects::FO_BALL];
    float balldistance = ball.estimatedDistance();
    float ballbearing = ball.estimatedBearing();
    
    if (m_data->isIncapacitated())                                   // if we are incapacitated then we can't chase a ball
        return time;
    else if (m_player_number == 1 and balldistance > 150)            // goal keeper is a special case, don't chase balls too far away
        return time;
    else if ((not ball.lost() and not self.lost()) or m_objects->mobileFieldObjects[FieldObjects::FO_BALL].TimeSeen() > 0)
    {   // if neither the ball or self are lost or if we can see the ball then we can chase.
        vector<float> walkspeed, maxspeed;
        m_data->get(NUSensorsData::MotionWalkSpeed, walkspeed);
        m_data->get(NUSensorsData::MotionWalkMaxSpeed, maxspeed);
        
        // Add time for the movement to the ball
        time = balldistance/maxspeed[0] + fabs(ballbearing)/maxspeed[2];
        
        if (balldistance > 30)
        {   // Add time for the 'acceleration' from the current speed to the speed required to the ball
            time += 1.0*fabs(cos(ballbearing) - walkspeed[0]/maxspeed[0]) + 1.0*fabs(sin(ballbearing) - walkspeed[1]/maxspeed[1]) + 1.0*fabs(ballbearing - walkspeed[2]/maxspeed[2]);
        }
        
        if (self.lost())
            time += 3;
    }
    return time;
}

void TeamPacket::summaryTo(ostream& output)
{
    output << "ID: " << ID;
    output << " Player: " << (int)PlayerNumber;
    output << " Team: " << (int)TeamNumber;
    output << " TimeToBall: " << TimeToBall;
    output << endl;
}


ostream& operator<< (ostream& output, const TeamPacket& packet)
{
    output.write((char*) &packet, sizeof(packet));
    return output;
}

istream& operator>> (istream& input, TeamPacket& packet)
{
    TeamPacket packetBuffer;
    input.read(reinterpret_cast<char*>(&packetBuffer), sizeof(packetBuffer));
    packet = packetBuffer;
    
    return input;
}

ostream& operator<< (ostream& output, TeamInformation& info)
{
    info.updateTeamPacket();
    output << info.m_packet;
    //System->displayTeamPacketSent(info.m_actions);
    return output;
}

ostream& operator<< (ostream& output, TeamInformation* info)
{
    if (info != NULL)
        output << (*info);
    return output;
}

istream& operator>> (istream& input, TeamInformation& info)
{
    TeamPacket temp;
    input >> temp;
    double timenow;
    if (info.m_data != NULL)
        timenow = info.m_data->CurrentTime;
    else
        timenow = Platform->getTime();
    temp.ReceivedTime = timenow;
    
    if (temp.PlayerNumber > 0 and (unsigned) temp.PlayerNumber < info.m_received_packets.size() and temp.PlayerNumber != info.m_player_number and temp.TeamNumber == info.m_team_number)
    {   // only accept packets from valid player numbers
        // System->displayTeamPacketReceived(info.m_actions);
        if (info.m_received_packets[temp.PlayerNumber].empty())
        {   // if there have been no previous packets from this player always accept the packet
            info.m_received_packets[temp.PlayerNumber].push_back(temp);
        }
        else
        {
            TeamPacket lastpacket = info.m_received_packets[temp.PlayerNumber].back();
            if (timenow - lastpacket.ReceivedTime > 2000)
            {   // if there have been no packets recently from this player always accept the packet
                info.m_received_packets[temp.PlayerNumber].push_back(temp);
            }
            else if (temp.ID > lastpacket.ID)
            {   // avoid out of order packets by only adding recent packets that have a higher ID
                info.m_received_packets[temp.PlayerNumber].push_back(temp);
            }
        }
    }
    else
    {
        #if DEBUG_NETWORK_VERBOSITY > 0
            debug << ">>TeamInformation. Rejected team packet:";
            temp.summaryTo(debug);
            debug << endl;
        #endif
    }
    return input;
}

istream& operator>> (istream& input, TeamInformation* info)
{
    if (info != NULL)
        input >> (*info);
    return input;
}
