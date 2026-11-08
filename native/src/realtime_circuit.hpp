#pragma once
#include "circuit.hpp"
#include "pt2399_core.hpp"
#include "mn3007_core.hpp"
#include <cstddef>
#include <array>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
namespace vbal {
class RealtimeCircuitProcessor {
public:
 RealtimeCircuitProcessor(const Circuit& circuit,double sampleRate);
 void reset();
 float process(float input);
 int probeCode(const std::string& node) const noexcept;
 float voltageAtCode(int code,float input) const noexcept;
 size_t unknownNodeCount() const noexcept { return unknowns_.size(); }
private:
 struct RuntimePart { Part part; int a{-2}; int b{-2}; int c{-2}; int d{-2}; int e{-2}; double previousVoltage{}; double previousCurrent{}; std::array<int,16> icPins{}; std::unique_ptr<Pt2399Core> pt2399; double pt2399Voltage{}; double pt2399ControlOhms{}; std::unique_ptr<Mn3007Core> bbd; double bbdOutput{}; double clockHz{}; double clockPhase{}; };
 Circuit circuit_;
 double sampleRate_{};
 std::vector<std::string> unknowns_;
 std::unordered_map<std::string,int> nodeIndex_,fixedCodes_;
 std::vector<RuntimePart> parts_;
 std::vector<double> matrix_,rhs_,solution_,guess_,fixedVoltages_;
 int outputIndex_{-2};
 double fixedVoltage(int index,float input) const noexcept;
 void stampConductance(int a,int b,double g,float input);
 void stampCurrent(int a,int b,double current);
 bool solveLinear();
};
}
