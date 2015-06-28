# remote_car
I have provided comments wherever I deemed them necessary. Other than that the code is self explanatory. I have also provided
the startup_ccs.c file which is the interrupt vector table necessary to use the appropriate interrupt number. 
I have used UART module 2 on the launchpad which is interrupt no. 33 to receive the bluetooth data from HC-05 bluetooth module 
which is used in receive mode. 
Pushing the buttons on the mobile app sends corresponding characters to the bluetooth module which are then checked to do the corresponding action.  
