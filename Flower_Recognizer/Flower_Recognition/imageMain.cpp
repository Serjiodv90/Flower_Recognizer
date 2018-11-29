#pragma warning(disable:4996)

#include "GLUT.H"
#include <math.h>
#include <time.h>
#include <stdio.h>
#include <Windows.h>
#include <string>
#include "dirent.h"
#include <vector>

using namespace std;


const int W = 600; // window width
const int H = 600; // window height

const int TMPSZ = 512;
const int SCRSZ = 256;	//matrix size
const int INPUT_SZ = SCRSZ + 1;		//+ 1 is for the bias neuron
const int HIDDEN_SZ = INPUT_SZ / 2 + 1;
const int OUTPUT_SZ = 3;	// 3 types of flowers

unsigned char originalImage[SCRSZ][SCRSZ][3]; //3- RGB, display of the original image in the top square
unsigned char imageContour[SCRSZ][SCRSZ][3]; //3- RGB, contour of the flower image (in black and white) - the lower square
unsigned char picture[TMPSZ][TMPSZ][3];		//temporary array for loading the 512 pixels image, and the rendering it to 256 pixels 'originalImage' array

double input[INPUT_SZ];	//for neuron network
double hidden[HIDDEN_SZ];
double output[OUTPUT_SZ];

//weight matrices
double input_to_hidden[INPUT_SZ][HIDDEN_SZ];	//weights matrix from input to hidden layer
double hidden_to_output[HIDDEN_SZ][OUTPUT_SZ];

double error[OUTPUT_SZ];
double delta_output[OUTPUT_SZ];
double delta_hidden[HIDDEN_SZ];
double learning_rate = 0.1;	//the rate in which the network will learn new digit and forget the previous, thus the higher the rate the faster the network will learn new but faster forget old

int network_digit = -1, tutor_digit = -1;	// tutor_digit is for correcting the output by clicking the number

unsigned char* bmp;	//each cell contains and RGB pixel of 1 byte

string flowerNames[] = { "chrysantemum", "rose", "tulip" };
char flowersFileNames[] = { 'c', 'r', 't' };
unsigned char trainingImagesChosen[18] = { 0 };
int straightHits = 0;
vector <string> testImagesSet;



const char* trainigPathImage = "training set\\";
const char* testPathImage = "test images\\";


const char chrysantemumFileName = 'c';
const char roseFileName = 'r';
const char tulipFileName = 't';

FILE* pf;
BITMAPFILEHEADER bf;
BITMAPINFOHEADER bi;

const char* LOG_FILE = "log.txt";
FILE* logFile;
const char* WEIGHTS_FILE_OUTPUT = "training_weights.txt";
FILE* weightsFile;

const double square_space = 0.1;
const double square_top = 0.25;
const double square_left = 0.3;

void training();
void clean();
void createMenu(int choice);





//load the 24-bitmap image
void loadBitmap(const char* filename)
{
	int sz;

	pf = fopen(filename, "rb");	//read binary fie

	fread(&bf, sizeof(bf), 1, pf);
	fread(&bi, sizeof(bi), 1, pf);
	sz = bi.biHeight * bi.biWidth * 3;	//3 colors - RGB

	if (bmp)
		free(bmp);

	bmp = (unsigned char*)malloc(sz);

	fread(bmp, 1, sz, pf);
	fclose(pf);

}

void renderImage512To256()
{
	int i, j, sz = TMPSZ * TMPSZ * 3, k;

	for (k = 0, j = 0, i = 0; k < sz; k += 3)
	{
		picture[i][j][2] = bmp[k];		//blue
		picture[i][j][1] = bmp[k + 1];	//green
		picture[i][j][0] = bmp[k + 2];	//red
		j++;

		//fill the next line
		if (j == TMPSZ)
		{
			j = 0;
			i++;
		}
	}

	//	copy picture to screen
	for (i = 0; i < SCRSZ; i++)
	{
		for (j = 0; j < SCRSZ; j++)
		{
			originalImage[i][j][0] = (picture[i * 2][j * 2][0] + picture[i * 2][j * 2 + 1][0] + picture[i * 2 + 1][j * 2][0] + picture[i * 2 + 1][j * 2 + 1][0]) / 4;
			originalImage[i][j][1] = (picture[i * 2][j * 2][1] + picture[i * 2][j * 2 + 1][1] + picture[i * 2 + 1][j * 2][1] + picture[i * 2 + 1][j * 2 + 1][1]) / 4;
			originalImage[i][j][2] = (picture[i * 2][j * 2][2] + picture[i * 2][j * 2 + 1][2] + picture[i * 2 + 1][j * 2][2] + picture[i * 2 + 1][j * 2 + 1][2]) / 4;

		}
	}
}

void writeWeightToFile()
{
	weightsFile = fopen(WEIGHTS_FILE_OUTPUT, "w");

	for (int i = 0; i < INPUT_SZ; i++)
		for (int j = 0; j < HIDDEN_SZ; j++)
			fprintf(weightsFile, "%lf\n", input_to_hidden[i][j]);

	for (int i = 0; i < HIDDEN_SZ; i++)
		for (int j = 0; j < OUTPUT_SZ; j++)
			fprintf(weightsFile, "%lf\n", hidden_to_output[i][j]);

	fclose(weightsFile);
}

void readWeightFromFile()
{
	int i, j;
	double tmpWeight = -1.0;
	fseek(weightsFile, 0, SEEK_SET);

	for (i = 0; i < INPUT_SZ; i++)
		for (j = 0; j < HIDDEN_SZ; j++)
		{
			fscanf(weightsFile, "%lf", &tmpWeight);
			input_to_hidden[i][j] = tmpWeight;
		}

	tmpWeight = -1;
	for (i = 0; i < HIDDEN_SZ; i++)
		for (j = 0; j < OUTPUT_SZ; j++)
		{
			fscanf(weightsFile, "%lf", &tmpWeight);
			hidden_to_output[i][j] = tmpWeight;
		}
}

void scanTestFolder()
{
	struct dirent *testImage;  // Pointer for directory entry 

	// opendir() returns a pointer of DIR type.  
	DIR *testDir = opendir("test images");

	if (testDir == NULL)  // opendir returns NULL if couldn't open directory 
		printf("Could not open current directory");

	while ((testImage = readdir(testDir)) != NULL)
	{
		if (strstr(testImage->d_name, ".bmp") != NULL)
		{
			string name = testImage->d_name;
			testImagesSet.push_back(name);
			printf("%s\n", testImage->d_name);
		}
	}

	closedir(testDir);

}

void addTestImagesToMenu()
{

	glutCreateMenu(createMenu);
	for (int i = 0; i < testImagesSet.size(); i++)
	{
		printf("the image: %s\n", testImagesSet[i].c_str());
		glutAddMenuEntry(testImagesSet[i].c_str(), i + 1);
	}
	glutAttachMenu(GLUT_RIGHT_BUTTON);
}

void init()
{

	weightsFile = fopen(WEIGHTS_FILE_OUTPUT, "r");

	int i, j, sz = TMPSZ * TMPSZ * 3, k;
	srand(time(0));

	if (weightsFile)
	{
		printf("/*****************************************************************/\n\t\tREADING FROM FILE\n\n");
		fseek(weightsFile, 0, SEEK_END);
		if (0 == ftell(weightsFile))	//if the file of weights is empty, randomize the weight (for the first time)
		{
			//initiallize input_to_hidden matrix with randomize weights
			for (i = 0; i < INPUT_SZ; i++)
				for (j = 0; j < HIDDEN_SZ; j++)
					input_to_hidden[i][j] = ((rand() % 1000) - 500) / 1000.0;	//-0.5 ~ 0.5

			//initiallize input_to_hidden matrix with randomize weights
			for (i = 0; i < HIDDEN_SZ; i++)
				for (j = 0; j < OUTPUT_SZ; j++)
					hidden_to_output[i][j] = ((rand() % 1000) - 500) / 1000.0;	//-0.5 ~ 0.5
		}
		else
			readWeightFromFile();

		fclose(weightsFile);

	}


	training();

	clean();

	scanTestFolder();

	addTestImagesToMenu();

	glClearColor(0.3, 0.3, 0.3, 0);

	glOrtho(-1, 1, -1, 1, -1, 1);
}

void DrawSquares()
{
	double space = square_space ;
	double top = square_top;
	double left = square_left;

	double right = left + space;
	double bottom = top - space;


	for (int i = 0; i < OUTPUT_SZ; i++)
	{
		if (tutor_digit == i)
		glColor3d(0, 1, 0);
		else if (network_digit == i)
		glColor3d(1, 0, 0);
		else
		glColor3d(0.5, 0.5, 0.5);

		glBegin(GL_POLYGON);
		glVertex2d(left, top);
		glVertex2d(right, top);
		glVertex2d(right, bottom);
		glVertex2d(left, bottom);
		glEnd();

		//flowers
		glColor3d(1, 1, 1);
		glRasterPos2d(right + 0.02, bottom + 0.02);

		for (int k = 0; k < flowerNames[i].length(); k++)
			glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, (int)(flowerNames[i][k]));

		top -= (space * 2);
		bottom = (top - space);
	}
}


void display()
{
	glClear(GL_COLOR_BUFFER_BIT);

	DrawSquares();


	//show screen
	glRasterPos2d(-0.95, 0);
	glDrawPixels(SCRSZ, SCRSZ, GL_RGB, GL_UNSIGNED_BYTE, originalImage);
	//show squres
	glRasterPos2d(-0.95, -0.95);
	glDrawPixels(SCRSZ, SCRSZ, GL_RGB, GL_UNSIGNED_BYTE, imageContour);
	glutSwapBuffers();// show what was drawn in "frame buffer"
}

void idle()
{
	glutPostRedisplay();// calls indirectly to display
}


void HPF()
{
	int i, j;
	double contrast;

	for (i = 1; i < SCRSZ - 1; i++)
		for (j = 1; j < SCRSZ - 1; j++)
		{
			// filtering the original image, to get the contour lines of the image, with high pass filter laplasian
			contrast = (int)fabs(4 * originalImage[i][j][0] - originalImage[i - 1][j][0] - originalImage[i + 1][j][0] - originalImage[i][j - 1][0] - originalImage[i][j + 1][0]);
			if (contrast > 20)
				imageContour[i][j][0] = imageContour[i][j][1] = imageContour[i][j][2] = 255;
			else
				imageContour[i][j][0] = imageContour[i][j][1] = imageContour[i][j][2] = 0;
		}
}



//return the maximum value in the output array, so it's the digid that have been drawed
int MaxOutput()
{
	int i, max = 0;
	for (i = 1; i < OUTPUT_SZ; i++)
		if (output[i] > output[max])
			max = i;
	return max;
}

void clean()
{
	int i, j;
	for (i = 0; i < SCRSZ; i++)
		for (j = 0; j < SCRSZ; j++)
		{	//paint the background in white (255,255,255)
			originalImage[i][j][0] = 255;	//red 
			originalImage[i][j][1] = 255;	//green
			originalImage[i][j][2] = 255;	//blue

		}

	for (i = 0; i < SCRSZ; i++)
		for (j = 0; j < SCRSZ; j++)
		{	//paint the background in white (255,255,255)
			imageContour[i][j][0] = 255;	//red 
			imageContour[i][j][1] = 255;	//green
			imageContour[i][j][2] = 255;	//blue
		}

	network_digit = -1;
	tutor_digit = -1;
}

//input --> hidden layer --> output layer
void FeedForward()
{
	int i, j;

	//1. setup input layer, all the newrons in that layer

	// every 'pixel' in the lower squre is 10X10 pixels, so if the 'first' pixel is black, the also the next 9 pixels will be black
	//and every step check if the next 10X10 pixel in the lower square is black, if it is, mark in the input array the correct cell
	//every newron is connected to a 10X10 cell, wich representing a single pixel
	//the print in the console will be inversed than the input draw, beacuse of the axis y of the computer is from top to bottom
	

	for (i = 8; i < SCRSZ; i += 16)
		for (j = 8; j < SCRSZ; j += 16)
		{

			if (imageContour[i][j][0] == 0)
				input[(i / 16) * 16 + (j / 16)] = 0;
			else
				input[(i / 16) * 16 + (j / 16)] = 1;
		}

	input[INPUT_SZ - 1] = 1;	//bias newron of the input layer

	//2. getting hidden layer
	for (i = 0; i < HIDDEN_SZ; i++)
		hidden[i] = 0;

	for (i = 0; i < INPUT_SZ; i++)
		for (j = 0; j < HIDDEN_SZ; j++)
		{
			hidden[j] += input[i] * input_to_hidden[i][j];	//multiply the input value on the weight of that edge
		}

	//add sigmoid to hidden layer
	for (i = 0; i < HIDDEN_SZ; i++)
		hidden[i] = 1 / (1 + exp(-hidden[i]));	//the filter function - sigmoid, make the weights under than 1

	hidden[HIDDEN_SZ - 1] = 1;	//bias newron of the hidden layer

	//3. getting output layer
	for (i = 0; i < OUTPUT_SZ; i++)
		output[i] = 0;

	for (i = 0; i < HIDDEN_SZ; i++)
		for (j = 0; j < OUTPUT_SZ; j++)
		{
			output[j] += hidden[i] * hidden_to_output[i][j];	//multiply the input value on the weight of that edge
		}

	//add sigmoid to output
	for (i = 0; i < OUTPUT_SZ; i++)
		output[i] = 1 / (1 + exp(-output[i]));

	//show
	fprintf(logFile, "\n\nOUTPUT:\n");
	printf("##########################\nOUTPUT:\n");
	for (i = 0; i < OUTPUT_SZ; i++)
	{
		fprintf(logFile, "%.3lf ", output[i]);	//the value that is the closest to 1, it's index is the newron that represents a digid
		printf("%lf\n", output[i]);
	}
	fprintf(logFile, "\n\n");
	printf("##########################\n\n");

	network_digit = MaxOutput();

	/*if (network_digit != tutor_digit)
		learning_rate =(learning_rate * 0.3);
	else
		learning_rate *= 3;*/

}

//according to the correct number that was clicked, that index in the output arr should be stronger 
//target vector is the vector that contains the numbers 0-9 that we should click, when we want to teach the newron network, so the clicked number will be 1 and the other numbers will be 0
void backPropogation()
{
	int i, j, k;
	//1. compute the error between in correct number and the newron output. E = (t(i)-y(i)), t - target vector, y - output layer
	for (i = 0; i < OUTPUT_SZ; i++)
	{
		if (i == tutor_digit)
			error[i] = (output[i] - 1);//(1 - output[i]);//output[i] - 1
		else
			error[i] = output[i];//-output[i];//output[i] - wihout minus
	}

	//2. compute delta of output layer
	//output[i] - is the output of newron i
	for (i = 0; i < OUTPUT_SZ; i++)
		delta_output[i] = output[i] * (1 - output[i]) * error[i];

	//3. compute delta of hidden layer
	for (j = 0; j < HIDDEN_SZ; j++)
	{
		double tmp = 0;
		//scan all the edges that come out from newron j, for every out 
		for (k = 0; k < OUTPUT_SZ; k++)
			tmp += delta_output[k] * hidden_to_output[j][k];

		delta_hidden[j] = hidden[j] * (1 - hidden[j]) * tmp;
	}

	//4. update weights in hidden to output
	//from newron i in the hidden layer, to newron j in the output layer
	for (i = 0; i < HIDDEN_SZ; i++)
		for (j = 0; j < OUTPUT_SZ; j++)
			hidden_to_output[i][j] -= learning_rate * hidden[i] * delta_output[j];


	//5. update weight in input to hidden
	for (i = 0; i < INPUT_SZ; i++)
		for (j = 0; j < HIDDEN_SZ; j++)
			input_to_hidden[i][j] -= learning_rate * input[i] * delta_hidden[j];


}

void training()
{
	int training_numOfIterations = 1000;
	int numOfPicsInSet = 6;

	srand(time(0));
	int pictureNumber, flowerType;		// flower type is the number of flower in the flowersFileNames array
	string imagePath;
	int sz = TMPSZ * TMPSZ * 3, k, j, l, i;
	logFile = fopen(LOG_FILE, "w");
	int counter = 0;

	for (l = 0; l < training_numOfIterations && straightHits < 18; l++)
	{
		printf("L #: %d\n", l);
		flowerType = rand() % 3;	//0 ~ 2
		pictureNumber = (rand() % numOfPicsInSet) + 1;	// 1 ~ 6
		i = (flowerType * 6) + pictureNumber - 1;
		int  localIteration;

		//iterate through all the 18 images
		for (localIteration = 0; localIteration < 18; localIteration++)
		{

			trainingImagesChosen[i] = 1;

			imagePath = trainigPathImage;
			imagePath += flowersFileNames[flowerType];
			imagePath += to_string(pictureNumber);
			imagePath += ".bmp";

			fprintf(logFile, "Iteration #: %d\n"
				"chosen flower type: %d,\tchosen picture #: %d,\timage path: %s\n", l, flowerType, pictureNumber, imagePath.c_str());

			loadBitmap(imagePath.c_str());
			renderImage512To256();
			HPF();
			tutor_digit = flowerType;

		
			FeedForward();
			backPropogation();

			if (network_digit == tutor_digit)
			{
				counter++;
				straightHits++;
			}
			else
				straightHits = 0;
			

			//keep choosing image until you find an unvisited image out of the 18 in the training set
			while (trainingImagesChosen[i] != 0 && localIteration < 17)
			{
				//printf("trainingImagesChosen[%d]: %d, \tlocal it: %d\n", i, trainingImagesChosen[i], localIteration);
				flowerType = rand() % 3;	//0 ~ 2
				pictureNumber = (rand() % numOfPicsInSet) + 1;	// 1 ~ 6
				i = (flowerType * 6) + pictureNumber - 1;
			}
		}

		for (int j = 0; j < 18; j++)
			trainingImagesChosen[j] = 0;

		printf("Straight hits : %d\n\n", straightHits);
	}

	fclose(logFile);
	writeWeightToFile();

	printf("The number of pc's correct guesses is: %d\n", counter);

}

void mouse(int button, int state, int x, int y)
{
	double new_x = (x / 300.0) - 1;
	double new_y = (-y / 300.0) + 1;

	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
	{
		if ((new_x > square_left && new_x < square_left + square_space) && (new_y < square_top && new_y > square_top - 5*square_space))
		{
			if (new_y < square_top && new_y > square_top - square_space)
				tutor_digit = 0;
			else if (new_y < square_top - 2 * square_space && new_y > square_top - 3 * square_space)
				tutor_digit = 1;
			else if(new_y < square_top - 4 * square_space && new_y > square_top - 5 * square_space)
				tutor_digit = 2;
		}

		if (tutor_digit != network_digit)
		{
			// start the backpropogation, after the clicking on the correct number, and teach the newron network
				backPropogation();
		}
	}
}

void createMenu(int choice)
{
	clean();
	string path;
	path = testPathImage;
	path += testImagesSet[choice - 1];
	loadBitmap(path.c_str());
	renderImage512To256();
	HPF();
	logFile = fopen(LOG_FILE, "w+");
	FeedForward();
	fclose(logFile);

}

void main(int argc, char* argv[])
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
	glutInitWindowSize(W, H);
	glutInitWindowPosition(200, 100);
	glutCreateWindow("Digits Example ");

	glutDisplayFunc(display); // refresh function
	glutIdleFunc(idle); // idle: when nothing happens
	glutMouseFunc(mouse);

	init();

	glutMainLoop();
}