#include "SetReferences.h"

#include "Log.h"
#include "BlockInformation.h"
#include "Signal.h"
#include "RobotInterface.h"
#include <yarp/dev/ControlBoardInterfaces.h>

#define _USE_MATH_DEFINES
#include <cmath>

using namespace wbt;

const std::string SetReferences::ClassName = "SetReferences";

SetReferences::SetReferences()
: m_resetControlMode(true)
{}

const std::vector<double> SetReferences::rad2deg(const double* buffer, const unsigned width)
{
    const double Rad2Deg = 180.0 / M_PI;

    std::vector<double> vectorDeg(width);

    for (auto i = 0; i < width; ++i) {
        vectorDeg[i] = buffer[i] * Rad2Deg;
    }

    return vectorDeg;
}

unsigned SetReferences::numberOfParameters()
{
    return WBBlock::numberOfParameters() + 1;
}

bool SetReferences::configureSizeAndPorts(BlockInformation* blockInfo)
{
    // Memory allocation / Saving data not allowed here

    if (!WBBlock::configureSizeAndPorts(blockInfo)) return false;

    const unsigned dofs = getConfiguration().getNumberOfDoFs();

    // INPUTS
    // ======
    //
    // 1) Joint refereces (1xDoFs vector)
    //

    // Number of inputs
    if (!blockInfo->setNumberOfInputPorts(1)) {
        Log::getSingleton().error("Failed to configure the number of input ports.");
        return false;
    }

    // Size and type
    bool success = blockInfo->setInputPortVectorSize(0, dofs);
    blockInfo->setInputPortType(0, PortDataTypeDouble);

    if (!success) {
        Log::getSingleton().error("Failed to configure input ports.");
        return false;
    }

    // OUTPUTS
    // =======
    //
    // No outputs
    //

    if (!blockInfo->setNumberOfOutputPorts(0)) {
        Log::getSingleton().error("Failed to configure the number of output ports.");
        return false;
    }

    return true;
}

bool SetReferences::initialize(const BlockInformation* blockInfo)
{
    if (!WBBlock::initialize(blockInfo)) return false;

    // Reading the control type
    std::string controlType;
    if (!blockInfo->getStringParameterAtIndex(WBBlock::numberOfParameters() + 1,
                                              controlType)) {
        Log::getSingleton().error("Could not read control type parameter.");
        return false;
    }

    // Initialize the std::vectors
    const unsigned dofs = getConfiguration().getNumberOfDoFs();
    m_controlModes.assign(dofs, VOCAB_CM_UNKNOWN);

    // IControlMode.h
    if (controlType == "Position") {
        m_controlModes.assign(dofs, VOCAB_CM_POSITION);
    }
    else if (controlType == "Position Direct") {
        m_controlModes.assign(dofs, VOCAB_CM_POSITION_DIRECT);
    }
    else if (controlType == "Velocity") {
        m_controlModes.assign(dofs, VOCAB_CM_VELOCITY);
    }
    else if (controlType == "Torque") {
        m_controlModes.assign(dofs, VOCAB_CM_TORQUE);
    }
    else if (controlType == "PWM") {
        m_controlModes.assign(dofs, VOCAB_CM_PWM);
    }
    else if (controlType == "Current") {
        m_controlModes.assign(dofs, VOCAB_CM_CURRENT);
    }
    else {
        Log::getSingleton().error("Control Mode not supported.");
        return false;
    }

    // The Position mode is used to set a discrete reference, and then the yarp interface
    // is responsible to generate a trajectory to reach this setpoint.
    // The generated trajectory takes an additional parameter: the speed.
    // If not properly initialized, this contol mode does not work as expected.
    if (controlType == "Position") {
        // Get the interface
        yarp::dev::IPositionControl* interface = nullptr;
        if (!getRobotInterface()->getInterface(interface) || !interface) {
            Log::getSingleton().error("Failed to get IPositionControl interface.");
            return false;
        }
        // TODO: Set this parameter from the mask
        std::vector<double> speedInitalization(dofs, 50.0);
        // Set the references
        if (!interface->setRefSpeeds(speedInitalization.data())) {
            Log::getSingleton().error("Failed to initialize speed references.");
            return false;
        }
    }

    // Retain the ControlBoardRemapper
    if (!getRobotInterface()->retainRemoteControlBoardRemapper()) {
        Log::getSingleton().error("Couldn't retain the RemoteControlBoardRemapper.");
        return false;
    }

    m_resetControlMode = true;
    return true;
}

bool SetReferences::terminate(const BlockInformation* blockInfo)
{
    using namespace yarp::dev;
    bool ok = true;

    // Get the interface
    IControlMode2* icmd2 = nullptr;
    ok = ok && getRobotInterface()->getInterface(icmd2);
    if (!ok || !icmd2) {
        Log::getSingleton().error("Failed to get the IControlMode2 interface.");
        // Don't return false here. WBBlock::terminate must be called in any case
    }

    // Set  all the controlledJoints VOCAB_CM_POSITION
    const unsigned dofs = getConfiguration().getNumberOfDoFs();
    m_controlModes.assign(dofs, VOCAB_CM_POSITION);

    ok = ok && icmd2->setControlModes(m_controlModes.data());
    if (!ok) {
        Log::getSingleton().error("Failed to set control mode.");
        // Don't return false here. WBBlock::terminate must be called in any case
    }

    // Release the RemoteControlBoardRemapper
    ok = ok && getRobotInterface()->releaseRemoteControlBoardRemapper();
    if (!ok) {
        Log::getSingleton().error("Failed to release the RemoteControlBoardRemapper.");
        // Don't return false here. WBBlock::terminate must be called in any case
    }

    return ok && WBBlock::terminate(blockInfo);
}

bool SetReferences::initializeInitialConditions(const BlockInformation* /*blockInfo*/)
{
    // This function is called when a subsystem with Enable / Disable support is used.
    // In this case all the blocks in this subsystem are configured and initialize,
    // but if they are disabled, output() is not called.
    // This initializeInitialConditions method is called when the block is enabled,
    // and in this case the control mode should be set.
    //
    // It is worth noting that this toolbox disables parameters to be tunable for
    // all the blocks

    m_resetControlMode = true;
    return true;
}

bool SetReferences::output(const BlockInformation* blockInfo)
{
    using namespace yarp::dev;

    // Set the control mode at the first run
    if (m_resetControlMode) {
        m_resetControlMode = false;
        // Get the interface
        IControlMode2* icmd2 = nullptr;
        if (!getRobotInterface()->getInterface(icmd2) || !icmd2) {
            Log::getSingleton().error("Failed to get the IControlMode2 interface.");
            return false;
        }
        // Set the control mode to all the controlledJoints
        if (!icmd2->setControlModes(m_controlModes.data())) {
            Log::getSingleton().error("Failed to set control mode.");
            return false;
        }
    }

    // Get the signal
    Signal references = blockInfo->getInputPortSignal(0);
    unsigned signalWidth = blockInfo->getInputPortWidth(0);

    double* bufferReferences = references.getBuffer<double>();
    if (!bufferReferences) {
        Log::getSingleton().error("Failed to get the buffer containing references.");
        return false;
    }

    bool ok = false;
    // TODO: here only the first element is checked
    switch (m_controlModes.front()) {
        case VOCAB_CM_UNKNOWN:
            Log::getSingleton().error("Control mode has not been successfully set.");
            return false;
            break;
        case VOCAB_CM_POSITION: {
            // Get the interface
            IPositionControl* interface = nullptr;
            if (!getRobotInterface()->getInterface(interface) || !interface) {
                Log::getSingleton().error("Failed to get IPositionControl interface.");
                return false;
            }
            // Convert from rad to deg
            auto referencesDeg = rad2deg(bufferReferences, signalWidth);
            // Set the references
            ok = interface->positionMove(referencesDeg.data());
            break;
        }
        case VOCAB_CM_POSITION_DIRECT: {
            // Get the interface
            IPositionDirect* interface = nullptr;
            if (!getRobotInterface()->getInterface(interface) || !interface) {
                Log::getSingleton().error("Failed to get IPositionDirect interface.");
                return false;
            }
            // Convert from rad to deg
            auto referencesDeg = rad2deg(bufferReferences, signalWidth);
            // Set the references
            ok = interface->setPositions(referencesDeg.data());
            break;
        }
        case VOCAB_CM_VELOCITY: {
            // Get the interface
            IVelocityControl* interface = nullptr;
            if (!getRobotInterface()->getInterface(interface) || !interface) {
                Log::getSingleton().error("Failed to get IVelocityControl interface.");
                return false;
            }
            // Convert from rad to deg
            auto referencesDeg = rad2deg(bufferReferences, signalWidth);
            // Set the references
            ok = interface->velocityMove(referencesDeg.data());
            break;
        }
        case VOCAB_CM_TORQUE: {
            // Get the interface
            ITorqueControl* interface = nullptr;
            if (!getRobotInterface()->getInterface(interface) || !interface) {
                Log::getSingleton().error("Failed to get ITorqueControl interface.");
                return false;
            }
            // Set the references
            ok = interface->setRefTorques(bufferReferences);
            break;
        }
        case VOCAB_CM_PWM: {
            // Get the interface
            IPWMControl* interface = nullptr;
            if (!getRobotInterface()->getInterface(interface) || !interface) {
                Log::getSingleton().error("Failed to get IPWMControl interface.");
                return false;
            }
            // Set the references
            ok = interface->setRefDutyCycles(bufferReferences);
            break;
        }
        case VOCAB_CM_CURRENT: {
            // Get the interface
            ICurrentControl* interface = nullptr;
            if (!getRobotInterface()->getInterface(interface) || !interface) {
                Log::getSingleton().error("Failed to get ICurrentControl interface.");
                return false;
            }
            // Set the references
            ok = interface->setRefCurrents(bufferReferences);
            break;
        }
    }

    if (!ok) {
        Log::getSingleton().error("Failed to set references.");
        return false;
    }

    return true;
}
