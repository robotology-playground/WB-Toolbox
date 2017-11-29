#ifndef ANYTYPE_H
#define ANYTYPE_H

#include <vector>
#include <unordered_map>
#include <memory>

class AnyType;

typedef std::shared_ptr<AnyType> AnyTypeSPtr;
typedef std::vector<AnyTypeSPtr> AnyCell;
typedef std::unordered_map<std::string, AnyTypeSPtr> AnyStruct;

class AnyType
{
protected:

public:
    AnyType() = default;
    virtual ~AnyType() = default;

    // Integers
    virtual bool asInt(int& i)       = 0;
    // virtual bool asInt8(int8_t& i)   = 0;
    // virtual bool asInt16(int16_t& i) = 0;
    virtual bool asInt32(int32_t& i) = 0;
    // virtual bool asInt64(int64_t& i) = 0;

    // Unsigned Integers
    virtual bool asUInt(unsigned& i) = 0;
    // virtual bool asUInt8(uint8_t& i)     = 0;
    // virtual bool asUInt16(uint16_t& i)   = 0;
    // virtual bool asUInt32(uint32_t& i)   = 0;
    // virtual bool asUInt64(uint64_t& i)   = 0;

    // Boolean
    virtual bool asBool(bool& b) = 0;

    // Floating-point
    // virtual bool asFloat(float& f)   = 0;
    virtual bool asDouble(double& d) = 0;

    // Characters
    virtual bool asString(std::string& s) = 0;

    // Struct
    virtual bool asAnyStruct(AnyStruct& map) = 0;

    // Cell array
    virtual bool asAnyCell(AnyCell& map) = 0;

    // Matrix
    // TODO: constraint max 2-dimension
    // virtual bool asMatrixFloat(Eigen::MatrixXf mat)  = 0;
    // virtual bool asMatrixDouble(Eigen::MatrixXd mat) = 0;

    // Vector
    virtual bool asVectorDouble(std::vector<double>& vec) = 0;
};

#endif /* ANYTYPE_H */
