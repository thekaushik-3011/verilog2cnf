CXX = g++
CXXFLAGS = -std=c++11 -O2 -Wall

ver2cnf: ver2cnf.cpp
	$(CXX) $(CXXFLAGS) -o ver2cnf ver2cnf.cpp

equiv_checker: equiv_checker.cpp ver2cnf.cpp
	$(CXX) $(CXXFLAGS) -o equiv_checker equiv_checker.cpp ver2cnf.cpp

clean:
	rm -f ver2cnf equiv_checker equivalence.cnf circuit.cnf

.PHONY: clean