/*
 * Copyright (C) 2018 Istituto Italiano di Tecnologia (IIT)
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms of the
 * GNU Lesser General Public License v2.1 or any later version.
 */

#include "ToolboxSingleton.h"
#include "Configuration.h"
#include "Log.h"
#include "Parameters.h"
#include "RobotInterface.h"

#include <array>
#include <ostream>
#include <stddef.h>
#include <string>
#include <vector>

using namespace wbt;

bool fillConfiguration(std::shared_ptr<wbt::Configuration>& configurationPtr,
                       const wbt::Parameters& parameters);

// CONSTRUCTOR / DESTRUCTOR
// ========================

ToolboxSingleton::ToolboxSingleton() = default;
ToolboxSingleton::~ToolboxSingleton() = default;

// UTILITIES
// =========

int ToolboxSingleton::numberOfDoFs(const std::string& confKey) const
{
    if (!isKeyValid(confKey)) {
        return -1;
    }

    return m_interfaces.at(confKey).lock()->getConfiguration().getNumberOfDoFs();
}

bool ToolboxSingleton::isKeyValid(const std::string& confKey) const
{
    if (m_interfaces.find(confKey) == m_interfaces.end()) {
        wbtError << "Failed to find entry in the ToolboxSingleton related to " << confKey
                 << " key.";
        return false;
    }

    if (!m_interfaces.at(confKey).lock()) {
        wbtError << "Failed to get the RobotInterface object from its weak pointer stored in "
                 << "TolboxSingleton.";
        return false;
    }

    return true;
}

// GET METHODS
// ===========

ToolboxSingleton& ToolboxSingleton::sharedInstance()
{
    static ToolboxSingleton instance;
    return instance;
}

const Configuration& ToolboxSingleton::getConfiguration(const std::string& confKey) const
{
    return getRobotInterface(confKey)->getConfiguration();
}

const std::shared_ptr<RobotInterface>
ToolboxSingleton::getRobotInterface(const std::string& confKey) const
{
    if (!isKeyValid(confKey)) {
        return nullptr;
    }

    return m_interfaces.at(confKey).lock();
}

const std::shared_ptr<iDynTree::KinDynComputations>
ToolboxSingleton::getKinDynComputations(const std::string& confKey) const
{
    if (!isKeyValid(confKey)) {
        return nullptr;
    }

    return m_interfaces.at(confKey).lock()->getKinDynComputations();
}

// TOOLBOXSINGLETON CONFIGURATION
// ==============================

std::shared_ptr<RobotInterface> ToolboxSingleton::createRobotInterface(const Configuration& config)
{
    if (!config.isValid()) {
        wbtError << "The passed configuration object does not contain valid data.";
        return {};
    }

    const std::string& confKey = config.getConfKey();

    // If there's already a key matching confKey, but the related smart pointer is expired, it means
    // that it is a leftofer from a previous state. Clean the entry.
    if (m_interfaces.find(confKey) != m_interfaces.end() && m_interfaces.at(confKey).expired()) {
        m_interfaces.erase(confKey);
    }

    // If there is no key matching confKey, it means that a new RobotInterface object should be
    // allocated
    if (m_interfaces.find(confKey) == m_interfaces.end()) {
        // Create a temporary shared pointer
        auto sharedTmp = std::make_shared<RobotInterface>(config);
        // Store a weak pointer
        m_interfaces[confKey] = sharedTmp;
        // Return the shared pointer
        return sharedTmp;
    }

    // If, instead, the key exists and the pointer has not expired, probably it was created
    // previously by another block that points to the same Configuration block. Just to be sure,
    // check if the Configuration objects match:
    if (!(m_interfaces[confKey].lock()->getConfiguration() == config)) {
        wbtError << "A RobotInterface pointing to " << confKey
                 << " Configuration block already exists, but contains a different "
                 << "wbt::Configuration object.";
        return {};
    }

    // At this point, it is safe to return the already stored RobotInterface object addressed by
    // confKey
    return m_interfaces[confKey].lock();
}

bool fillConfiguration(std::shared_ptr<wbt::Configuration>& configurationPtr,
                       const wbt::Parameters& parameters)
{
    bool ok = true;

    std::string robotName;
    std::string urdfFile;
    std::string localName;
    std::string confBlockName;
    std::vector<double> gravityVector;
    std::vector<std::string> controlledJoints;
    std::vector<std::string> controlBoardsNames;

    ok = ok && parameters.getParameter("RobotName", robotName);
    ok = ok && parameters.getParameter("UrdfFile", urdfFile);
    ok = ok && parameters.getParameter("LocalName", localName);
    ok = ok && parameters.getParameter("ControlledJoints", controlledJoints);
    ok = ok && parameters.getParameter("ControlBoardsNames", controlBoardsNames);
    ok = ok && parameters.getParameter("GravityVector", gravityVector);
    ok = ok && parameters.getParameter("ConfBlockName", confBlockName);

    if (!ok) {
        wbtError << "The parameters passed do not contain all the required information to create a "
                 << "Configuration object.";
        return false;
    }

    // Populate the Configuration object
    // =================================

    configurationPtr.reset(new wbt::Configuration(confBlockName));
    configurationPtr->setRobotName(robotName);
    configurationPtr->setUrdfFile(urdfFile);
    configurationPtr->setControlledJoints(controlledJoints);
    configurationPtr->setControlBoardsNames(controlBoardsNames);
    configurationPtr->setLocalName(localName);

    std::array<double, 3> gravityArray;
    for (size_t i = 0; i < 3; ++i) {
        gravityArray[i] = gravityVector[i];
    }
    configurationPtr->setGravityVector(gravityArray);

    return true;
}

std::shared_ptr<RobotInterface>
ToolboxSingleton::storeConfiguration(const wbt::Parameters& parameters)
{
    std::shared_ptr<Configuration> configurationPtr = nullptr;

    // Create the Configuration object. This checks if parameters contain all the required
    // parameters.
    if (!fillConfiguration(configurationPtr, parameters)) {
        wbtError << "Failed to fill the configuration with input parameters.";
        return {};
    }

    if (!configurationPtr || !configurationPtr->isValid()) {
        wbtError << "The parsed Configuration object is not valid.";
        return {};
    }

    // Create a RobotInterface from the Configuration block. A weak pointer is stored in the
    // ToolboxSingleton and a shared pointer is returned to the caller (which owns the memory)
    auto robotInterface = createRobotInterface(*configurationPtr);

    if (!robotInterface) {
        wbtError << "Failed to get the RobotInterface object.";
        return {};
    }

    return robotInterface;
}

void ToolboxSingleton::eraseConfiguration(const std::string& confKey)
{
    m_interfaces.erase(confKey);
}
