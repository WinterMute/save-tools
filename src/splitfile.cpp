#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>

#include <string>

void savePart(const char *name, uint8_t *buffer, size_t size) {

    FILE *part = fopen(name,"wb");

    if (part==NULL) {
      fprintf(stderr,"unable to open %s for writing.\n",name);
      exit(1);
    }

    fprintf(stderr,"writing %s\n",name);

    size_t written = fwrite(buffer,1,size,part);

    fclose(part);

    if (written!=size) {
      fprintf(stderr,"Error writing to %s.\n",name);
      exit(1);
    }
}

int main(int argc, char **argv) {

    if(argc<4) {
        fprintf(stderr,"Usage: %s file size1 size2 ...",argv[0]);
        exit(1);
    }

    std::string infile, outfile, ext;
    size_t lastdot, namestart;

    infile = argv[1];

    if ( (lastdot = infile.rfind("."))!= std::string::npos ) {
        ext = infile.substr(lastdot);
    }

    if ( (namestart = infile.rfind("/"))!= std::string::npos ) {
        namestart++;
    } else if ( (namestart = infile.rfind("\\"))!= std::string::npos) {
        namestart++;
    }

    if (namestart == std::string::npos ) namestart = 0;

    FILE *in = fopen(infile.c_str(),"rb");

    if(in==NULL) {
        fprintf(stderr,"Can't open %s for reading.\n",infile.c_str());
        exit(1);
    }


    for(int i=2;i<argc;i++) {
        errno = 0;
        size_t val = strtoul(argv[i],0,0);
        if(errno!=0) {
            perror(NULL);
            exit(1);
        }
    }

    fseek(in,0,SEEK_END);

    int size = ftell(in);

    uint8_t *buffer = (uint8_t *)malloc(size);

    fseek(in,0,SEEK_SET);

    fread(buffer,size,1,in);

    fclose(in);

    uint8_t *ptr = buffer;
    size_t written = 0;

    char numbuf[33];

    for(int i=2; i<argc; i++ ) {
        size_t partlength = strtoul(argv[i], 0, 0);


        snprintf(numbuf,33, "%d", i-1);

        outfile = infile.substr(namestart,lastdot-namestart) + numbuf + ".bin";

        if ( (size - written) < partlength) partlength = size - written;

        savePart(outfile.c_str(),ptr,partlength);

        ptr += partlength;
        written += partlength;
        if ((size - written) == 0) break;
    }

    if ((size - written) != 0 ) {

        snprintf(numbuf,33, "%d", argc - 1);

        outfile = infile.substr(namestart,lastdot-namestart) + numbuf + ".bin";

        savePart(outfile.c_str(),ptr,size-written);

    }

    return 0;
}