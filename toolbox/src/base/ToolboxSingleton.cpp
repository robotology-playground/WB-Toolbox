#include "ToolboxSingleton.h"

#include "Log.h"
#include "RobotInterface.h"
#include <string>
#include <iDynTree/KinDynComputations.h>
#include <iDynTree/ModelIO/ModelLoader.h>
#include <yarp/os/ResourceFinder.h>
#include <yarp/os/Network.h>

// TODO: remove nesting
namespace wbt {

    // CONSTRUCTOR / DESTRUCTOR
    // ========================

    ToolboxSingleton::ToolboxSingleton() {}
    ToolboxSingleton::~ToolboxSingleton() {}

    // UTILITIES
    // =========

    int ToolboxSingleton::numberOfDoFs(const std::string& confKey)
    {
        if (!isKeyValid(confKey))
            return -1;
        else
            return m_interfaces[confKey]->getConfiguration().getNumberOfDoFs();
    }

    bool ToolboxSingleton::isKeyValid(const std::string& confKey)
    {
        if (m_interfaces.find(confKey) != m_interfaces.end()) {
            if (m_interfaces[confKey])
                return true;
            else
                return false;
        }
        else {
            return false;
        }
    }

    // GET METHODS
    // ===========

    ToolboxSingleton& ToolboxSingleton::sharedInstance()
    {
        static ToolboxSingleton instance;
        return instance;
    }

    const Configuration& ToolboxSingleton::getConfiguration(const std::string& confKey)
    {
        return getRobotInterface(confKey)->getConfiguration();
    }

    const std::shared_ptr<RobotInterface> ToolboxSingleton::getRobotInterface(const std::string& confKey)
    {
        if (!isKeyValid(confKey)) {
            return nullptr;
        }

        return m_interfaces[confKey];
    }

    const std::shared_ptr<iDynTree::KinDynComputations> ToolboxSingleton::getModel(const std::string& confKey)
    {
        if (!isKeyValid(confKey)) {
            return nullptr;
        }

        return m_interfaces[confKey]->getKinDynComputations();
    }

    // TOOLBOXSINGLETON CONFIGURATION
    // ==============================

    bool ToolboxSingleton::storeConfiguration(const std::string& confKey, const Configuration& config)
    {
        if (!config.isValid()) {
            return false;
        }

        // Add the new Configuration object and override an existing key if it already exist.
        // Note: Simulink doesn't flush memory unless Matlab is closed, and static objects stay in memory.
        //       This may cause problems if the config block's mask is changed after the first compilation.
        if (m_interfaces.find(confKey) == m_interfaces.end()) {
            m_interfaces[confKey] = std::make_shared<RobotInterface>(config);
            return static_cast<bool>(m_interfaces[confKey]);
        }

        if (!(m_interfaces[confKey]->getConfiguration() == config)) {
            assert(m_interfaces[confKey]);

            // Check that at top there are 2 instances of the model:
            // the private member of RobotInterface and the one returned
            // by the model() called by the assert itself
            assert(getModel(confKey).use_count() <= 2);
            // TODO: and also m_robotDeviceCounter should be max 1
            //
            // TODO: domani: quando finisce la simulazione (terminate) dovrei avere
            //       dentro tutti i Configuration al massimo 1 sia di model che di
            //       device (e le interfacce tutte chiuse?)

            // Delete the old configuration (calling the destructor for cleaning garbage)
            m_interfaces[confKey].reset();
            m_interfaces.erase(confKey);

            // Allocate a new configuration
            m_interfaces[confKey] = std::make_shared<RobotInterface>(config);
            return static_cast<bool>(m_interfaces[confKey]);
        }

        return true;
    }

    void ToolboxSingleton::eraseConfiguration(const std::string& confKey)
    {
        m_interfaces.erase(confKey);
    }
}
