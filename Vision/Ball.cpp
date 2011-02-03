#include "Ball.h"
#include "Vision.h"
#include "ClassificationColours.h"
#include "TransitionSegment.h"
#include "ScanLine.h"
#include "ClassifiedSection.h"
#include "debug.h"
#include "debugverbosityvision.h"
#include "Infrastructure/NUSensorsData/NUSensorsData.h"
//#include <QDebug>
Ball::Ball()
{
    //debug<< "Vision::DetectBall : Ball Class created" << endl;
}
Ball::~Ball()
{
}

//! Finds the ball segments and groups updates the ball in fieldObjects (Vision is used to further classify the object)
Circle Ball::FindBall(std::vector <ObjectCandidate> FO_Candidates, FieldObjects* AllObjects, Vision* vision,int height,int width)
{
    ObjectCandidate largestCandidate;
    int sizeOfLargestCandidate = 0;
    Circle result;
    result.centreX = 0;
    result.centreY = 0;
    result.radius = 0;
    result.sd = 0;

    //! Go through all the candidates: to find a possible ball
    //debug <<"FO_Candidates.size():"<< FO_Candidates.size();
    for(unsigned int i = 0; i  < FO_Candidates.size(); i++)
    {

        ObjectCandidate PossibleBall = FO_Candidates[i];

        if(!isObjectAPossibleBall(PossibleBall)) continue;

        if(isObjectTooBig(PossibleBall, vision)) continue;

        //! Check if the ratio is correct: Height and Width ratio should be 1 as it is a circle,
        //! through can be skewed (camera moving), so we better put some threshold on it.
        if(!isCorrectCheckRatio(PossibleBall, height, width)) continue;
        if(isObjectInRobot(PossibleBall, AllObjects)) continue;
        //debug << "BALL::FindBall  Possible Ball Found ";

        int sizeOfCandidate = (PossibleBall.getBottomRight().y - PossibleBall.getTopLeft().y); //Uses the height of the candidate, as width can be 0

        if(sizeOfCandidate > sizeOfLargestCandidate)
        {

            sizeOfLargestCandidate = sizeOfCandidate;
            largestCandidate = PossibleBall;
        }

    }
    //! Closely Classify the candidate: to obtain more information about the object (using closely classify function in vision)
    if(sizeOfLargestCandidate > 0)
    {
        largestCandidate.setColour(ClassIndex::orange);
        std::vector < Vector2<int> > ballPoints = classifyBallClosely(largestCandidate, vision,height, width);

        //! Perform Circle Fit: Must pass a threshold on fit to become a circle!
        //debug << "BALL::FindBall  Circle Fit ";

        result = isCorrectFit(ballPoints,largestCandidate, vision);
    }
    return result;
}

bool Ball::isObjectAPossibleBall(const ObjectCandidate &PossibleBall)
{
    if(PossibleBall.getColour()== ClassIndex::orange ||
       PossibleBall.getColour()== ClassIndex::pink_orange ||
       PossibleBall.getColour() == ClassIndex::yellow_orange)
    {
        std::vector<TransitionSegment >segments = PossibleBall.getSegments();
        int orangeSize = 0;
        //int pinkSize = 0;
        for(unsigned int i = 0; i <segments.size(); i++)
        {
            /*if(segments[i].getColour() == ClassIndex::pink || segments[i].getColour() == ClassIndex::pink_orange)
            {
                pinkSize = pinkSize + segments[j].getSize();
            }*/
            if(segments[i].getColour() == ClassIndex::orange)
            {
                orangeSize = orangeSize + segments[i].getSize();
            }

        }
        if(orangeSize > 0)
        {
            return true;
        }
        else
        {
            return false;
        }
    }
    else{
        return false;
    }

}
bool Ball::isObjectTooBig(const ObjectCandidate &PossibleBall, Vision* vision)
{
    float MaxPixels = getMaxPixelsOfBall(vision);
    int heightOfPossibleBall = PossibleBall.getBottomRight().y - PossibleBall.getTopLeft().y;
    int widthOfPossibleBall = PossibleBall.getBottomRight().x - PossibleBall.getTopLeft().x;

    if(heightOfPossibleBall > MaxPixels || widthOfPossibleBall > MaxPixels)
    {
        return true;
    }
    else
    {
        return false;
    }
}
float Ball::getMaxPixelsOfBall(Vision* vision)
{
    float ClosestDistance = 30.0; //CM with Robot sitting down (40cm), standing about 50cm.
    float MaxPixels = vision->EFFECTIVE_CAMERA_DISTANCE_IN_PIXELS() * ORANGE_BALL_DIAMETER / ClosestDistance;
    return MaxPixels;
}

bool Ball::isObjectInRobot(const ObjectCandidate &PossibleBall, FieldObjects *AllObjects)
{
    Vector2<int> topLeft = PossibleBall.getTopLeft();
    Vector2<int> bottomRight = PossibleBall.getBottomRight();
    bool isInRobot = false;
    //! Check the Ambiguous Objects: for robots:
    for(unsigned int i = 0; i < AllObjects->ambiguousFieldObjects.size(); i++)
    {
        if(AllObjects->ambiguousFieldObjects[i].getID() == FieldObjects::FO_PINK_ROBOT_UNKNOWN)
        {
            Vector2<int> robotTopLeft, robotBottomLeft;
            robotTopLeft.x = AllObjects->ambiguousFieldObjects[i].ScreenX() -  AllObjects->ambiguousFieldObjects[i].getObjectWidth()/2;
            robotTopLeft.y = AllObjects->ambiguousFieldObjects[i].ScreenY() -  AllObjects->ambiguousFieldObjects[i].getObjectHeight()/2;
            robotBottomLeft.x = AllObjects->ambiguousFieldObjects[i].ScreenX() +  AllObjects->ambiguousFieldObjects[i].getObjectWidth()/2;
            robotBottomLeft.y = AllObjects->ambiguousFieldObjects[i].ScreenY() +  AllObjects->ambiguousFieldObjects[i].getObjectHeight()/2;

            if( topLeft.x >= robotTopLeft.x && topLeft.y >= robotTopLeft.y && bottomRight.x <= robotBottomLeft.x &&  bottomRight.y <= robotBottomLeft.y)
            {
                if( (bottomRight.y - topLeft.y)  <  (robotBottomLeft.y - robotTopLeft.y)/10)
                {
                    isInRobot = true;
                    return isInRobot;
                }
            }
        }
    }
    return isInRobot;
}

std::vector < Vector2<int> > Ball::classifyBallClosely(const ObjectCandidate &PossibleBall,Vision* vision,int height, int width)
{
    int buffer = 20;
    Vector2<int> TopLeft = PossibleBall.getTopLeft();
    Vector2<int> BottomRight = PossibleBall.getBottomRight();
    int midX =  (int)((BottomRight.x-TopLeft.x)/2)+TopLeft.x;
    int midY =  (int)((BottomRight.y-TopLeft.y)/2)+TopLeft.y;
    Vector2<int> SegStart;
    SegStart.x = midX;
    SegStart.y = TopLeft.y;
    Vector2<int> SegEnd;
    SegEnd.x = midX;
    SegEnd.y = BottomRight.y;
    TransitionSegment tempSeg(SegStart,SegEnd,ClassIndex::unclassified,PossibleBall.getColour(),ClassIndex::unclassified);
    ScanLine tempLine = ScanLine();

    //! Maximum ball points = 4*2 = 8;
    int spacings = (int)(BottomRight.y - TopLeft.y)/4;
    if(spacings < 2)
    {
        spacings = 2;
    }
    //qDebug() << spacings ;
    std::vector<unsigned char> colourlist;
    colourlist.reserve(3);
    colourlist.push_back(ClassIndex::orange);
    colourlist.push_back(ClassIndex::pink_orange);
    colourlist.push_back(ClassIndex::yellow_orange);
    int direction = ScanLine::DOWN;
    //qDebug() << "Horizontal Scan : ";
    vision->CloselyClassifyScanline(&tempLine,&tempSeg,spacings, direction,colourlist);

    std::vector< Vector2<int> > BallPoints;

    BallPoints.push_back(SegStart);
    BallPoints.push_back(SegEnd);
    //! Debug Output for small scans:
    for(int i = 0; i < tempLine.getNumberOfSegments(); i++)
    {
        TransitionSegment* tempSegement = tempLine.getSegment(i);
        //! Check if the segments are at the edge of screen
        if(!(tempSegement->getStartPoint().x < buffer || tempSegement->getStartPoint().y < buffer))
        {
            BallPoints.push_back(tempSegement->getStartPoint());
        }
        if(!(tempSegement->getEndPoint().x >= height-buffer || tempSegement->getEndPoint().x >= width-buffer))
        {
            BallPoints.push_back(tempSegement->getEndPoint());
        }

        /*qDebug() << "Horizontal Points At " <<i<<"\t Size: "<< tempSegement->getSize()<< "\t Start(x,y),End(x,y):("<< tempSegement->getStartPoint().x
                <<","<< tempSegement->getStartPoint().y << ")("<< tempSegement->getEndPoint().x
                <<","<< tempSegement->getEndPoint().y << ")";*/

    }

    SegStart.x = TopLeft.x;
    SegStart.y = midY;
    SegEnd.x = BottomRight.x;
    SegEnd.y = midY;
    tempSeg = TransitionSegment(SegStart,SegEnd,ClassIndex::unclassified,PossibleBall.getColour(),ClassIndex::unclassified);
    tempLine = ScanLine();

    BallPoints.push_back(SegStart);
    BallPoints.push_back(SegEnd);

    //! Maximum ball points = 4*2 = 8;
    spacings = (int)(BottomRight.y - TopLeft.y)/4;
    if(spacings < 2)
    {
        spacings = 2;
    }
    //qDebug() << "Vertical Scan : ";
    direction = ScanLine::LEFT;
    vision->CloselyClassifyScanline(&tempLine,&tempSeg,spacings, direction, colourlist);
    for(int i = 0; i < tempLine.getNumberOfSegments(); i++)
    {

        TransitionSegment* tempSegement = tempLine.getSegment(i);
        //! Check if the segments are at the edge of screen
        if(!(tempSegement->getStartPoint().x < buffer || tempSegement->getStartPoint().y < buffer))
        {
            BallPoints.push_back(tempSegement->getStartPoint());
        }

        float headElevation = 0.0;
        vision->getSensorsData()->getPosition(NUSensorsData::HeadPitch,headElevation);

        if(!(tempSegement->getEndPoint().y >= height-buffer || tempSegement->getEndPoint().x >= width-buffer) &&  headElevation < 0.3)
        {
            BallPoints.push_back(tempSegement->getEndPoint());
        }
        //vision->getSensorsData()->getJointPosition(NUSensorsData::HeadPitch,headElevation);
        //qDebug() << "Ball Head Elevation:" << headElevation;
        /*qDebug() << "Veritcal Points At " <<i<<"\t Size: "<< tempSegement->getSize()<< "\t Start(x,y),End(x,y):("<< tempSegement->getStartPoint().x
                <<","<< tempSegement->getStartPoint().y << ")("<< tempSegement->getEndPoint().x
                <<","<< tempSegement->getEndPoint().y << ")";*/

    }

    return BallPoints;

}
bool Ball::isCorrectCheckRatio(const ObjectCandidate &PossibleBall,int height, int width)
{
    //debug << "Checking Ratio: " << PossibleBall.aspect();

    //! Check if at Edge of Screen, if so continue with other checks, otherwise, look at ratio and check if in thresshold
    int boarder = 10; //! Boarder of pixels
    if (( PossibleBall.getBottomRight().y - PossibleBall.getTopLeft().y) <= 3) return false;
    if (PossibleBall.getBottomRight().x <= width-boarder &&
        PossibleBall.getBottomRight().y <= height-boarder &&
        PossibleBall.getTopLeft().x >=0+boarder  &&
        PossibleBall.getTopLeft().y >=0+boarder  )
    {
        //POSSIBLE BALLS ARE:
        //      Objects which have grouped segments,
        //      or objects with one segment, but very small (still like to consider).
        if((PossibleBall.aspect() > 0.3 && PossibleBall.aspect() < 2 )|| PossibleBall.aspect()==0)
        {
            return true;
        }
        else
        {
            //debug << "Thrown out due to incorrect ratio";
            return false;
        }
    }
    else
    {
        //debug << "Returned True at edge of screen";
        return true;
    }
}
Circle Ball::isCorrectFit(const std::vector < Vector2<int> > &ballPoints, const ObjectCandidate &PossibleBall, Vision* vision)
{
    Circle circ;
    circ.radius = 0.0;
    circ.isDefined = false;
    CircleFitting CircleFit;

    //debug << "Points:";
   /* for(int i =0; i < ballPoints.size(); i++)
    {
        debug << "("<<ballPoints[i].x << ","<<ballPoints[i].y<< ")";
    }*/
    if(ballPoints.size() > 10)
    {

            circ = CircleFit.FitCircleLMA(ballPoints);
            if(circ.sd > 5 ||  circ.radius*2 > getMaxPixelsOfBall(vision) )
            {
                circ.isDefined = false;
            }
            //qDebug() << "Circle found " << circ.isDefined<<": (" << circ.centreX << "," << circ.centreY << ") Radius: "<< circ.radius << " Fitting: " << circ.sd<< endl;

    }
    else if ((ballPoints.size() <= 10))
    {
        Vector2<int> bottomRight = PossibleBall.getBottomRight();
        Vector2<int> topLeft = PossibleBall.getTopLeft();
        //! find midPoints of the Candidate:
        circ.centreX = (bottomRight.x + topLeft.x)/2;
        circ.centreY = (bottomRight.y + topLeft.y)/2;
        circ.isDefined = true;
        circ.sd = fabs(fabs(bottomRight.x - topLeft.x) - fabs(bottomRight.y - topLeft.y));      //!< Uncertianty, is somewhere between the candidates height and widths
        //! Select the Largest side as radius:
        if(fabs(bottomRight.x - topLeft.x) > fabs(bottomRight.y - topLeft.y))
        {
             circ.radius= fabs(bottomRight.x - topLeft.x)/2;
        }
        else
        {
            circ.radius = fabs(bottomRight.y - topLeft.y)/2;
        }
        //debug << "Circle cannot be fitted: Used Candidate information" << endl;
    }

    //debug << "BALL::CircleFit returning circle r =" << circ.radius;
    //delete CircleFit;
    return circ;
}
