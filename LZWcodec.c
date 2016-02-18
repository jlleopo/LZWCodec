#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BIGNUM 10000 //number of characters in each entry
#define BIGGERNUM 32767 //number of entries in dictionary

void compress(FILE *fptIn, FILE *fptOut);
short patternInDictionary(unsigned char **dictionary, int *lengths, unsigned char *string, int slen);
int compare(unsigned char *str1, int len1, unsigned char *str2, int len2);
void addPatternDictionary(unsigned char **dictionary, int *lengths, unsigned char *newStr, int nlen);

void decompress(FILE *fptIn, FILE *fptOut);
int codeInDictionary(unsigned char **dictionary, int *lengths, short code, unsigned char *patt, int *plen);
void outputCodes(FILE *fptOut, unsigned char *c1, int l1, unsigned char *c2, int l2);
void addCodeDictionary(unsigned char **dictionary, int *lengths, unsigned char *string, int slen );

int main(int argc, char *argv[]){
    FILE *fptIn;
    FILE *fptOut;

    if(argc!=4) {
        printf("usage: LZWcodec 'c|d' 'fileNameIn' ' fileNameOut'");
        exit(0);
    }

    fptIn = fopen(argv[2], "rb");
    fptOut = fopen(argv[3], "wb");

    if(strcmp(argv[1], "c") == 0){
        compress(fptIn, fptOut);
    } else if(strcmp(argv[1], "d") == 0){
        decompress(fptIn, fptOut);
    } else {
        printf("second argument must be c or d");
        exit(0);
    }

	return 0;
}

void compress(FILE *fptIn, FILE *fptOut){
    int i;
    unsigned char **dictionary;
    int *lengths;
    unsigned char *string;
    int slen;
    unsigned char c;
    short code;

    dictionary = (unsigned char **) malloc(sizeof(unsigned char *)*BIGGERNUM);
    lengths = (int *) calloc(BIGGERNUM, sizeof(int));
    for(i=0; i<BIGGERNUM; i++){
        dictionary[i] = (unsigned char *) malloc(sizeof(unsigned char)*BIGNUM);
    }

    for(i=0; i<256; i++){
        dictionary[i][0] = (unsigned char) i;
        lengths[i]=1;
    }

    //start
    string = (unsigned char *) malloc(sizeof(unsigned char)*BIGNUM);
    fread (string, 1, 1, fptIn);
    slen = 1;
    while(fread(&c, 1, 1, fptIn) == 1){
        string[slen] = c;
        if(patternInDictionary(dictionary, lengths, string, slen+1) != -1){
            //string is already in dictionary

            //do nothing.
            //string is already string+c
            slen++;
        } else {
            //string isn't in dictionary yet
            //find code for string (without c)
            code = patternInDictionary(dictionary, lengths, string, slen);

            //write out this code
            fwrite(&code, sizeof(short), 1, fptOut);

            //add the new string to the dictionary
            addPatternDictionary(dictionary, lengths, string, slen+1);

            string[0] = c;
            slen = 1;
        }
    }
    code = patternInDictionary(dictionary, lengths, string, slen);
    fwrite(&code, sizeof(short), 1, fptOut);
}

//returns code if in dictionary (non negative)
//-1 if not in dictionary
short patternInDictionary(unsigned char **dictionary, int *lengths, unsigned char *string, int slen){
    short i=0;
    while(lengths[i] != 0){
        if(compare(dictionary[i], lengths[i], string, slen) == 1) return i;
        i++;
    }

    return -1;
}

//1 if same
//0 if different
int compare(unsigned char *str1, int len1, unsigned char *str2, int len2){
    int i;
    if(len1!=len2) return 0;

    for(i=0; i<len1; i++){
        if(str1[i] != str2[i]) return 0;
    }
    return 1;
}

//adds newStr to the end of dictionary
void addPatternDictionary(unsigned char **dictionary, int *lengths, unsigned char *newStr, int nlen){
    int i=0;
    int j;
    while(lengths[i] != 0) i++;

    lengths[i] = nlen;
    //strcpy from newStr to dictionary
    for(j=0; j<nlen; j++){
        dictionary[i][j] = newStr[j];
    }
}

void decompress(FILE *fptIn, FILE *fptOut){
    int i;
    unsigned char **dictionary;
    int *lengths;
    unsigned short c, p;
    unsigned char *x, *y, *z;
    int xlen, ylen, zlen;

    dictionary = (unsigned char **) malloc(sizeof(unsigned char *)*BIGGERNUM);
    lengths = (int *) calloc(BIGGERNUM, sizeof(int));
    for(i=0; i<BIGGERNUM; i++){
        dictionary[i] = (unsigned char *) malloc(sizeof(unsigned char)*BIGNUM);
    }

    //initialize dictionary with all roots
    for(i=0; i<256; i++){
        dictionary[i][0] = (unsigned char) i;
        lengths[i]=1;
    }

    x = (unsigned char *) calloc(BIGNUM, sizeof(unsigned char));
    y = (unsigned char *) calloc(BIGNUM, sizeof(unsigned char));
    z = (unsigned char *) calloc(BIGNUM, sizeof(unsigned char));

    //start
    fread(&c, sizeof(short), 1, fptIn);
    codeInDictionary(dictionary, lengths, c, x, &xlen);
    fwrite(&c, 1, 1, fptOut);
    p=c;
    while(fread(&c, sizeof(short), 1, fptIn) == 1){
        if(codeInDictionary(dictionary, lengths, c, x, &xlen) == 0){
            //not in dictionary
            //let x=pattern for p
            codeInDictionary(dictionary, lengths, p, x, &xlen);

            //let z =1st char of pattern for p
            zlen = 1;
            z[0] = x[0]; //let z = 1st char of pattern for p

            //outputCode for X+Z
            outputCodes(fptOut, x, xlen, z, zlen);

            //add X+Z to dictionary
            x[xlen] = z[0];
            addCodeDictionary(dictionary, lengths, x, xlen+1);
        } else {
            //c in dictionary
            //let x = pattern for p
            codeInDictionary(dictionary, lengths, p, x, &xlen);

            //let y= (1st char) of pattern for c
            codeInDictionary(dictionary, lengths, c, y, &ylen);

            //output pattern for c
            outputCodes(fptOut, y, ylen, NULL, 0);

            //make y just first char
            ylen = 1;

            //add X+Y to the dictionary
            x[xlen] = y[0];
            addCodeDictionary(dictionary, lengths, x, xlen+1);
        }
        p=c;
    }

}

//return 0 if not in dictionary
//returns 1 if is, and pattern is the pattern
int codeInDictionary(unsigned char **dictionary, int *lengths, short code, unsigned char *patt, int *plen){
    int i;
    if(lengths[code] == 0) return 0;

    for(i=0; i<lengths[code]; i++){
        patt[i] = dictionary[code][i];
    }
    *plen = lengths[code];
    return 1;
}

void outputCodes(FILE *fptOut, unsigned char *c1, int l1, unsigned char *c2, int l2){
    int i;
    for(i=0; i<l1; i++) fwrite((c1+i), 1, 1, fptOut);

    for(i=0; i<l2; i++) fwrite((c2+i), 1, 1, fptOut);
}

void addCodeDictionary(unsigned char **dictionary, int *lengths,unsigned char *string, int slen ){
    int j=0;
    while(lengths[j++]!=0);
    j--;

    int i;
    for(i=0; i<slen; i++){
        dictionary[j][i] = string[i];
    }
    lengths[j]=slen;
}
