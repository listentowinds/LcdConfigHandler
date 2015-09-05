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
#include <dirent.h>

#define DEBUG
#define SOURCE_VERSION "1.0.0"
#define FILE_NUM_MAX 64
#define BUF_SIZE 2048
#define TURE 0
#define FALSE -1


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

struct current_file{
	int num;
	char *file_ptr[FILE_NUM_MAX];
};

struct current_file global_file_info;


/* create_directory function
* @szDirectoryPath: there must be "/" at the end, like "./out/"
 */
char create_directory( const char *szDirectoryPath , mode_t  iDirPermission)
{

	if ( NULL == szDirectoryPath ) {
		DBG( "[%s][%d][%s][parameter < szDirectoryPath > for < CreateDirectory > should not be NULL]\n" , \
			__FILE__ , __LINE__ , __FUNCTION__ );

		return FALSE;
	}

	const int iPathLength = strlen( szDirectoryPath ) ;

	if ( iPathLength > PATH_MAX ) {
		DBG("[%s][%d][%s][the path length(%d) exceeds system max path length(%d)]\n" , \
			__FILE__ , __LINE__ , __FUNCTION__ , iPathLength , PATH_MAX );
		return FALSE;
	}

	char szPathBuffer[ PATH_MAX ] = { 0 };

	memcpy( szPathBuffer , szDirectoryPath , iPathLength );
	DBG("memcpy end\n");
	DBG("szPathBuffer = %s \n", szPathBuffer);
	DBG("iPathLength = %d \n", iPathLength);
	int i;
	for ( i = 0 ; i < iPathLength ; ++i ) {
		char *refChar = &(szPathBuffer[ i ]);

		if ( ( '/' == *refChar ) && ( 0 != i ) ) {

			*refChar = '\0';

			int iStatus = access( szPathBuffer , F_OK );

			DBG("iStatus = %d \n", iStatus);
			DBG("szPathBuffer = %s \n", szPathBuffer);
			if ( 0 != iStatus ) {
				if ( ( ENOTDIR == errno ) || ( ENOENT == errno ) ) {				
					iStatus = mkdir( szPathBuffer , iDirPermission );
					
					if ( 0 != iStatus ) {
						DBG("[%s][%d][%s][< mkdir > fail , ErrCode:%d , ErrMsg:%s]\n" , \
							__FILE__ , __LINE__ , __FUNCTION__ , errno , strerror( errno ) );

						return FALSE;						
					}					
				} else {

					DBG("[%s][%d][%s][< access > fail , RetCode: %d , ErrCode:%d , ErrMsg:%s]\n" , \
						__FILE__ , __LINE__ , __FUNCTION__ , iStatus , errno , strerror( errno ) );

					return FALSE;
				}
			} else 
				DBG("The directory already here .\n");

			*refChar = '/';
		}
	}

	return TURE;

}



int trave_dir(char* path)
{
	DIR *d; 
	struct dirent *file; 
	struct stat buf;    

	if(!(d = opendir(path))) {
        	printf("error opendir %s!!!\\n",path);
        	return -1;
    	}
	
    	chdir(path);//Change the dir. Add this, so that it can scan the children dir

	printf("Files under current directory are below: \n");
    	while((file = readdir(d)) != NULL)
    	{

        	if(strncmp(file->d_name, ".", 1) == 0) //skip the "." and ".." dir
            		continue;

      	 	if(stat(file->d_name, &buf) >= 0 && !S_ISDIR(buf.st_mode) )
	        {
	        	int i = 0;
	        	printf("%s",file->d_name);
//	    		printf("\\tfile size=%d\\n",buf.st_size);
//	                printf("\\tfile last modify time=%d\\n",buf.st_mtime);
			if (malloc(strlen(file->d_name) + 1) != NULL) {
				global_file_info.file_ptr[i++] = file->d_name;
				global_file_info.num++;
				printf("\n");
			} else {
				fprintf(stderr, "cannot open, please check it out. \n");
				return -1; 
			}
	        }
	
  	 }
   	 closedir(d);

	 if (global_file_info.num == 0) {
	 	fprintf(stderr, "No file here. \n");
	 	return -1; 
 	}
	 
    	 return 0;
}



void help(char *progname)
{
	
  fprintf(stderr, "-----------------------------------------------------------------------\n");
  fprintf(stderr, "Usage: %s\n" \
                  "\t-m | -s		platform selection, \"m\" stands for mtk, \"s\" is sprd, and it 's necessary to select one of them.\n" \
                  "\t-t  [0 | 1 ]	source file type, it supports 2 types so far, and it 's necessary to select one of them.\n" \
                  "\t-h		show the help information.\n" \
                  "\t-v		show the version.\n" , progname);
  
//  fprintf(stderr, "-----------------------------------------------------------------------\n");
  fprintf(stderr, "Example1:\n" \
                  "./lcd_config_handler -p m -t 0 -d f\n");
  fprintf(stderr, "Example2:\n" \
                  "./lcd_config_handler -h\n");
  fprintf(stderr, "Example3:\n" \
                  "./lcd_config_handler -v\n");
 
  fprintf(stderr, "-----------------------------------------------------------------------\n");
}


int main(int argc, char *argv[])
{
	struct cmdarg config = {0};
	char error_flag = 0;

	DBG("this is lcd_config_handler\n");

	while(1) {
		int option_index = 0, c=0;
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

			 /* f */
			 case 5:
				config.force_flag= ForceOn;
				break;
				
			 /* v */
			 case 6:
			 	printf("Lcd config handler Version: %s\n" \
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
		
		printf("please input your choice(\"y\" or \"n\")\n");
		if (getchar() != 'y')
			return -1;			
	}

	DBG("platform = %d, type = %d, dsi_mode = %d, force_flag =%d\n", config.platform,
		config.type, config.dsi_mode, config.force_flag);


	if (trave_dir(".") < 0 ) {
		fprintf(stderr, "trave_dir error, exit.\n");
		return -1;
	}

	/* create the "out" directory */
	if (create_directory("./out/", S_IRWXU | S_IRWXG |S_IRWXO))
		fprintf(stderr, "create \"out\" false, exit it.\n");
		

	/*Extract the data from the source file according to the type*/
	int j = 0;
	while (j < global_file_info.num) {
		FILE * file_s, * file_d;
		char byte, flag = 0, buf[BUF_SIZE] = {0};
		if ((file_s = fopen(global_file_info.file_ptr[j], "r")) == NULL) {
			fprintf(stderr, "%s open fail and skip it.\n", global_file_info.file_ptr[j++]);
			continue;
		}

		chdir("./out/");
		if ((file_d = fopen(global_file_info.file_ptr[j], "w")) == NULL) {
			fprintf(stderr, "%s open fail and skip it.\n", global_file_info.file_ptr[j++]);
			continue;
		}
		
		chdir("..");
		j++;
		
		switch (config.type) {
			case Type0:
				while((byte = getc(file_s)) != EOF) {		
					if (byte == '0')
						flag = 1;
					else
						flag = 0;

					if ((byte == 'x' || byte == 'X') && flag)
						flag = 2;
					else
						flag = 0;

					

						

				}
			break;
			default:
			break;
		}
	}
	

	return 0;
	

}
