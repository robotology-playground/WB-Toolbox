/*
 * Copyright (C) 2018 Istituto Italiano di Tecnologia (IIT)
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms of the
 * GNU Lesser General Public License v2.1 or any later version.
 */

#include "SetLowLevelPID.h"
#include "BlockInformation.h"
#include "Configuration.h"
#include "Log.h"
#include "Parameter.h"
#include "Parameters.h"
#include "RobotInterface.h"

#include <yarp/dev/ControlBoardPid.h>
#include <yarp/dev/IPidControl.h>

#include <algorithm>
#include <iterator>
#include <ostream>
#include <unordered_map>
#include <vector>

using namespace wbt;
const std::string SetLowLevelPID::ClassName = "SetLowLevelPID";

// INDICES: PARAMETERS, INPUTS, OUTPUT
// ===================================

enum ParamIndex
{
    Bias = WBBlock::NumberOfParameters - 1,
    PidStruct,
    CtrlType
};

// BLOCK PIMPL
// ===========

class SetLowLevelPID::impl
{
public:
    enum Pid
    {
        P = 0,
        I = 1,
        D = 2
    };

    using PidData = std::tuple<double, double, double>;

    std::vector<yarp::dev::Pid> appliedPidValues;
    std::vector<yarp::dev::Pid> defaultPidValues;
    std::unordered_map<std::string, PidData> pidJointsFromParameters;
    yarp::dev::PidControlTypeEnum controlType;
};

// BLOCK CLASS
// ===========

SetLowLevelPID::SetLowLevelPID()
    : pImpl{new impl()}
{}

unsigned SetLowLevelPID::numberOfParameters()
{
    return WBBlock::numberOfParameters() + 2;
}

bool SetLowLevelPID::parseParameters(BlockInformation* blockInfo)
{
    const std::vector<ParameterMetadata> metadata{
        {ParameterType::STRUCT_CELL_DOUBLE, ParamIndex::PidStruct, 1, 1, "P"},
        {ParameterType::STRUCT_CELL_DOUBLE, ParamIndex::PidStruct, 1, 1, "I"},
        {ParameterType::STRUCT_CELL_DOUBLE, ParamIndex::PidStruct, 1, 1, "D"},
        {ParameterType::CELL_STRING, ParamIndex::PidStruct, 1, 1, "jointList"},
        {ParameterType::STRING, ParamIndex::CtrlType, 1, 1, "ControlType"}};

    for (const auto& md : metadata) {
        if (!blockInfo->addParameterMetadata(md)) {
            wbtError << "Failed to store parameter metadata";
            return false;
        }
    }

    return blockInfo->parseParameters(m_parameters);
}

bool SetLowLevelPID::configureSizeAndPorts(BlockInformation* blockInfo)
{
    if (!WBBlock::configureSizeAndPorts(blockInfo)) {
        return false;
    }
    // INPUTS
    // ======
    //
    // No inputs
    //
    // OUTPUTS
    // =======
    //
    // No outputs
    //

    const bool ok = blockInfo->setIOPortsData({{}, {}});

    if (!ok) {
        wbtError << "Failed to configure input / output ports.";
        return false;
    }

    return true;
}

bool SetLowLevelPID::initialize(BlockInformation* blockInfo)
{
    if (!WBBlock::initialize(blockInfo)) {
        return false;
    }

    // INPUT PARAMETERS
    // ================

    if (!SetLowLevelPID::parseParameters(blockInfo)) {
        wbtError << "Failed to parse parameters.";
        return false;
    }

    // Reading the control type
    std::string controlType;
    std::vector<double> p_Gains;
    std::vector<double> i_Gains;
    std::vector<double> d_Gains;
    std::vector<std::string> pidJointList;

    bool ok = true;
    ok = ok && m_parameters.getParameter("ControlType", controlType);
    ok = ok && m_parameters.getParameter("P", p_Gains);
    ok = ok && m_parameters.getParameter("I", i_Gains);
    ok = ok && m_parameters.getParameter("D", d_Gains);
    ok = ok && m_parameters.getParameter("jointList", pidJointList);

    if (!ok) {
        wbtError << "Failed to get parameters after their parsing.";
        return false;
    }

    // CLASS INITIALIZATION
    // ====================

    // Create a map {joint => PID} from the parameters
    // -----------------------------------------------

    // Get the DoFs
    const auto robotInterface = getRobotInterface(blockInfo).lock();
    if (!robotInterface) {
        wbtError << "RobotInterface has not been correctly initialized.";
        return false;
    }
    const auto& configuration = robotInterface->getConfiguration();
    const auto& controlledJoints = configuration.getControlledJoints();

    // Populate the map
    for (unsigned i = 0; i < pidJointList.size(); ++i) {
        // Find if the joint of the processed PID is a controlled joint
        auto findElement = std::find(
            std::begin(controlledJoints), std::end(controlledJoints), controlledJoints[i]);
        // Edit the PID for this joint
        if (findElement != std::end(controlledJoints)) {
            pImpl->pidJointsFromParameters.emplace(
                pidJointList[i], std::make_tuple(p_Gains[i], i_Gains[i], d_Gains[i]));
        }
        else {
            wbtWarning << "Attempted to set PID of joint " << controlledJoints[i]
                       << " which is not currently controlled. Skipping it.";
        }
    }

    // Configure the class
    // -------------------

    if (controlType == "Position") {
        pImpl->controlType = yarp::dev::VOCAB_PIDTYPE_POSITION;
    }
    else if (controlType == "Torque") {
        pImpl->controlType = yarp::dev::VOCAB_PIDTYPE_TORQUE;
    }
    else {
        wbtError << "Control type not recognized.";
        return false;
    }

    const auto dofs = configuration.getNumberOfDoFs();

    // Initialize the vector size to the number of dofs
    pImpl->defaultPidValues.resize(dofs);

    // Get the interface
    yarp::dev::IPidControl* iPidControl = nullptr;
    if (!robotInterface->getInterface(iPidControl) || !iPidControl) {
        wbtError << "Failed to get IPidControl interface.";
        return false;
    }

    // Store the default gains
    if (!iPidControl->getPids(pImpl->controlType, pImpl->defaultPidValues.data())) {
        wbtError << "Failed to get default data from IPidControl.";
        return false;
    }

    // Initialize the vector of the applied pid gains with the default gains
    pImpl->appliedPidValues = pImpl->defaultPidValues;

    // Override the PID with the gains specified as block parameters
    for (unsigned i = 0; i < dofs; ++i) {
        const std::string& jointName = controlledJoints[i];
        // If the pid has been passed, set the new gains
        if (pImpl->pidJointsFromParameters.find(jointName)
            != pImpl->pidJointsFromParameters.end()) {
            const impl::PidData& gains = pImpl->pidJointsFromParameters[jointName];
            pImpl->appliedPidValues[i].setKp(std::get<impl::Pid::P>(gains));
            pImpl->appliedPidValues[i].setKi(std::get<impl::Pid::I>(gains));
            pImpl->appliedPidValues[i].setKd(std::get<impl::Pid::D>(gains));
        }
    }

    // Apply the new pid gains
    if (!iPidControl->setPids(pImpl->controlType, pImpl->appliedPidValues.data())) {
        wbtError << "Failed to set PID values.";
        return false;
    }

    return true;
}

bool SetLowLevelPID::terminate(const BlockInformation* blockInfo)
{
    // Get the RobotInterface
    const auto robotInterface = getRobotInterface(blockInfo).lock();
    if (!robotInterface) {
        wbtError << "Failed to retrieve the RobotInterface.";
        return false;
    }

    bool ok;

    // Get the IPidControl interface
    yarp::dev::IPidControl* iPidControl = nullptr;
    ok = robotInterface->getInterface(iPidControl);
    if (!ok || !iPidControl) {
        wbtError << "Failed to get IPidControl interface.";
        return false;
    }

    // Reset default PID gains
    ok = iPidControl->setPids(pImpl->controlType, pImpl->defaultPidValues.data());
    if (!ok) {
        wbtError << "Failed to reset PIDs to the default values.";
        return false;
    }

    return WBBlock::terminate(blockInfo);
}

bool SetLowLevelPID::output(const BlockInformation* /*blockInfo*/)
{
    return true;
}
