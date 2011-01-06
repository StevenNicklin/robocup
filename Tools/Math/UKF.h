#ifndef UKF_H
#define UKF_H
#include "Tools/Math/Eigen/Core"
#include "Tools/Math/Eigen/Cholesky"
#define N_SIGMA_POINTS (2*NStates+1)
template<int NStates>
class UKF
{
public:
    typedef Eigen::Matrix<double, NStates, N_SIGMA_POINTS> SigmaMatrix;
    typedef Eigen::Matrix<double, NStates, NStates> ProcessMatrix;
    typedef Eigen::Matrix<double, 1, N_SIGMA_POINTS> WeightVector;
    typedef Eigen::Matrix<double, NStates, 1> StateVector;

    UKF(){};
    UKF(const UKF& source){};
    ~UKF(){};
    void CalculateSigmaWeights(float kappa = 1.0f)
    {
        double meanWeight = kappa/(NStates+kappa);
        double outerWeight = (1.0-meanWeight)/(N_SIGMA_POINTS - 1);
        // First weight
        m_sigmaWeights(0) = meanWeight;
        m_sqrtSigmaWeights(0) = sqrt(meanWeight);
        // The rest
        for(unsigned int i = 1; i < NStates; i++)
        {
            m_sigmaWeights(i) = outerWeight;
            m_sqrtSigmaWeights(i) = sqrt(outerWeight);
        }
    }

    SigmaMatrix GenerateSigmaPoints() const
    {
        SigmaMatrix sigmaPoints;

        sigmaPoints.col(0) = m_mean; // First sigma point is the current mean with no deviation
        StateVector deviation;
        ProcessMatrix covariance = (NStates / (1-m_sigmaWeights(0,0)) * m_covariance);

        ProcessMatrix sqtCovariance = ProcessMatrix::Zero();
        sqtCovariance = covariance.ldlt().matrixL();

        for(unsigned int i = 1; i < NStates + 1; i++){
            int negIndex = i+NStates;
            deviation = sqtCovariance.col(i - 1);        // Get weighted deviation
            sigmaPoints.col(i) = (m_mean + deviation);                // Add mean + deviation
            sigmaPoints.col(negIndex) = (m_mean - deviation);  // Add mean - deviation
        }
        return sigmaPoints;
    };

    template <int NDimensions, int NPoints>
    Eigen::Matrix<double,NDimensions,1> CalculateMeanFromSigmas(const Eigen::Matrix<double,NDimensions,NPoints>& sigmaPoints) const
    {
        Eigen::Matrix<double,NDimensions,1> mean;
        mean = sigmaPoints * m_sigmaWeights.transpose();
        return mean;
    };

    template <int NDimensions, int NPoints>
    Eigen::Matrix<double,NDimensions,NDimensions> CalculateCovarianceFromSigmas(const Eigen::Matrix<double,NDimensions,NPoints>& sigmaPoints, const Eigen::Matrix<double,NDimensions,1>& mean) const
    {
        unsigned int numPoints = sigmaPoints.cols();
        Eigen::Matrix<double,NDimensions,NDimensions> covariance;
        Eigen::Matrix<double,NDimensions,1> diff;
        for(unsigned int i = 0; i < numPoints; ++i)
        {
            diff = sigmaPoints.col(i) - mean;
            covariance = covariance + m_sigmaWeights(i)*diff*diff.transpose();
        }
        return covariance;
    };

    void setMean(const StateVector& newMean)
    {
        m_mean = newMean;
    };
    void setCovariance(const ProcessMatrix& newCovariance)
    {
        m_covariance = newCovariance;
    };
    double getMean(int stateId) const
    {
        return m_mean(stateId);
    };
    double calculateSd(int stateId) const
    {
        return m_covariance(stateId,stateId);
    };
    void setState(const StateVector& mean, const ProcessMatrix& covariance)
    {
        setMean(mean);
        setCovariance(covariance);
    };

    bool timeUpdate(const SigmaMatrix& updatedSigmaPoints, const ProcessMatrix& processNoise)
    {
        m_mean = CalculateMeanFromSigmas(updatedSigmaPoints);
        // Update covariance assuming additive process noise.
        m_covariance = CalculateCovarianceFromSigmas(updatedSigmaPoints, m_mean) + processNoise;
        return true;
    };

    template<int NMeasurements>
    bool measurementUpdate(const Eigen::Matrix<double,NMeasurements,1>& measurement, const Eigen::Matrix<double,NMeasurements,NMeasurements>& measurementNoise, const Eigen::Matrix<double,NMeasurements,2*NStates+1>& predictedMeasurementSigmas, const SigmaMatrix& stateEstimateSigmas)
    {
        // Find mean of predicted measurement
        Eigen::Matrix<double,NMeasurements,1> predictedMeasurement = CalculateMeanFromSigmas(predictedMeasurementSigmas);

        Eigen::Matrix<double,NMeasurements,NMeasurements> Pyy(measurementNoise);
        Eigen::Matrix<double,N_SIGMA_POINTS,NMeasurements> Pxy = SigmaMatrix::Zero();

        Eigen::Matrix<double,NMeasurements,1> temp;
        float numberOfSigmaPoints = N_SIGMA_POINTS;

        for(int i =0; i < numberOfSigmaPoints; i++)
        {
            // store difference between prediction and measurment.
            temp = predictedMeasurementSigmas.col(i) - predictedMeasurement;
            // Innovation covariance - Add Measurement noise
            Pyy += m_sigmaWeights(i)*temp * temp.transpose();
            // Cross correlation matrix
            Pxy = Pxy + m_sigmaWeights(i)*(stateEstimateSigmas.col(i) - m_mean) * temp.transp();
        }

        Eigen::Matrix<double,N_SIGMA_POINTS,NMeasurements> K;
        K = Pxy * Pyy.inverse();
        /*
        if(numMeasurements == 2)
        {
            K = Pxy * Invert22(Pyy);
        }
        else
        {
            K = Pxy * InverseMatrix(Pyy);
        }
        */
        m_mean  = m_mean + K * (measurement - predictedMeasurement);
        m_covariance = m_covariance - K*Pyy*K.transp();
/*
        // Alternate calculation
        //m_covariance = m_covariance - Pxy*Pyy*Pxy.transp();

        // Stolen from last years code... does not all seem right for this iplementation.
        //m_covariance = HT(horzcat(stateEstimateSigmas-m_mean*m_sigmaWeights - K*predictedMeasurementSigmas +
        //                          K*predictedMeasurement*m_sigmaWeights,K*measurementNoise));
    */
        return true;
    }
protected:

   StateVector m_mean;
   ProcessMatrix m_covariance;
   WeightVector m_sigmaWeights;
   WeightVector m_sqrtSigmaWeights;
   float m_kappa;
};

#endif // UKF_H
