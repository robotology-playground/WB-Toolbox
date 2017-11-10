#include "YarpClock.h"

#include "Log.h"
#include "BlockInformation.h"
#include "Signal.h"

#include <yarp/os/Network.h>
#include <yarp/os/Time.h>

namespace wbt {

    const std::string YarpClock::ClassName = "YarpClock";

    unsigned YarpClock::numberOfParameters() { return 0; }

    bool YarpClock::configureSizeAndPorts(BlockInformation* blockInfo)
    {
        // INPUTS
        // ======
        //
        // No inputs
        //

        if (!blockInfo->setNumberOfInputPorts(0)) {
            Log::getSingleton().error("Failed to set input port number to 0.");
            return false;
        }

        // OUTPUT
        // ======
        //
        // 1) The yarp time. In short, it streams yarp::os::Time::now().
        //

        if (!blockInfo->setNumberOfOutputPorts(1)) {
            Log::getSingleton().error("Failed to set output port number.");
            return false;
        }

        blockInfo->setOutputPortVectorSize(0, 1);
        blockInfo->setOutputPortType(0, PortDataTypeDouble);

        return true;
    }

    bool YarpClock::initialize(BlockInformation* blockInfo)
    {
        yarp::os::Network::init();

        if (!yarp::os::Network::initialized() || !yarp::os::Network::checkNetwork(5.0)) {
            Log::getSingleton().error("YARP server wasn't found active!!");
            return false;
        }

        return true;
    }

    bool YarpClock::terminate(BlockInformation* /*S*/)
    {
        yarp::os::Network::fini();
        return true;
    }

    bool YarpClock::output(BlockInformation* blockInfo)
    {
        Signal signal = blockInfo->getOutputPortSignal(0);
        signal.set(0, yarp::os::Time::now());
        return true;
    }
}
