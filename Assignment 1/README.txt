V01000551
CSC360 A01
Harsh Sreelal

Instructions to compile and run pman.c 
1. Compile the program by typing 'make' in the terminal. 
2. To run the executable object code of 'pman', enter ./pman in the terminal. "Pman: >" is prompted.

Instructions to use pman
1. bg <cmd> <args>: runs the executable code of the desired process along with required args (Ex: bg ./inf hello 10)
2. bglist: lists all the processes that have been started with the prompt ()
3. bgstop <pid>: stops running the process of the respective pid by sending the STOP signal to the process
4. bgstart <pid>: starts running the process of the respective pid by sending the CONT signal to the process
5. bgkill <pid>: kills running the process of the respective pid by sending the TERM signal to the process
6. pstat <pid>: lists the comm, state, utime, stime, rss, voluntary context switches and involuntary context switches for the process with the respective pid