/*
 * Copyright (C) 2018 Istituto Italiano di Tecnologia (IIT)
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms of the
 * GNU Lesser General Public License v2.1 or any later version.
 */

#include "YarpRead.h"
#include "BlockInformation.h"
#include "Log.h"
#include "Parameter.h"
#include "Parameters.h"
#include "Signal.h"

#include <yarp/os/BufferedPort.h>
#include <yarp/os/Network.h>
#include <yarp/os/PortReaderBuffer-inl.h>
#include <yarp/os/Stamp.h>
#include <yarp/os/SystemClock.h>
#include <yarp/os/Time.h>
#include <yarp/sig/Vector.h>

#include <algorithm>
#include <iostream>
#include <string>

using namespace wbt;
const std::string YarpRead::ClassName = "YarpRead";

// INDICES: PARAMETERS, INPUTS, OUTPUT
// ===================================

enum ParamIndex
{
    Bias = Block::NumberOfParameters - 1,
    PortName,
    PortSize,
    WaitData,
    ReadTimestamp,
    Autoconnect,
    ErrMissingPort,
    Timeout
};

// BLOCK PIMPL
// ===========

class YarpRead::impl
{
public:
    bool autoconnect = false;
    bool blocking = false;
    bool shouldReadTimestamp = false;
    bool errorOnMissingPort = true;
    int bufferSize;
    double timeout = 1.0;
    std::string sourcePortName;

    yarp::os::BufferedPort<yarp::sig::Vector> port;
};

// BLOCK CLASS
// ===========

YarpRead::YarpRead()
    : pImpl{new impl()}
{}

unsigned YarpRead::numberOfParameters()
{
    return Block::numberOfParameters() + 7;
}

bool YarpRead::parseParameters(BlockInformation* blockInfo)
{
    const std::vector<ParameterMetadata> metadata{
        {ParameterType::STRING, ParamIndex::PortName, 1, 1, "PortName"},
        {ParameterType::INT, ParamIndex::PortSize, 1, 1, "SignalSize"},
        {ParameterType::BOOL, ParamIndex::WaitData, 1, 1, "WaitData"},
        {ParameterType::BOOL, ParamIndex::ReadTimestamp, 1, 1, "ReadTimestamp"},
        {ParameterType::BOOL, ParamIndex::Autoconnect, 1, 1, "Autoconnect"},
        {ParameterType::DOUBLE, ParamIndex::Timeout, 1, 1, "Timeout"},
        {ParameterType::BOOL, ParamIndex::ErrMissingPort, 1, 1, "ErrorOnMissingPort"}};

    for (const auto& md : metadata) {
        if (!blockInfo->addParameterMetadata(md)) {
            wbtError << "Failed to store parameter metadata";
            return false;
        }
    }

    return blockInfo->parseParameters(m_parameters);
}

bool YarpRead::configureSizeAndPorts(BlockInformation* blockInfo)
{
    // INPUTS
    // ======
    //
    // No inputs
    //

    if (!blockInfo->setNumberOfInputPorts(0)) {
        wbtError << "Failed to set input port number to 0.";
        return false;
    }

    // OUTPUTS
    // =======
    //
    // 1) Vector with the data read from the port (1 x signalSize)
    // 2) Optional: Timestamp read from the port
    // 3) Optional: If autoconnect is disabled, this output becomes 1 when data starts arriving
    //              (and hence it means that the user connected manually the port)
    //

    if (!YarpRead::parseParameters(blockInfo)) {
        wbtError << "Failed to parse parameters.";
        return false;
    }

    bool readTimestamp = false;
    bool autoconnect = false;
    int signalSize = 0;

    bool ok = true;
    ok = ok && m_parameters.getParameter("ReadTimestamp", readTimestamp);
    ok = ok && m_parameters.getParameter("Autoconnect", autoconnect);
    ok = ok && m_parameters.getParameter("SignalSize", signalSize);

    if (!ok) {
        wbtError << "Failed to read input parameters.";
        return false;
    }

    if (signalSize < 0) {
        wbtError << "Signal size must be non negative.";
        return false;
    }

    int numberOfOutputPorts = 1;
    numberOfOutputPorts += static_cast<unsigned>(readTimestamp); // timestamp is the second port
    numberOfOutputPorts +=
        static_cast<unsigned>(!autoconnect); // !autoconnect => additional port with 1/0
                                             // depending on the connection status

    if (!blockInfo->setNumberOfOutputPorts(numberOfOutputPorts)) {
        wbtError << "Failed to set output port number.";
        return false;
    }

    blockInfo->setOutputPortVectorSize(0, static_cast<int>(signalSize));
    blockInfo->setOutputPortType(0, DataType::DOUBLE);

    int portIndex = 1;
    if (readTimestamp) {
        blockInfo->setOutputPortVectorSize(portIndex, 2);
        blockInfo->setOutputPortType(portIndex, DataType::DOUBLE);
        portIndex++;
    }
    if (!autoconnect) {
        blockInfo->setOutputPortVectorSize(portIndex, 1);
        blockInfo->setOutputPortType(
            portIndex, DataType::DOUBLE); // use double anyway. Simplifies simulink stuff
        portIndex++;
    }
    return true;
}

bool YarpRead::initialize(BlockInformation* blockInfo)
{
    using namespace yarp::os;
    using namespace yarp::sig;

    if (!YarpRead::parseParameters(blockInfo)) {
        wbtError << "Failed to parse parameters.";
        return false;
    }

    Network::init();

    if (!Network::initialized() || !Network::checkNetwork(5.0)) {
        wbtError << "YARP server wasn't found active.";
        return false;
    }

    std::string portName;

    bool ok = true;
    ok = ok && m_parameters.getParameter("PortName", portName);
    ok = ok && m_parameters.getParameter("ReadTimestamp", pImpl->shouldReadTimestamp);
    ok = ok && m_parameters.getParameter("Autoconnect", pImpl->autoconnect);
    ok = ok && m_parameters.getParameter("WaitData", pImpl->blocking);
    ok = ok && m_parameters.getParameter("ErrorOnMissingPort", pImpl->errorOnMissingPort);
    ok = ok && m_parameters.getParameter("Timeout", pImpl->timeout);

    if (!ok) {
        wbtError << "Failed to read input parameters.";
        return false;
    }

    std::string destinationPortName;

    // Autoconnect: the block opens a temporary input port ..., and it connects to an existing
    //              port portName (which is streaming data).
    if (pImpl->autoconnect) {
        pImpl->sourcePortName = portName;
        destinationPortName = "...";
    }
    // Manual connection: the block opens an input port portName, and waits a manual connection to
    // an output port.
    else {
        destinationPortName = portName;
    }

    if (!pImpl->port.open(destinationPortName)) {
        wbtError << "Error while opening yarp port.";
        return false;
    }

    if (pImpl->autoconnect) {
        if (!Network::connect(pImpl->sourcePortName, pImpl->port.getName())) {
            wbtWarning << "Failed to connect " + pImpl->sourcePortName + " to "
                              + pImpl->port.getName() + ".";
            if (pImpl->errorOnMissingPort) {
                wbtError << "Failed connecting ports.";
                return false;
            }
        }
    }
    return true;
}

bool YarpRead::terminate(const BlockInformation* /*blockInfo*/)
{
    if (pImpl->autoconnect) {
        yarp::os::Network::disconnect(pImpl->port.getName(), pImpl->sourcePortName);
    }

    // Close the port
    pImpl->port.close();

    yarp::os::Network::fini();
    return true;
}

bool YarpRead::output(const BlockInformation* blockInfo)
{
    int timeStampPortIndex = 0;
    int connectionStatusPortIndex = 0;

    if (pImpl->shouldReadTimestamp) {
        timeStampPortIndex = 1;
    }
    if (!pImpl->autoconnect) {
        connectionStatusPortIndex = timeStampPortIndex + 1;
    }

    yarp::sig::Vector* v = nullptr;

    if (pImpl->blocking) {
        // Initialize the time counter for the timeout
        const double t0 = yarp::os::SystemClock::nowSystem();

        // Loop until something has been read or timeout is reached
        while (true) {
            const int new_bufferSize = pImpl->port.getPendingReads();

            if (new_bufferSize > pImpl->bufferSize) {
                const bool shouldWait = false;
                v = pImpl->port.read(shouldWait);
                pImpl->bufferSize = pImpl->port.getPendingReads();
                break;
            }

            yarp::os::Time::delay(0.0005);
            const double now = yarp::os::Time::now();
            if ((now - t0) > pImpl->timeout) {
                wbtError << "The port didn't receive any data for longer "
                         << "than the configured timeout.";
                return false;
            }
        }
    }
    else {
        bool shouldWait = false;
        v = pImpl->port.read(shouldWait);
    }

    if (v) {
        if (pImpl->shouldReadTimestamp) {
            connectionStatusPortIndex++;
            yarp::os::Stamp timestamp;
            if (!pImpl->port.getEnvelope(timestamp)) {
                wbtError << "Failed to read port envelope (timestamp). Be sure"
                         << " that the input port actually writes this data.";
                return false;
            }

            Signal pY1 = blockInfo->getOutputPortSignal(timeStampPortIndex);
            pY1.set(0, timestamp.getCount());
            pY1.set(1, timestamp.getTime());
        }

        Signal signal = blockInfo->getOutputPortSignal(0);

        if (!signal.isValid()) {
            wbtError << "Output signal not valid.";
            return false;
        }

        // Crop the buffer if it exceeds the OutputPortWidth.
        if (!signal.setBuffer(v->data(),
                              std::min(signal.getWidth(), static_cast<int>(v->size())))) {
            wbtError << "Failed to set the output buffer.";
            return false;
        }

        if (!pImpl->autoconnect) {
            Signal statusPort = blockInfo->getOutputPortSignal(connectionStatusPortIndex);
            if (!statusPort.set(0, 1.0)) {
                wbtError << "Failed to write data to output buffer.";
                return false;
            }
        }
    }

    return true;
}
