#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ROOT 0

#define MATRIX_SMOOTH  {{1.0 / 9, 1.0 / 9, 1.0 / 9}, {1.0 / 9, 1.0 / 9, 1.0 / 9}, {1.0 / 9, 1.0 / 9, 1.0 / 9}}
#define MATRIX_EMBOSS  {{0, 1, 0}, {0, 0, 0}, {0, -1, 0}}
#define MATRIX_BLUR    {{1.0 / 16, 2.0 / 16, 1.0 / 16}, {2.0 / 16, 4.0 / 16, 2.0 / 16}, {1.0 / 16, 2.0 / 16, 1.0 / 16}}
#define MATRIX_SHARPEN {{0, -2.0 / 3, 0}, {-2.0 / 3, 11.0 / 3, -2.0 / 3}, {0, -2.0 / 3, 0}}
#define MATRIX_MEAN    {{-1, -1, -1}, {-1, 9, -1}, {-1, -1, -1}}

typedef struct {
	unsigned char value[3];
} pixel;

// Structura unei imagini PGM sau PNM
typedef struct {
	unsigned char file_type;  // 5 pentru PGM sau 6 pentru PNM
	int width;			      // latimea imaginii
	int height;    			  // inaltimea imaginii
	unsigned char max_value;  // valoare maxima a unui pixel
	unsigned char num_colors; // numarul de culori dintr-un pixel

	pixel** pixels;  // matrice de pixeli, fiecare cu cate num_colors culori
} image;

// Primeste un file pointer si intoarce
// primul numar gasit pana la un spatiu alb,
// citindu-l si pe acela
int read_next_int(FILE** file) {
	char c;
	int number = 0;

	fread(&c, sizeof(char), 1, *file);

	while (c >= '0' && c <= '9') {
		number = number * 10 + c - 48;
		fread(&c, sizeof(char), 1, *file);
	}

	return number;
}

// Afla numarul de culori in functie
// de formatul imaginii
void set_num_colors(image* img) {
	switch ((*img).file_type) {
		case 5:
			(*img).num_colors = 1;
			break;

		case 6:
			(*img).num_colors = 3;
			break;
	}
}

// Citeste o imagine de tip P5 sau P6
void readInput(const char* file_name, image* img) {

	// Deschidere fisier binar
	FILE* file = fopen(file_name, "rb");

	// Skip litera P
	fseek(file, 1, SEEK_SET);

	(*img).file_type =  read_next_int(&file);
	(*img).width     =  read_next_int(&file);
	(*img).height    =  read_next_int(&file);
	(*img).max_value =  read_next_int(&file);

	set_num_colors(img);

	int width      = (*img).width;
	int height     = (*img).height;
	int num_colors = (*img).num_colors;

	(*img).pixels = (pixel**) malloc(sizeof(pixel*) * height);
	int i, j;

	// Citire pixel cu pixel
	for (i = 0; i < height; ++i) {
		(*img).pixels[i] = (pixel*) malloc(sizeof(pixel) * width);
		
		if (num_colors == 3) {
			fread(&(*img).pixels[i][0], width * num_colors, 1, file);
		} else {
			for (j = 0; j < width; ++j) {
				fread(&(*img).pixels[i][j].value, num_colors, 1, file);
			}
		}
	}
	fclose(file);
}

// Primeste un numar si scrie numarul
// intr-un fisier binar, cifra cu cifra
void write_int(FILE** file, int number) {
	
	int final_zero = 0;

	// Reverse number
	int aux = 0;
	while (number != 0) {
		aux = aux * 10 + number % 10;
		number /= 10;

		if (aux == 0) {
			final_zero++;
		}
	}
	number = aux;

	// Scriere cifra cu cifra in fisier
	while (number != 0) {
		unsigned char digit = number % 10 + 48;
		number /= 10;

		fwrite(&digit, 1, sizeof(char), *file);
	}

	while (final_zero != 0) {
		fwrite("0", 1, sizeof(char), *file);
		final_zero--;
	}
}

// Scrie intr-un fisier binar o imagine
// de tip P5 sau P6
void writeData(const char* file_name, image* img) {
	FILE* file = fopen(file_name, "wb");

	fwrite("P", 1, sizeof(char), file);  // Litera P
	write_int(&file, (*img).file_type);  // Tip 5 sau 6
	fwrite("\n", 1, sizeof(char), file); // new line
	
	write_int(&file, (*img).width);      // width
	fwrite(" ", 1, sizeof(char), file);  // space
	write_int(&file, (*img).height);     // height
	fwrite("\n", 1, sizeof(char), file); // new line

	write_int(&file, (*img).max_value);  // max_value
	fwrite("\n", 1, sizeof(char), file); // new line

	int height     = (*img).height;
	int width      = (*img).width;
	int num_colors = (*img).num_colors;

	// Scriere pixel cu pixel
	for (int i = 0; i < height; ++i) {
		if (num_colors == 3) {
			fwrite(&(*img).pixels[i][0], 1, width * num_colors, file);
		} else {
			for (int j = 0; j < width; ++j) {
				fwrite(&(*img).pixels[i][j], 1, num_colors, file);
			}
		}
	}

	fclose(file);
}

// Functia pentru initializare MPI
void init_MPI(int* argc, char** argv[], int* rank, int* nProcesses) {
	MPI_Init(&(*argc), &(*argv));
	MPI_Comm_rank(MPI_COMM_WORLD, &(*rank));
	MPI_Comm_size(MPI_COMM_WORLD, &(*nProcesses));
}

// Functia pentru inchidere MPI
void finalize_MPI() {
	MPI_Finalize();
}

void apply_filter_helper(image *in, image *out, int start, int stop, float matrix[3][3]) {
	// Pentru fiecare linie, coloana si culoare, calculez noua culoare
	for (int i = start; i < stop; i++) {
		for (int j = 0; j < (*in).width; j++) {
			for (int k = 0; k < 3; k++) {

				// Daca este pe margine, il las nemodificat
				if (i == 0 || i == (*in).height - 1 || j == 0 || j == (*in).width - 1) {
					(*out).pixels[i][j].value[k] = (*in).pixels[i][j].value[k];
				} else {
					float sum = 0;

					for (int offseti = -1; offseti <= 1; offseti++) {
						for (int offsetj = -1; offsetj <= 1; offsetj++) {
							sum += (*in).pixels[i + offseti][j + offsetj].value[k] * matrix[offseti + 1][offsetj + 1];
						}
					}
					(*out).pixels[i][j].value[k] = sum;
				}
			}
		}
	}
}

void alloc(image *in, image* out) {
	// Copiere header
	(*out).file_type  = (*in).file_type;
	(*out).width      = (*in).width;
	(*out).height     = (*in).height;
	(*out).max_value  = (*in).max_value;
	(*out).num_colors = (*in).num_colors;

	int width  = (*out).width;
	int height = (*out).height;

	(*out).pixels = (pixel**) malloc(sizeof(pixel*) * height);

	for (int i = 0; i < height; ++i) {
		(*out).pixels[i] = (pixel*) malloc(sizeof(pixel) * width);
	}
}

// Functia aplica un filtru pe imaginea in
void apply_filter(image* in, image* out, char *filter_name, int start, int stop) {
	if (strcmp(filter_name, "smooth") == 0) {
		float matrix[3][3] = MATRIX_SMOOTH;
		apply_filter_helper(&(*in), &(*out), start, stop, matrix);
	} else if (strcmp(filter_name, "emboss") == 0) {
		float matrix[3][3] = MATRIX_EMBOSS;
		apply_filter_helper(&(*in), &(*out), start, stop, matrix);
	} else if (strcmp(filter_name, "blur") == 0) {
		float matrix[3][3] = MATRIX_BLUR;
		apply_filter_helper(&(*in), &(*out), start, stop, matrix);
	} else if (strcmp(filter_name, "sharpen") == 0) {
		float matrix[3][3] = MATRIX_SHARPEN;
		apply_filter_helper(&(*in), &(*out), start, stop, matrix);
	} else if (strcmp(filter_name, "mean") == 0) {
		float matrix[3][3] = MATRIX_MEAN;
		apply_filter_helper(&(*in), &(*out), start, stop, matrix);
	}
}

int main(int argc, char* argv[]) {
	int rank;
	int nProcesses;
	MPI_Status status;
	init_MPI(&argc, &argv, &rank, &nProcesses);

	image in;
	image out;

	// Citesc imagine de catre ROOT
	if (rank == ROOT) {
		readInput(argv[1], &in);
	}

	// Trimitire si primire imaginea 'in' broadcast
	MPI_Bcast(&in.file_type,  1, MPI_CHAR, ROOT, MPI_COMM_WORLD);
	MPI_Bcast(&in.width,      1, MPI_INT,  ROOT, MPI_COMM_WORLD);
	MPI_Bcast(&in.height,     1, MPI_INT,  ROOT, MPI_COMM_WORLD);
	MPI_Bcast(&in.max_value,  1, MPI_CHAR, ROOT, MPI_COMM_WORLD);
	MPI_Bcast(&in.num_colors, 1, MPI_CHAR, ROOT, MPI_COMM_WORLD);

	// Alocare memorie poza pentru pentru fiecare in
	if (rank != ROOT) {
		int width  = in.width;
		int height = in.height;

		in.pixels = (pixel**) malloc(sizeof(pixel*) * height);

		for (int i = 0; i < height; ++i) {
			in.pixels[i] = (pixel*) malloc(sizeof(pixel) * width);
		}
	}

	// Alocare memorie pentru fiecare out
	alloc(&in, &out);

	// Aplic fiecare filtru
	for (int filter_index = 3; filter_index < argc; filter_index++) {
		
		// ROOT trimite poza broadcast linie cu linie
		for (int i = 0; i < in.height; i++) {
			MPI_Bcast(&in.pixels[i][0], 3 * in.width, MPI_CHAR, ROOT, MPI_COMM_WORLD);
		}

		// Fiecare prelucreaza poza
		int start = (in.height / nProcesses) * rank;
		int stop  = (in.height / nProcesses) * (rank + 1);

		if (rank == nProcesses - 1) {
			stop = in.height;
		}
		
		apply_filter(&in, &out, argv[filter_index], start, stop);

		// Fiecare trimite la ROOT poza cu filtrul creat
		for (int line = start; line < stop; line++) {
			MPI_Send(&out.pixels[line][0], 3 * in.width, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
		}

		// ROOT primeste toate partile prelucrate
		if (rank == ROOT) {
			for (int r = 0; r < nProcesses; r++) {
				int start = (in.height / nProcesses) * r;
				int stop  = (in.height / nProcesses) * (r + 1);

				if (r == nProcesses - 1) {
					stop = in.height;
				}

				for (int line = start; line < stop; line++) {
					MPI_Recv(&in.pixels[line][0], 3 * in.width, MPI_CHAR, r, 0, MPI_COMM_WORLD, &status);
				}
			}
		}
	}

	// Scriu imaginea finala in fisier
	if (rank == ROOT) {
		writeData(argv[2], &in);
	}
	
	finalize_MPI();
	return 0;
}
