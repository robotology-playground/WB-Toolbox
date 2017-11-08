#include "GetMeasurement.h"

#include "Log.h"
#include "BlockInformation.h"
#include "Signal.h"
#include "RobotInterface.h"
#include <yarp/dev/IEncoders.h>
#include <yarp/dev/ITorqueControl.h>

#define _USE_MATH_DEFINES
#include <cmath>

namespace wbt {

    const std::string GetMeasurement::ClassName = "GetMeasurement";

    void deg2rad(std::vector<double>& v)
    {
        const double deg2rad = (2 * M_PI) / 180.0;
        for (auto& element : v) {
            element *= deg2rad;
        }
    }

    unsigned GetMeasurement::numberOfParameters()
    {
        return WBBlock::numberOfParameters() + 1;
    }

    bool GetMeasurement::configureSizeAndPorts(BlockInformation* blockInfo)
    {
        if (!WBBlock::configureSizeAndPorts(blockInfo)) return false;

        // INPUTS
        // ======
        //
        // No inputs
        //

        if (!blockInfo->setNumberOfInputPorts(0)) {
            Log::getSingleton().error("Failed to configure the number of input ports.");
            return false;
        }

        // OUTPUTS
        // =======
        //
        // 1) Vector with the information asked (1xDoFs)
        //

        if (!blockInfo->setNumberOfOutputPorts(1)) {
            Log::getSingleton().error("Failed to configure the number of output ports.");
            return false;
        }

        const unsigned dofs = getConfiguration().getNumberOfDoFs();

        bool success = blockInfo->setOutputPortVectorSize(0, dofs);
        blockInfo->setOutputPortType(0, PortDataTypeDouble);
        if (!success) {
            Log::getSingleton().error("Failed to configure output ports.");
            return false;
        }

        return true;
    }

    bool GetMeasurement::initialize(BlockInformation* blockInfo)
    {
        if (!WBBlock::initialize(blockInfo)) return false;

        // Reading the control type
        std::string informationType;
        if (!blockInfo->getStringParameterAtIndex(WBBlock::numberOfParameters() + 1, informationType)) {
            Log::getSingleton().error("Could not read estimate type parameter.");
            return false;
        }

        if (informationType == "Joints Position") {
            m_estimateType = wbt::ESTIMATE_JOINT_POS;
        } else if (informationType == "Joints Velocity") {
            m_estimateType = wbt::ESTIMATE_JOINT_VEL;
        } else if (informationType == "Joints Acceleration") {
            m_estimateType = wbt::ESTIMATE_JOINT_ACC;
        } else if (informationType == "Joints Torque") {
            m_estimateType = wbt::ESTIMATE_JOINT_TORQUE;
        } else {
            Log::getSingleton().error("Estimate not supported.");
            return false;
        }

        // Initialize the size of the output vector
        const unsigned dofs = getConfiguration().getNumberOfDoFs();
        m_estimate.reserve(dofs);

        // Retain the ControlBoardRemapper
        if (!getRobotInterface()->retainRemoteControlBoardRemapper()) {
            Log::getSingleton().error("Failed to initialize the Robot Interface containing the Control Board Remapper.");
            return false;
        }

        return true;
    }

    bool GetMeasurement::terminate(BlockInformation* blockInfo)
    {
        // Release the RemoteControlBoardRemapper
        bool ok = true;
        ok = ok & getRobotInterface()->releaseRemoteControlBoardRemapper();
        if (!ok) {
            Log::getSingleton().error("Failed to release the RemoteControlBoardRemapper.");
        }

        return ok && WBBlock::terminate(blockInfo);
    }

    bool GetMeasurement::output(BlockInformation* blockInfo)
    {
        bool ok;
        switch (m_estimateType) {
            case ESTIMATE_JOINT_POS: {
                // Get the interface
                std::weak_ptr<yarp::dev::IEncoders> iEncoders;
                if (!getRobotInterface()->getInterface(iEncoders)) {
                    Log::getSingleton().error("Failed to get IPidControl interface.");
                    return false;
                }
                // Get the measurement
                ok = iEncoders.lock()->getEncoders(m_estimate.data());
                deg2rad(m_estimate);
                break;
            }
            case ESTIMATE_JOINT_VEL: {
                // Get the interface
                std::weak_ptr<yarp::dev::IEncoders> iEncoders;
                if (!getRobotInterface()->getInterface(iEncoders)) {
                    Log::getSingleton().error("Failed to get IEncoders interface.");
                    return false;
                }
                // Get the measurement
                ok = iEncoders.lock()->getEncoderSpeeds(m_estimate.data());
                deg2rad(m_estimate);
                break;
            }
            case ESTIMATE_JOINT_ACC: {
                // Get the interface
                std::weak_ptr<yarp::dev::IEncoders> iEncoders;
                if (!getRobotInterface()->getInterface(iEncoders)) {
                    Log::getSingleton().error("Failed to get IEncoders interface.");
                    return false;
                }
                // Get the measurement
                ok = iEncoders.lock()->getEncoderAccelerations(m_estimate.data());
                deg2rad(m_estimate);
                break;
            }
            case ESTIMATE_JOINT_TORQUE: {
                // Get the interface
                std::weak_ptr<yarp::dev::ITorqueControl> iTorqueControl;
                if (!getRobotInterface()->getInterface(iTorqueControl)) {
                    Log::getSingleton().error("Failed to get ITorqueControl interface.");
                    return false;
                }
                // Get the measurement
                ok = iTorqueControl.lock()->getTorques(m_estimate.data());
                break;
            }
            default:
                Log::getSingleton().error("Estimate type not recognized.");
                return false;
        }

        if (!ok) {
            Log::getSingleton().error("Failed to get estimate.");
            return false;
        }

        Signal signal = blockInfo->getOutputPortSignal(0);
        signal.setBuffer(m_estimate.data(), blockInfo->getOutputPortWidth(0));

        return true;
    }
}