# ProjectTemplate

This repository contains a basic ClearCore project that can be used as a template for new application development.
Place this repository rooted in the same parent directory as libClearCore and LwIP to properly find include files and libraries.

#(AXF) Manager
Glues all of the components together.
Tracks performance, states, comms, and IO.  
Includes any required algorithms or operation functions that require running across multiple categories of operation

#Comms Manager
All modules required for sending and receiving data

#Data Manager
Tracks all datapoints that are connected externally.

#IO Manager
Tracks all IO points and provides an interface to interact with them

#State Manager(s)
Tracks the machine state, and ties together each type of state management

#Power State
Tracks the machine power state - All other state managers are dependent on the power state being set to ready

#Machine State
Tracks the machine state, and controls what operation the machine can currently perform

#Cycle State
Tracks the machine cycling state - checks for cycle errors, and successes.

