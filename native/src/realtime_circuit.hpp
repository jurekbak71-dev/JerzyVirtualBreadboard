#pragma once
#include "circuit.hpp"
#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>
namespace vbal {
class RealtimeCircuitProcessor {
public:
 RealtimeCircuitProcessor(const Circuit& circuit,double sampleRate);
 void reset();
 float process(float input);
 size_t unknownNodeCount() const noexcept { return unknowns_.size(); }
private:
 struct RuntimePart { Part part; int a{-2}; int b{-2}; double previousVoltage{}; double previousCurrent{}; };
 Circuit circuit_;
 double sampleRate_{};
 std::vector<std::string> unknowns_;
 std::unordered_map<std::string,int> nodeIndex_;
 std::vector<RuntimePart> parts_;
 std::vector<double> matrix_,rhs_,solution_,guess_,fixedVoltages_;
 int outputIndex_{-2};
 double fixedVoltage(int index,float input) const noexcept;
 void stampConductance(int a,int b,double g,float input);
 void stampCurrent(int a,int b,double current);
 bool solveLinear();
};
}
