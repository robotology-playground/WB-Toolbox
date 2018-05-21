/*
 * Copyright (C) 2018 Istituto Italiano di Tecnologia (IIT)
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms of the
 * GNU Lesser General Public License v2.1 or any later version.
 */

#include "YarpWrite.h"
#include "BlockInformation.h"
#include "Log.h"
#include "Parameter.h"
#include "Parameters.h"
#include "Signal.h"

#include <yarp/os/BufferedPort.h>
#include <yarp/os/Network.h>
#include <yarp/os/PortReaderBuffer-inl.h>
#include <yarp/sig/Vector.h>

#include <ostream>
#include <tuple>

using namespace wbt;
const std::string YarpWrite::ClassName = "YarpWrite";

// INDICES: PARAMETERS, INPUTS, OUTPUT
// ===================================

enum ParamIndex
{
    Bias = Block::NumberOfParameters - 1,
    PortName,
    Autoconnect,
    ErrMissingPort,
};

enum InputIndex
{
    Signal = 0,
};

// BLOCK PIMPL
// ===========

class YarpWrite::impl
{
public:
    bool autoconnect = false;
    bool errorOnMissingPort = true;

    std::string destinationPortName;
    yarp::sig::Vector outputVector;
    yarp::os::BufferedPort<yarp::sig::Vector> port;
};

// BLOCK CLASS
// ===========

YarpWrite::YarpWrite()
    : pImpl{new impl()}
{}

unsigned YarpWrite::numberOfParameters()
{
    return Block::numberOfParameters() + 3;
}

bool YarpWrite::parseParameters(BlockInformation* blockInfo)
{
    const std::vector<ParameterMetadata> metadata{
        {ParameterType::STRING, ParamIndex::PortName, 1, 1, "PortName"},
        {ParameterType::BOOL, ParamIndex::Autoconnect, 1, 1, "Autoconnect"},
        {ParameterType::BOOL, ParamIndex::ErrMissingPort, 1, 1, "ErrorOnMissingPort"}};

    for (const auto& md : metadata) {
        if (!blockInfo->addParameterMetadata(md)) {
            wbtError << "Failed to store parameter metadata";
            return false;
        }
    }

    return blockInfo->parseParameters(m_parameters);
}

bool YarpWrite::configureSizeAndPorts(BlockInformation* blockInfo)
{
    // INPUT
    // =====
    //
    // 1) The signal to stream to the specified yarp port
    //
    // OUTPUT
    // ======
    //
    // No outputs
    //

    const bool ok = blockInfo->setIOPortsData({
        {
            // Inputs
            std::make_tuple(
                InputIndex::Signal, std::vector<int>{Signal::DynamicSize}, DataType::DOUBLE),
        },
        {
            // Outputs
        },
    });

    if (!ok) {
        wbtError << "Failed to configure input / output ports.";
        return false;
    }

    return true;
}

bool YarpWrite::initialize(BlockInformation* blockInfo)
{
    using namespace yarp::os;
    using namespace yarp::sig;

    // PARAMETERS
    // ==========

    if (!YarpWrite::parseParameters(blockInfo)) {
        wbtError << "Failed to parse parameters.";
        return false;
    }

    std::string portName;

    bool ok = true;
    ok = ok && m_parameters.getParameter("PortName", portName);
    ok = ok && m_parameters.getParameter("Autoconnect", pImpl->autoconnect);
    ok = ok && m_parameters.getParameter("ErrorOnMissingPort", pImpl->errorOnMissingPort);

    if (!ok) {
        wbtError << "Failed to read input parameters.";
        return false;
    }

    // CLASS INITIALIZATION
    // ====================

    Network::init();

    if (!Network::initialized() || !Network::checkNetwork(5.0)) {
        wbtError << "YARP server wasn't found active.";
        return false;
    }

    std::string sourcePortName;

    // Autoconnect: the block opens a temporary output port ..., and it connects to an existing
    //              port portName (which will receive data).
    if (pImpl->autoconnect) {
        sourcePortName = "...";
        pImpl->destinationPortName = portName;
    }
    // Manual connection: the block opens an output port portName, and waits a manual connection to
    // an input port.
    else {
        sourcePortName = portName;
    }

    if (!pImpl->port.open(sourcePortName)) {
        wbtError << "Error while opening yarp port.";
        return false;
    }

    if (pImpl->autoconnect) {
        if (!Network::connect(pImpl->port.getName(), pImpl->destinationPortName)) {
            wbtWarning << "Failed to connect " << pImpl->port.getName() << " to "
                       << pImpl->destinationPortName << ".";
            if (pImpl->errorOnMissingPort) {
                wbtError << "Failed connecting ports.";
                return false;
            }
        }
    }

    // Get the input port width
    int inputPortWidth = std::get<BlockInformation::Port::Dimensions>(
                             blockInfo->getInputPortData(InputIndex::Signal))
                             .front();

    // Initialize the buffer
    pImpl->outputVector = pImpl->port.prepare();
    pImpl->outputVector.resize(inputPortWidth);
    return true;
}

bool YarpWrite::terminate(const BlockInformation* /*blockInfo*/)
{
    if (pImpl->autoconnect) {
        yarp::os::Network::disconnect(pImpl->port.getName(), pImpl->destinationPortName);
    }

    // Close the port
    pImpl->port.close();

    yarp::os::Network::fini();
    return true;
}

bool YarpWrite::output(const BlockInformation* blockInfo)
{
    const Signal signal = blockInfo->getInputPortSignal(InputIndex::Signal);

    if (!signal.isValid()) {
        wbtError << "Input signal not valid.";
        return false;
    }

    pImpl->outputVector = pImpl->port.prepare();
    pImpl->outputVector.resize(signal.getWidth()); // this should be a no-op

    for (unsigned i = 0; i < signal.getWidth(); ++i) {
        pImpl->outputVector[i] = signal.get<double>(i);
    }

    pImpl->port.write();

    return true;
}
