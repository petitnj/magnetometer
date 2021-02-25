// =====================================================================
// GBOMag_v3.c
// THEMIS Ground Based Magnetometer control 
// data collection software
// 
// Author: Don Dearboarn (original author)
// Date:   2004/07/26 01:07:34
// Revision: 2.9 
// 
// This is a silent (i.e. no interactive interface) version
// that mimics the version done in LabView by Dave Pierce.
// This is intended for operation in a PC with Linux, (orig RedHat V9)
//
// -------------------------------------
// Version 3 (verbose network telemetry)
// Date:   2014/04/08 10:30:00
// Author: Ian Schofield
// -------------------------------------
//
// Current version (3.0) will compile on 32/64-bit x86 and ARM Linux (Ian S.)
// 
// Usage
// GBOMag -n -i gbo.ini (verbose network ouput) 
//
// Compiling
// gcc -o GBOMag_v3 GBOMag_v3.c ini.c ini.h -lm
// =====================================================================
       
#define SoftwareType "3"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <syslog.h>
#include <string.h>
#include <malloc.h>
#include <math.h>
#include <time.h>
#include "ini.h"

//#include "ini.c"   // No longer need to include this, now that ini file 
                     // is handled in standard way (pcruce 2013-02-06)
#include <errno.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/types.h>
#include <ctype.h>
#include <sys/ioctl.h>
#define BAUDRATE B115200
#define FALSE 0
#define TRUE 1

//# following added for heartbeat generator (krowe 2013-6-13)
#include <unistd.h>
#include <sys/file.h>
#define BUFLEN 7
// krowe end

// schofield begin =============================================================
// following added for network data display (schofield 2014-04-10)
#define DATADISPLAYSVR "127.0.0.1"
#define DATADISPLAYPORT 10234
// schofield end ===============================================================

IniFile my_ini_file;
extern IniFile my_ini_file;
char inifilenam[255] = "/usr/local/etc/fluxgate/gbo.ini";
int Hr = -1, Min, Sec, DGramN;
int gpshr, gpsmo, gpsdy, gpsyr, gpsstat;
int gpsmin, gpssec, INIT = FALSE, MagVerbose = FALSE;
int GPSVerbose = FALSE, StatVerbose = FALSE, TempVerbose = FALSE;
int MsgCntr = 0, RMDOpen = FALSE, syssig = 0;
int Sock, SockLngth, n, itmp[3], NumHosts = 0;
int logmask;
double Nt1, Nt2, Nt3;
struct sockaddr_in Server[3];
struct hostent *Host;
struct timespec req, rem;
char buffer[256], FilID[10], *SiteNam, *MagDevice;
char inbuf[256], passbuf[600], RMDFilePath[256], LogFilePath[256];
char RecPath[256], RecPathL[256], RecPathH[256];
char *DACV1, *DACV2, *DACV3, *Phase, *Drvlat, *Datlat;
char *PolarityC, *SawtoothEnableC, *Decimation, *FeedBackEnable;
char *AutoDacC, *GPSTolerance, HkdFilePath[256], *GBOFileRoot;
int AutoDacB, PolarityB, SawtoothEnableB;
int incnt, NomDeltaT = 500;
int MajVer, MinVer;
float Latitude, Longitude;
float MagScal1 = 0.0, MagScal2 = 0.0, MagScal3 = 0.0;
float DacScal1 = 0.0, DacScal2 = 0.0, DacScal3 = 0.0;
int Dac1 = -1, Dac2 = -1, Dac3 = -1, Sod = -1, Ch1, Ch2, Ch3;
double set[3], tmp[3], cntl[3], Kc[3], Ti[3], Td[3], DelT[3];
double B[3], K1[3], K2[3], K3[3], K4[3], K5[3];


// schofield begin =============================================================
// following added for verbose network telemetry / dynamic web UI
// by Ian Schofield: 2014-04-08
int    NetVerbose = FALSE;
int    AXsock;
int    AXport = 12345;
struct sockaddr_in AXserver;
struct hostent *AXhp;
char   AXbuffer[512];
// schofield end ===============================================================


//# following added for heartbeat generator (krowe 2013-6-13)   
int HBPort = 9100;                      // Port to send data to
char *HBAddress = "128.97.94.198";      // iBoot's IP or hostname
struct sockaddr_in client, server;
int s, slen=sizeof(server);
char buf[BUFLEN];
//  krowe end


struct DGBuf
{
  char ProjectName[4];		// Project Name		Added by William Tam 6-5-2014
  char SitNam[4];		// Site Name
  unsigned int DelT:16;		// Implied delta T between samples
  unsigned int Year:16;		// Year of last data sample
  unsigned int Mon:16;		// Month "  "     "    "
  unsigned int Day:16;		// Date  "  "     "    "
  unsigned long MSOD;		// Milliseconds of day of last data sample
  unsigned int DacScal1:16;	// Dac 1 scale factor * 10
  unsigned int DacScal2:16;	//  "  2   "      "   "  "
  unsigned int DacScal3:16;	//  "  3   "      "   "  "
  unsigned int Dac1:16;		// Dac 1 value during last data sample
  unsigned int Dac2:16;		//  "  2   "     "      "    "    "
  unsigned int Dac3:16;		//  "  3   "     "      "    "    "
  unsigned int GTemp:16;	// GPS         Temperature
  unsigned int STemp:16;	// Sensor          "
  unsigned int ETemp:16;	// Electronics     "
  unsigned int MagScal1:16;	// Mag Channel 1 scale factor * 100
  unsigned int MagScal2:16;	//  "     "    2   "      "   "  "
  unsigned int MagScal3:16;	//  "     "    3   "      "   "  "
  unsigned int GPSStat:16;	// GPS status flags
  unsigned int LatD:16;		// Latitude (degrees) + 90 (i.e. > 90 is N) 
  unsigned int LatM:16;		//     "    (minutes) * 100
  unsigned int LongD:16;	// Longitude (degrees) + 180 (i.e. > 180 is E)
  unsigned int LongM:16;	//     "    (minutes) * 100
  unsigned int DGCount:16;	// Data Gram count
  unsigned int NumVec:16;	// Number of Sample to follow
  char ChBuf[30][3][3];		// 3 byte x 3 x NumVec samples, last sample first
};

struct DGBuf DG;
char *HostName, *HostPort;
char TMPCMND[3][20] =
{  
   "GPSTEMPSET=%i\r\n", 
   "SENSORTEMPSET=%i\r\n", 
   "BOARDTEMPSET=%i\r\n" 
};
  
char LABL[3][34] = 
{ 
   "GPS Temperature Settings",
   "Sensor Temperature Settings", 
   "Electronics Temperature Settings"
};

char TLOG[3][34] = 
{ 
   "GPS%s\r\n[GPS%s%i]\r\n", 
   "Sensor%s\r\n[SENSOR%s%i]\r\n",
   "Electronics%s\r\n[BOARD%s%i]\r\n"
};

char LogStr[512];
int magdes;
FILE *magin, *magout;
FILE *RMDout, *LODout, *hkdout;
int LOG (char *logmsg);
int HKD (int Hr, int Min, int Flag);

static int LMsg ();
static int GMsg ();
static int HMsg ();
static int TMsg ();
static int MMsg ();  // static routine prototypes in main produce compile error.  
                     // Moving outside of main(pcruce 2013-02-06)

void error (char *msg)
{
  perror (msg);
  syslog(LOG_INFO,"GBOMag Shutting Down (error - %s\n",msg);
  exit (0);
}

void RecIntr (int sig)
{
  syssig = sig;
  (void) signal (SIGINT, SIG_DFL);
}

void RecHup (int sig)
{
  syssig = sig;
  (void) signal (SIGHUP, SIG_DFL);
}

void RecTerm (int sig)
{
  syssig = sig;
  (void) signal (SIGTERM, SIG_DFL);
}



int main (int argc, char *argv[])
{
   char strtchar;
   int TimeoutCnt = 0, index, opt;
   char *inival, *inptr;
   time_t timeval;
   struct tm *tm_ptr;
   struct termios ttyUSBtio;
   struct sigaction sa;
   extern int errno; 
   int flags;
   char VerStr[30];
   fpos_t *maginpos;
   int Error, nread;
   
   // -------------------------------------------------------
   // static int LMsg (), GMsg (), HMsg (), TMsg (), MMsg (); 
   // static routine prototypes in main produce compile error.  
   // Moving outside of main(pcruce 2013-02-06)
   // --------------------------------------------------------

   /*********************************
   * Get Major & Minor Version #'s *
   *********************************/
   strncpy(VerStr, "$Revision: 2.9 $", 20);
   sscanf(&VerStr[1], "Revision: %i.%i", &MajVer, &MinVer);

   /******************
   * set up logfile *
   ******************/
   openlog("GBOMag", LOG_PID|LOG_CONS, LOG_USER);
   syslog(LOG_INFO,"GBOMag Version %i.%i started\n",
      SoftwareType, MajVer, MinVer);

   /************************************
   * Get command line options, if any *
   ************************************/
   while ((opt = getopt(argc, argv, "gmstvnVi:")) != -1) 
   {
      switch(opt)
      {
       case 'g':
         GPSVerbose = TRUE;
         break;
       case 'm':
         MagVerbose = TRUE;
         break;
       case 's':
         StatVerbose = TRUE;
         break;
       case 't':
         TempVerbose = TRUE;
         break;
       case 'v':	// verbose mode
         MagVerbose = TRUE;
         GPSVerbose = TRUE;
         StatVerbose = TRUE;
         TempVerbose = TRUE;
         break;
       case 'n':
         NetVerbose = TRUE;
		 break;
       // schofield begin ========================================================
       // 2014/05/01 - more descriptive help and version messages
       case 'V': 	// Version Query
         printf("GBOMag -- command line data logger for THEMIS GMAG Magnetometers\n");
		   printf("Version %s-%i.%i\n", SoftwareType, MajVer, MinVer);
		   printf("UCLA Earth Planetary and Space Sciences 2004-2014\n");
		   printf("Don Dearborn, Patrick Cruce, Kathryn Rowe, Ian Schofield\n");
         syslog(LOG_INFO,"GBOMag Shutting Down (Version Request Only)\n");
         exit(0);
       case 'i':	// "ini" file is not default
         strncpy (inifilenam, optarg, 254);
         break;
       case ':':	// missing "ini" file name
         printf ("-i option needs a value\n");
         break;
       case '?':	// List options
		   printf("GBOMag -- usage: GBOMag -i gbo.ini\n");
		   printf("sudo GBOMag <switches>\n");
		   printf("\t-V\t\t version\n");
		   printf("\t-v\t\t full verbose\n");
		   printf("\t-g\t\t verbose GPS messages\n");
		   printf("\t-m\t\t verbose Magnetometer messages\n");
		   printf("\t-s\t\t verbose Stats messages\n");
		   printf("\t-t\t\t verbose Temperature messages\n");
		   printf("\t-i <ini file>\t include ini file\n");
         exit(0);
        // schofield end ======================================================== 
      }
   }

   // Set umask to 0000 so file permissions can be set during file creation
   // Added by William Tam
   umask(0000);
   
   /***************************
   * Set up signal handler(s)*
   ***************************/
   syssig = 0;
   (void) signal (SIGINT, RecIntr);
   (void) signal (SIGHUP, RecHup);
   (void) signal (SIGTERM, RecTerm);
   
   /***************************************************
   * Initialize UTC parameters so file names will    *
   * be correct (probably) when created before first *
   * GPS message arrives.                            *
   ***************************************************/
   (void) time (&timeval);
   tm_ptr = gmtime (&timeval);
   gpsyr = tm_ptr->tm_year + 1900;
   gpsdy = tm_ptr->tm_mday;
   gpsmo = tm_ptr->tm_mon + 1;
   gpshr = tm_ptr->tm_hour;
   
   /**********************
   * Read in "ini" file *
   **********************/
   GtIni (inifilenam);
  
   /*******************************
   * Open communication with MAG *
   *******************************/
  
   magdes = open (MagDevice, O_RDWR | O_NOCTTY);

   if (magdes == -1)
   {
      magin = NULL;
      magout = NULL;
      
      printf("%02d/%02d/%4d %02d:%02d:%02d Unable to open mag device %s\n",gpsmo, gpsdy, gpsyr, gpshr, tm_ptr->tm_min, tm_ptr->tm_sec, MagDevice);
      exit(-1);
   }
   else
   {
      magin = fdopen (magdes, "r");
      magout = fdopen (magdes, "w");
      ttyUSBtio.c_iflag = IGNPAR;
      ttyUSBtio.c_oflag = 0;
      ttyUSBtio.c_lflag = 0;
      ttyUSBtio.c_cc[VMIN] = 0;
      ttyUSBtio.c_cc[VTIME] = 50;
      ttyUSBtio.c_cflag = BAUDRATE | CRTSCTS | CS8 | CSTOPB | CLOCAL | CREAD;
      tcflush (magdes, TCIFLUSH);
      tcsetattr (magdes, TCSANOW, &ttyUSBtio);
      fprintf (magout, "GPSPORT=1\r\n");
      fprintf (magout, "GPSON=1\r\n");
      passbuf[0] = '\0';
   }
   FILE* fp;
   fp = fopen("/home/GMAG/checkusb.txt", "a");
   while (TRUE)
   {
      if (syssig != 0)
      {
          syslog(LOG_INFO,"GBOMag shutting down (sig = %d)\n",syssig);
          exit (0);
      }

      if ((magdes == -1) || (incnt = read (magdes, inbuf, 254)) <= 0)
      {
         sprintf (LogStr, "USB read time out - USB closed & reopened\r\n");
         if (TimeoutCnt++ == 5)
         {
            DG.NumVec = -1;
            for (index = 0; index < NumHosts; index++)
            {
               n = sendto (Sock, &DG, sizeof(struct DGBuf), 0,
                  (struct sockaddr *) &Server[index], SockLngth);
               if (n < 0)
                  error ("Sendto");
            }
            DG.NumVec++;
            DG.DGCount++;
         }
         
         if (errno = LOG (LogStr))
            printf ("%02d/%02d/%4d %02d:%02d:%02d Log Error = %i\n", gpsmo, gpsdy, gpsyr, gpshr, tm_ptr->tm_min, tm_ptr->tm_sec, errno);
         if (magin != NULL)
            fclose (magin);
         if (magout != NULL)
            fclose (magout);
         if (magdes != -1)
            close (magdes);
            
         sleep (1);
         
         if ((magdes = open (MagDevice, O_RDWR | O_NOCTTY)) > 0)
         {
            magin = fdopen (magdes, "r");
            magout = fdopen (magdes, "w");
            ttyUSBtio.c_iflag = IGNPAR;
            ttyUSBtio.c_oflag = 0;
            ttyUSBtio.c_lflag = 0;
            ttyUSBtio.c_cc[VMIN] = 0;
            ttyUSBtio.c_cc[VTIME] = 50;
            ttyUSBtio.c_cflag =
               BAUDRATE | CRTSCTS | CS8 | CSTOPB | CLOCAL | CREAD;
            tcflush (magdes, TCIFLUSH);
            tcsetattr (magdes, TCSANOW, &ttyUSBtio);
            fprintf (magout, "GPSPORT=1\r\n");
            fprintf (magout, "GPSON=1\r\n");
            passbuf[0] = '\0';
            ioctl(magdes, TIOCMGET, &flags);	// enable RTS
            flags |= TIOCM_RTS;
            ioctl(magdes, TIOCMSET, &flags);
         }
         else
         {
            printf ("%02d/%02d/%4d %02d:%02d:%02d errno = %i\n",gpsmo, gpsdy, gpsyr, gpshr, tm_ptr->tm_min, tm_ptr->tm_sec, errno);
            magin = NULL;
            magout = NULL;
         }
      }
      else
      {
         TimeoutCnt = 0;
         
         // put end of string marker on end of input
         inbuf[incnt] = '\0';
         fputs(inbuf, fp);
         // add input to current holding buffer   
         strncat (passbuf, inbuf, 255);
         
         // look for next end of line
         while ((inptr = strpbrk ((char *) passbuf, "\n")) != NULL)
         {
            //and replace with end of string
            *inptr = '\0';
            
            //copy line to buffer
            strncpy (buffer, passbuf, 255);
         
            // and delete it from holding buffer
            strncpy (passbuf, inptr + 1, 599);
         
            //parse 1st char to determine msg type
            nread = sscanf (&buffer[0], "%c", &strtchar);

            switch (strtchar)
            {
               case 'T':
                  if (!TMsg ())
                     break;
                  else
                  {
                     printf ("%02d/%02d/%4d %02d:%02d:%02d Bad 'T' Message \n", gpsmo, gpsdy, gpsyr, gpshr, tm_ptr->tm_min, tm_ptr->tm_sec);
                     break;
                  }
               case 'G':
                  if (!GMsg ())
                     break;
                  else
                  {
                     printf ("%02d/%02d/%4d %02d:%02d:%02d Bad 'G' Message \n", gpsmo, gpsdy, gpsyr, gpshr, tm_ptr->tm_min, tm_ptr->tm_sec);
                     break;
                  }
               case 'L':
                  if (!LMsg ())
                     break;
                  else
                  {
                     printf ("%02d/%02d/%4d %02d:%02d:%02d Bad 'L' Message \n", gpsmo, gpsdy, gpsyr, gpshr, tm_ptr->tm_min, tm_ptr->tm_sec);
                     break;
                  }
               case 'M':
                  if (!MMsg ())
                     break;
                  else
                  {
                     printf ("%02d/%02d/%4d %02d:%02d:%02d Bad 'M' Message \n", gpsmo, gpsdy, gpsyr, gpshr, tm_ptr->tm_min, tm_ptr->tm_sec);
                     break;
                  }
               case 'H':
                  if (!HMsg ())
                     break;
                  else
                  {
                     printf ("%02d/%02d/%4d %02d:%02d:%02d Bad 'H' Message \n", gpsmo, gpsdy, gpsyr, gpshr, tm_ptr->tm_min, tm_ptr->tm_sec);
                     break;
                  }
               default:
                  printf ("%02d/%02d/%4d %02d:%02d:%02d Unknown Message \n", gpsmo, gpsdy, gpsyr, gpshr, tm_ptr->tm_min, tm_ptr->tm_sec);
            }
         }
      }
   } // end of while(TRUE)
   
   // exit signal raised, shutdown the program
   fclose (fp);
   fclose (LODout);
   fclose (hkdout);
   fclose (RMDout);
   fclose (magin);
   fclose (magout);
   Ini_FreeIniFile (&my_ini_file);
   syslog(LOG_INFO,"GBOMag shutting down (No More Truth)\n");
   
   exit (0);
}


void IniErr (char *msg)
{
  fprintf (stderr, "Ini File Error : %s\n", msg);
  syslog(LOG_INFO,"GBOMag Shutting Down (Ini File Error : %s)\n",msg);
  exit - 1;
}


// Read and process INI configuration file
int GtIni (char *inifilenam)
{
   static char *EndPtr = NULL, *StrIniParm;
   long LLong;
   ulong ipaddr;
   int index, ipadr1, ipadr2, ipadr3, ipadr4;
   char HN[12] = "Host Name x";
   char HP[12] = "Host Port x";
   char myname[256], dmy;
   struct hostent *hostinfo;


   if (!(Ini_ReadIniFile (inifilenam, &my_ini_file)))
      IniErr ("Initial Read");

   /**************************************
   * Set up (up to 3) DataGram clients. *
   **************************************/
   Sock = socket (AF_INET, SOCK_DGRAM, 0);
   for (index = 65; index < 90 && NumHosts < 3; index++)	//Check for Host A-Z
   {
      HN[10] = index;
      HP[10] = index;
      HostName = Ini_GetValue (&my_ini_file, "General", HN);
      HostPort = Ini_GetValue (&my_ini_file, "General", HP);

      if (HostName && HostPort)
      {
         Host = gethostbyname (HostName);
         bcopy ((char *) Host->h_addr,
               (char *) &Server[NumHosts].sin_addr, Host->h_length);
         Server[NumHosts].sin_family = AF_INET;
         Server[NumHosts].sin_port = htons (atoi (HostPort));
         NumHosts++;
      }
   }
   NumHosts--;
   SockLngth = sizeof (struct sockaddr_in);
  
   // --------------------------------------------------------------------------
   // Following added for heartbeat generator (krowe 2013-6-13) 
   // read ini file for HB info
   if (!(HBAddress = Ini_GetValue (&my_ini_file, "General", "HeartBeat Name")))
      IniErr ("HB Name");

   // create socket
   if ((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))==-1)
      error ("socket");
      
   // set up client socket
   memset((char *) &client, 0, sizeof(client));
   client.sin_family = AF_INET;
   client.sin_port = htons(HBPort);
   client.sin_addr.s_addr = htonl(INADDR_ANY);
   if (bind(s, (struct sockaddr *) &client, sizeof(client)) <0)
 	   error ("bind");
      
   //set up server socket
   memset((char *) &server, 0, sizeof(server));
   server.sin_family = AF_INET;
   server.sin_port = htons(HBPort);
  
   //convert IP into binary form.  Exites program if unable to convert
   if (inet_aton(HBAddress, &server.sin_addr)==0)
      error ("inet_aton");
   // krowe end ----------------------------------------------------------------
		  
   // schofield begin ==========================================================
   // Following added for AJAX data display (schofield 2014/04/10)
	AXsock = socket(AF_INET, SOCK_DGRAM, 0);  // create UDP socket
	if(AXsock < 0) 
      error("socket");
	AXserver.sin_family = AF_INET;
	AXhp = gethostbyname(DATADISPLAYSVR);
	if(AXhp == 0) 
      error("socket");
	bcopy((char *)AXhp->h_addr,(char *)&AXserver.sin_addr,AXhp->h_length);
	AXserver.sin_port = htons(DATADISPLAYPORT);
   // schofield end ============================================================
		  
        
   /*****************************
   * Get other 'General' stuff *
   *****************************/
   if (!(MagDevice = Ini_GetValue (&my_ini_file, "General", "Com Port")))
      IniErr ("Com Port");
      
   if (!(SiteNam = Ini_GetValue (&my_ini_file, "General", "Site Name")))
      IniErr ("Site Name");
      
   memcpy (DG.SitNam, SiteNam, 4);

   char* projectName;
   if (!(projectName = Ini_GetValue (&my_ini_file, "General", "Project Name")))
      IniErr ("Project Name");
   
   memcpy (DG.ProjectName, projectName, 4 * sizeof(char));
   free(projectName);
  
   /**********************************
   * Get Mag & DAC scale parameters *
   **********************************/
   if(!(StrIniParm = Ini_GetValue (&my_ini_file, "Calibration", "Mag1 Scale")))
      IniErr ("Mag1 Scale");
   MagScal1 = strtod (StrIniParm, &EndPtr);
   DG.MagScal1 = MagScal1 * 100 + 0.5;
   
   if(!(StrIniParm = Ini_GetValue (&my_ini_file, "Calibration", "Mag2 Scale")))
      IniErr ("Mag2 Scale");
   MagScal2 = strtod (StrIniParm, &EndPtr);
   DG.MagScal2 = MagScal2 * 100 + 0.5;
   
   if(!(StrIniParm = Ini_GetValue (&my_ini_file, "Calibration", "Mag3 Scale")))
      IniErr ("Mag3 Scale");
   MagScal3 = strtod (StrIniParm, &EndPtr);
   DG.MagScal3 = MagScal3 * 100 + 0.5;
   
   if (!(StrIniParm = Ini_GetValue (&my_ini_file, "Calibration", "DAC1 Scale")))
      IniErr ("DAC1 Scale");
   DacScal1 = strtod (StrIniParm, &EndPtr);
   DG.DacScal1 = DacScal1 * 10 + 0.5;
   
   if (!(StrIniParm = Ini_GetValue (&my_ini_file, "Calibration", "DAC2 Scale")))
      IniErr ("DAC2 Scale");
   DacScal2 = strtod (StrIniParm, &EndPtr);
   DG.DacScal2 = DacScal2 * 10 + 0.5;
   
   if (!(StrIniParm = Ini_GetValue (&my_ini_file, "Calibration", "DAC3 Scale")))
      IniErr ("DAC3 Scale");
   DacScal3 = strtod (StrIniParm, &EndPtr);
   DG.DacScal3 = DacScal3 * 10 + 0.5;
  
   // ===========================================================
   // Automatic DAC auto-ranging parameter. 
   // This needs to be set to TRUE in gbo.ini (Ian S. 2014/04/10)
   // ===========================================================
   if (!(AutoDacC = Ini_GetValue (&my_ini_file, "DAC Values", "Automatic")))
      IniErr ("Automatic");
   AutoDacB = (!strcmp (AutoDacC, "TRUE"));
   
   if (!(DACV1 = Ini_GetValue (&my_ini_file, "DAC Values", "DAC1")))
      IniErr ("DAC1");
      
   if (!(DACV2 = Ini_GetValue (&my_ini_file, "DAC Values", "DAC2")))
      IniErr ("DAC2");
      
   if (!(DACV3 = Ini_GetValue (&my_ini_file, "DAC Values", "DAC3")))
      IniErr ("DAC3");
    
    
   /***********************
   * Get GPS parameters. *
   ***********************/
   DG.DelT = NomDeltaT;
   if (!(GPSTolerance = Ini_GetValue (&my_ini_file, "GPS", "Time Tolerance")))
      IniErr ("Time Tolerance");
      
   if (!(StrIniParm = Ini_GetValue (&my_ini_file, "GPS", "Latitude")))
      IniErr ("Latitude");
   Latitude = strtod (StrIniParm, &EndPtr);
   DG.LatD = Latitude;
   DG.LatM = (Latitude - DG.LatD) * 6000.0 + 0.5;
   DG.LatD += 90;
   
   if(!(StrIniParm = Ini_GetValue (&my_ini_file, "GPS", "Longitude")))
      IniErr ("Longitude");
   Longitude = strtod (StrIniParm, &EndPtr);
   LLong = Longitude;
   DG.LongM = labs ((Longitude - LLong) * 6000.0 + 0.5);
   DG.LongD = LLong + 180;
   
   
   /********************************
   * Get File Path's. *
   ********************************/
   if(!(GBOFileRoot = Ini_GetValue (&my_ini_file,"File Path Structure","File Path Root")))
      IniErr ("File Path");
   strncpy (RecPath, GBOFileRoot, 240);
   strncpy (RecPathH, GBOFileRoot, 240);
   strncpy (RecPathL, GBOFileRoot, 240); 
   strncat (RecPath, "/recent", 7);
   strncat (RecPathH, "/recenth", 8);
   strncat (RecPathL, "/recentl", 8);
   
   
   /*******************************
   * Get Sigma-Delta parameters. *
   *******************************/
   if (!(Phase = Ini_GetValue (&my_ini_file, "SIGMADELTA", "Phase")))
      IniErr ("Phase");
      
   if (!(Drvlat = Ini_GetValue (&my_ini_file, "SIGMADELTA", "Drive Latch")))
      IniErr ("Drive Latch");
      
   if (!(Datlat = Ini_GetValue (&my_ini_file, "SIGMADELTA", "Data Latch")))
      IniErr ("Data Latch");
      
   if (!(StrIniParm = Ini_GetValue (&my_ini_file, "SIGMADELTA", "Polarity")))
      IniErr ("Polarity");
   PolarityB = (!strcmp (StrIniParm, "TRUE"));
   
   if (!(StrIniParm = Ini_GetValue (&my_ini_file, "SIGMADELTA", "SawtoothEnable")))
      IniErr ("SawtoothEnable");
   SawtoothEnableB = (!strcmp (StrIniParm, "TRUE"));
   
   if (!(Decimation = Ini_GetValue (&my_ini_file, "SIGMADELTA", "Decimation")))
      IniErr ("Decimation");
      
   if (!(FeedBackEnable=Ini_GetValue (&my_ini_file, "SIGMADELTA", "FeedBackEnable")))
    IniErr ("FeedBackEnable");
    
    
   /*******************************************
   * Get Temperature PID control parameters. *
   *******************************************/
   for (index = 0; index < 3; ++index)
   {
      if(!(StrIniParm = Ini_GetValue (&my_ini_file, LABL[index], "Setpoint")))
         IniErr ("Setpoint");
      sscanf (StrIniParm, "%lf", &set[index]);
      
      if(!(StrIniParm = Ini_GetValue (&my_ini_file, LABL[index], "Kc")))
         IniErr ("Kc");
      sscanf (StrIniParm, "%lf", &Kc[index]);
      
      if(!(StrIniParm = Ini_GetValue (&my_ini_file, LABL[index], "Ti")))
         IniErr ("Ti");
      sscanf (StrIniParm, "%lf", &Ti[index]);
      
      if(!(StrIniParm = Ini_GetValue (&my_ini_file, LABL[index], "Td")))
         IniErr ("Td");
      sscanf (StrIniParm, "%lf", &Td[index]);
      
      if(!(StrIniParm = Ini_GetValue (&my_ini_file, LABL[index], "Delta T")))
         IniErr ("Delta T");
      sscanf (StrIniParm, "%lf", &DelT[index]);
      
      if(!(StrIniParm = Ini_GetValue (&my_ini_file, LABL[index], "B")))
         IniErr ("B");
      sscanf (StrIniParm, "%lf", &B[index]);
      
      if(!(StrIniParm = Ini_GetValue (&my_ini_file, LABL[index], "K1")))
         IniErr ("K1");
      sscanf (StrIniParm, "%lf", &K1[index]);
      
      if(!(StrIniParm = Ini_GetValue (&my_ini_file, LABL[index], "K2")))
         IniErr ("K2");
      sscanf (StrIniParm, "%lf", &K2[index]);
      
      if(!(StrIniParm = Ini_GetValue (&my_ini_file, LABL[index], "K3")))
         IniErr ("K3");
      sscanf (StrIniParm, "%lf", &K3[index]);
      
      if(!(StrIniParm = Ini_GetValue (&my_ini_file, LABL[index], "K4")))
         IniErr ("K4");
      sscanf (StrIniParm, "%lf", &K4[index]);
      
      if(!(StrIniParm = Ini_GetValue (&my_ini_file, LABL[index], "K5")))
         IniErr ("K5");
      sscanf (StrIniParm, "%lf", &K5[index]);
   }   
}


// Temperature data packet
// data display packet format: 
// TMSG:GTEMP,%f,STEMP,%f,ETEMP,%f
int TMsg ()
{
	double pow (double x, double y);
	int nread;
   static double Iout[3], lastptmp[3], laste[3];
   static int lastcntl[3] = { -99, -99, -99 }, lastMin;
   double pset, ptmp, pcntl, deriv, Integ;
   double trap, Up, Ud, e, DTemp;
   int cntl, index;
  
  
   /****************************************************
   * First change input temperature values to degrees * 
   * celcius using formula in Mag documentation.      *
   ****************************************************/
   nread = sscanf (&buffer[1], "%4x%4x%4x ", &itmp[0], &itmp[1], &itmp[2]);
   if (nread == 3)
   {
      for (index = 0; index <= 2; ++index)
      {
         DTemp = (itmp[index] * 5.0) / 65536.0;
         tmp[index] = B[index] + K1[index] * DTemp +
         K2[index] * pow (DTemp, 2.0) +
         K3[index] * pow (DTemp, 3.0) +
         K4[index] * pow (DTemp, 4.0) + K5[index] * pow (DTemp, 5.0);
      }	//endfor (index = 0; index <= 2; ++index)
      
      // schofield begin =======================================================
      // added by ian schofield - network data display - 2014/04/10
      
      sprintf(AXbuffer,"TMSG:GTEMP,%f,STEMP,%f,ETEMP,%f",tmp[0],tmp[1],tmp[2]);
      
      /* printf("gitmp %d sitmp %d eitmp %d \n", itmp[0], itmp[1], itmp[2]); */
      if (TempVerbose)
      {
         printf ("%s\n", AXbuffer);
      }
      
      if (NetVerbose) 
      {   
         // this packet has been disabled as GMAG temperature is
         //   also reported in the housekeeping message (HMSG)
            
         // n = sendto(AXsock, AXbuffer, strlen(AXbuffer), 0,
			//	       (const struct sockaddr *)&AXserver,
         //        sizeof(struct sockaddr_in));
         // if(n < 0) 
         //    error ("sendto");	 
         
      }         
      // schofield end =========================================================
      
      /**************************************************************
      * next calculate heater control parameter using PID algorithm *
      ***************************************************************/
      for (index = 0; index <= 2; ++index)
      {
         if ((set[index] != -99.0) && INIT)
         {
            pset = (set[index] + 30.0) * 100.0 / 90.0;
            ptmp = (tmp[index] + 30.0) * 100.0 / 90.0;
            e = pset - ptmp;
            
            if (lastptmp[index] == 0.0)
               Ud = 0.0;
            else
               Ud = (lastptmp[index] - ptmp) * Kc[index] * Td[index] / DelT[index];
            
            lastptmp[index] = ptmp;
            Up = Kc[index] * e;
            
            if (laste[index] == 0)
               trap = e * DelT[index];
            else
               trap = (e + laste[index]) * DelT[index] / 2.0;
            
            laste[index] = e;
            Integ = (trap * Kc[index] / Ti[index]) + Iout[index];
            Iout[index] = Integ + Up;
            
            if (Iout[index] > 100.0)
               Iout[index] = 100.0;
            
            if (Iout[index] < -100.0)
               Iout[index] = -100.0;
            
            Iout[index] = Iout[index] - Up;
            pcntl = Integ + Up + Ud;
            
            if (pcntl > 100.0)
               pcntl = 100.0;
            if (pcntl < -100.0)
               pcntl = -100.0;
            
            cntl = ((pcntl + 100.0) * 254.0 / 200.0) + 0.5;
            
            /*printf("%9.4f %9.4f %9.4f %9.4f %9.4f %9.4f %9.4f %9.4f %9.4f %4i\n",
               pset, ptmp, e, Ud, Up, trap, Integ, Iout, pcntl, cntl); */
            /*printf ("index = %i Temp = %9.4f Setpoint = %9.4f Cntl = %4i\n",
               index, tmp[index], set[index], cntl); */
            
            /**************************************************
            * Put out temperature control parameter at least *
            * once a minute or sooner if there is a change.  *
            * Also, if (when) the heater is turned off,      * 
            * record this in the log.                        *
            **************************************************/
            if ((cntl != lastcntl[index]) || (lastMin != Min))
            {
               if ((lastcntl[index] != 0) && (cntl == 0))
               {
                  sprintf (LogStr, &TLOG[index][0],
                     " Temperature is above setpoint; heater turned off:","TEMPSET=", cntl);
                  if (errno = LOG (LogStr))
                     printf ("Log Error = %i\n", errno);
               }
               
               lastcntl[index] = cntl;
               lastMin = Min;
               fprintf (magout, TMPCMND[index], cntl);
               /*printf (TMPCMND[index], cntl); */
            }
         }
      }
      return (0);
   }
   else
   {
      return (1);
   }
}


// High speed data acquisition (message)
int HMsg ()
{
   int nread, Ch1, Ch2, Ch3, Dac1, Dac2, Dac3;
   
   nread = sscanf (&buffer[1], "%6x%6x%6x%2x%2x%2x",
      &Ch3, &Ch2, &Ch1, &Dac3, &Dac2, &Dac1);
   if (nread == 6)
   {
         printf ("hi rate - %i7%i7%i7%i4%i4%i4\n",
            Ch1, Ch2, Ch3, Dac1, Dac2, Dac3);
   }
   else
   {
      return (1);
   }
}


// Low speed data acquisition (2 Hz)
int LMsg ()
{
   static int LastHr = -99, LastVecCnt;
   static int FileHr = -99, BufHr = -99, FileDy = -99, BufDy = -99;
   static int BufMo, BufYr, BufMi, BufSec, BufMSec, BufDac1;
   static int BufDac2, BufDac3, DGBuffer[31][3];
   static int Buf[11][4], NinBuf = 0, LastMin;
   static char RMDFileNamPart[256], RMDFileNamFull[512], BufTime[256];
   static char RMDFileNamTest[256];
   int GPSFlg, SOD, MSec, NSamp, FilCnt;
   int nread, Errno, DeltaT, index, index2;
   char Date[128];
   static long NewT, LastT, MSOD, DGBufMSOD, LastDGBMSOD;
   static int FrstTime = 1, DGBMod = FALSE;
   int inCh1, inCh2, inCh3, inDac1, inDac2, inDac3;
   static int LastDac1, LastDac2, LastDac3;
   int n;

   // Added by William Tam
   mode_t file_permissions = S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
   
   if (INIT)
   {
      nread = sscanf (&buffer[1], "%6x%6x%6x%2x%2x%2x%2x%6x%4x%2x ", 
              &inCh3, &inCh2, &inCh1, &inDac3, &inDac2, &inDac1, &GPSFlg,
              &SOD, &MSec, &NSamp);
              
      if (nread == 10)
	   {
         if (SOD == 0)
         {
            MSOD += 500;
            if (MSOD >= 86400000)
               MSOD -= 86400000;
	         SOD = MSOD / 1000;
         }
         
         Hr = SOD / 3600;
         Min = (SOD - Hr * 3600) / 60;
         Sec = (SOD - Hr * 3600 - Min * 60);
         NewT = Min * 60000 + Sec * 1000 + MSec;
         MSOD = NewT + Hr * 3600000;
      
         /*printf("MSOD = %d\n",MSOD); */
         DeltaT = NewT - LastT;
         LastT = NewT;
         
         if (Min != LastMin)
         {
            HKD (Hr, Min, gpsstat);
            LastMin = Min;
         }
         
         Ch1 = (((inCh1 * 256) / NSamp) + 8) / 16;
         Ch2 = (((inCh2 * 256) / NSamp) + 8) / 16;
         Ch3 = (((inCh3 * 256) / NSamp) + 8) / 16;
         Dac1 = inDac1;
         Dac2 = inDac2;
         Dac3 = inDac3;
 
         Nt1 = (Ch1 - 524288.0) / MagScal1 - (Dac1 - 128) * DacScal1;
         Nt2 = (Ch2 - 524288.0) / MagScal2 - (Dac2 - 128) * DacScal2;
         Nt3 = (Ch3 - 524288.0) / MagScal3 - (Dac3 - 128) * DacScal3;

         // schofield Begin ====================================================
         // added by ian schofield - network data display - 2014/04/10

         sprintf(AXbuffer,"LMSG:CH1,%10f,CH2,%10f,CH3,%10f,FG,%02d,NS,%02d,DAC1,%02d,DAC2,%02d,DAC3,%02d",
            Nt1, Nt2, Nt3, GPSFlg, NSamp, Dac1, Dac2, Dac3);
                               
         if ( MagVerbose ) 
         {
            printf("%s\n",AXbuffer);
         }
          
         if ( NetVerbose) // schofield - 2014/04/10
         {   
            n = sendto(AXsock, AXbuffer, strlen(AXbuffer), 0,
				       (const struct sockaddr *)&AXserver,
                   sizeof(struct sockaddr_in));
            if(n < 0){ 
               error ("sendto");	   
	    }
         }         
         // schofield end ======================================================
         
         if (NSamp != 50)
         {
            if (MagVerbose)
               printf("Sampling Sync Event Occurred - %d Samples in data\r\n", NSamp);
	      
            sprintf (LogStr,"Sampling Sync Event Occurred - %d Samples in data\r\n", NSamp);
            
	         if (errno = LOG (LogStr))
		         printf ("Log Error = %i\n", errno);
         }
         if (GPSFlg & 0x80)
         {
            if (GPSFlg & 0x20)
            {
               if (MagVerbose)
                  printf("GPS Sync Event Occurred - Mag Time Base Slow\r\n");
                  
		         sprintf (LogStr,"GPS Sync Event Occurred - Mag Time Base Slow\r\n");
            }
	         else
            {
               if (MagVerbose)
                  printf("GPS Sync Event Occurred - Mag Time Base Fast\r\n");
		      
               sprintf (LogStr,"GPS Sync Event Occurred - Mag Time Base Fast\r\n");
            }
            if (errno = LOG (LogStr))
               printf ("Log Error = %i\n", errno);
         }
         
         /********************************************************************
         * Before we add vector to RMD File buffer, see if we need to flush *
         * the buffer. We will need to do that if: the hour has changed,    *
         * or there are already 10 vectors in the buffer, or the time       *
         * since the last vector is not ~500mSecs, or the Dac values have   *
         * changes, but only if there are vectors in the buffer to flush.   *
         ********************************************************************/
         if (NinBuf && ((Hr != LastHr) || (DeltaT > 501) ||
            (DeltaT < 499) || (NinBuf > 9) || (Dac1 != LastDac1) ||
            (Dac2 != LastDac2) || (Dac3 != LastDac3)) )
         {
            /***********************************************************************
            * Before we can flush the RMD File buffer, we have to see if there is *
            * a file open and if it is of the same day & hour of the buffer.      *
            * If it is, we can just add the group of vectors to the file.         *
            * Otherwise, we have to close the current output file and open a      *
            * new one.                                                            *
            ***********************************************************************/
            if (RMDOpen && ((BufHr != FileHr) || (BufDy != FileDy)))
            {
               RMDout = fopen (RMDFileNamFull, "a");
               fprintf (RMDout, " EOF");
               fclose (RMDout);
               RMDOpen = FALSE;
               sprintf (LogStr,"Data File Closed at Hour Change Vector Count=%i\r\n",
                  LastVecCnt);
               if (errno = LOG (LogStr))
                  printf ("Log Error = %i\n", errno);
            }  
            if (!RMDOpen)
            {	
               // no file open, set up for new file
               FileHr = BufHr;
               FileDy = BufDy;
               
               /********************************
               * Construct new RMD File Name  *
               ********************************/
               strncpy (RMDFilePath, GBOFileRoot, 240); //First construct directory path 
               sprintf (Date, "/%04i", BufYr); // year directory
               strncat (RMDFilePath, Date, 5);
               mkdir (RMDFilePath, file_permissions);		  // create directory, if needed
               sprintf (Date, "/%02i", BufMo); // month directory
               strncat (RMDFilePath, Date, 3);
               mkdir (RMDFilePath, file_permissions);		  // create directory, if needed
               sprintf (Date, "/%02i", BufDy); // day directory
               strncat (RMDFilePath, Date, 3);
               mkdir (RMDFilePath, file_permissions);		  // create directory, if needed
               strncpy (RMDFileNamPart, SiteNam, 5);	//Now, the file name, start with the site name
               sprintf (Date, "%02i%02i%02i_MAG_%02i_", BufYr % 100,	//then add date & hour
      	         BufMo, BufDy, BufHr);
               strncat (RMDFileNamPart, Date, 14);
               strncpy (RMDFileNamTest, RMDFileNamPart, 19);	// make copy of what we have so far
               for (FilCnt = 0; FilCnt < 99; FilCnt++)	// iterate  FilID until we have new file
               {
	               sprintf (FilID, "%02i.RMD", FilCnt);
                  strncpy (RMDFileNamFull, RMDFilePath, 240);
                  strncat (RMDFileNamFull, "/", 1);
                  strncat (RMDFileNamFull, RMDFileNamTest, 18);
                  strncat (RMDFileNamFull, FilID, 6);
                  if (!fopen (RMDFileNamFull, "r"))
                     break;	//quit when we have a new file
               }
               if (FilCnt == 99)
                  return (2);	//too many files; somethings wrong!
                  
               /************************************
               * Open RMD File and write Preamble *
               ************************************/
               sprintf (LogStr,
                  "Data File Opened at Hour Change %s%s\r\n",
               RMDFileNamTest, FilID);
               if (errno = LOG (LogStr))
                  printf ("Log Error = %i\n", errno);
               RMDout = fopen (RMDFileNamFull, "w");
               unlink (RecPath);
               symlink (RMDFileNamFull, RecPath);
               RMDOpen = TRUE;
               fprintf (RMDout, "THEM%s%s", RMDFileNamTest, FilID);
               fwrite ((char *) &NomDeltaT + 3, 1, 1, RMDout);
               fwrite ((char *) &NomDeltaT + 2, 1, 1, RMDout);
               fwrite ((char *) &NomDeltaT + 1, 1, 1, RMDout);
               fwrite ((char *) &NomDeltaT + 0, 1, 1, RMDout);
               fclose (RMDout);
            } //endif (!RMDOpen)
            
            /*************************************
            * Write RMD File Buffer to RMD File *
            *************************************/
            RMDout = fopen (RMDFileNamFull, "a");
            fprintf (RMDout, "%s", BufTime);
            fwrite ((char *) &BufDac1, 1, 1, RMDout);
            fwrite ((char *) &BufDac2, 1, 1, RMDout);
            fwrite ((char *) &BufDac3, 1, 1, RMDout);
            fwrite ((char *) &NinBuf + 3, 1, 1, RMDout);
            fwrite ((char *) &NinBuf + 2, 1, 1, RMDout);
            fwrite ((char *) &NinBuf + 1, 1, 1, RMDout);
            fwrite ((char *) &NinBuf + 0, 1, 1, RMDout);
            for (index = 1; index <= NinBuf; index++)
            {
               for (index2 = 1; index2 <= 3; index2++)
               {
                  fwrite ((char *) &Buf[index][index2] + 3, 1, 1, RMDout);
                  fwrite ((char *) &Buf[index][index2] + 2, 1, 1, RMDout);
                  fwrite ((char *) &Buf[index][index2] + 1, 1, 1, RMDout);
                  fwrite ((char *) &Buf[index][index2] + 0, 1, 1, RMDout);
                  /*printf ("index = %i index2 = %i", index, index2); */
               }	//endfor (index2 = 1; index2 <= 3; index2++)
            }		//endfor (index = 1; index <= NinBuf; index++)
            LastVecCnt = NinBuf;
            NinBuf = 0;
            fclose (RMDout);
         }	//endif (NinBuf && ((Hr != LastHr)
         
         /***********************************
         * Put new data in RMD File buffer *
         ***********************************/
         Buf[++NinBuf][1] = Ch1;
         Buf[NinBuf][2] = Ch2;
         Buf[NinBuf][3] = Ch3;
         LastHr = Hr;
         if (NinBuf == 1)
         {
	         BufDy = gpsdy;
            BufMo = gpsmo;
            BufYr = gpsyr;
            BufHr = Hr;
            BufMi = Min;
            BufSec = Sec;
            BufMSec = MSec;
            BufDac1 = Dac1;
            BufDac2 = Dac2;
            BufDac3 = Dac3;
            sprintf (BufTime, "%02i%02i%04i-%02i:%02i:%02i-%03i",
               BufMo, BufDy, BufYr, BufHr, BufMi, BufSec, BufMSec);
         } //endif (NinBuf == 1)
         
         /********************************************************************
         * Before we add vector to DataGram buffer, see if we need to flush *
         * the buffer. We will need to do that if: there is a data gap, i.e.*
         * the delta t between the last vector added and the new one is not *
         * ~500mSecs or the Dac values have changes,but, only if there have *
         * been new vectors added to the buffer since the last DataGram.    *
         ********************************************************************/
         if ((((MSOD - DG.MSOD) < 498) || ((MSOD - DG.MSOD) > 501)
            || (Dac1 != LastDac1) || (Dac2 != LastDac2)
            || (Dac3 != LastDac3)) && DGBMod)
         {
            /***********************************	    
            * pack the data into 3 byte words *
            ***********************************/
            for (index2 = 0; index2 < DG.NumVec; index2++)
            {
               for (index = 0; index < 3; index++)
               {
                  memcpy (&DG.ChBuf[index2][index][0],
                     &DGBuffer[index2][index], 3);
               }
            }
            /*****************************		
            * and transmit the DataGram *
            *****************************/
            for (index = 0; index <= NumHosts; index++)
            {
               n = sendto (Sock, &DG, sizeof(struct DGBuf), 0,
               (struct sockaddr *) &Server[index], SockLngth);
               if (n < 0)
                  error ("Sendto");
            }
            DG.DGCount++;
            DG.NumVec = 0;
            LastDGBMSOD = DG.MSOD;
            DGBMod = FALSE;
         }
         
         /******************************************
         * Add the input vector to the DGBuffer,  *
         * after shifting any existing data down. *
         ******************************************/
         if (DG.NumVec)
            memmove (&DGBuffer[1], &DGBuffer, DG.NumVec * 12);
         if (++DG.NumVec > 30)
            DG.NumVec = 30;
         DGBuffer[0][0] = Ch1;
         DGBuffer[0][1] = Ch2;
         DGBuffer[0][2] = Ch3;
         DG.Dac1 = Dac1;
         DG.Dac2 = Dac2;
         DG.Dac3 = Dac3;
         DG.Year = gpsyr;
         DG.Mon = gpsmo;
         DG.Day = gpsdy;
         DG.MSOD = MSOD;
         DG.GPSStat = gpsstat;
         DG.GTemp = (tmp[0] + 50.0) * 100.0;
         DG.STemp = (tmp[1] + 50.0) * 100.0;
         DG.ETemp = (tmp[2] + 50.0) * 100.0;
         DGBMod = TRUE;
         
         /*************************************************
         * Now check if 5 seconds has elapsed since last *
         * DataGram transmission. If so, then transmit.  *
         *************************************************/
         if ((DG.MSOD - LastDGBMSOD) >= 5000)
         {
            /***********************************	    
            * pack the data into 3 byte words *
            ***********************************/
            for (index2 = 0; index2 < DG.NumVec; index2++)
            {
               for (index = 0; index < 3; index++)
               {
                  memcpy (&DG.ChBuf[index2][index][0],
                     &DGBuffer[index2][index], 3);
               }
            }
            
            /*****************************
            * and transmit the DataGram *
            *****************************/
            for (index = 0; index <= NumHosts; index++)
            {
               n = sendto (Sock, &DG, sizeof(struct DGBuf), 0,
                  (struct sockaddr *) &Server[index], SockLngth);
               if (n < 0)
                  error ("Sendto");
            }
            DG.DGCount++;
            LastDGBMSOD = DG.MSOD;
            DGBMod = FALSE;
            
            /********************************************		
            * and transmit heartbeat   //krowe 6/18/13 *
            ********************************************/
            //printf("Sending packet to %s port:%d\n", inet_ntoa(server.sin_addr),HBPort);
            //sprintf(buf, "iBootHB");
            //if (sendto(s, buf, BUFLEN, 0, (struct sockaddr *) &server, slen)==-1)
            //   error ("sendto()");
            //krowe end
         }
      }
      else
      {
         return (1);
      }
   }
   
   LastDac1 = Dac1;
   LastDac2 = Dac2;
   LastDac3 = Dac3;
   
   return (0);
}


// GPS messages
int GMsg ()
{
   int nread, gpsyrl, gpsyrm;
   int n;
   
   /******************************************************
   * We enter this routine with a GPS message, from      *
   * which we extract the current date, time and status. *
   *******************************************************/
   nread = sscanf (&buffer[1], "%2x%2x%2x%2x%2x%2x%2x%2x ",
      &gpsstat, &gpsyrl, &gpsyrm, &gpsmo, &gpsdy,
      &gpssec, &gpsmin, &gpshr);

   if (nread == 8)
   {
      gpsyr = gpsyrl + gpsyrm * 256;

      // Schofield begin ======================================================
      // AUTUMNX data display (AJAX UI) - Schofield 2014/04/10
     
      sprintf(AXbuffer,"GMSG:GPS_STAT,%d,YEAR,%d,MONTH,%d,DAY,%d,HOUR,%d,MINUTE,%d,SECOND,%d",
         gpsstat, gpsyr, gpsmo, gpsdy, gpshr, gpsmin, gpssec);   
                  
      if (GPSVerbose)
      {
         printf("%s\n",AXbuffer);
      }
      
      if (NetVerbose)
      { 
         n = sendto(AXsock, AXbuffer, strlen(AXbuffer), 0,
				       (const struct sockaddr *)&AXserver,
                   sizeof(struct sockaddr_in));
         if(n < 0) 
            error ("sendto");	                 
      }
      // Schofield end =========================================================
            
      if ((gpsstat > 7) && !INIT)	// log first time with good GPS data 
      {
         if (errno = LOG (""))
         {
            printf ("Log Error = %i\n", errno);
         }
      }
      
      return (0);
   }
   else
   {
      return (1);
   }	
}


// Miscellaneous magnetometer data packets
int MMsg ()
{
   int nread, pktid, Dac1, Dac2, Dac3, CompLat, DrvLat;
   int DecRate, DriftTol, FPGAVer, BootVer;
   int ProgVer, FirmVer, ErrFlgs, iPolarity, iPhase;
   int index1, index2, length;
   long SerNum;
   char tmpchar[1], tmpstr[128];
   
   
   /*******************************************************
   * We enter this routine with a miscellaneous mag data *
   * message. The only one we expect is the Mag Status   *
   * packet, which we have requested for log purposes.   *
   *******************************************************/
   length = strlen (buffer);
   switch (buffer[length - 1])	//check first byte (bytes in reverse order)
   {
      case '1':			// we shouldn't be getting download errors
      case '3':
      case '4':
      case '5':
         printf ("Download error type %01i \n", buffer[length - 1]);
         break;
      case '2':			// this is also a download error unless 35 bytes long
         if (buffer[2] == '\n')	// 2nd byte is a zero
         {
            printf ("Download error type %01i \n", buffer[length - 1]);
            break;
         }
         else
         {
            if ((buffer[length - 2] == '0') && (buffer[35] == '\0'))
            {
               /*************************************************
               * Now we know we have a Mag Status Packet.      *
               * This message comes in backwards, so I need to *
               * reverse the bytes.                            *
               *************************************************/
               for (index1 = 1; index1 < 18; index1++)
               {
                  tmpchar[1] = buffer[index1];
                  buffer[index1] = buffer[35 - index1];
                  buffer[35 - index1] = tmpchar[1];
               }
               nread = sscanf (buffer,	//get mag status data and make a 'log message'
                  "M20%2x%2x%2x%2x%2x%2x%2x%2d%6d%2d%2d%2d%2d%2x",
                  &iPhase, &Dac1, &Dac2, &Dac3, &CompLat,
                  &DrvLat, &DecRate, &DriftTol, &SerNum,
                  &FPGAVer, &BootVer, &ProgVer, &FirmVer,
                  &ErrFlgs);
               if (iPhase & 0x80)
               {
                  iPhase = iPhase & 0x7f;
                  iPolarity = 1;
               }
               else
                  iPolarity = 0;
               
               strncpy (LogStr, "Operating Parameters Received from MAG\r\n",41);
               sprintf (tmpstr,"Phase=%i\r\nData Latch=%i\r\nGPS Tolerance=%i\r\n",
                  iPhase, CompLat, DriftTol);
               strncat (LogStr, tmpstr, 100);
               sprintf(tmpstr,"Drive Latch=%i\r\nDecimation Rate=%i\r\nDAC1=%i\r\nDAC2=%i\r\nDAC3=%i\r\n",
                  DrvLat, DecRate, Dac1, Dac2, Dac3);
               strncat (LogStr, tmpstr, 100);
               
               if (iPolarity)
                  sprintf (tmpstr, "Polarity=TRUE\r\n");
               else
                  sprintf (tmpstr, "Polarity=FALSE\r\n");
                  
               strncat (LogStr, tmpstr, 17);
               sprintf (tmpstr, "StatusFlags=%01i\r\n", ErrFlgs);
               strncat (LogStr, tmpstr, 17);
               sprintf (tmpstr,"Version Number=%01i%s%i%i%01i.%01i%01i\r\n",
                  BootVer, SoftwareType, MajVer, MinVer,ProgVer, FirmVer, FPGAVer);
               syslog(LOG_INFO, "GBOMag communicating with MAG V:%01i%s%i%i%01i.%01i%01i S:%i\n",
                  BootVer, SoftwareType, MajVer, MinVer, ProgVer, FirmVer, FPGAVer, SerNum);
               strncat (LogStr, tmpstr, 30);
               sprintf (tmpstr, "Mag Serial number = %i\r\n", SerNum);
               strncat (LogStr, tmpstr, 30);
               
               if (StatVerbose)
                  printf(LogStr);
               if (errno = LOG (LogStr))
                  printf ("Log Error = %i\n", errno);
                  
               break;
            }
            else
               return (1);
         }
      default:
         break;
   }
   
   return (0);
}


int LOG (char *logmsg)
{
   static char LogFileNamFull[512];
   char Date[128], *Param;
   static int LogDay = -99;
   double tset;
   static char LogFileNamPart[256];
   int FilCnt, index, errno;
   int AutoDac;
   
   // Added by William Tam
   mode_t file_permissions = S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;

   /*********************************************
   * First time through with good GPS data, or *
   * when day changes, open a new log file.    *
   *********************************************/
   if (gpsdy != LogDay)
   {				
      //construct a new file path-name
      strncpy (LogFilePath, GBOFileRoot, 240);	//First construct directory path
      sprintf (Date, "/%04i", gpsyr);	// year directory
      strncat (LogFilePath, Date, 5);
      mkdir (LogFilePath, file_permissions);	// create directory, if needed
      sprintf (Date, "/%02i", gpsmo);	// month directory
      strncat (LogFilePath, Date, 3);
      mkdir (LogFilePath, file_permissions);	// create directory, if needed
      sprintf (Date, "/%02i", gpsdy);	// date directory
      strncat (LogFilePath, Date, 3);
      mkdir (LogFilePath, file_permissions);	// create directory, if needed
      strncpy (LogFileNamPart, LogFilePath, 240);
      strncat (LogFileNamPart, "/", 1);	// now, the file name
      strncat (LogFileNamPart, SiteNam, 4);	// start with the site name
      sprintf (Date, "%02i%02i%02i_MAG_%02i_",	// then add date and & hour
         gpsyr % 100, gpsmo, gpsdy, gpshr);
      strncat (LogFileNamPart, Date, 14);
      
      for (FilCnt = 0; FilCnt < 99; FilCnt++)	//iterate FilID until we have new file
      {
         strncpy (LogFileNamFull, LogFileNamPart, 240);
         sprintf (FilID, "%02i.LOD", FilCnt);
         strncat (LogFileNamFull, FilID, 6);
         if (!fopen (LogFileNamFull, "r"))
            break;		//quit when we have a new file
      }
      
      if (FilCnt == 99)
         return (2);		// too many files; somethings wrong!
      LODout = fopen (LogFileNamFull, "w");
      
      if (LODout == 0) 
      { 
         //Checking to make sure that LOD file is opened correctly (pcruce 2013-02-07)
         printf("Unable to open LOD file\n");
         exit(-1);
      }
      
      unlink (RecPathL);
      symlink (LogFileNamFull, RecPathL);
      sprintf (Date, "%02i/%02i/%04i-%02i:%02i:%02i_",
         gpsmo, gpsdy, gpsyr, gpshr, gpsmin, gpssec);
      fprintf (LODout, "Log File Opened For Site %s on %s\n", SiteNam, Date);
      LogDay = gpsdy;
      fclose (LODout);
      
      if (INIT)			//if new file and not the first time
      {
         //then request configuration data
         req.tv_sec = 0;
         req.tv_nsec = 3000;	// set sleep timer for 3 milliseconds 
         fprintf (magout, "STOREPARAM=1\r\n");
         errno = nanosleep (&req, &rem);
      }
   }
   
   /******************************************
   * First time through with good GPS data, *
   * Output Configuration Data.             *
   ******************************************/
   if (!INIT && (gpsstat > 7))
   {
      req.tv_sec = 0;
      req.tv_nsec = 3000;	// set sleep timer for 3 milliseconds 
      LODout = fopen (LogFileNamFull, "a");
      
      if (AutoDacB)
         fprintf (LODout, "Automatic DAC adjustment\r\n");
         
      fprintf (LODout, "SawtoothEnable=%s\r\n", SawtoothEnableC);
      
      for (index = 0; index < 3; ++index)
      {
         if (set[index] != -99.0)
            fprintf (LODout, "PID Control of %s - Setpoint = %lf\r\n",&LABL[index], set[index]);
      }
      
      fprintf (LODout,
         "%02i%02i%04i-%02i:%02i:%02i-Configuration Command(s):\r\n",
         gpsmo, gpsdy, gpsyr, gpshr, gpsmin, gpssec);
         
      fprintf (LODout, "[GPSPORT=1\r\n");
      fprintf (LODout, "GPSON=1\r\n");
      fprintf (LODout, "PHASE=%s\r\n", Phase);
      fprintf (magout, "PHASE=%s\r\n", Phase);
      fclose (LODout);
      errno = nanosleep (&req, &rem);
      
      LODout = fopen (LogFileNamFull, "a");
      fprintf (LODout, "DRIVELAT=%s\r\n", Drvlat);
      fprintf (magout, "DRIVELAT=%s\r\n", Drvlat);
      fclose (LODout);
      errno = nanosleep (&req, &rem);
      
      LODout = fopen (LogFileNamFull, "a");
      fprintf (LODout, "LATCH=%s\r\n", Datlat);
      fprintf (magout, "LATCH=%s\r\n", Datlat);
      fclose (LODout);
      
      errno = nanosleep (&req, &rem);
      LODout = fopen (LogFileNamFull, "a");
      fprintf (LODout, "POLARITY=%s\r\n", PolarityC);
      fprintf (magout, "POLARITY=%i\r\n", PolarityB);
      fclose (LODout);
      errno = nanosleep (&req, &rem);
      
      LODout = fopen (LogFileNamFull, "a");
      fprintf (LODout, "DECIMATION=%s\r\n", Decimation);
      fprintf (magout, "DECIMATION=%s\r\n", Decimation);
      fclose (LODout);
      errno = nanosleep (&req, &rem);
      
      LODout = fopen (LogFileNamFull, "a");
      fprintf (LODout, "FEEDBACKENABLE=%s\r\n", FeedBackEnable);
      fprintf (magout, "FEEDBACKENABLE=%s\r\n", FeedBackEnable);
      fclose (LODout);
      errno = nanosleep (&req, &rem);
      
      LODout = fopen (LogFileNamFull, "a");
      fprintf (LODout, "DAC1VALUE=%s\r\n", DACV1);
      fprintf (magout, "DAC1VALUE=%s\r\n", DACV1);
      fclose (LODout);
      errno = nanosleep (&req, &rem);
      
      LODout = fopen (LogFileNamFull, "a");
      fprintf (LODout, "DAC2VALUE=%s\r\n", DACV2);
      fprintf (magout, "DAC2VALUE=%s\r\n", DACV2);
      fclose (LODout);
      errno = nanosleep (&req, &rem);
      
      LODout = fopen (LogFileNamFull, "a");
      fprintf (LODout, "DAC3VALUE=%s\r\n", DACV3);
      fprintf (magout, "DAC3VALUE=%s\r\n", DACV3);
      fclose (LODout);
      errno = nanosleep (&req, &rem);
      
      LODout = fopen (LogFileNamFull, "a");
      if (AutoDacB)
      {
         fprintf (LODout, "DACSET=1\r\n");
         fprintf (magout, "DACSET=1\r\n");
      }
      else
      {
         fprintf (LODout, "DACSET=0\r\n");
         fprintf (magout, "DACSET=0\r\n");
      }
      
      fclose (LODout);
      errno = nanosleep (&req, &rem);
      LODout = fopen (LogFileNamFull, "a");
      fprintf (LODout, "HIGHRATE=0\r\n");
      fprintf (magout, "HIGHRATE=0\r\n");
      fclose (LODout);
      errno = nanosleep (&req, &rem);
      LODout = fopen (LogFileNamFull, "a");
      
      for (index = 0; index < 3; ++index)
      {
         fprintf (LODout, TMPCMND[index], 0);
         fprintf (magout, TMPCMND[index], 0);
         fclose (LODout);
         errno = nanosleep (&req, &rem);
         LODout = fopen (LogFileNamFull, "a");
      }
      
      fprintf (LODout, "GPSTOLERANCE=%s]\r\n", GPSTolerance);
      fprintf (magout, "GPSTOLERANCE=%s\r\n", GPSTolerance);
      fclose (LODout);
      errno = nanosleep (&req, &rem);
      LODout = fopen (LogFileNamFull, "a");
      
      fprintf (LODout,
         "%02i%02i%04i-%02i:%02i:%02i-Read Ram Command Sent\r\n",
         gpsmo, gpsdy, gpsyr, gpshr, gpsmin, gpssec);
      fprintf (LODout, "[STOREPARAM=1]\r\n");
      fclose (LODout);
      fprintf (magout, "STOREPARAM=1\r\n");
      errno = nanosleep (&req, &rem);
      
      if (SawtoothEnableB)
         fprintf (magout, "DECIMATION=255\r\n");
         
      INIT = TRUE;
   }
   if ((logmsg != "") && INIT)
   {
      LODout = fopen (LogFileNamFull, "a");
      fprintf (LODout, "%02i%02i%04i-%02i:%02i:%02i-%s",
         gpsmo, gpsdy, gpsyr, gpshr, gpsmin, gpssec, logmsg);
      fclose (LODout);
   }
   return (0);
}


// Housekeeping data (*.HKD file creation & updating)
// HMSG: housekeeping message
int HKD (int Hr, int Min, int Flag)
{

   static int FirstTime = TRUE, HkdDay = -99;
   static char HkdFileNamFull[512];
   char Date[128];
   int FilCnt;
   static char HkdFileNamPart[256];
   int n;
   
   // Added by William Tam
   mode_t file_permissions = S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
   
   /****************************************************
   * First time through with good GPS data, or        *
   * when day changes, open a new housekeepiing file. *
   ****************************************************/
   if ((gpsdy != HkdDay) || FirstTime)
   {
      if (HkdDay > 0)
      {
         sprintf (Date, "%02i/%02i/%04i %02i:00", gpsmo, gpsdy, gpsyr,gpshr);
         sprintf (LogStr, "HKD File Closed For Site %s on %s\r\n",SiteNam, Date);
         
         if (errno = LOG (LogStr))
            printf ("Log Error = %i\n", errno);
      }
      
      //construct a new file path-name
      strncpy (HkdFilePath, GBOFileRoot, 240);	//First construct directory path
      sprintf (Date, "/%04i", gpsyr);	// year directory
      strncat (HkdFilePath, Date, 5);
      mkdir (HkdFilePath, file_permissions);	// create directory, if needed
      sprintf (Date, "/%02i", gpsmo);	// month directory
      strncat (HkdFilePath, Date, 3);
      mkdir (HkdFilePath, file_permissions);	// create directory, if needed
      sprintf (Date, "/%02i", gpsdy);	// date directory
      strncat (HkdFilePath, Date, 3);
      mkdir (HkdFilePath, file_permissions);	// create directory, if needed
      strncpy (HkdFileNamPart, HkdFilePath, 240);	//now, the file name,
      strncat (HkdFileNamPart, "/", 1);
      strncat (HkdFileNamPart, SiteNam, 4);	// start with the site name
      sprintf (Date, "%02i%02i%02i_MAG_%02i_",	// then add date & hour
         gpsyr % 100, gpsmo, gpsdy, gpshr);
      strncat (HkdFileNamPart, Date, 14);
      
      for (FilCnt = 0; FilCnt < 99; FilCnt++)	// iterate FilID until we have new file
      {
         strncpy (HkdFileNamFull, HkdFileNamPart, 240);
         sprintf (FilID, "%02i.HKD", FilCnt);
         strncat (HkdFileNamFull, FilID, 6);
         if (!fopen (HkdFileNamFull, "r"))
            break;		//quit when we have a new file
      }
      if (FilCnt == 99)
         return (2);		// too many files; somethings wrong!
      
      hkdout = fopen (HkdFileNamFull, "w");	//so now, log that we made a new housekeeping file
      unlink (RecPathH);
      symlink (HkdFileNamFull, RecPathH);
      sprintf (Date, "%02i/%02i/%04i %02i:00", gpsmo, gpsdy, gpsyr, gpshr);
      sprintf (LogStr, "HKD File Opened For Site %s on %s\r\n",SiteNam, Date);
      
      if (errno = LOG (LogStr))
         printf ("Log Error = %i\n", errno);
         
      fprintf (hkdout, "  DATE     TIME    ETemp   STemp   GTemp    CH1");
      fprintf (hkdout, "       CH2       CH3    GPS\r\n");
      fclose (hkdout);
      HkdDay = gpsdy;
      FirstTime = FALSE;
   }
   				
   // and now, write housekeeping data to file
   Nt1 = (Ch1 - 524288.0) / MagScal1 - (Dac1 - 128) * DacScal1;
   Nt2 = (Ch2 - 524288.0) / MagScal2 - (Dac2 - 128) * DacScal2;
   Nt3 = (Ch3 - 524288.0) / MagScal3 - (Dac3 - 128) * DacScal3;
   hkdout = fopen (HkdFileNamFull, "a");
    
   // Schofield begin ==========================================================
   // AUTUMNX data display (AJAX UI) - Schofield 2014/04/10
   if (NetVerbose)
   { 
      sprintf(AXbuffer,
"HMSG:GPSMO,%02d,GPSDY,%02d,GPSYR,%04d,HR,%02d,MIN,%02d,ETEMP,%f,STEMP,%f,GTEMP,%f,XCOMP,%10f,YCOMP,%10f,ZCOMP,%10f,FLAG,%02d",
      gpsmo, gpsdy, gpsyr, Hr, Min, tmp[2], tmp[1], tmp[0], Nt1, Nt2, Nt3, Flag);
         
      n = sendto(AXsock, AXbuffer, strlen(AXbuffer), 0,
         (const struct sockaddr *)&AXserver,
         sizeof(struct sockaddr_in));
      if(n < 0) 
         error ("sendto");	                 
   }
   // Schofield end ============================================================    
    
      
   fprintf(hkdout,
      "%02i%02i%04i% 5i:%02i% 8.2f% 8.2f% 8.2f% 10.2f% 10.2f% 10.2f% 3i\r\n",
      gpsmo, gpsdy, gpsyr, Hr, Min, tmp[2], tmp[1], tmp[0], Nt1, Nt2,Nt3, Flag);
   fclose (hkdout);
   
   return (0);
}
