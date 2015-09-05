/*
 * lcd_config_handler.c
 *
 *  Created on: 4 Sep, 2015
 *      Author: zhoujianxiang
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <signal.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <getopt.h>
#include <pthread.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <syslog.h>

#define DEBUG
#define SOURCE_VERSION "1.0.0"

#ifdef DEBUG
#define DBG(...) fprintf(stderr, " DBG(%s, %s(), %d): ", __FILE__, __FUNCTION__, __LINE__); fprintf(stderr, __VA_ARGS__)
#else
#define DBG(...)
#endif


enum Platform
{
	Mtk = 0,
	Sprd,
	PlatformMax
};

enum Type
{
	Type0 = 0,	 //recognized by 0x and newline
	Type1,		//recognized by 0x and continuous newlines
	TypeMax
};

enum DsiMode
{
	Gen = 0,		//generic write 
	Dcs,			//dcs write
	For,			//force write
	DsiModeMax
};

enum ForceFlag 
{
	ForceOff = 0,
	ForceOn
};
struct cmdarg 
{
	enum Platform platform;
	enum Type type;
	enum DsiMode dsi_mode;
	enum ForceFlag force_flag;
};



void help(char *progname)
{
	
  fprintf(stderr, "-----------------------------------------------------------------------\n");
  fprintf(stderr, "Usage: %s\n" \
                  "\t-m | -s		platform selection, \"m\" stands for mtk, \"s\" is sprd, and it 's necessary to select one of them.\n" \
                  "\t-t  [0 | 1 ]	source file type, it supports 2 types so far, and it 's necessary to select one type of them.\n" \
                  "\t-h		show the help information.\n" \
                  "\t-v		show the version.\n" , progname);
  
  fprintf(stderr, "-----------------------------------------------------------------------\n");
  fprintf(stderr, "Example:\n" \
                  "./lcd_config_handler -p m -t 0 -d f\n");
 
  fprintf(stderr, "-----------------------------------------------------------------------\n");
}


int main(int argc, char *argv[])
{
	struct cmdarg config = {0};
	char error_flag = 0;

	DBG("this is lcd_config_handler\n");

	while(1) {
		int option_index = 0, c=0, cal = 0;
		static struct option long_options[] = \
		{
		        {"h", no_argument, 0, 0},
		        {"help", no_argument, 0, 0},
		        {"p", required_argument, 0, 0},
		        {"t", required_argument, 0, 0},
		        {"d", required_argument, 0, 0},
		        {"f", no_argument, 0, 0},
		        {"v", no_argument, 0, 0},
		        {0, 0, 0, 0}
		};

   		c = getopt_long_only(argc, argv, "", long_options, &option_index);

		DBG("c = %d\n, option_index = %d \n", c, option_index);
		

		/* no more options to parse */
		if (c == -1) 
			break;


	       	/* unrecognized option */
		if (c=='?') { 
		 	help(argv[0]);
			return 0; 
		}

		switch (option_index) {
			/* h, help */
			case 0:
			case 1:
				help(argv[0]);
			        return 0;

		        /* p ... platform */
	                case 2:
				if ( strcmp("m", optarg) == 0 ) 
					config.platform = Mtk;
				else if ( strcmp("s", optarg) == 0 ) 
					config.platform = Sprd; 
			        else {
				        fprintf(stderr, "unsupported platform.\n");
					error_flag = -1;
			        }
				break;

			/* t , souce file type */		
		        case 3:
			  	if ( strcmp("0", optarg) == 0 ) 
					config.type= Type0;
				else if ( strcmp("1", optarg) == 0 ) 
					config.type = Type1; 
			        else {
				        fprintf(stderr, "unsupported source file type.\n");
					error_flag = -1;
			        }
				break;

		        /* c, cmdmode */
		        case 4:
				if ( strcmp("g", optarg) == 0 ) 
					config.dsi_mode= Gen;
				else if ( strcmp("d", optarg) == 0 ) 
					config.dsi_mode = Dcs; 
			        else if ( strcmp("f", optarg) == 0 ) 
					config.dsi_mode = For; 
				else {
				        fprintf(stderr, "unsupported source file type.\n");
					error_flag = -1;
				}
				break;

			 case 5:
				config.dsi_mode= Gen;
				break;
			
			 case 6:
			 	  printf("lcd config handler Version: %s\n" \
   				            "Compilation Date.....: %s\n" \
				            "Compilation Time.....: %s\n", SOURCE_VERSION, __DATE__, __TIME__);
				return 0;

		        default:
		       		error_flag = -1;
		        	return 0;
    		}
  	}
	
	if (error_flag ) {
		fprintf(stderr, "some arguments are incorrect, please see the help information and try again.\n");
		return -1;
	}

	DBG("platform = %d, type = %d, dsi_mode = %d, force_flag =%d\n", config.platform,
		config.type, config.dsi_mode, config.force_flag);

	
	if (config.platform == PlatformMax) {
		config.platform = Mtk;
		error_flag =-1;
	}
	if (config.type == TypeMax) {
		config.type = Type0;
		error_flag =-1;
	}
	if (config.dsi_mode == DsiModeMax) {
		config.dsi_mode = Gen;
		error_flag =-1;
	}
	
	if (error_flag && config.force_flag == ForceOff) {
		fprintf(stderr, "some arguments are missing, do you wanna use the default config?\n");
		printf("please input your choice(\"y\" or \"n\")\n");
		if (getchar() != 'y')
			return -1;			
	}

	DBG("platform = %d, type = %d, dsi_mode = %d, force_flag =%d\n", config.platform,
		config.type, config.dsi_mode, config.force_flag);

	

	return 0;
	

}
