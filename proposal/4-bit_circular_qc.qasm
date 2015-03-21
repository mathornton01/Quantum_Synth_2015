# 
# File: 4-bit_circular_hash_qc.qasm
# Date: March 21 2015
# Authors: Josh Rendon, Micah Thornton
#
 def not,0,'$\oplus$'
 def toffoli4,3,'$\oplus$'

 qubit in4
 qubit in3
 qubit in2
 qubit in1

 toffoli4      in4,in3,in2,in1
 toffoli       in4,in3,in2
 cnot          in4,in3
 not           in4
