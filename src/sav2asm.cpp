#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <string>
#include <vector>


struct saveBlock {
    uint32_t value;
    uint8_t *start;
    size_t repeat;
};

int main(int argc, char **argv) {

    if (argc<2) {
        fprintf(stderr, "Usage: %s infile [outfile]\n",argv[0]);
        exit(1);
    }

    std::string infile, outfile, ext, outext;
    size_t lastdot;

    infile = argv[1];

    if ( (lastdot = infile.rfind("."))!= std::string::npos ) {
        ext = infile.substr(lastdot);
    }

    if (argc >= 3) {
        outfile =argv[2];
    } else {
        outfile = infile.substr(0,lastdot);
        outfile += ".s";
    }

    FILE *savefile = fopen(infile.c_str(),"rb");

    if(savefile == NULL) {
        printf("Error: cannot open %s for reading\n", infile.c_str());
        exit(1);
    };

    fseek(savefile, 0, SEEK_END);

    size_t fileSize = ftell(savefile);

    fseek(savefile, 0, SEEK_SET);

    uint8_t *buffer = (uint8_t*)malloc(fileSize);

    fread(buffer, 1, fileSize, savefile);

    size_t len = fileSize, repeat = 0;

    std::vector<struct saveBlock> saveBlocks;

    uint8_t *ptr = buffer;
    uint8_t nextchar, thischar = *ptr++;
    repeat++;
    len--;

    struct saveBlock lastBlock;
    struct saveBlock thisBlock;

    lastBlock.start = buffer;
    thisBlock.start = buffer;
    lastBlock.value = 512;

    do {

        nextchar = *ptr++;
        len--;

        if (len==0) {

            if (thischar == nextchar) {

                repeat++;
                nextchar = ~thischar;

            } else {

                    lastBlock.value = 512;
                    lastBlock.repeat = (size_t)(ptr - lastBlock.start);
                    saveBlocks.push_back(lastBlock);

            }
        }

        if(thischar == nextchar) {
            repeat++;
        } else {
            thisBlock.repeat = repeat;

            if(repeat>16) {
                thisBlock.value = thischar;

                if(lastBlock.start != thisBlock.start) {
                    lastBlock.value = 512;
                    lastBlock.repeat = (size_t)(thisBlock.start - lastBlock.start);
                    saveBlocks.push_back(lastBlock);
                }
                saveBlocks.push_back(thisBlock);
                lastBlock.start = ptr -1;
            }

            repeat=1;
            thischar=nextchar;
            thisBlock.start = ptr -1;
        }

    } while(len > 0);

    lastBlock = saveBlocks[saveBlocks.size()-1];

    printf("%d blocks found.\n", saveBlocks.size());

    FILE *asmFile = fopen(outfile.c_str(),"wb");

    fprintf(asmFile, "\t.global	_start\n_start:\n");

    for(uint32_t i=0; i<saveBlocks.size(); i++) {

        uint8_t *blockEnd = (uint8_t *)(saveBlocks[i].start + saveBlocks[i].repeat);

        if (saveBlocks[i].value == 512) {

            uint8_t *ptr = saveBlocks[i].start;

            uint32_t word;
            int element = 0, maxElements = 4, outsize = 4, lastsize = -1;

            blockEnd = (uint8_t*)((intptr_t)blockEnd&~3);
            if(blockEnd - ptr >= 4) {
                do {

                    switch(((intptr_t)(ptr - buffer)) & 3) {

                    case 0:
                        word = *(ptr++) ;
                        word += (*(ptr++) <<8);
                        word += (*(ptr++) <<16);
                        word += (*(ptr++) <<24);
                        outsize = 4;
                        break;

                    case 1:
                    case 3:
                        word = *(ptr++);
                        saveBlocks.push_back(thisBlock);

                        outsize = 1;
                        break;

                    case 2:
                        word = *(ptr++);
                        word += (*(ptr++) <<8);
                        outsize = 2;
                        break;
                    }

                    if(lastsize!=outsize || element == maxElements) {
                        fprintf(asmFile, "\n\t");
                        element = 0;

                        switch(outsize) {

                        case 4:
                            fprintf(asmFile, ".word\t");
                            maxElements = 4;
                            break;

                        case 2:
                            fprintf(asmFile,".hword\t");
                            maxElements = 8;
                            break;

                        case 1:
                            fprintf(asmFile, ".byte\t");
                            maxElements = 16;
                            break;


                        }
                    }

                    if(element != 0) fprintf(asmFile,", ");

                    switch(outsize) {

                    case 4:
                        fprintf(asmFile, "0x%08x", word);
                        break;

                    case 2:
                        fprintf(asmFile,"0x%04x",word & 0xffff);
                        break;

                    case 1:
                        fprintf(asmFile, "0x%02x",word & 0xff);
                        break;

                    }

                    element++;

                    lastsize = outsize;

                } while(ptr<blockEnd);
            }
            blockEnd = (uint8_t*)(saveBlocks[i].start + saveBlocks[i].repeat);

            if(blockEnd - ptr) {
                fprintf(asmFile, "\n\t.byte\t");

                switch(blockEnd - ptr) {
                case 3:
                    fprintf(asmFile, "0x%02x, ", *ptr++);

                case 2:
                    fprintf(asmFile, "0x%02x, ", *ptr++);

                case 1:
                    fprintf(asmFile, "0x%02x\n", *ptr++);
                    break;
                }
            } else {
                fprintf(asmFile,"\n");
            }

        } else {

            fprintf(asmFile, "\n\t.space	_start + 0x%p - . , 0x%02x\t@ %d bytes\n", (uint8_t*)(blockEnd - buffer), saveBlocks[i].value, saveBlocks[i].repeat);
        }
    }

    fclose(asmFile);
    return 0;
}
