# Pitt CS2210
### Compiler Design projects
### Fall 2023, Dr. Youtao Zhang
### Birju Patel

Dependencies - gcc, yacc, lex\
The program successfully compiles all test cases except src9.\
I compiled and ran the code generator on oxygen.cs.pitt.edu.\
I tested the compiled code with QTSpim, a Windows based MIPS simulator. (https://spimsimulator.sourceforge.net/)

Instructions -\
Spin up a docker container with a 32 bit image of Ubuntu (docker run --rm -it i686/ubuntu bash)\
Install the following dependencies\
	&nbsp;&nbsp;&nbsp;&nbsp;apt update\
	&nbsp;&nbsp;&nbsp;&nbsp;apt install build-essential\
	&nbsp;&nbsp;&nbsp;&nbsp;apt-get install flex\
	&nbsp;&nbsp;&nbsp;&nbsp;apt-get install bison\
	&nbsp;&nbsp;&nbsp;&nbsp;apt-get install byacc\
Transfer binaries to container and make all\
Compile a code sample in examples directory by running the following command\
	&nbsp;&nbsp;&nbsp;&nbsp;./main < examples/src1\
Copy the code written to code.s and run it on QTSpim
