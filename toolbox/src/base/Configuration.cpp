#include "Configuration.h"

#include <algorithm>

using namespace wbt;

Configuration::Configuration(const std::string& confKey)
    : m_confKey(confKey)
    , m_dofs(0)
{}

// SET METHODS
// ===========

void Configuration::setParameters(const std::string& robotName,
                                  const std::string& urdfFile,
                                  const std::vector<std::string>& controlledJoints,
                                  const std::vector<std::string>& controlBoardsNames,
                                  const std::string& localName,
                                  const std::array<double, 3>& gravityVector)
{
    setRobotName(robotName);
    setUrdfFile(urdfFile);
    setControlledJoints(controlledJoints);
    setControlBoardsNames(controlBoardsNames);
    setLocalName(localName);
    setGravityVector(gravityVector);
}

void Configuration::setRobotName(const std::string& robotName)
{
    m_robotName = robotName;
}

void Configuration::setUrdfFile(const std::string& urdfFile)
{
    m_urdfFile = urdfFile;
}

void Configuration::setControlledJoints(const std::vector<std::string>& controlledJoints)
{
    m_controlledJoints = controlledJoints;
    m_dofs = controlledJoints.size();
}

void Configuration::setControlBoardsNames(const std::vector<std::string>& controlBoardsNames)
{
    m_controlBoardsNames = controlBoardsNames;
}

void Configuration::setLocalName(const std::string& localName)
{
    m_localName = localName;

    // Add the leading "/" if missing
    if (m_localName.compare(0, 1, "/")) {
        m_localName = "/" + m_localName;
    }
}

const std::string Configuration::getUniqueId() const
{
    std::string uniqueId(m_confKey);

    // Remove spaces
    auto it = std::remove(uniqueId.begin(), uniqueId.end(), ' ');
    uniqueId.erase(it, uniqueId.end());

    // Remove '/'
    it = std::remove(uniqueId.begin(), uniqueId.end(), '/');
    uniqueId.erase(it, uniqueId.end());

    return uniqueId;
}

void Configuration::setGravityVector(const std::array<double, 3>& gravityVector)
{
    m_gravityVector = gravityVector;
}

// GET METHODS
// ===========

const std::string& Configuration::getRobotName() const
{
    return m_robotName;
}

const std::string& Configuration::getUrdfFile() const
{
    return m_urdfFile;
}

const std::vector<std::string>& Configuration::getControlledJoints() const
{
    return m_controlledJoints;
}

const std::vector<std::string>& Configuration::getControlBoardsNames() const
{
    return m_controlBoardsNames;
}

const std::string& Configuration::getLocalName() const
{
    return m_localName;
}

const std::array<double, 3>& Configuration::getGravityVector() const
{
    return m_gravityVector;
}

const size_t& Configuration::getNumberOfDoFs() const
{
    return m_dofs;
}

// OTHER METHODS
// =============

bool Configuration::isValid() const
{
    bool status = !m_robotName.empty() && !m_urdfFile.empty() && !m_localName.empty()
                  && !m_controlledJoints.empty() && !m_controlBoardsNames.empty()
                  && !m_gravityVector.empty() && m_dofs > 0;
    return status;
}

// OPERATORS OVERLOADING
// =====================

bool Configuration::operator==(const Configuration& config) const
{
    return this->m_robotName == config.m_robotName && this->m_urdfFile == config.m_urdfFile
           && this->m_localName == config.m_localName
           && this->m_controlledJoints == config.m_controlledJoints
           && this->m_controlBoardsNames == config.m_controlBoardsNames
           && this->m_gravityVector == config.m_gravityVector && this->m_dofs == config.m_dofs;
}
