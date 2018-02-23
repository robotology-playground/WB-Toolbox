#include "CentroidalMomentum.h"
#include "BlockInformation.h"
#include "Log.h"
#include "RobotInterface.h"
#include "Signal.h"

#include <iDynTree/Core/EigenHelpers.h>
#include <iDynTree/Core/SpatialMomentum.h>
#include <iDynTree/KinDynComputations.h>

#include <memory>

using namespace wbt;

const std::string CentroidalMomentum::ClassName = "CentroidalMomentum";

const unsigned CentroidalMomentum::INPUT_IDX_BASE_POSE = 0;
const unsigned CentroidalMomentum::INPUT_IDX_JOINTCONF = 1;
const unsigned CentroidalMomentum::INPUT_IDX_BASE_VEL = 2;
const unsigned CentroidalMomentum::INPUT_IDX_JOINT_VEL = 3;
const unsigned CentroidalMomentum::OUTPUT_IDX_CENTRMOM = 0;

CentroidalMomentum::CentroidalMomentum() {}

bool CentroidalMomentum::configureSizeAndPorts(BlockInformation* blockInfo)
{
    // Memory allocation / Saving data not allowed here

    if (!WBBlock::configureSizeAndPorts(blockInfo))
        return false;

    // INPUTS
    // ======
    //
    // 1) Homogeneous transform for base pose wrt the world frame (4x4 matrix)
    // 2) Joints position (1xDoFs vector)
    // 3) Base frame velocity (1x6 vector)
    // 4) Joints velocity (1xDoFs vector)
    //

    // Number of inputs
    if (!blockInfo->setNumberOfInputPorts(4)) {
        Log::getSingleton().error("Failed to configure the number of input ports.");
        return false;
    }

    const unsigned dofs = getConfiguration().getNumberOfDoFs();

    // Size and type
    bool success = true;
    success = success && blockInfo->setInputPortMatrixSize(INPUT_IDX_BASE_POSE, 4, 4);
    success = success && blockInfo->setInputPortVectorSize(INPUT_IDX_JOINTCONF, dofs);
    success = success && blockInfo->setInputPortVectorSize(INPUT_IDX_BASE_VEL, 6);
    success = success && blockInfo->setInputPortVectorSize(INPUT_IDX_JOINT_VEL, dofs);

    blockInfo->setInputPortType(INPUT_IDX_BASE_POSE, PortDataTypeDouble);
    blockInfo->setInputPortType(INPUT_IDX_JOINTCONF, PortDataTypeDouble);
    blockInfo->setInputPortType(INPUT_IDX_BASE_VEL, PortDataTypeDouble);
    blockInfo->setInputPortType(INPUT_IDX_JOINT_VEL, PortDataTypeDouble);

    if (!success) {
        Log::getSingleton().error("Failed to configure input ports.");
        return false;
    }

    // OUTPUTS
    // =======
    //
    // 1) Vector representing the centroidal momentum (1x6)
    //

    // Number of outputs
    if (!blockInfo->setNumberOfOutputPorts(1)) {
        Log::getSingleton().error("Failed to configure the number of output ports.");
        return false;
    }

    // Size and type
    success = blockInfo->setOutputPortVectorSize(OUTPUT_IDX_CENTRMOM, 6);
    blockInfo->setOutputPortType(OUTPUT_IDX_CENTRMOM, PortDataTypeDouble);

    return success;
}

bool CentroidalMomentum::initialize(const BlockInformation* blockInfo)
{
    if (!WBBlock::initialize(blockInfo))
        return false;

    // OUTPUT
    // ======

    m_centroidalMomentum =
        std::unique_ptr<iDynTree::SpatialMomentum>(new iDynTree::SpatialMomentum());
    return static_cast<bool>(m_centroidalMomentum);
}

bool CentroidalMomentum::terminate(const BlockInformation* blockInfo)
{
    return WBBlock::terminate(blockInfo);
}

bool CentroidalMomentum::output(const BlockInformation* blockInfo)
{
    const auto& model = getRobotInterface()->getKinDynComputations();

    if (!model) {
        Log::getSingleton().error("Failed to retrieve the KinDynComputations object.");
        return false;
    }

    // GET THE SIGNALS POPULATE THE ROBOT STATE
    // ========================================

    Signal basePoseSig = blockInfo->getInputPortSignal(INPUT_IDX_BASE_POSE);
    Signal jointsPosSig = blockInfo->getInputPortSignal(INPUT_IDX_JOINTCONF);
    Signal baseVelocitySignal = blockInfo->getInputPortSignal(INPUT_IDX_BASE_VEL);
    Signal jointsVelocitySignal = blockInfo->getInputPortSignal(INPUT_IDX_JOINT_VEL);

    bool ok =
        setRobotState(&basePoseSig, &jointsPosSig, &baseVelocitySignal, &jointsVelocitySignal);

    if (!ok) {
        Log::getSingleton().error("Failed to set the robot state.");
        return false;
    }

    // OUTPUT
    // ======

    // Calculate the centroidal momentum
    *m_centroidalMomentum = model->getCentroidalTotalMomentum();

    // Forward the output to Simulink
    Signal output = blockInfo->getOutputPortSignal(OUTPUT_IDX_CENTRMOM);
    output.setBuffer(toEigen(*m_centroidalMomentum).data(),
                     blockInfo->getOutputPortWidth(OUTPUT_IDX_CENTRMOM));
    return true;
}
