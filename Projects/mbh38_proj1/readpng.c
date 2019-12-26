#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

// read the assignment instructions about this!
#define MAX_REASONABLE_TEXT_CHUNK_SIZE 1024

// the PNG file signature.
const char PNG_SIGNATURE[8] = {137, 'P', 'N', 'G', '\r', '\n', 26, '\n'};
const int SIZE_OF_CRC_SECTION = 4;

// see if two strings are equal.
bool streq(const char* a, const char* b) { return strcmp(a, b) == 0; }

// see if the first n characters of two strings are equal.
bool strneq(const char* a, const char* b, int n) { return strncmp(a, b, n) == 0; }

// given a chunk's identifier and a type, sees if they're equal. use like:
//    <read a chunk's length and identifier>
//    if(is_chunk(identifier, "IHDR"))
bool is_chunk(const char* identifier, const char* type) {
	return strneq(identifier, type, 4);
}

// byte-swaps a 32-bit integer.
unsigned int bswap32(unsigned int x) {
	return
		((x & 0xFF) << 24) |
		((x & 0xFF00) << 8) |
		((x & 0xFF0000) >> 8) |
		((x & 0xFF000000) >> 24);
}

// ------------------------------------------------------------------------------------------------
// Commands (this is what you'll implement!!!!)
// ------------------------------------------------------------------------------------------------

typedef enum{
	Grayscale = 0;
	RGB = 2;
	Indexed = 3;
	GrayscaleAlpha = 4;
	RGBAlpha = 6;
}

typedef struct {
	unsigned int length;
	char identifier[4];
} chunkHeader;

typedef struct {
	unsigned int width;
	unsigned int height;
	char bitDepth;
	char colorType;
	char compression;
	char filtering;
	char interlaced;
} chunk;


FILE* openFile(const char* fileName){
	FILE* f = fopen(fileName, "rb"); //open fileName as a "read binary" file
	if(f == NULL) { //throw error if unsuccessful
		printf("ERROR: Could not open file.\n");
		exit(1);
	}
	char sig[8]; 
	fread(sig, sizeof(char), 8, f); //read the signature of the file into sig array
	
	//the first 8 bytes of the file don't match the PNG signature
	if(!(strneq(sig, PNG_SIGNATURE, 8))){
		printf("ERROR: Not a valid .png file.\n");
		exit(1);
	}
	return f; //if this line executes, it is a valid .png file
}

chunkHeader readHeader(FILE* file){
	chunkHeader header; //stores the info for the chunk header
	fread(&header.length, sizeof(header.length), 1, file); 
	header.length = bswap32(header.length);
	fread(header.identifier, sizeof(char), 4, file);
	return header;
}

chunk readChunk(FILE* file){
	chunk c;
	fread(&c, 13, 1, file); //reads next 13 bytes into c which is a chunk struct
	c.width = bswap32(c.width);
	c.height = bswap32(c.height);
	return c;
}

//prints the color type
void printColorType(char type){
	printf("\tColor Type: ");
	switch(type){
		case Grayscale: printf("Grayscale\n"); break;
		case RGB: printf("RGB\n"); break;
		case Indexed: printf("Indexed\n"); break;
		case GrayscaleAlpha: printf("Grayscale + Alpha\n"); break;
		case RGBAlpha: printf("RGB + Alpha\n"); break;
		default: printf("Invalid Color Type.");
	}
}

void printInterlaced(char type){
	printf("\tInterlaced: ");
	if(type == 0) printf("no\n");
	else printf("yes\n");
}

//prints error and exits if the length passed in is longer than the max reasonable length
void checkLength(int len){
	if(len > MAX_REASONABLE_TEXT_CHUNK_SIZE){
		printf("ERROR: Too big of a text chunk.");
		exit(1);
	}
}

//allocates space for the text chunk that we need to print and prints it
void printText(FILE* file, chunkHeader header){
	char* text = calloc(sizeof(char), header.length+1); //{name}'\0'{data}'\0'
	fread(text, sizeof(char), header.length, file); //read the whole chunk into memory
	text[header.length] = '\0'; //put a null terminator at the end so it knows where to stop.

	int nameLength = strlen(text);
	printf("%s:\n", text); //print the name section of the text chunk
	printf("\t%s\n\n", text+nameLength+1); //print the value section of the text chunk
	fseek(file, SIZE_OF_CRC_SECTION, SEEK_CUR); //skip the final section of the chunk to put the file pointer at the start of the next chunk
	free(text);

	// This works too, but isn't lovely:
	// text = text + nameLength + 1; //start of the value
	// printf("\t%s\n\n", text);
	// fseek(file, 4, SEEK_CUR); //skip the final section of the chunk to put the file pointer at the start of the next chunk
	// free(text - nameLength - 1);
}

//prints the info of the .png file
void show_info(const char* filename) {
	FILE* file = openFile(filename);
	readHeader(file); //moves file pointer to start of the info section
	chunk info = readChunk(file); //reads info from file into chunk struct called info
	printf("File info: \n");
	printf("\tDimensions: %u x %u\n", info.width, info.height);
	printf("\tBit Depth: %d\n", info.bitDepth);
	printColorType(info.colorType);
	printInterlaced(info.interlaced);
	fclose(file);
}

void dump_chunks(const char* filename) {
	FILE* file = openFile(filename);
	chunkHeader header;
	do{
		header = readHeader(file);
		printf("'%.4s' (length = %d)\n", header.identifier, header.length);
		fseek(file, header.length + SIZE_OF_CRC_SECTION, SEEK_CUR);
	} while(!strneq(header.identifier, "IEND", 4));
	fclose(file);
}

void show_text(const char* filename) {
	FILE* file = openFile(filename);
	chunkHeader header;
	while(1){
		header = readHeader(file);
		//if the header for the current chunk is "IEND", we're at the end of the file, so break
		if (strneq(header.identifier, "IEND", 4)) break;
		//if the header for the current chunk is not "tEXt", we do nothing and skip ahead to the next header.
		if (!(strneq(header.identifier, "tEXt", 4))){
			fseek(file, header.length + SIZE_OF_CRC_SECTION, SEEK_CUR); 
		}
		else{ //we have a chunk of text!
			checkLength(header.length); //if the length of the text segment is too long, the program exits
			printText(file, header);			
		}
	}
	fclose(file);
}

// ------------------------------------------------------------------------------------------------
// Argument parsing
// ------------------------------------------------------------------------------------------------

typedef enum {
	Info,
	DumpChunks,
	Text,
} Mode;

typedef struct {
	const char* input;
	Mode mode;
} Arguments;

void show_usage_and_exit(const char* reason) {
	if(reason) {
		printf("Error: %s\n", reason);
	}

	printf("Usage:\n");
	printf("  ./pngedit input.png [command]\n");
	printf("Commands:\n");
	printf("  (no command)         displays PNG file info.\n");
	printf("  dump                 dump all chunks in the file.\n");
	printf("  text                 show all text chunks in the file.\n");
	exit(1);
}

Arguments parse_arguments(int argc, const char** argv) {
	Arguments ret = {};

	switch(argc) {
		case 1: show_usage_and_exit(NULL);
		case 2: ret.mode = Info; break;
		case 3: {
			if(streq(argv[2], "dump")) {
				ret.mode = DumpChunks;
			} else if(streq(argv[2], "text")) {
				ret.mode = Text;
			} else {
				show_usage_and_exit("Invalid command.\n");
			}
			break;
		}
		default: show_usage_and_exit("Invalid arguments.");
	}

	// if we get here, argv[1] is valid.
	ret.input = argv[1];
	return ret;
}

int main(int argc, const char** argv) {
	Arguments args = parse_arguments(argc, argv);

	switch(args.mode) {
		case Info:       show_info(args.input);   break;
		case DumpChunks: dump_chunks(args.input); break;
		case Text:       show_text(args.input);   break;
		default:
			printf("well this should never happen!\n");
			return 1;
	}

	return 0;
}