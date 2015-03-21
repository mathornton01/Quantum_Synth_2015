# 
# File: full-adder_qc.qasm
# Date: March 21 2015
# Authors: Josh Rendon, Micah Thornton
#
 qubit c
 qubit x
 qubit y
 qubit g1,0
 qubit g2,0

 nop           c
 nop           x
 nop           y
 toffoli       x,y,g2
 toffoli       c,x,g2
 toffoli       c,y,g2
 cnot          c,g1
 cnot          x,g1
 cnot          y,g1
