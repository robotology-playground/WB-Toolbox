/*
 * Copyright (C) 2018 Istituto Italiano di Tecnologia (IIT)
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms of the
 * GNU Lesser General Public License v2.1 or any later version.
 */

#ifndef WBT_WBBLOCK_H
#define WBT_WBBLOCK_H

#include "Block.h"
#include <memory>
#include <string>

namespace wbt {
    class WBBlock;
    class Signal;
    class BlockInformation;
    class RobotInterface;
} // namespace wbt

namespace iDynTree {
    class KinDynComputations;
} // namespace iDynTree

/**
 * @brief Extension of wbt::Block for simplifying the development of whole-body blocks
 *
 * This class provides support of parsing the parameters for creating a wbt::RobotInterface object,
 * and helpers for retrieving iDynTree::KinDynComputations and wbt::RobotInterface objects.
 *
 * @see wbt::Block
 *
 * @section wbblock_parameters WBBlock Parameters
 *
 * In addition to @ref block_parameters, wbt::WBBlock requires:
 *
 * | Type | Index | Rows  | Cols  | Name  |
 * | ---- | :---: | :---: | :---: | ----- |
 * | ::STRUCT_STRING      | 0 + Block::NumberOfParameters | 1 | 1 | "RobotName"          |
 * | ::STRUCT_STRING      | 0 + Block::NumberOfParameters | 1 | 1 | "UrdfFile"           |
 * | ::STRUCT_CELL_STRING | 0 + Block::NumberOfParameters | 1 | 1 | "ControlledJoints"   |
 * | ::STRUCT_CELL_STRING | 0 + Block::NumberOfParameters | 1 | 1 | "ControlBoardsNames" |
 * | ::STRUCT_STRING      | 0 + Block::NumberOfParameters | 1 | 1 | "LocalName"          |
 * | ::STRUCT_DOUBLE      | 0 + Block::NumberOfParameters | 1 | 3 | "GravityVector"      |
 * | ::STRING             | 1 + Block::NumberOfParameters | 1 | 1 | "ConfBlockName"      |
 *
 * @note The first set of parameters are fields of the same struct. For this reason they share the
 * same index.
 */
class wbt::WBBlock : public wbt::Block
{
protected:
    // TODO: pImpl
    struct iDynTreeRobotState;
    std::unique_ptr<iDynTreeRobotState> m_robotState;
    std::string m_confBlockName;
    std::shared_ptr<wbt::RobotInterface> m_robotInterface;

    /**
     * @brief Helper for retrieving the iDynTree::KinDynComputations object
     * @return A pointer to iDynTree::KinDynComputations.
     */
    std::shared_ptr<iDynTree::KinDynComputations> getKinDynComputations() const;

    /**
     * @brief Helper for retrieving the wbt::RobotInterface object
     * @return A pointer to wbt::RobotInterface.
     */
    const std::shared_ptr<wbt::RobotInterface> getRobotInterface() const;

    /**
     * @brief Helper for setting the robot state inside the iDynTree::KinDynComputations object
     *
     * @param basePose The vector containing the base pose.
     * @param jointsPos The vector containing the joints positions.
     * @param baseVelocity The vector containing the base velocity.
     * @param jointsVelocity The vector containing the joints velocities.
     * @param kinDyn A pointer to the block's KinDynComputations object.
     * @return True if success, false otherwise.
     *
     * @see iDynTree::KinDynComputations::setRobotState, wbt::iDynTreeRobotState
     */
    bool setRobotState(const wbt::Signal* basePose,
                       const wbt::Signal* jointsPos,
                       const wbt::Signal* baseVelocity,
                       const wbt::Signal* jointsVelocity,
                       iDynTree::KinDynComputations* kinDyn);

public:
    /// The number of parameters WBBlock requires
    static constexpr unsigned NumberOfParameters = Block::NumberOfParameters + 2;

    WBBlock();
    ~WBBlock() override;
    unsigned numberOfParameters() override;
    bool parseParameters(BlockInformation* blockInfo) override;
    bool configureSizeAndPorts(BlockInformation* blockInfo) override;
    bool initialize(BlockInformation* blockInfo) override;
};

#endif // WBT_WBBLOCK_H
