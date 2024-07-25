/*  
 *  RAiSD, Raised Accuracy in Sweep Detection
 *
 *  Copyright January 2017 by Nikolaos Alachiotis and Pavlos Pavlidis
 *
 *  This program is free software; you may redistribute it and/or modify its
 *  under the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your option)
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 *  or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *  for more details.
 *
 *  For any other enquiries send an email to
 *  Nikolaos Alachiotis (n.alachiotis@gmail.com)
 *  Pavlos Pavlidis (pavlidisp@gmail.com)  
 *  
 */
 
#ifdef _RSDAI

#include "RAiSD.h"

RSDGrid_t * RSDGrid_new (RSDCommandLine_t * RSDCommandLine)
{
	assert(RSDCommandLine!=NULL);
	
	if((RSDCommandLine->opCode!=OP_USE_CNN) && (RSDCommandLine->opCode!=OP_CREATE_IMAGES))
		return NULL;
		
	if(RSDCommandLine->opCode==OP_CREATE_IMAGES)
	{
		if(RSDCommandLine->trnObjDetection!=1)
			return NULL;
	}
	
	assert(RSDCommandLine->opCode==OP_USE_CNN || (RSDCommandLine->opCode==OP_CREATE_IMAGES && RSDCommandLine->trnObjDetection==1));
		
	RSDGrid_t * RSDGrid = (RSDGrid_t *)malloc(sizeof(RSDGrid_t));
	assert(RSDGrid!=NULL);
	
	RSDGrid->sizeAcc = 0;
	RSDGrid->size = 0;
	RSDGrid->firstPoint = 0.0;
	RSDGrid->pointOffset = 0.0;	
	strncpy(RSDGrid->destinationPath, "\0", STRING_SIZE);

	return RSDGrid;
}

void RSDGrid_free (RSDGrid_t * RSDGrid)
{
	if(RSDGrid==NULL)
		return;
	
	free(RSDGrid);
	
	RSDGrid=NULL;
}

void RSDGrid_init (RSDGrid_t * RSDGrid, RSDChunk_t * RSDChunk, RSDMuStat_t * RSDMuStat, RSDCommandLine_t * RSDCommandLine, int setDone)
{
	assert(RSDGrid!=NULL);
	assert(RSDChunk!=NULL);
	assert(RSDMuStat!=NULL);
	assert(RSDCommandLine!=NULL);
	assert(setDone==0 || setDone==1);
	
	//TODO-AI-6 cleanup?
	
	int imageStep = RSDCommandLine->imageWindowStep; // grid is applied on bp positions, step refers to snps
	int imagesPerGridPosition = 1; // TODO: clean up
	
	int size = (int)RSDChunk->chunkSize;
	
	int stepsLeft = imagesPerGridPosition / 2;
	int stepsRight = imagesPerGridPosition - stepsLeft;
	
	int firstValidSNP = stepsLeft*imageStep+RSDMuStat->windowSize/2;
	int lastValidSNP = size-stepsRight*imageStep-RSDMuStat->windowSize/2;
	
	double firstValidPosition = RSDChunk->sitePosition[firstValidSNP];
	double lastValidPosition = RSDChunk->sitePosition[lastValidSNP];
	
	double chunkFraction = (RSDChunk->sitePosition[size-1] - RSDChunk->sitePosition[0]) / RSDCommandLine->regionLength; // TODO: check if regionlength get value if VCF
	
	int64_t chunkGridSize = chunkFraction * RSDCommandLine->gridSize;
	
	//printf("chunkGridSize %d chunkFraction %f RSDCommandLine->gridSize %d\n", (int)chunkGridSize, chunkFraction, (int)RSDCommandLine->gridSize);
	
	if(RSDChunk->chunkID==0)
		RSDGrid->sizeAcc = 0;
	
	if(setDone==1) 
	{
		chunkGridSize = RSDCommandLine->gridSize - RSDGrid->sizeAcc;
		
		//printf("setdone: chunkgridsize %d sizeacc %d\n", chunkGridSize,  RSDGrid->sizeAcc); 
		//fflush(stdout);
	}	
	
	double pointOffset = (lastValidPosition-firstValidPosition)/((double)chunkGridSize);
	
	//printf("pointOffset source0: %f\n", pointOffset);
	//fflush(stdout);
	
	if(setDone==1)
	{
		if(chunkGridSize>1)
			pointOffset = (lastValidPosition-firstValidPosition)/((double)chunkGridSize-1);
		else
			pointOffset = (lastValidPosition-firstValidPosition)/((double)chunkGridSize);
	}	
			//printf("pointOffset source1: %f\n", pointOffset);
	//fflush(stdout);//
	
	RSDGrid->size = chunkGridSize;
	RSDGrid->sizeAcc += chunkGridSize;
	
	if(setDone==1)
		assert(RSDCommandLine->gridSize==RSDGrid->sizeAcc);
		
	RSDGrid->firstPoint = firstValidPosition;
	RSDGrid->pointOffset = pointOffset;
	
	//printf("pointOffset source: %f\n", pointOffset);
	//fflush(stdout);
	
	/*printf("first valid snp: %d\n", firstValidSNP);
	fflush(stdout);
	
			*///printf("first valid snp: %d and pos %f [%f]\n", firstValidSNP, firstValidPosition, RSDChunk->sitePosition[0]);
			//printf("last valid snp: %d and pos %f [%f]\n", lastValidSNP, lastValidPosition, RSDChunk->sitePosition[size-1]);
			/*//printf("fraciton: %f gridsize %lli offset %f\n", chunkFraction, chunkGridSize, gridPositionOffset);
			
			printf("first target: %f\n", firstValidPosition);
			printf("last target: %f\n", firstValidPosition+(chunkGridSize-1)*gridPositionOffset);
			int i;
			for(i=0;i<chunkGridSize;i++)
			{
				printf("%d target: %f\n", i, firstValidPosition+i*gridPositionOffset); // store firstposition and size and offset in rsdchunkstruct
			}
	fflush(stdout);
	*/	
	

}

void RSDGrid_makeDirectory (RSDGrid_t * RSDGrid, RSDCommandLine_t * RSDCommandLine, RSDImage_t * RSDImage)
{
	assert(RSDCommandLine!=NULL);
	
	if(RSDGrid==NULL)
		return;
		
	if((RSDCommandLine->opCode!=OP_USE_CNN) && (RSDCommandLine->opCode!=OP_CREATE_IMAGES))
		return;
		
	if(RSDCommandLine->opCode==OP_CREATE_IMAGES)
	{
		if(RSDCommandLine->trnObjDetection!=1)
			return;
	}
	
	assert(RSDCommandLine->opCode==OP_USE_CNN || (RSDCommandLine->opCode==OP_CREATE_IMAGES && RSDCommandLine->trnObjDetection==1));
			
	assert(RSDGrid!=NULL);
	assert(RSDImage!=NULL);
	
	char tstring [STRING_SIZE];
	int ret = 0;
	
	if(RSDCommandLine->opCode==OP_USE_CNN)
	{
		if(RSDCommandLine->forceRemove)
		{
			strcpy(tstring, "rm -r ");
			strcat(tstring, "RAiSD_Grid."); 
			strcat(tstring, RSDCommandLine->runName);
			strcat(tstring, " 2>/dev/null");
			
			ret = system(tstring);
			assert(ret!=-1);
		}
			
		strcpy(tstring, "mkdir ");
		strcat(tstring, "RAiSD_Grid.");
		strcat(tstring, RSDCommandLine->runName);
		strcat(tstring, " 2>/dev/null");
		
		ret = system(tstring);
		assert(ret!=-1);
		
		//TODO-AI-7 threads?
	/**********
		strcpy(tstring, "mkdir ");
		strcat(tstring, "RAiSD_Grid.");
		strcat(tstring, RSDCommandLine->runName);
		strcat(tstring, "/T0 2>/dev/null");
		
		ret = system(tstring);
		assert(ret!=-1);
		
		strcpy(tstring, "mkdir ");
		strcat(tstring, "RAiSD_Grid.");
		strcat(tstring, RSDCommandLine->runName);
		strcat(tstring, "/T1 2>/dev/null");
		
		ret = system(tstring);
		assert(ret!=-1);
	**********/	
		
		strncpy(RSDGrid->destinationPath, "\0", STRING_SIZE);
		strcat(RSDGrid->destinationPath, "RAiSD_Grid.");
		strcat(RSDGrid->destinationPath, RSDCommandLine->runName);
		strcat(RSDGrid->destinationPath, "/");
	}
	else
	{
		strncpy(RSDGrid->destinationPath, RSDImage->destinationPath, STRING_SIZE);
	}
}

void RSDGrid_cleanDirectory (RSDGrid_t * RSDGrid, RSDCommandLine_t * RSDCommandLine)
{
	assert(RSDGrid!=NULL);
	assert(RSDCommandLine!=NULL);
	
	char tstring [STRING_SIZE];
	int ret = 0;	

	strcpy(tstring, "rm ");
	strcat(tstring, "RAiSD_Grid."); 
	strcat(tstring, RSDCommandLine->runName);
	strcat(tstring, "/* 2>/dev/null");
	
	ret = system(tstring);
	assert(ret!=-1);
}


/*
int64_t get_SNPIndex (char * tstring)
{
 	assert(tstring!=NULL);
 	
 	printf("extracting from %s\n", tstring);
 	
 	int i=-1, sec=0, ind1=-1, ind2=-1;
  	
 	for(i=0;i<(int)strlen(tstring);i++)
 	{
 		if(tstring[i]=='_')
 			sec++;
 		
 		if(tstring[i]=='.')
 		{
 			ind2=i-1;	
 			i = strlen(tstring)+1;	
 		}
 			
 		if(sec==2 && ind1==-1)
 			ind1 = i+1;
 	}
 	
 	char nstring[STRING_SIZE];
 	strncpy(nstring, tstring+ind1, ind2-ind1+1);
 	nstring[ind2-ind1+1]='\0';
 	printf("ind: %s\n", nstring);
 	
 	return (int64_t)atoi(nstring);
 	
}
*/

int validGridPosition (RSDCommandLine_t * RSDCommandLine, uint64_t pos)
{
	assert(RSDCommandLine!=NULL);
	
	int isValid = 0;
	
	if(RSDCommandLine->gridRngLeBor!=0 && RSDCommandLine->gridRngRiBor!=0)
	{
		if(pos>=RSDCommandLine->gridRngLeBor && pos <= RSDCommandLine->gridRngRiBor)
			isValid=1;	
	}
	else
		isValid=1;
		
	return isValid;
}

void RSDGrid_process (RSDGrid_t * RSDGrid, RSDImage_t * RSDImage, RSDMuStat_t * RSDMuStat, RSDChunk_t * RSDChunk, RSDPatternPool_t * RSDPatternPool, RSDDataset_t * RSDDataset, RSDCommandLine_t * RSDCommandLine, RSDResults_t * RSDResults)
{
	// This function is called per chunk.
	
	assert(RSDGrid!=NULL);


	int i=-1;
	
	//TODO-AI this shhould be process-grid-part-A
	// the network run should be part-B

	//int prevPointIndex = 0;
	
	RSDMuStat_storeOutputConfigure(RSDCommandLine);
	
	//printf("RSDGrid->sizePre : %d\n", RSDGrid->size);
	//fflush(stdout);
	
	assert(RSDGrid->size>=1);
		
		//printf("RSDGrid->size : %d\n", RSDGrid->size);
		
	// grid points are defined per chunk
	// score indices are defined per set
	

	for(i=0;i<RSDGrid->size;i++)
	{
		
		//int pointIndex = RSDGrid->sizeAcc-RSDGrid->size + i;
		
		//printf("RSDCommandLine->fullReport= %d %d target: %f total %d chunk %d chunksize %d [%f %f]\n", RSDCommandLine->fullReport, pointIndex, RSDGrid->firstPoint+i*RSDGrid->pointOffset, (int)RSDGrid->size, RSDChunk->chunkID, RSDChunk->chunkSize, RSDChunk->sitePosition[0], RSDChunk->sitePosition[RSDChunk->chunkSize-1]);
		//fflush(stdout);
		//RSDImage_createImagesTMP (RSDImage, RSDMuStat, RSDChunk, RSDPatternPool, RSDDataset, RSDCommandLine, RSDSortRows, RSDSortColumns, fpOut, &remainingImages, prevPointIndex, (double)RSDGrid->firstPoint+i*RSDGrid->pointOffset);
		
		RSDMuStat->currentScoreIndex++;
		
		//printf("\n\nBEF muscore: %d target %f (%f %f %f)\n", RSDMuStat->currentScoreIndex, (double)RSDGrid->firstPoint+i*RSDGrid->pointOffset, (double)RSDGrid->firstPoint,(double)i,(double)RSDGrid->pointOffset);
		
		RSDImage_resetRemainingSetImages (RSDImage, RSDChunk, RSDCommandLine);
		
		
		if(validGridPosition(RSDCommandLine, (uint64_t)RSDGrid->firstPoint+i*RSDGrid->pointOffset))
		{
		
			//assert(0);
			RSDGridPoint_t * RSDGridPoint = RSDImage_createImagesFlex (RSDImage, RSDMuStat, RSDChunk, RSDPatternPool, RSDDataset, RSDCommandLine, RAiSD_Info_FP,  
									   (double)RSDGrid->firstPoint+i*RSDGrid->pointOffset, RSDGrid->destinationPath, RSDMuStat->currentScoreIndex);
									   
		//RSDResults_setGridPointSize (RSDResults, RSDMuStat->currentScoreIndex, gridPointSize);
			RSDResults_setGridPoint (RSDResults, RSDMuStat->currentScoreIndex, RSDGridPoint);
		}
		
		//TODO: add new grid point to right place in 2D grid of the whole run
		
		//printf("AFTmuscore: %d target %f (%f %f %f)\n", RSDMuStat->currentScoreIndex, (double)RSDGrid->firstPoint+i*RSDGrid->pointOffset, (double)RSDGrid->firstPoint,(double)i,(double)RSDGrid->pointOffset);
		




	
		
		
		
		
		// call CNN
		// read results
		// store in file

	//printf ("%d: %f vs %f\n", i, (double)(RSDGrid->firstPoint+i*RSDGrid->pointOffset), RSDChunk->sitePosition[3626]);
		//printf ("%d: %f vs %f\n", i, (double)(RSDGrid->firstPoint+i*RSDGrid->pointOffset), RSDChunk->sitePosition[3648]);
		//printf ("%d: %f vs %f\n", i, (double)(RSDGrid->firstPoint+i*RSDGrid->pointOffset), RSDChunk->sitePosition[3662]);
		//printf ("%d: %f vs %f\n", i, (double)(RSDGrid->firstPoint+i*RSDGrid->pointOffset), RSDChunk->sitePosition[75]);
		//printf ("%d: %f vs %f\n", i, (double)(RSDGrid->firstPoint+i*RSDGrid->pointOffset), RSDChunk->sitePosition[129]);

		//RSDMuStat_storeOutput (RSDMuStat, (double)(RSDGrid->firstPoint+i*RSDGrid->pointOffset), (double)RSDGrid->firstPoint+i*RSDGrid->pointOffset, (double)RSDGrid->firstPoint+i*RSDGrid->pointOffset, (double)1, (double)2, (double)3, (double)4);
		
		//prevPointIndex = pointIndex;
		
		
		 
	}
	
	
	
	
	/*char testCommand[STRING_SIZE], tstring[STRING_SIZE];
	
	
	strncpy(testCommand, "python3 ", STRING_SIZE);
	strcat(testCommand, RSDNeuralNetwork->pyPath);
	
	strcat(testCommand, " -n ");
	strcat(testCommand, "predict");
	
	strcat(testCommand, " -m ");
	strcat(testCommand, RSDNeuralNetwork->modelPath);
	
	strcat(testCommand, " -d ");
	strcat(testCommand, RSDGrid->destinationPath);
	
	strcat(testCommand, " -h ");
	sprintf(tstring, "%d", RSDCommandLine->imageHeight);
	strcat(testCommand, tstring);
	
	strcat(testCommand, " -w ");
	sprintf(tstring, "%d", RSDCommandLine->imageWidth);
	strcat(testCommand, tstring);

	
	strcat(testCommand, " -o ");
	strcat(testCommand, "tempOutputFolderTMP");

	//if(!RSDCommandLine->displayProgress==1) TODO
	strcat(testCommand, " 2>/dev/null");
	
		
		fprintf(fpOut, " Python-cmd :\t%s\n", testCommand);
		fprintf(stdout, " Python-cmd :\t%s\n", testCommand);
		
		exec_command (testCommand);
	
	*/
	
	
	
	
	//printf("x %d\n", RSDChunk->chunkID);
	
	//char runCommand[STRING_SIZE];
	//RSDNeutralNetwork_createRunCommand (RSDNeuralNetwork, RSDCommandLine, 0, runCommand, RSDGrid->destinationPath);
		
	//exec_command (runCommand);
	//printf("run cmd: %s\n", runCommand);
	
	
	// TODO SOS
	// the following part needs to happen per chunk. 
	// if not per chunk, i must store the chunkpositions for the entire set
	// i am going to change the chunk mem size for testing.. sos to bring it back.
	
	
	
	// if we generate several images for every grid point.. the distance between the first and last can be used to estimate snp density
	
	// approach: for every grid point -> average?
	// final score per grid point with distance between first and last images into account
	// postprocessing based on the results of the previous steps... another sliding window.. moving average?
	
	
	//FILE * fp = fopen("tempOutputFolder/PredResults.txt", "r");
	//assert(fp!=NULL);
		
	//	RSDNeuralNetwork_readPredictionFile (fp);
	
	
	/*char tstring[STRING_SIZE];
	int ret=0, clIndex = -1;
	
	ret = fscanf(fp, "%s", tstring);
	int cnt=0;
	
	
	
	int cmrow[2];
	cmrow[0]=0;
	cmrow[1]=0;
	while(ret!=EOF)
	{
		int64_t SNPIndex = get_SNPIndex (tstring);
	
		fscanf(fp, "%d", &clIndex);
		assert(clIndex>=0);
		//classIndex = clIndex;
		
		fscanf(fp, "%s", tstring);
		fscanf(fp, "%s", tstring);
		
		double windowCenter = RSDChunk->sitePosition[(int)SNPIndex];
		printf("pos: %d %f size %d\n", (int)SNPIndex, windowCenter, RSDChunk->chunkSize);
		
		//double windowCenter = RSDChunk->sitePosition[RSDGrid->firstPoint+i*RSDGrid->pointOffset];
		RSDMuStat_storeOutput (RSDMuStat, windowCenter, (double)RSDGrid->firstPoint+i*RSDGrid->pointOffset, (double)RSDGrid->firstPoint+i*RSDGrid->pointOffset, (double)1, (double)2, (double)3, (double)atof(tstring));

		//skipLine (fp);

		//printf("%d: %d\n", cnt++, clIndex);
		//fflush(stdout);

		cmrow[clIndex]++;
	
		ret = fscanf(fp, "%s", tstring);
	}

		
	printf("true class1: %d %d\n", cmrow[0], cmrow[1]);	
	*/
		
}


/*
void RSDChunk_initGrid (RSDChunk_t * RSDChunk, RSDMuStat_t * RSDMuStat, RSDCommandLine_t * RSDCommandLine) // TODO: Clean this up
{
	assert(RSDChunk!=NULL);
	assert(RSDMuStat!=NULL);
	assert(RSDCommandLine!=NULL);
	
	int imageStep = 10; // clean up
	int imagesPerGridPosition = 10; // clean up
	
	
	static int64_t gridAccum = 0;
	
	static int cnt = -1;
	
	cnt++;
	
	printf("\t\t\tcnt %d\n", cnt);

	// in chunk 0, first valid pos:
	
	int size = (int)RSDChunk->chunkSize;
	
	//if(RSDChunk->chunkID==0) // first chunk
	{
		int stepsLeft = imagesPerGridPosition / 2;
		int stepsRight = imagesPerGridPosition - stepsLeft;
		
		int firstValidSNP = stepsLeft*imageStep+RSDMuStat->windowSize/2;
		int lastValidSNP = size-stepsRight*imageStep-RSDMuStat->windowSize/2;
		
		double firstValidPosition = RSDChunk->sitePosition[firstValidSNP];
		double lastValidPosition = RSDChunk->sitePosition[lastValidSNP];
		
		double chunkFraction = (RSDChunk->sitePosition[size-1] - RSDChunk->sitePosition[0]) / RSDCommandLine->regionLength; // TODO: check if regionlength get value if VCF
		
		int64_t chunkGridSize = chunkFraction * RSDCommandLine->gridSize;
		
		if(cnt==2) // change this to the write variable
			chunkGridSize = RSDCommandLine->gridSize - gridAccum;
			
		double gridPositionOffset = (lastValidPosition-firstValidPosition)/((double)chunkGridSize);
		
		if(cnt==2)
			gridPositionOffset = (lastValidPosition-firstValidPosition)/((double)chunkGridSize-1);
		
		gridAccum += chunkGridSize; // store this in chunk struct, not static
		
		//printf("check : %lli %lli\n", RSDCommandLine->gridSize, gridAccum);
		
		if(cnt==2)
		assert(RSDCommandLine->gridSize==gridAccum);
		
		printf("first valid snp: %d\n", firstValidSNP);
		fflush(stdout);
		
				printf("first valid snp: %d and pos %f [%f]\n", firstValidSNP, firstValidPosition, RSDChunk->sitePosition[0]);
				printf("last valid snp: %d and pos %f [%f]\n", lastValidSNP, lastValidPosition, RSDChunk->sitePosition[size-1]);
				//printf("fraciton: %f gridsize %lli offset %f\n", chunkFraction, chunkGridSize, gridPositionOffset);
				
				printf("first target: %f\n", firstValidPosition);
				printf("last target: %f\n", firstValidPosition+(chunkGridSize-1)*gridPositionOffset);
				int i;
				for(i=0;i<chunkGridSize;i++)
				{
					printf("%d target: %f\n", i, firstValidPosition+i*gridPositionOffset); // store firstposition and size and offset in rsdchunkstruct
				}
		fflush(stdout);
		
	}
	
	//if
	
	
	
	 
}
*/


#endif
