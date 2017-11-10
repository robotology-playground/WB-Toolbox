#ifndef WBT_WBIBLOCK_H
#define WBT_WBIBLOCK_H

#include "Block.h"
#include <string>
#include <memory>
#include <iDynTree/Core/VectorFixSize.h>
#include <iDynTree/Core/VectorDynSize.h>
#include <iDynTree/Core/Twist.h>
#include <iDynTree/Core/Transform.h>

namespace wbt {
    class WBBlock;
    class Configuration;
    class BlockInformation;
    class RobotInterface;
}

namespace iDynTree {
    class MatrixDynSize;
}

/**
 * \struct iDynTreeRobotState WBBlock.h
 *
 * This struct contains the iDynTree objects used to configure the
 * state of iDynTree::KinDynComputations objects.
 */
struct iDynTreeRobotState {
    iDynTree::Twist         m_baseVelocity;
    iDynTree::Vector3       m_gravity;
    iDynTree::Transform     m_world_T_base;
    iDynTree::VectorDynSize m_jointsVelocity;
    iDynTree::VectorDynSize m_jointsPosition;

    iDynTreeRobotState() = default;
    ~iDynTreeRobotState() = default;

    iDynTreeRobotState(const unsigned& dofs, const std::vector<double>& gravity);
};

/**
 * Basic class for Whole-Body related blocks.
 * This class (the whole toolbox in reality) assumes the block represent
 * an instantaneous system (i.e. not a dynamic system).
 *
 * You can create a new block by deriving this class and implementing at least
 * the output method.
 *
 * This block implements the following default behaviours:
 * - it ask for 4 parameters (robot name, local (module) name, names of the remote control boards,
 *   and the list of joints which should all belong to one of the remote control boards)
 * - It initializes the yarp network and the whole body interface object
 * - During terminate it closes and release the interface object and terminate the yarp network
 *
 * @note Usually you want to call this class implementations at some point in your
 * method overridings, unless you want to completely change the code (but at that point
 * you probabily want to derive from Block instead)
 */
class wbt::WBBlock : public wbt::Block
{
private:
    static const unsigned ConfigurationParameterIndex;
    static const unsigned ConfBlockNameParameterIndex;

protected:
    std::string confKey;
    iDynTreeRobotState robotState;
    bool getWBToolboxParameters(Configuration& config, BlockInformation* blockInfo);
    const std::shared_ptr<wbt::RobotInterface> getRobotInterface();
    const Configuration& getConfiguration();

public:
    WBBlock() = default;
    ~WBBlock() override = default;
    unsigned numberOfParameters() override;
    bool configureSizeAndPorts(BlockInformation* blockInfo) override;
    bool initialize(BlockInformation* blockInfo) override;
    bool terminate(BlockInformation* blockInfo) override;
};

#endif /* end of include guard: WBT_WBIBLOCK_H */
