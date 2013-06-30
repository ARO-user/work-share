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
// $Id: altls.h,v 1.1.1.1 2004/12/16 14:27:28 mwyrick Exp $
// $Log: altls.h,v $
// Revision 1.1.1.1  2004/12/16 14:27:28  mwyrick
// ALLinux 2004
//
// Revision 1.1.1.1  2004/04/29 19:20:14  mwyrick
// AcqLog Linux Driver
//
// Revision 1.5  2002/10/24 15:59:48  mwyrick
// Clean up under way
// NO boards except ALTLS
//
// Revision 1.4  2002/10/22 18:23:26  mwyrick
// First Working version with ALtls
//
// Revision 1.3  2002/10/21 19:13:09  mwyrick
// Added new Constants
//
// Revision 1.2  2002/10/21 19:10:05  mwyrick
// TLS Board Uploads OK
//
// Revision 1.1  2002/10/21 18:54:42  mwyrick
// Working in the new ALTLS board
//
// Revision 1.4  2002/10/16 16:06:29  mwyrick
// ----------------------------------------------------------------
// Moved All Files into one compile so every function can be Static
// ----------------------------------------------------------------
//
// Revision 1.3  2002/10/11 18:20:41  mwyrick
// More Registers
//
// Revision 1.2  2002/10/11 15:01:11  mwyrick
// Moved upload code to seperate files
//
// Revision 1.1.1.1  2002/08/21 13:37:36  mwyrick
// Acquisition Logic Linux Driver
//
//
//-----------------------------------------------------------------------------
#define ALTLS_CONF_OFFSET	0x0C

#define CONF_MASK	0x0D
#define CONF_START	0x00
#define CONF_NEXT	0x05
#define CONF_ERROR	0x01
#define CONF_DONE	0x0D

#define ALTLS_DTR	0x40
#define ALTLS_VERSION	0x7C
#define ALTLS_GCR	0x80
#define ALTLS_SR	0x81
#define ALTLS_MR	0x82
#define ALTLS_FRR	0x83
#define ALTLS_EEPROMCR	0x95


static void initALTLS(void);
static void ALTLS_BrdAbortDMA(void);
static int UploadTLSRBF(char *data, long size);

