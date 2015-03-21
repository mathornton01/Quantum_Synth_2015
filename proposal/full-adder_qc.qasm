# 
# File: full-adder_qc.qasm
# Date: March 21 2015
# Authors: Josh Rendon, Micah Thornton
#
 qubit c
 qubit x
 qubit y
 qubit z1,0
 qubit z2,0

 toffoli       x,y,z2
 toffoli       c,x,z2
 toffoli       c,y,z2
 cnot          c,z1
 cnot          x,z1
 cnot          y,z1
