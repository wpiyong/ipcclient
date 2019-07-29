#include <stdlib.h>
#include <stdio.h>
#include <dlfcn.h>

#ifdef __linux__
#include <syslog.h>
#else
int round(double number)
{
	return (number >= 0) ? (int)(number + 0.5) : (int)(number - 0.5);
}
#endif

#include <math.h>
#include <string.h>

typedef enum
{
	INCREASE_EXP = 1,
	DECREASE_EXP = 2,
	CONTINUE = 3,
	REFER = 4,
	PASS_FL = 5,
	PASS = 6
} SPECTRUM_ANALYZER_RESULT;

typedef enum
{
	NO_NV = 1,
	NV_SMALLER_THAN_N3 = 2,
	NV_GREATER_THAN_N3 = 3
}NV_RESULT;

typedef enum
{
	SPECTRUM_CCD = 0,
	SPECTRUM_CMOS = 1,
	SPECTRUM_UKNOWN = 2
}SPECTRUM_TYPE;

static SPECTRUM_TYPE spectrumType = SPECTRUM_UKNOWN;
static double QE_factor[2048];

int get_qe_factor();

int init()
{
	int res = -1;
	int (*get_parameter)();
	int *n3lib;

    setlogmask (LOG_UPTO (LOG_NOTICE));
    openlog ("n3-spectrum", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);

	spectrumType = SPECTRUM_UKNOWN;
	n3lib = dlopen("/usr/lib/n3lib.so", RTLD_LAZY);
	if(n3lib != NULL){
		*(void **)(&get_parameter) = dlsym(n3lib, "get_parameter");

		if(!get_parameter){
			syslog (LOG_NOTICE, "get_parameter function not found");
			dlclose(n3lib);
			return -1;
		}
		int type = get_parameter();
		syslog (LOG_NOTICE, "get_parameter: get sensor type: %d", type);
		if(type == -1) {
			return -1;
		}

		if(type == 10) {
			spectrumType = SPECTRUM_CCD;
			res = 0;
		} else {
			spectrumType = SPECTRUM_CMOS;
			res = get_qe_factor();
		}
	}

	if(n3lib != NULL) {
		dlclose(n3lib);
	}
	closelog();

	return res;
}

int get_qe_factor()
{
	char buffer[1024] ;
    char *record, *line;
    int i = 0;

    setlogmask (LOG_UPTO (LOG_NOTICE));
    openlog ("n3-spectrum", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);

    FILE *fstream = fopen("myFile.csv", "r");
    if(fstream == NULL)
	{
    	syslog (LOG_NOTICE, "QE_factor file myFile.csv does not exist.");
        return -1;
    }
    while((line = fgets(buffer, sizeof(buffer), fstream)) != NULL)
    {
		record = strtok(line, ",");
		QE_factor[i] = atof(record);
		syslog (LOG_NOTICE, "QE_factor %d, %f", i, QE_factor[i]);
		++i ;
	}

    closelog ();

	return 0;
}