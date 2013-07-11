//------------------------------------------------------------------------------  
//title: Acquisition Logic A/D Board Kernel Driver
//version: Linux 0.0
//date: July 2002                                                              
//designer: Michael Wyrick                                                      
//programmer: Michael Wyrick                                                    
//platform: Linux 2.4.x
//language: GCC 2.95 and 3.0
//module: aldriver
//------------------------------------------------------------------------------  
//  Purpose: Provide a Kernel Driver to Linux for the ALI A/D Boards
//  Docs:                                  
//    This driver supplies an interface to the raw Registers on the boards.
//    in is upto a user library or program to determine what to do with those
//    registers.
//------------------------------------------------------------------------------  
// RCS:
// $Id: al8100_funcs.h,v 1.2 2004/12/22 19:29:06 mwyrick Exp $
// $Log: al8100_funcs.h,v $
// Revision 1.2  2004/12/22 19:29:06  mwyrick
// Working on Multi Board
//
// Revision 1.1.1.1  2004/12/16 14:27:28  mwyrick
// ALLinux 2004
//
// Revision 1.1.1.1  2004/04/29 19:20:14  mwyrick
// AcqLog Linux Driver
//
// Revision 1.2  2002/12/18 21:50:37  mwyrick
// IOCTL_GET_BRDTYPE
//
// Revision 1.1  2002/12/09 22:23:17  mwyrick
// Functions for only the AL8100
//
//
//-----------------------------------------------------------------------------
static void initAL8100(int);
static int Upload8100RBF(int, char *data, long size);
static void AL8100_SWTrigger(int);
