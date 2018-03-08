#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <getopt.h>
#include <errno.h>

//---------------------------------------------------------------------------------
void showHelp() {
//---------------------------------------------------------------------------------
	puts("Usage: inject\n");
	puts("--help, -h      Display this information");
	puts("--address, -a   Address in target file");
	puts("--input,   -i   Input file ");
	puts("--output,  -o   Output file (overwrites target if not specified");
	puts("--target,  -t   Target file ");
	puts("\n");
}

//---------------------------------------------------------------------------------
size_t copydata(FILE *inh, FILE *outh) {
//---------------------------------------------------------------------------------

		uint8_t copybuf[16*1024];

		size_t readbytes, writtenbytes;
		do {

			readbytes = fread(copybuf, 1, sizeof(copybuf), inh);
    		if (readbytes) writtenbytes = fwrite(copybuf, 1, readbytes, outh);
    			else   writtenbytes = 0;
		} while ((readbytes > 0) && (writtenbytes == readbytes));

		return writtenbytes;
}

//---------------------------------------------------------------------------------
int main(int argc, char ** argv) {
//---------------------------------------------------------------------------------

	size_t address = 0;
	char *infile = NULL;
	char *outfile = NULL;
	char *targetfile = NULL;
	char *endarg = NULL;

	if(argc <3) {
		showHelp();
		exit(EXIT_FAILURE);
	}

	while(1) {
		static struct option long_options[] = {
			{"input",	required_argument,	0,	'i'},
			{"output",	required_argument,	0,	'o'},
			{"target",	required_argument,	0,	't'},
			{"address",	required_argument,	0,	'a'},
			{"help",	no_argument,		0,	'h'},
			{0, 0, 0, 0}
		};

		/* getopt_long stores the option index here. */
		int option_index = 0, c;

		c = getopt_long (argc, argv, "i:o:t:a:h", long_options, &option_index);

		/* Detect the end of the options. */
		if (c == -1)
		break;

		switch(c) {

		case 'i':
			infile = optarg;
			break;
		case 'o':
			outfile = optarg;
			break;
		case 't':
			targetfile = optarg;
			break;
		case 'a':
			errno = 0;
			address = strtoul(optarg, &endarg, 0);
			if (endarg == optarg) errno = EINVAL;
			if (errno != 0) {
				perror("--address");
				exit(EXIT_FAILURE);
			}
			break;
		case 'h':
			showHelp();
			break;
		}

	}

	if (infile == NULL) {
		fprintf(stderr,"input must be specified!\n");
		exit(EXIT_FAILURE);
	}

	if (targetfile == NULL) {
		fprintf(stderr,"target must be specified!\n");
		exit(EXIT_FAILURE);
	}


	if (outfile != NULL) {
		FILE *outh = fopen(outfile, "wb");
		if (outh == NULL) {
			perror(outfile);
			exit(EXIT_FAILURE);
		}
		FILE *inh = fopen(targetfile,"rb");
		if (inh == NULL) {
			perror(targetfile);
			fclose(outh);
			exit(EXIT_FAILURE);
		}

		size_t writtenbytes = copydata(inh, outh);

		fclose(inh);
		fclose(outh);

		if(writtenbytes) {
			perror(targetfile);
			exit(EXIT_FAILURE);
		}

		targetfile = outfile;
	}


	FILE *outh = fopen(targetfile,"rb+");

	if (outh == NULL) {
		perror(targetfile);
		exit(EXIT_FAILURE);
	}


	if (fseek(outh, address, SEEK_SET) == -1) {
		perror("seek");
		fclose(outh);
		exit(EXIT_FAILURE);
	}

	FILE *inh = fopen(infile,"rb");

	if (inh == NULL) {
		perror(infile);
		fclose(outh);
		exit(EXIT_FAILURE);
	}

	size_t writtenbytes = copydata(inh, outh);

	if(writtenbytes) {
		fprintf(stderr, "inject failure.\n" );
	}

	fclose(outh);
	fclose(inh);

	return 0;
}