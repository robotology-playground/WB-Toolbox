/*
 * Copyright (C) 2018 Istituto Italiano di Tecnologia (IIT)
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms of the
 * GNU Lesser General Public License v2.1 or any later version.
 */

#include "CoderBlockInformation.h"
#include "Log.h"
#include "Parameter.h"
#include "Parameters.h"

#include <string>
#include <tuple>
#include <utility>
#include <vector>

using namespace wbt;

class CoderBlockInformation::impl
{
public:
    unsigned numberOfInputs;
    unsigned numberOfOutputs;

    std::vector<wbt::ParameterMetadata> paramsMetadata;
    std::unordered_map<BlockInformation::PortIndex, wbt::Signal> inputSignals;
    std::unordered_map<BlockInformation::PortIndex, wbt::Signal> outputSignals;

    std::string confBlockName;
    Parameters parametersFromRTW;

    std::unordered_map<BlockInformation::PortIndex, BlockInformation::PortDimension>
        inputPortDimensions;
    std::unordered_map<BlockInformation::PortIndex, BlockInformation::PortDimension>
        outputPortDimensions;
};

CoderBlockInformation::CoderBlockInformation()
    : pImpl{new CoderBlockInformation::impl()}
{}

CoderBlockInformation::~CoderBlockInformation() = default;

// BLOCK OPTIONS METHODS
// =====================

bool CoderBlockInformation::optionFromKey(const std::string& /*key*/, double& /*option*/) const
{
    return true;
}

// PORT INFORMATION SETTERS
// ========================

bool CoderBlockInformation::setIOPortsData(const BlockInformation::IOData& /*ioData*/)
{
    // This method is called only in the Block::configureSizeAndPorts method, which is never called
    // in the Simulink Coder pipeline.
    return false;
}

// PORT INFORMATION GETTERS
// ========================

BlockInformation::VectorSize CoderBlockInformation::getInputPortWidth(const PortIndex idx) const
{
    if (pImpl->inputPortDimensions.find(idx) == pImpl->inputPortDimensions.end()) {
        wbtError << "Failed to get width of signal at index " << idx << ".";
        return 0;
    }

    return static_cast<unsigned>(pImpl->inputPortDimensions.at(idx).front());
}

BlockInformation::VectorSize CoderBlockInformation::getOutputPortWidth(const PortIndex idx) const
{
    if (pImpl->outputPortDimensions.find(idx) == pImpl->outputPortDimensions.end()) {
        wbtError << "Failed to get width of signal at index " << idx << ".";
        return 0;
    }

    return static_cast<unsigned>(pImpl->outputPortDimensions.at(idx).front());
}

wbt::Signal CoderBlockInformation::getInputPortSignal(const PortIndex idx,
                                                      const VectorSize size) const
{
    if (pImpl->inputSignals.find(idx) == pImpl->inputSignals.end()) {
        wbtError << "Trying to get non-existing signal " << idx << ".";
        return {};
    }

    // TODO: portWidth is used only if the signal is dynamically sized. In Simulink, in this case
    // the size is gathered from the SimStruct. From the coder instead? Is it possible having
    // a signal with dynamic size in the rtw file??
    // TODO: is it better this check or the one implemented in getOutputPortSignal?
    if (size != Signal::DynamicSize && pImpl->inputSignals.at(idx).getWidth() != size) {
        wbtError << "Signals with dynamic sizes (index " << idx
                 << ") are not supported by the CoderBlockInformation.";
        return {};
    }

    return pImpl->inputSignals.at(idx);
}

wbt::Signal CoderBlockInformation::getOutputPortSignal(const PortIndex idx,
                                                       const VectorSize /*size*/) const
{
    if (pImpl->outputSignals.find(idx) == pImpl->outputSignals.end()) {
        wbtError << "Trying to get non-existing signal " << idx << ".";
        return {};
    }

    if (pImpl->outputSignals.at(idx).getWidth() == Signal::DynamicSize) {
        wbtError << "Signals with dynamic sizes (index " << idx
                 << ") are not supported by the CoderBlockInformation.";
        return {};
    }

    return pImpl->outputSignals.at(idx);
}

BlockInformation::MatrixSize
CoderBlockInformation::getInputPortMatrixSize(const BlockInformation::PortIndex idx) const
{
    if (pImpl->inputPortDimensions.find(idx) == pImpl->inputPortDimensions.end()) {
        wbtError << "Trying to get the size of non-existing signal " << idx << ".";
        return {};
    }

    return {pImpl->inputPortDimensions.at(idx)[0], pImpl->inputPortDimensions.at(idx)[1]};
}

BlockInformation::MatrixSize
CoderBlockInformation::getOutputPortMatrixSize(const BlockInformation::PortIndex idx) const
{
    if (pImpl->outputPortDimensions.find(idx) == pImpl->outputPortDimensions.end()) {
        wbtError << "Trying to get the size of non-existing signal " << idx << ".";
        return {};
    }

    return {pImpl->outputPortDimensions.at(idx)[0], pImpl->outputPortDimensions.at(idx)[1]};
}

bool CoderBlockInformation::addParameterMetadata(const wbt::ParameterMetadata& paramMD)
{
    for (auto md : pImpl->paramsMetadata) {
        if (md.name == paramMD.name) {
            wbtError << "Trying to store an already existing " << md.name << " parameter.";
            return false;
        }
    }

    pImpl->paramsMetadata.push_back(paramMD);
    return true;
}

// PARAMETERS METHODS
// ==================

bool CoderBlockInformation::parseParameters(wbt::Parameters& parameters)
{
    if (pImpl->parametersFromRTW.getNumberOfParameters() == 0) {
        wbtError << "The Parameters object containing the parameters to parse is empty.";
        return false;
    }

    for (wbt::ParameterMetadata& md : pImpl->paramsMetadata) {
        // Check that all the parameters that are parsed have already been stored from the coder
        if (!pImpl->parametersFromRTW.existName(md.name)) {
            wbtError << "Trying to get a parameter value for " << md.name
                     << ", but its value has never been stored.";
            return false;
        }

        // Handle the case of dynamically sized columns. In this case the metadata passed
        // from the Block (containing DynamicSize) is modified with the length of the
        // vector that is going to be stored.
        if (md.cols == ParameterMetadata::DynamicSize) {
            const auto colsFromRTW = pImpl->parametersFromRTW.getParameterMetadata(md.name).cols;
            if (colsFromRTW == ParameterMetadata::DynamicSize) {
                wbtError << "Trying to store the cols of a dynamically sized parameters, but the "
                         << "metadata does not specify a valid size. Probably the block didn't "
                         << "updat the size in its initialization phase.";
                return false;
            }
            md.cols = colsFromRTW;
        }

        if (md != pImpl->parametersFromRTW.getParameterMetadata(md.name)) {
            wbtError << "Trying to parse a parameter which metadata differs from the metadata "
                     << "stored by Simulink Coder.";
            return false;
        }
    }

    // This implementation of BlockInformation contains all the parameters from the very beginning,
    // stored using the storeRTWParameters method. Here for simplicity all the stored parameters are
    // returned, even if the metadata contain only a subset of them.
    parameters = pImpl->parametersFromRTW;
    return true;
}

BlockInformation::PortData
CoderBlockInformation::getInputPortData(BlockInformation::PortIndex idx) const
{
    // TODO: hardcoded DataType::DOUBLE.
    return std::make_tuple(idx, pImpl->inputPortDimensions.at(idx), DataType::DOUBLE);
}

BlockInformation::PortData
CoderBlockInformation::getOutputPortData(BlockInformation::PortIndex idx) const
{
    // TODO: hardcoded DataType::DOUBLE.
    return std::make_tuple(idx, pImpl->outputPortDimensions.at(idx), DataType::DOUBLE);
}

bool CoderBlockInformation::storeRTWParameters(const Parameters& parameters)
{
    if (parameters.getNumberOfParameters() == 0) {
        wbtError << "The Parameters object passed doesn't contain any parameter.";
        return false;
    }

    pImpl->parametersFromRTW = parameters;
    return true;
}

bool CoderBlockInformation::setInputSignal(const PortIndex portNumber,
                                           void* address,
                                           const PortDimension& dims)
{
    if (pImpl->inputSignals.find(portNumber) != pImpl->inputSignals.end()) {
        wbtError << "The signal " << portNumber << "has already been previously stored.";
        return false;
    }

    if (!address) {
        wbtError << "The pointer to the signal to store is a nullptr.";
        return false;
    }

    if (dims.size() > 2) {
        wbtError << "Signal with more than 2 dimensions are not currently supported.";
        return false;
    }

    // TODO: hardcoded DataType::DOUBLE
    bool isConst = true;
    pImpl->inputSignals.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(portNumber),
        std::forward_as_tuple(Signal::DataFormat::CONTIGUOUS_ZEROCOPY, DataType::DOUBLE, isConst));

    unsigned numElements = 1;
    for (auto dimension : dims) {
        numElements *= dimension;
    }

    pImpl->inputSignals[portNumber].setWidth(numElements);
    return pImpl->inputSignals[portNumber].initializeBufferFromContiguousZeroCopy(address);
}

bool CoderBlockInformation::setOutputSignal(const PortIndex portNumber,
                                            void* address,
                                            const PortDimension& dims)
{
    if (pImpl->outputSignals.find(portNumber) != pImpl->outputSignals.end()) {
        wbtError << "The signal " << portNumber << "has already been previously stored.";
        return false;
    }

    if (!address) {
        wbtError << "The pointer to the signal to store is a nullptr.";
        return false;
    }

    if (dims.size() > 2) {
        wbtError << "Signal with more than 2 dimensions are not currently supported.";
        return false;
    }

    // TODO: hardcoded DataType::DOUBLE.
    bool isConst = false;
    pImpl->outputSignals.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(portNumber),
        std::forward_as_tuple(Signal::DataFormat::CONTIGUOUS_ZEROCOPY, DataType::DOUBLE, isConst));

    unsigned numElements = 1;
    for (auto dimension : dims) {
        numElements *= dimension;
    }

    pImpl->outputSignals[portNumber].setWidth(numElements);
    return pImpl->outputSignals[portNumber].initializeBufferFromContiguousZeroCopy(address);
}
