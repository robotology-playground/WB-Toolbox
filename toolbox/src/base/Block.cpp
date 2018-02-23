#include "Block.h"
#include "toolbox.h"

using namespace wbt;

Block::~Block() {}
std::vector<std::string> Block::additionalBlockOptions()
{
    return std::vector<std::string>();
}

void Block::parameterAtIndexIsTunable(unsigned /*index*/, bool& tunable)
{
    tunable = false;
}

bool Block::checkParameters(const BlockInformation* /*blockInfo*/)
{
    return true;
}

unsigned Block::numberOfDiscreteStates()
{
    return 0;
}

unsigned Block::numberOfContinuousStates()
{
    return 0;
}

bool Block::updateDiscreteState(const BlockInformation* /*blockInfo*/)
{
    return true;
}

bool Block::stateDerivative(const BlockInformation* /*blockInfo*/)
{
    return true;
}

bool Block::initializeInitialConditions(const BlockInformation* /*blockInfo*/)
{
    return true;
}
