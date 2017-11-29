#ifndef WBT_ToolboxSingleton_H
#define WBT_ToolboxSingleton_H

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include "Configuration.h"

namespace wbt {
    class ToolboxSingleton;
    class RobotInterface;
}

namespace iDynTree {
    class KinDynComputations;
}

// TODO: check class doxygen macro
/**
 * \class ToolboxSingleton ToolboxSingleton.h
 *
 * This class handles and contains configuration and devices used by all the blocks
 * allocated by a running model. It is possible obtaining a singleton of this class
 * by calling WBToolbox::sharedInstance.
 *
 * @see wbt::RobotInterface
 * @see wbt::Configuration
 * @see wbt::yarpDevices
 *
 */
class wbt::ToolboxSingleton { // TODO: wbt::ToolboxSingleton
private:
    /// Object that stores all the configurations labelled by the name of the Simulink Block's
    /// name.
    /// @see wbt::RobotInterface
    std::unordered_map<std::string, std::shared_ptr<wbt::RobotInterface>> m_interfaces;

public:

    // CONSTRUCTOR / DESTRUCTOR
    // ========================

    ToolboxSingleton();
    ~ToolboxSingleton();

    ToolboxSingleton(const ToolboxSingleton&) = delete;
    ToolboxSingleton& operator=(const ToolboxSingleton&) = delete;

    // UTILITIES
    // =========

    /**
     * Check if a Configuration labelled by confKey exists and is a
     * valid object.
     *
     * @param  confKey The key describing the configuration (name of the Simulink block)
     * @return         True if the configuration is valid
     */
    bool isKeyValid(const std::string& confKey);

    /**
     * Returns the DoFs associated with the configuration labelled by confKey
     *
     * @param  confKey The key describing the configuration (name of the Simulink block)
     * @return         The number of degrees of freedom. It returns -1 when failing.
     */
    int numberOfDoFs(const std::string& confKey);

    // GET METHODS
    // ===========

    /**
     * Returns the singleton instance to this object.
     *
     * @note The singleton stays in memory until the entire matlab is closed. The WBToolbox
     *       is designed in such a way that assures to reach a clean state when all blocks
     *       have been terminated.
     *
     * @return the singleton instance
     */
    static wbt::ToolboxSingleton& sharedInstance();

    // TODO: check if doxygen ref work

    /**
     * Returns the Configuration object labelled by confKey.
     * This object is contained into the wbt::RobotInterface object.
     *
     * @see ToolboxSingleton::getRobotInterface
     * @param  confKey The key describing the configuration (name of the Simulink block)
     * @return         A constant reference to the Configuration object
     */
    const Configuration& getConfiguration(const std::string& confKey);

    /**
    * Returns a \c shared_ptr to the RobotInterface object containing the configuration
    * labelled by confKey and all the objects used to gather and set robot data
    *
    * @param confKey The key describing the configuration (name of the Simulink block)
    * @return        A \c shared_ptr to the RobotInterface of the requested configuration
    */
    const std::shared_ptr<RobotInterface> getRobotInterface(const std::string& confKey);

    /**
     * Returns a \c shared_ptr to the KinDynComputations object used to perform calculation
     * on the provided model
     *
     * @param confKey The key describing the configuration (name of the Simulink block)
     * @return        A \c shared_ptr to the iDynTree::KinDynComputations of the requested
     *                configuration
     */
    const std::shared_ptr<iDynTree::KinDynComputations> getKinDynComputations(const std::string& confKey);

    // TOOLBOXSINGLETON CONFIGURATION
    // ==============================

    /*! Saves in the singleton a new configuration \c config.
     *
     * If the config is valid and hasn't been already stored, it creates a new entry
     * in ToolboxSingleton::m_interfaces. If a configuration with matching confKey is found,
     * if the Configuration object is the same it does nothing, otherwise it overrides it.
     *
     * @note Since confKey is the name of the block, the overriding can happen only after
     *       subsequent compilation in Simulink, when the block's parameters have been
     *       changed.
     *
     * @param  confKey The key describing the configuration (name of the Simulink block)
     * @param  config  The wbt::Configuration object parsed from Simulink's parameters
     * @return         Returns \c true if configure is successful, \c false otherwise
     * @see            ToolboxSingleton::isKeyValid
     */
    bool storeConfiguration(const std::string& confKey, const Configuration& config);

    /**
     * Delete the wbt::RobotInterface referred by confKey. No-op if it doesn't exist.
     *
     * @param confKey The key describing the configuration (name of the Simulink block)
     */
    void eraseConfiguration(const std::string& confKey);
};

#endif /* end of include guard: WBT_ToolboxSingleton_H */
