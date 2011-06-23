/**
7 states calculation
Xhat[0][0]=x
Xhat[1][0]=y
Xhat[2][0]=orientation
Xhat[3][0]=ballX
Xhat[4][0]=ballY
Xhat[5][0]=ballVeocityX
Xhat[6][0]=ballVeocityY
**/

#include "KF.h"
#include "Tools/Math/Matrix.h"
#include "Tools/Math/General.h"
#include <iostream>
#include "debug.h"
#include "odometryMotionModel.h"
#include "pose2d.h"
#include <sstream>

using namespace mathGeneral;

// Tuning Values (Constants)
const float KF::c_Kappa = 1.0f; // weight used in w matrix. (Constant)
const float KF::c_ballDecayRate = 0.985f; // Ball weighting 0.985 => speed halves every 1.5 seconds. (Constant)
const float KF::c_threshold2 = 15.0f; // Threshold for outlier rejection. Magic Number. (Constant)

const float KF::c_outlierLikelyhood = 1e-3;

// const float KF::sb_alpha1 = 0.05;
// const float KF::sb_alpha2 = 0.0005;
// const float KF::sb_alpha3 = 0.005;
// const float KF::sb_alpha4 = 0.0005;
// const float KF::sb_alpha5 = 0.0005;


// Ball distance measurement error weightings (Constant)
const float KF::c_R_ball_theta = 0.0001f;
const float KF::c_R_ball_range_offset = 25.0f; // (5cm)^2
const float KF::c_R_ball_range_relative = 0.0025f; // 5% of range added.


double muXx     = 0.07;
double muXy     = 0.00005;

double muYy     = 0.00005;
double muYx     = 0.000005;
// odom_Model(0.0005,0.0001,0.000005,0.0008)
KF::KF():odom_Model(0.07,0.00005,0.00005,0.000005)
{
    /**************************Initialization**************************/
	/*This is where values can be adjusted*/
  //frameRate = g_fps;
  frameRate = 30;

  m_alpha = 1.0; // Accuracy of model (0.0 -> 1.0)
  isActive = false; // Model currently in use.
  toBeActivated = false; // Model to be in use.

// Update Uncertainty
  updateUncertainties = Matrix(7, 7, true);
  updateUncertainties[5][5] = c_ballDecayRate; // Ball velocity x
  updateUncertainties[6][6] = c_ballDecayRate; // Ball velocity y
  updateUncertainties[3][5] = 1.0f/frameRate; // [ballX][ballXvelocity]
  updateUncertainties[4][6] = 1.0f/frameRate; // [ballY][ballYvelocity]

  init();									//Initialisation of Xhat and S

// Process Noise - Matrix Square Root of Q
  sqrtOfProcessNoise = Matrix(7,7,true);
//  sqrtOfProcessNoise[0][0] = 0.1; // Robot X coord.
//  sqrtOfProcessNoise[1][1] = 0.1; // Robot Y coord.
//  sqrtOfProcessNoise[2][2] = 0.001; // Robot Theta. 0.00001
  sqrtOfProcessNoise[0][0] = 0.1; // Robot X coord.
  sqrtOfProcessNoise[1][1] = 0.1; // Robot Y coord.
  sqrtOfProcessNoise[2][2] = 0.001; // Robot Theta. 0.00001
  sqrtOfProcessNoise[3][3] = 20.0; // Ball X.
  sqrtOfProcessNoise[4][4] = 20.0; // Ball Y.
  sqrtOfProcessNoise[5][5] = 5.6569; // Ball X Velocity.
  sqrtOfProcessNoise[6][6] = 5.6569; // Ball Y Velocity.

	//RHM 7/7/08: Extra matrix for resetting
  // Process noise for used for reset.
//  sqrtOfProcessNoiseReset = Matrix(7,7,false);
//  sqrtOfProcessNoiseReset[0][0] = 50.0; // extra 50cm sd when kidnapped?
//  sqrtOfProcessNoiseReset[1][1] = 50.0; // extra 50cm sd when kidnapped?
  //sqrtOfProcessNoiseReset[2][2] = 0.25; // extra 15deg shift when kidnapped? 0.25
//  sqrtOfProcessNoiseReset[2][2] = 0.5; // extra 15deg shift when kidnapped? 0.25
//  sqrtOfProcessNoiseReset[3][3] = 20.0; // ball itself shouldn't have moved much?
//  sqrtOfProcessNoiseReset[4][4] = 20.0; // just being cautious
	
  sqrtOfProcessNoiseReset = Matrix(7,7,false);
  sqrtOfProcessNoiseReset[0][0] = 150.0; // extra 50cm sd when kidnapped?
  sqrtOfProcessNoiseReset[1][1] = 100.0; // extra 50cm sd when kidnapped?
  //sqrtOfProcessNoiseReset[2][2] = 0.25; // extra 15deg shift when kidnapped? 0.25
  sqrtOfProcessNoiseReset[2][2] = 2.0; // extra 15deg shift when kidnapped? 0.25
  sqrtOfProcessNoiseReset[3][3] = 20.0; // ball itself shouldn't have moved much?
  sqrtOfProcessNoiseReset[4][4] = 20.0; // just being cautious


  nStates = stateEstimates.getm(); // number of states.
// Create Sigma Points matrix
 
  sigmaPoints = Matrix (nStates,2*nStates+1,false);

// Create square root of W matrix
  sqrtOfTestWeightings = Matrix(1,2*nStates+1,false);
  sqrtOfTestWeightings[0][0] = sqrt(c_Kappa/(nStates+c_Kappa));
  double outerWeight = sqrt(1.0/(2*(nStates+c_Kappa)));
  for(int i=1; i <= 2*nStates; i++){
    sqrtOfTestWeightings[0][i] = (outerWeight);
  }
  Matrix srukfCovX = Matrix(7,7,false);  // Original covariance mat
  Matrix srukfSq = Matrix(7,7,false);    // State noise square root covariance
  Matrix srukfSr = Matrix(7,7,false);    // Measurement noise square root cov
  Matrix srukfSx = Matrix(7,7,false);
  return;
}

void KF::init(){
  // Initial state estimates
    stateEstimates= Matrix(7,1,true);
    stateEstimates[2][0]=3+3.1416/2.0; // 0 for all values but robot bearing = 3.
  // S = Standard deviation matrix.
  // Initial Uncertainty
    stateStandardDeviations = Matrix(7,7,false);
    stateStandardDeviations[0][0] = 150; // 100 cm
    stateStandardDeviations[1][1] = 100; // 150 cm
    stateStandardDeviations[2][2] = 2;   // 2 radians
    stateStandardDeviations[3][3] = 150; // 100 cm
    stateStandardDeviations[4][4] = 100; // 150 cm
    stateStandardDeviations[5][5] = 10;  // 10 cm/s
    stateStandardDeviations[6][6] = 10;  // 10 cm/s
    
    srukfCovX = stateStandardDeviations*stateStandardDeviations.transp();
    srukfSx = cholesky(srukfCovX);
}

unsigned int KF::id() const
{
    return m_id;
}
unsigned int KF::parentId() const
{
    return m_parentId;
}

double KF::alpha() const
{
    return m_alpha;
}

void KF::setAlpha(double new_alpha)
{
    m_alpha = new_alpha;
    return;
}

unsigned int KF::GenerateId()
{
    static unsigned int s_next_id = 0;
    return s_next_id++;
}

unsigned int KF::spawnFromModel(const KF& parent)
{
    m_parentId = parent.id();
    m_id = KF::GenerateId();
    //! @TODO: add copy model lines. (not sure what is required yet).
    return id();
}

void KF::performFiltering(double odom_X, double odom_Y, double odom_Theta)
{
	
	
// 	cout<<"Cov Matrix : " <<srukfCovX<<endl;
	// Step 1 : Calculate sqaure root of Covariance
	
	//-----------------------------------------------------------------------------------------------
	
	
	
	
	// Step 2 : Calculate new sigma points based on previous covariance
	double sigmaAngleMax = 2.5;               // required for normalising Angle
	
	sigmaPoints.setCol(0, stateEstimates);    // First sigma point is the mean itself
			
	for(int i=1;i<nStates+1;i++)
	{
		// Eqn 17
		sigmaPoints.setCol(i, stateEstimates + sqrt((double)nStates + c_Kappa) * stateStandardDeviations.getCol(i-1)  );  

		// Crop angle
		sigmaPoints[2][i] = crop(sigmaPoints[2][i], (-sigmaAngleMax + stateEstimates[2][0]), (sigmaAngleMax + stateEstimates[2][0])); 

		// Eqn 18
		sigmaPoints.setCol(nStates + i,stateEstimates - sqrt((double)nStates + c_Kappa) * stateStandardDeviations.getCol(i-1) );

		// Crop angle again for the negative equation
	  	sigmaPoints[2][nStates + i] = crop(sigmaPoints[2][nStates + i], (-sigmaAngleMax + stateEstimates[2][0]), (sigmaAngleMax + stateEstimates[2][0]));
	}
	//-----------------------------------------------------------------------------------------------
	
	
	
	// Step 3: Update the state estimate - Pass all sigma points through motion model
	
	Pose2D oldPose, diffOdom;
	double *newPose;

	for (int i = 0 ; i < 2 * nStates + 1; i++)
	{
		oldPose.X = sigmaPoints[0][i];
		oldPose.Y = sigmaPoints[1][i];
		oldPose.Theta = sigmaPoints[2][i];
   
		diffOdom.X = odom_X;
		diffOdom.Y = odom_Y;
		diffOdom.Theta = odom_Theta;
   
		newPose = odom_Model.getNextSigma(diffOdom,oldPose);

		sigmaPoints[0][i] = *newPose;
		sigmaPoints[1][i] = *(newPose+1);
		sigmaPoints[2][i] = *(newPose+2);
//  		   cout<<"\nOld Sigma = [ "<<oldPose.X<<", "<<oldPose.Y<<", "<<oldPose.Theta<<" ]"<<"\tNew Sigma = [ "<<sigmaPoints[0][i]<<", "<<sigmaPoints[1][i]<<", "<<sigmaPoints[2][i]<<" ]";
 
	}
	//-----------------------------------------------------------------------------------------------
	
	
	
	
	// Step 4: Calculate new state based on propagated sigma points and the weightings of the sigmaPoints
	Matrix newStateEstimates(stateEstimates.getm(),stateEstimates.getn(), false);
	
	for(int i=0; i <= 2*nStates; i++)
	{
		// Eqn 20
		newStateEstimates = newStateEstimates + sqrtOfTestWeightings[0][i]*sqrtOfTestWeightings[0][i]*sigmaPoints.getCol(i);
	}
	//-----------------------------------------------------------------------------------------------
	
	// Step 5: Calculate measurement error and then find new srukfSx
	Matrix newSrukfSx(stateStandardDeviations.getm(),stateStandardDeviations.getn(), false);
	Matrix Mx(sigmaPoints.getm(),sigmaPoints.getn(), false);
  	
  	for(int i=0; i <= 2*nStates; i++)
	{
		// Eqn 21 part
		Mx.setCol(i, sqrtOfTestWeightings[0][i] * (sigmaPoints.getCol(i) - newStateEstimates));      // Error matrix
		
 	}
	
	// Eqn 23 originally it should be a Mx*Mz
	stateStandardDeviations = HT(Mx);
	stateEstimates = newStateEstimates;  
/*	return KF_OK;*/
}



void KF::timeUpdate(double deltaTime){
		
   	//-----------------------Update for ball velocity	
	stateEstimates[3][0] = stateEstimates[3][0] + stateEstimates[5][0]/frameRate; // Update ball x position by ball x velocity.
	stateEstimates[4][0] = stateEstimates[4][0] + stateEstimates[6][0]/frameRate; // Update ball y position by ball y velocity.
	stateEstimates[5][0] = c_ballDecayRate*stateEstimates[5][0]; // Reduce ball x velocity assuming deceleration
	stateEstimates[6][0] = c_ballDecayRate*stateEstimates[6][0]; // Reduce ball y velocity assuming deceleration

  // Householder transform. Unscented KF algorithm. Takes a while.
	stateStandardDeviations=HT(horzcat(updateUncertainties*stateStandardDeviations, sqrtOfProcessNoise));	
        stateEstimates[2][0] = normaliseAngle(stateEstimates[2][0]); // unwrap the robots angle to keep within -pi < theta < pi.
	return;
}


void KF::timeUpdate(float odom_x, float odom_y, float odom_theta, double deltaTime)
{
    double deltaTimeSeconds = deltaTime / 1000;

    Matrix updateSd = updateUncertainties;

    //-----------------------Update for ball velocity
    stateEstimates[KF::ballX][0] = stateEstimates[KF::ballX][0] + stateEstimates[KF::ballXVelocity][0]*deltaTimeSeconds; // Update ball x position by ball x velocity.
    stateEstimates[KF::ballY][0] = stateEstimates[KF::ballY][0] + stateEstimates[KF::ballYVelocity][0]*deltaTimeSeconds; // Update ball y position by ball y velocity.
    stateEstimates[KF::ballXVelocity][0] = c_ballDecayRate*stateEstimates[KF::ballXVelocity][0]; // Reduce ball x velocity assuming deceleration
    stateEstimates[KF::ballYVelocity][0] = c_ballDecayRate*stateEstimates[KF::ballYVelocity][0]; // Reduce ball y velocity assuming deceleration

    // Update for odometry
    float mid_theta = stateEstimates[KF::selfTheta][0] + 0.5*odom_theta;
    stateEstimates[KF::selfX][0] = stateEstimates[KF::selfX][0] + odom_x*cos(mid_theta) - odom_y*sin(mid_theta);
    stateEstimates[KF::selfY][0] = stateEstimates[KF::selfY][0] + odom_x*sin(mid_theta) - odom_y*cos(mid_theta);
    stateEstimates[KF::selfTheta][0] = stateEstimates[KF::selfTheta][0] + odom_theta;

    // Estimate noise due to odometry motion
    float muXx = 0.01;
    float muXy = 0.0;     // don't know why but it looks like the covariance's should be 0
    float muYy = 0.01;
    float muYx = 0.0;
    float muTt = 0.01;
    float muXt = 0.0003;
    float muYt = 0.0003;

    double varX = muXx*fabs(odom_x) + muYx*fabs(odom_y);
    double varY = muYy*fabs(odom_y) + muXy*fabs(odom_x);
    double varTheta = muTt*fabs(odom_theta) + muXt*fabs(odom_x) + muYt*fabs(odom_y);

    updateSd[KF::selfX][KF::selfX] += muXx*fabs(odom_x);
    updateSd[KF::selfX][KF::selfY] += muYx*fabs(odom_y);
    updateSd[KF::selfY][KF::selfY] += muYy*fabs(odom_y);
    updateSd[KF::selfY][KF::selfX] += muXy*fabs(odom_x);
    updateSd[KF::selfTheta][KF::selfTheta] += muTt*fabs(odom_theta);
    updateSd[KF::selfTheta][KF::selfX] += muXt*fabs(odom_x);
    updateSd[KF::selfTheta][KF::selfY] += muYt*fabs(odom_y);


    // Householder transform. Unscented KF algorithm. Takes a while.
    stateStandardDeviations=HT(horzcat(updateSd*stateStandardDeviations, sqrtOfProcessNoise));
    stateEstimates[2][0] = normaliseAngle(stateEstimates[2][0]); // unwrap the robots angle to keep within -pi < theta < pi.
}



// RHM 7/7/08: Additional function for resetting
void KF::Reset(){
	// Add extra uncertainty
	stateStandardDeviations = HT(horzcat(stateStandardDeviations, sqrtOfProcessNoiseReset));    
}



KfUpdateResult KF::odometeryUpdate(double odom_X, double odom_Y, double odom_Theta, double R_X, double R_Y, double R_Theta)
{
	
// 	std::cout << "Calculating sigma points." << std::endl;
  // Unscented KF Stuff.
	Matrix yBar;                                  	//reset
	Matrix Py;
	Matrix Pxy=Matrix(7, 2, false);                    //Pxy=[0;0;0];
	Matrix scriptX=Matrix(stateEstimates.getm(), 2 * nStates + 1, false);
	scriptX.setCol(0, stateEstimates);                         //scriptX(:,1)=Xhat;                  
    
  //----------------Saturate ScriptX angle sigma points to not wrap
	double sigmaAngleMax = 2.5;
	for(int i=1;i<nStates+1;i++){//hack to make test points distributed
    // Addition Portion.
		scriptX.setCol(i, stateEstimates + sqrt((double)nStates + c_Kappa) * stateStandardDeviations.getCol(i - 1));
	// Crop heading
                scriptX[2][i] = crop(scriptX[2][i], (-sigmaAngleMax + stateEstimates[2][0]), (sigmaAngleMax + stateEstimates[2][0]));
    // Subtraction Portion.
		scriptX.setCol(nStates + i,stateEstimates - sqrt((double)nStates + c_Kappa) * stateStandardDeviations.getCol(i - 1));
	// Crop heading
                scriptX[2][nStates + i] = crop(scriptX[2][nStates + i], (-sigmaAngleMax + stateEstimates[2][0]), (sigmaAngleMax + stateEstimates[2][0]));
	}
	//----------------------------------------------------------------
// 	std::cout << "Running motion model." << std::endl;
	Pose2D oldPose, diffOdom;
	double *newPose;

	sigmaPoints = scriptX;

	for (int i = 0 ; i < 2 * nStates + 1; i++)
	{
		oldPose.X = scriptX[0][i];
		oldPose.Y = scriptX[1][i];
		oldPose.Theta = scriptX[2][i];
   
		diffOdom.X = odom_X;
		diffOdom.Y = odom_Y;
		diffOdom.Theta = odom_Theta;
   
		newPose = odom_Model.getNextSigma(diffOdom,oldPose);

		sigmaPoints[0][i] = *newPose;
		sigmaPoints[1][i] = *(newPose+1);
		sigmaPoints[2][i] = *(newPose+2);
// 		cout<<"\nOld Sigma = [ "<<oldPose.X<<", "<<oldPose.Y<<", "<<oldPose.Theta<<" ]"<<"\tNew Sigma = [ "<<sigmaPoints[0][i]<<", "<<sigmaPoints[1][i]<<", "<<sigmaPoints[2][i]<<" ]";
   
   
	}

  // RUN MOTION MODEL HERE -> motionModel(MX,lambda)
//   std::cout << "Calculating new mean and variance." << std::endl;
    
  // Update Mean
	Matrix newStateEstimates(stateEstimates.getm(),stateEstimates.getn(), false);
	Matrix newCovariance(stateStandardDeviations.getm(),stateStandardDeviations.getn(), false);

//   std::cout << "Calculating Mean." << std::endl;
	for(int i=0; i <= 2*nStates; i++){
		newStateEstimates = newStateEstimates + sqrtOfTestWeightings[0][i]*sqrtOfTestWeightings[0][i]*sigmaPoints.getCol(i);
	}
	cout<<"New Mean    = ["<<newStateEstimates[0][0]<<", "<<newStateEstimates[1][0]<<", "<<newStateEstimates[1][0]<<" ]"<<endl;
// std::cout << "Calculating Covariance." << std::endl;
	Matrix temp;
  // Update Covariance
	for(int i=0; i <= 2*nStates; i++){
		temp = sigmaPoints.getCol(i) - newStateEstimates;
		newCovariance = newCovariance + sqrtOfTestWeightings[0][i]*sqrtOfTestWeightings[0][i]*temp*temp.transp();
	}
//   std::cout << "Updating current state." << std::endl;
	stateEstimates = newStateEstimates;
//   newCovariance.print();
	stateStandardDeviations = cholesky(newCovariance);
//   cout<<"\nNew current State = ["<<stateStandardDeviations[0][0]<<", "<<stateStandardDeviations[1][0]<<", "<<stateStandardDeviations[1][0]<<" ]";
	return KF_OK; 
}

KfUpdateResult KF::ballmeas(double Ballmeas, double theta_Ballmeas){
  double ballX_rel = Ballmeas * cos(theta_Ballmeas);
  double ballY_rel = Ballmeas * sin(theta_Ballmeas);
  // RHM 8/7/08:
  double R_range = c_R_ball_range_offset + c_R_ball_range_relative*pow(Ballmeas,2);
  double R_bearing = c_R_ball_theta;
    
  // Calculate update uncertainties - S_ball_rel & R_ball_rel.
  Matrix S_ball_rel = Matrix(2,2,false);
  S_ball_rel[0][0] = cos(theta_Ballmeas) * sqrt(R_range);
  S_ball_rel[0][1] = -sin(theta_Ballmeas) * Ballmeas * sqrt(R_bearing);
  S_ball_rel[1][0] = sin(theta_Ballmeas) * sqrt(R_range);
  S_ball_rel[1][1] = cos(theta_Ballmeas) * Ballmeas * sqrt(R_bearing);

  Matrix R_ball_rel = S_ball_rel * S_ball_rel.transp();  // R = S^2

  Matrix yBar;                                  	//reset
  Matrix Py;
  Matrix Pxy = Matrix(7,2,false);                    //Pxy=[0;0;0];

  Matrix scriptX = Matrix(stateEstimates.getm(),2 * nStates + 1,false);
  scriptX.setCol(0,stateEstimates);                         //scriptX(:,1)=Xhat; Current state.
  for(int i = 1; i <= nStates; i++){  // Unscented KF. Creates test points used to compare against vision data.
    // Addition Portion.
    scriptX.setCol(i, stateEstimates + sqrt(nStates + c_Kappa) * stateStandardDeviations.getCol(i - 1));
    // Subtraction Portion.
    scriptX.setCol(nStates + i, stateEstimates - sqrt(nStates + c_Kappa) * stateStandardDeviations.getCol(i - 1));
  }
    
  Matrix scriptY = Matrix(2, 2 * nStates + 1, false);
  Matrix temp = Matrix(2,1,false);
  for(int i = 0; i < 2 * nStates + 1; i++){
    temp[0][0] = (scriptX[3][i] - scriptX[0][i]) * cos(scriptX[2][i]) + (scriptX[4][i] - scriptX[1][i]) * sin(scriptX[2][i]);
    temp[1][0] = -(scriptX[3][i] - scriptX[0][i]) * sin(scriptX[2][i]) + (scriptX[4][i] - scriptX[1][i]) * cos(scriptX[2][i]);
    scriptY.setCol(i,temp.getCol(0));
  }
    
  Matrix Mx = Matrix(scriptX.getm(), 2 * nStates + 1, false);
  Matrix My = Matrix(scriptY.getm(), 2 * nStates + 1, false);  
  for(int i = 0; i < 2 * nStates + 1; i++){
    Mx.setCol(i, sqrtOfTestWeightings[0][i] * scriptX.getCol(i));
    My.setCol(i, sqrtOfTestWeightings[0][i] * scriptY.getCol(i));
  }                                      

  Matrix M1 = sqrtOfTestWeightings;
  yBar = My * M1.transp(); // Predicted Measurement
  Py = (My - yBar * M1) * (My - yBar * M1).transp();
  Pxy = (Mx - stateEstimates * M1) * (My -yBar * M1).transp();
    
  Matrix K = Pxy * Invert22(Py + R_ball_rel);   // Kalman Filter Gain.

  Matrix y = Matrix(2,1,false); // Measurement.
  y[0][0] = ballX_rel;
  y[1][0] = ballY_rel;
	
	//RHM: (3) Outlier rejection.
	// (i) Compute
  // double innovation2 = (yBar-temp).transp()*Invert22(Py+R_ball_rel)*(yBar-temp);
	double innovation2 = convDble((yBar - y).transp()*Invert22(Py + R_ball_rel)*(yBar - y));
	// (ii) Now compare this with a 'magic number' (threshold2) for outliers, and also vision
  // object confidence, something like if ((innovation2> threshold2) && (visionObject.confidence is small) )
  if (innovation2 > c_threshold2){
	//std::cout<<"not update"<<std::endl;
   	return KF_OUTLIER; // Outlier, so chuck it all out...
  }

  stateStandardDeviations = HT( horzcat(Mx - stateEstimates*M1 - K*My + K*yBar* M1, K*S_ball_rel) ); // Update uncertainties.
  stateEstimates = stateEstimates - K * (yBar - y); // Update States.
    
  return KF_OK; // It all worked OK!
}



// RHM 7/7/08: Change for resetting (return int)
KfUpdateResult KF::fieldObjectmeas(double distance,double bearing,double objX, double objY, double distanceErrorOffset, double distanceErrorRelative, double bearingError){
  double objX_rel = distance * cos(bearing);
  double objY_rel = distance * sin(bearing);

  double R_range = distanceErrorOffset + distanceErrorRelative * pow(distance, 2);
  double R_bearing = bearingError;
  //if(not_goal && INGORE_RANGE) R_range= 22500;	//150^2

  // Calculate update uncertainties - S_obj_rel & R_obj_rel
  Matrix S_obj_rel = Matrix(2,2,false);
  S_obj_rel[0][0] = cos(bearing) * sqrt(R_range);
  S_obj_rel[0][1] = -sin(bearing) * distance * sqrt(R_bearing);
  S_obj_rel[1][0] = sin(bearing) * sqrt(R_range);
  S_obj_rel[1][1] = cos(bearing) * distance * sqrt(R_bearing);

  Matrix R_obj_rel = S_obj_rel * S_obj_rel.transp(); // R = S^2

  // Unscented KF Stuff.
  Matrix yBar;                                  	//reset
  Matrix Py;
  Matrix Pxy=Matrix(7, 2, false);                    //Pxy=[0;0;0];
  Matrix scriptX=Matrix(stateEstimates.getm(), 2 * nStates + 1, false);
  scriptX.setCol(0, stateEstimates);                         //scriptX(:,1)=Xhat;                  
    
  //----------------Saturate ScriptX angle sigma points to not wrap
  double sigmaAngleMax = 2.5;
  for(int i=1;i<nStates+1;i++){//hack to make test points distributed
    // Addition Portion.
    scriptX.setCol(i, stateEstimates + sqrt((double)nStates + c_Kappa) * stateStandardDeviations.getCol(i - 1));
	// Crop heading.
    scriptX[2][i] = crop(scriptX[2][i], (-sigmaAngleMax + stateEstimates[2][0]), (sigmaAngleMax + stateEstimates[2][0]));
    // Subtraction Portion.
    scriptX.setCol(nStates + i,stateEstimates - sqrt((double)nStates + c_Kappa) * stateStandardDeviations.getCol(i - 1));
	// Crop heading.
    scriptX[2][nStates + i] = crop(scriptX[2][nStates + i], (-sigmaAngleMax + stateEstimates[2][0]), (sigmaAngleMax + stateEstimates[2][0]));
  }
	//----------------------------------------------------------------
  Matrix scriptY = Matrix(2, 2 * nStates + 1, false);
  Matrix temp = Matrix(2, 1, false);
 
  double dX,dY,Cc,Ss;
 
  for(int i = 0; i < 2 * nStates + 1; i++){
 	  dX = objX-scriptX[0][i];
 	  dY = objY-scriptX[1][i];
 	  Cc = cos(scriptX[2][i]);
 	  Ss = sin(scriptX[2][i]);       
    temp[0][0] = dX * Cc + dY * Ss;
    temp[1][0] = -dX * Ss + dY * Cc; 
    scriptY.setCol(i, temp.getCol(0));
  }
  Matrix Mx = Matrix(scriptX.getm(), 2 * nStates + 1, false);
  Matrix My = Matrix(scriptY.getm(), 2 * nStates + 1, false);  
  for(int i = 0; i < 2 * nStates + 1; i++){
    Mx.setCol(i, sqrtOfTestWeightings[0][i] * scriptX.getCol(i));
    My.setCol(i, sqrtOfTestWeightings[0][i] * scriptY.getCol(i));
  }
     
  Matrix M1 = sqrtOfTestWeightings;
  yBar = My * M1.transp(); // Predicted Measurement.
  Py = (My - yBar * M1) * (My - yBar * M1).transp();
  Pxy = (Mx - stateEstimates * M1) * (My - yBar * M1).transp();

  Matrix K = Pxy * Invert22(Py + R_obj_rel); // K = Kalman filter gain.

  Matrix y = Matrix(2,1,false); // Measurement. I terms of relative (x,y).
  y[0][0] = objX_rel;
  y[1][0] = objY_rel;
  //end of standard ukf stuff
	//RHM: 20/06/08 Outlier rejection.
  double innovation2 = convDble((yBar - y).transp() * Invert22(Py + R_obj_rel) * (yBar - y));

  // Update Alpha
  double innovation2measError = convDble((yBar - y).transp() * Invert22(R_obj_rel) * (yBar - y));
  m_alpha *= 1 / (1 + innovation2measError);
  //alpha *= CalculateAlphaWeighting(yBar - y,Py+R_obj_rel,c_outlierLikelyhood);

  if (innovation2 > c_threshold2){
		return KF_OUTLIER;
	}
  if (innovation2 < 0){
	  //std::cout<<"**********************************"<<std::endl;
	  //Py.print();
	  //R_obj_rel.print();
	  //std::cout<<"**********************************"<<std::endl;
  }

  stateStandardDeviations = HT( horzcat(Mx - stateEstimates*M1 - K*My + K*yBar*M1, K*S_obj_rel) );
  stateEstimates = stateEstimates - K*(yBar - y);
  return KF_OK;   
}



//
//RHM: 26/6/08 New function, needed for shared ball stuff (and maybe others....)
//======================================================================
//
void KF::linear2MeasurementUpdate(double Y1,double Y2, double SR11, double SR12, double SR22, int index1, int index2){
  //
  // KF update (called for example by shared ball) where a pair of measurements, [Y1;Y2];
  // with variance Square Root of Covariance (SR) [SR11 SR12; 0 SR22]
  // are predicted to be Xhat[index1][0] and Xhat[index2][0] respectively
  // This is based on the function fieldObjectmeas, but simplified.
  //
  // Example Call (given data from wireless: ballX, ballY, SRballXX, SRballXY, SRballYY)
  //      linear2MeasurementUpdate( ballX, ballY, SRballXX, SRballXY, SRballYY, 3, 4 )
  Matrix SR = Matrix(2,2,false);
  SR[0][0] = SR11;
  SR[0][1] = SR12;
  SR[1][1] = SR22;

  Matrix R = Matrix(2,2,false);
  R = SR * SR.transp();

  Matrix Py = Matrix(2, 2, false);
  Matrix Pxy = Matrix(7, 2, false);                   
 
  Matrix CS = Matrix(2, 7, false);
  CS.setRow(0, stateStandardDeviations.getRow(index1));
  CS.setRow(1, stateStandardDeviations.getRow(index2));

  Py = CS * CS.transp();
  Pxy = stateStandardDeviations * CS.transp();

  Matrix K = Pxy * Invert22(Py + R);   //Invert22

  Matrix y = Matrix(2, 1, false);
  y[0][0] = Y1;
  y[1][0] = Y2;
    
  Matrix yBar = Matrix(2, 1, false); //Estimated values of the measurements Y1,Y2
  yBar[0][0] = stateEstimates[index1][0];
  yBar[1][0] = stateEstimates[index2][0]; 
	//RHM: (3) Outlier rejection.
	double innovation2 = convDble( (yBar - y).transp() * Invert22(Py + SR * SR.transp()) * (yBar - y) ); 	
	//std::cout<<innovation2<<std::endl;
	if (innovation2 > c_threshold2){
		//std::cout<<"+++++++++++++++++not update+++++++++++++++++"<<std::endl;
		return;
	}
	if(innovation2 < 0){
		//std::cout<<"**********************************"<<std::endl;
		//Py.print();
		//std::cout<<"**********************************"<<std::endl;
	}

	stateStandardDeviations = HT(horzcat(stateStandardDeviations - K * CS, K * SR));
  stateEstimates = stateEstimates - K * (yBar - y);
  return;
}
//======================================================================================================
//
// RHM: 26/6/08 New Pseudo code for ball information to send over wireless
// Information to add to the broadcast to other robots:
// (along with other information)
// * Frame Number {timing indicator)
// * Frames since I last saw the ball
// * Xhat[3][0] (est ball X)
// * Xhat[4][0] (est ball Y)
// * SRxx, SRxy, SRyy (3 elements of 2x2 sqrt matrix formed as: HT(vertical_cat(S.getRow[3], S.getRow[4])))
// * Xhat[0][0]
// * Xhat[1][0] (could be used in behaviour)
// * other stuff probably
//
//

KfUpdateResult KF::updateAngleBetween(double angle, double x1, double y1, double x2, double y2, double sd_angle)
{
        // Method to take the angle between two objects, that is,
        // angle between object 1 (with fixed field coords (x1,y1))
        // and object 2 (with fixed field coords (x2,y2) ) and perform an update.
        // Note: as distinct from almost all other updates, the actual robot orientation
        // doesn't matter for this update, and so 'cropping' etc. of the sigma points is not requied.

        double R_angle;

    // Unscented KF Stuff.
    double yBar;                                  	//reset
    double Py;
    Matrix Pxy = Matrix(7, 1, false);                    //Pxy=[0;0;0];
    Matrix scriptX = Matrix(stateEstimates.getm(), 2 * nStates + 1, false);
    scriptX.setCol(0, stateEstimates);                         //scriptX(:,1)=Xhat;
    float weight = sqrt((double)nStates + c_Kappa);

    for (int i = 1; i <= nStates; i++) //populate matrix of test points
    {  // Unscented KF. Creates test points used to compare against vision data.
        scriptX.setCol(i, stateEstimates + weight * stateStandardDeviations.getCol(i - 1));
        scriptX.setCol(nStates + i, stateEstimates - weight * stateStandardDeviations.getCol(i - 1));
    }

    //----------------------------------------------------------------
    Matrix scriptY = Matrix(1, 2 * nStates + 1, false);

    double angleToObj1;
    double angleToObj2;

    for (int i = 0; i < 2 * nStates + 1; i++)
    {
        angleToObj1 = atan2 ( y1 - scriptX[1][i], x1 - scriptX[0][i] );
        angleToObj2 = atan2 ( y2 - scriptX[1][i], x2 - scriptX[0][i] );
        scriptY[0][i] = normaliseAngle(angleToObj1 - angleToObj2);
    }

    Matrix Mx = Matrix(scriptX.getm(), 2 * nStates + 1, false);
    Matrix My = Matrix(scriptY.getm(), 2 * nStates + 1, false);
    for (int i = 0; i < 2 * nStates + 1; i++)
    {
        Mx.setCol(i, sqrtOfTestWeightings[0][i] * scriptX.getCol(i));
        My.setCol(i, sqrtOfTestWeightings[0][i] * scriptY.getCol(i));
    }

    Matrix M1 = sqrtOfTestWeightings;
    yBar = convDble ( My * M1.transp() ); // Predicted Measurement.
    Py = convDble ((My - yBar * M1) * (My - yBar * M1).transp());
    Pxy = (Mx - stateEstimates * M1) * (My - yBar * M1).transp();

    R_angle  = sd_angle * sd_angle;

    Matrix K = Pxy /( Py + R_angle ); // K = Kalman filter gain.

    double y = angle;    //end of standard ukf stuff
    //Outlier rejection.
    double innovation2 = (yBar - y) * (yBar - y ) / ( Py + R_angle );

    // Update Alpha (used in multiple model)
    double innovation2measError = (yBar - y) * (yBar - y ) / R_angle ;
    m_alpha *= 1 / (1 + innovation2measError);

    if (innovation2 > c_threshold2)
    {
        return KF_OUTLIER;
    }

    stateStandardDeviations = HT( horzcat(Mx - stateEstimates*M1 - K*My + K*yBar*M1, K*sd_angle) );
    stateEstimates = stateEstimates - K*(yBar - y);
    return KF_OK;
}


double KF::variance(int Xi) const
{
	return convDble(stateStandardDeviations.getRow(Xi)*stateStandardDeviations.getRow(Xi).transp());
}



double KF::sd(int Xi) const
{
	return sqrt(variance(Xi));
}



double KF::state(int stateID) const
{
  return stateEstimates[stateID][0];
}



Matrix KF::GetBallSR() const
{
  return HT(vertcat(stateStandardDeviations.getRow(3), stateStandardDeviations.getRow(4)));
}



double KF::getDistanceToPosition(double posX, double posY) const
{
  double selfX = stateEstimates[0][0];
  double selfY = stateEstimates[1][0];
  double diffX = selfX - posX;
  double diffY = selfY - posY;
  double distance = sqrt( diffX * diffX + diffY * diffY );
  return distance;
}



double KF::getBearingToPosition(double posX, double posY) const
{
  double selfX = stateEstimates[0][0];
  double selfY = stateEstimates[1][0];
  double selfHeading = stateEstimates[2][0];
  double diffX = posX - selfX;
  double diffY = posY - selfY;
  if( (diffX == 0) && (diffY == 0)) diffY = 0.0001;
  double positionHeading = atan2(diffY, diffX);
  double bearing = normaliseAngle(positionHeading - selfHeading);
  return bearing;
}



bool KF::clipState(int stateIndex, double minValue, double maxValue){
    bool clipped = false;
	if(stateEstimates[stateIndex][0] > maxValue){
		double mult, Pii;
		Matrix Si;
		Si = stateStandardDeviations.getRow(stateIndex);
		Pii = convDble(Si * Si.transp());
		mult = (stateEstimates[stateIndex][0] - maxValue) / Pii;
		stateEstimates = stateEstimates - mult * stateStandardDeviations * Si.transp();
		stateEstimates[stateIndex][0] = maxValue;
        clipped = true;
	}
	if(stateEstimates[stateIndex][0] < minValue){
		double mult, Pii;
		Matrix Si;
		Si = stateStandardDeviations.getRow(stateIndex);
		Pii = convDble(Si * Si.transp());
		mult = (stateEstimates[stateIndex][0] - minValue) / Pii;
		stateEstimates = stateEstimates - mult * stateStandardDeviations * Si.transp();
		stateEstimates[stateIndex][0] = minValue;
        clipped = true;
	}
        stateEstimates[2][0] = normaliseAngle(stateEstimates[2][0]);
    return clipped;
}

Matrix KF::CalculateSigmaPoints() const
{
    Matrix scriptX=Matrix(stateEstimates.getm(), 2 * nStates + 1, false);
    scriptX.setCol(0, stateEstimates);                         //scriptX(:,1)=Xhat;

//----------------Saturate ScriptX angle sigma points to not wrap
    double sigmaAngleMax = 2.5;
    for(int i=1;i<nStates+1;i++){//hack to make test points distributed
// Addition Portion.
            scriptX.setCol(i, stateEstimates + sqrt((double)nStates + c_Kappa) * stateStandardDeviations.getCol(i - 1));
    // Crop heading
            scriptX[2][i] = crop(scriptX[2][i], (-sigmaAngleMax + stateEstimates[2][0]), (sigmaAngleMax + stateEstimates[2][0]));
// Subtraction Portion.
            scriptX.setCol(nStates + i,stateEstimates - sqrt((double)nStates + c_Kappa) * stateStandardDeviations.getCol(i - 1));
    // Crop heading
            scriptX[2][nStates + i] = crop(scriptX[2][nStates + i], (-sigmaAngleMax + stateEstimates[2][0]), (sigmaAngleMax + stateEstimates[2][0]));
    }
    return scriptX;
}

float KF::CalculateAlphaWeighting(const Matrix& innovation, const Matrix& innovationVariance, float outlierLikelyhood) const
{
    const int numMeas = 2;
    float notOutlierLikelyhood = 1.0 - outlierLikelyhood;
    float expRes = exp(-0.5*convDble(innovation.transp()*Invert22(innovationVariance)*innovation));
    float fracRes = 1.0 / ( sqrt( pow(2*PI,numMeas)*determinant(innovationVariance) ) );
    return notOutlierLikelyhood * fracRes * expRes + outlierLikelyhood;
}

std::string KF::summary(bool brief) const
{
    std::stringstream buffer;
    buffer << "Model Number " << id() << " Alpha: " << alpha() << " Position: (" << state(KF::selfX) << ",";
    buffer << state(KF::selfY) << "," << state(KF::selfTheta) << ") Ball Position: (" << state(KF::ballX) << ",";
    buffer << state(KF::ballY) << ")" << endl;
    if(!brief)
    {
            buffer << "Variance - Position: (" << sd(KF::selfX) << ",";
            buffer << sd(KF::selfY) << "," << sd(KF::selfTheta) << ") Ball Position: (" << sd(KF::ballX) << ",";
            buffer << sd(KF::ballY) << ")" << endl;
    }
    return buffer.str();
}

std::ostream& operator<< (std::ostream& output, const KF& p_kf)
{
    output.write(reinterpret_cast<const char*>(&p_kf.isActive), sizeof(p_kf.isActive));
    if(p_kf.isActive)
    {
        output.write(reinterpret_cast<const char*>(&p_kf.m_alpha), sizeof(p_kf.m_alpha));
        WriteMatrix(output,p_kf.stateEstimates);
        WriteMatrix(output,p_kf.stateStandardDeviations);
    }
    return output;
}

std::istream& operator>> (std::istream& input, KF& p_kf)
{
    input.read(reinterpret_cast<char*>(&p_kf.isActive), sizeof(p_kf.isActive));
    if(p_kf.isActive)
    {
        input.read(reinterpret_cast<char*>(&p_kf.m_alpha), sizeof(p_kf.m_alpha));
        p_kf.stateEstimates  = ReadMatrix(input);
        p_kf.stateStandardDeviations = ReadMatrix(input);
    }
    return input;
}
