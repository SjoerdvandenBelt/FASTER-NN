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

uint32_t sampleBitCount (RSDImage_t * RSDImage, int8_t * data, int sampleIndex)
{	
	assert(RSDImage!=NULL);
	assert(sampleIndex>=0);
	assert(sampleIndex<RSDImage->height);
	
	int i;
	uint32_t cnt = 0ull;
	for(i=0;i<RSDImage->width;i++)
		cnt+=data[sampleIndex*RSDImage->width+i];
		
	return cnt;
}

uint64_t snpBitCount (RSDImage_t * RSDImage, int8_t * data, int snpIndex)
{	
	assert(RSDImage!=NULL);
	assert(snpIndex>=0);
	assert(snpIndex<RSDImage->width);
	
	int i;
	uint64_t cnt = 0ull;
	for(i=0;i<RSDImage->height;i++)
		cnt+=data[i*RSDImage->width+snpIndex];
		
	return cnt;
}

RSDImage_t * RSDImage_new (RSDCommandLine_t * RSDCommandLine)
{
	assert(RSDCommandLine!=NULL);
	
	if((RSDCommandLine->opCode!=OP_CREATE_IMAGES)&&(RSDCommandLine->opCode!=OP_USE_CNN))
		return NULL;
		
	RSDImage_t * RSDImage = NULL;
	
	RSDImage = (RSDImage_t *)malloc(sizeof(RSDImage_t));
	assert(RSDImage!=NULL);
	
	RSDImage->width=-1; 
	RSDImage->height=-1;
	RSDImage->snpLengthInBytes=-1; 	
	RSDImage->firstSNPIndex=-1ll;
	RSDImage->lastSNPIndex=-1ll;	
	RSDImage->firstSNPPosition=0ull; 
	RSDImage->lastSNPPosition=0ull;
	strncpy(RSDImage->destinationPath, "\0", STRING_SIZE);
	RSDImage->remainingSetImages=0ull;
	RSDImage->generatedSetImages=0ull;
	RSDImage->totalGeneratedImages=0ull;
	RSDImage->data=NULL;
	RSDImage->dataT=NULL;
	RSDImage->incomingSNP=NULL;
	RSDImage->rowSortScore=NULL;
	RSDImage->rowSorter=RSDSort_new(RSDCommandLine);
	RSDImage->colSortScore=NULL;
	RSDImage->colSorter=RSDSort_new(RSDCommandLine);
	RSDImage->nextSNPDistance=NULL;
	RSDImage->avgImageWidthBP=0;
	RSDImage->byteBuffer = NULL;
	RSDImage->longIntBuffer = NULL;	
	RSDImage->derivedAlleleFrequency = NULL;
	
	return RSDImage;
}

void RSDImage_makeDirectory (RSDImage_t * RSDImage, RSDCommandLine_t * RSDCommandLine)
{
	assert(RSDCommandLine!=NULL);
	
	if(RSDImage==NULL)
		return;
		
	if(RSDCommandLine->opCode!=OP_CREATE_IMAGES)
		return;
		
	assert(RSDImage!=NULL);
	
	char tstring [STRING_SIZE];
	int ret = 0;
	
	if(RSDCommandLine->forceRemove)
	{
		strcpy(tstring, "rm -r ");
		strcat(tstring, "RAiSD_Images."); 
		strcat(tstring, RSDCommandLine->runName);
		strcat(tstring, " 2>/dev/null");
		
		ret = system(tstring);
		assert(ret!=-1);
	}
		
	strcpy(tstring, "mkdir ");
	strcat(tstring, "RAiSD_Images.");
	strcat(tstring, RSDCommandLine->runName);
	strcat(tstring, " 2>/dev/null");
	
	ret = system(tstring);
	assert(ret!=-1);
	
	strcpy(tstring, "mkdir ");
	strcat(tstring, "RAiSD_Images.");
	strcat(tstring, RSDCommandLine->runName);
	strcat(tstring, "/");
	strcat(tstring, RSDCommandLine->imageClassLabel);
	strcat(tstring, " 2>/dev/null");
	
	ret = system(tstring);
	assert(ret!=-1);
	
	strncpy(RSDImage->destinationPath, "\0", STRING_SIZE);
	strcat(RSDImage->destinationPath, "RAiSD_Images.");
	strcat(RSDImage->destinationPath, RSDCommandLine->runName);
	strcat(RSDImage->destinationPath, "/");
	strcat(RSDImage->destinationPath, RSDCommandLine->imageClassLabel);
	strcat(RSDImage->destinationPath, "/");	
}

void RSDImage_checkDimensions (RSDImage_t * RSDImage, RSDCommandLine_t * RSDCommandLine)
{
	assert(RSDImage!=NULL);
	assert(RSDCommandLine!=NULL);

	char tstring[STRING_SIZE], format[STRING_SIZE];	

	if(RSDCommandLine->opCode==OP_USE_CNN)
	{
		strncpy(tstring, "RAiSD_Grid.", STRING_SIZE); 
		strcat(tstring, RSDCommandLine->runName);
		strcat(tstring, "/info.txt");  
	}
	else
	{
		strncpy(tstring, "RAiSD_Images.", STRING_SIZE); 
		strcat(tstring, RSDCommandLine->runName);
		strcat(tstring, "/info.txt");
	}	

	FILE * fp = fopen(tstring, "r");
	
	if(fp==NULL)
	{
		fp = fopen(tstring, "w");

		fprintf(fp, "***DO_NOT_REMOVE_OR_EDIT_THIS_FILE***\n%d\n%d\n%s\n%d\n***DO_NOT_REMOVE_OR_EDIT_THIS_FILE***", RSDImage->width, RSDImage->height, RSDCommandLine->enBinFormat==1?"bin":"2D", RSDCommandLine->imgDataType);
	}
	else
	{
		int height=-1, width=-1, type=-1;
		
		int rcnt = fscanf(fp, "%s %d %d %s %d", tstring, &width, &height, format, &type);
		assert(rcnt==5);
		
		if((height!=RSDImage->height)||(width!=RSDImage->width))
		{
		
			if(RSDCommandLine->opCode==OP_USE_CNN)
				fprintf(stderr, "\nERROR: Image dimension mismatch between input sets in directory RAiSD_Grid.%s\n       (width x height in file RAiSD_Grid.%s/info.txt: %d x %d, current: %d x %d)\n\n",RSDCommandLine->runName, RSDCommandLine->runName, width, height, RSDImage->width, RSDImage->height);
			else
				fprintf(stderr, "\nERROR: Image dimension mismatch between class folders in directory RAiSD_Images.%s\n       (width x height in file RAiSD_Images.%s/info.txt: %d x %d, current: %d x %d)\n\n",RSDCommandLine->runName, RSDCommandLine->runName, width, height, RSDImage->width, RSDImage->height);
				
			exit(0);			
		}
		
		if((RSDCommandLine->enBinFormat==0 && (!strcmp(format, "bin")))||(RSDCommandLine->enBinFormat==1 && (!strcmp(format, "2D"))))
		{
			fprintf(stderr, "\nERROR: Data format mismatch between class folders in directory RAiSD_Images.%s\n       (data format in file RAiSD_Images.%s/info.txt: %s, current: %s)\n\n",RSDCommandLine->runName, RSDCommandLine->runName, format, RSDCommandLine->enBinFormat==1?"bin":"2D");
			
			exit(0);
		}
		
		if(RSDCommandLine->imgDataType!= type)
		{
			fprintf(stderr, "\nERROR: Data-type code mismatch between class folders in directory RAiSD_Images.%s\n       (data-type code in file RAiSD_Images.%s/info.txt: %d, current: %d)\n\n", RSDCommandLine->runName, RSDCommandLine->runName, type, RSDCommandLine->imgDataType);
			
			exit(0);
		}
	
	}
	
	fclose(fp);
	fp=NULL;
}

void RSDImage_print (RSDImage_t * RSDImage, RSDCommandLine_t * RSDCommandLine, FILE * fpOut)
{
	assert(RSDCommandLine!=NULL);
	
	if(RSDImage==NULL)
		return;
		
	if(RSDCommandLine->opCode!=OP_CREATE_IMAGES)
		return;
		
	assert(RSDImage!=NULL);
	assert(fpOut);
	
	fprintf(fpOut, " Target position     :\t%lu\n", RSDCommandLine->imageTargetSite);
	fprintf(fpOut, " Image step          :\t%d\n", RSDCommandLine->imageWindowStep);
	
	//if(RSDCommandLine->imageReorderOpt)
	//	fprintf(fpOut, " Pixel rearrangement :\tON\n");
	//else
	//	fprintf(fpOut, " Pixel rearrangement :\tOFF\n");
}

void RSDImage_initGeneratedSetImages (RSDImage_t * RSDImage, RSDChunk_t * RSDChunk)
{
	assert(RSDImage!=NULL);
	assert(RSDChunk!=NULL);
	
	if(RSDChunk->chunkID==0)
		RSDImage->generatedSetImages = 0ull;
}

void RSDImage_init (RSDImage_t * RSDImage, RSDDataset_t * RSDDataset, RSDMuStat_t * RSDMuStat, RSDPatternPool_t * RSDPatternPool, RSDCommandLine_t * RSDCommandLine, RSDChunk_t * RSDChunk, int setIndex, FILE * fpOut)
{  
	assert(RSDImage!=NULL);
	assert(RSDDataset!=NULL);
	assert(RSDMuStat!=NULL);
	assert(RSDPatternPool!=NULL);
	assert(RSDCommandLine!=NULL);
	assert(RSDChunk!=NULL);
	
	RSDImage_setRemainingSetImages (RSDImage, RSDChunk, RSDCommandLine); // only if chunkID==0
	RSDImage_initGeneratedSetImages (RSDImage, RSDChunk); // only if chunkID==0
		
	if(!(RSDChunk->chunkID==0 && setIndex==0))
		return;
		
	fprintf(stdout, " Generating images ...\n");

	if(RSDCommandLine->displayProgress==1)
		fprintf(stdout, "\n");
		
	fprintf(fpOut, " Generating images ...\n\n");
	
	fflush(stdout);
		
	RSDImage->width = RSDMuStat->windowSize;	
	RSDImage->height = RSDDataset->setSamples;
	RSDImage->snpLengthInBytes = (RSDImage->height + (8 - 1)) / 8; 
	
	RSDImage_checkDimensions (RSDImage, RSDCommandLine);
	
	RSDImage->firstSNPIndex = -1;
	RSDImage->lastSNPIndex = -1;
	RSDImage->firstSNPPosition = 0ull;
	RSDImage->lastSNPPosition = 0ull;
	
	assert(RSDImage->width>=1);
	assert(RSDImage->height>=1);
	
	RSDImage->data = (int8_t*)rsd_malloc(sizeof(int8_t)*RSDImage->height*RSDImage->width); 
	assert(RSDImage->data!=NULL);
	
	RSDImage->dataT = (int8_t*)rsd_malloc(sizeof(int8_t)*RSDImage->height*RSDImage->width); 
	assert(RSDImage->dataT!=NULL);	
	
	RSDImage->incomingSNP = (int8_t*)rsd_malloc(sizeof(int8_t)*RSDImage->height);
	assert(RSDImage->incomingSNP);
	
	RSDImage->rowSortScore = NULL;
	RSDImage->colSortScore = NULL;
	
	if(RSDCommandLine->imageReorderOpt==PIXEL_REORDERING_ENABLED)
	{
		assert(RSDImage->height!=-1);
		assert(RSDImage->width!=-1);
		
		RSDImage->rowSortScore = (uint32_t*)rsd_malloc(sizeof(uint32_t)*RSDImage->height);
		assert(RSDImage->rowSortScore!=NULL);
		
		RSDSort_init (RSDImage->rowSorter, RSDImage->height);
		
		RSDImage->colSortScore = (uint32_t*)rsd_malloc(sizeof(uint32_t)*RSDImage->width);
		assert(RSDImage->colSortScore!=NULL);
		
		RSDSort_init (RSDImage->colSorter, RSDImage->width);
	}
	
	RSDImage->nextSNPDistance = (double*)rsd_malloc(sizeof(double)*RSDImage->width);
	assert(RSDImage->nextSNPDistance);
	
	RSDImage->nextSNPDistance[RSDImage->width-1]=0.0;	
}

void RSDImage_free (RSDImage_t * RSDImage)
{
	if(RSDImage==NULL)
		return;		

	if(RSDImage->incomingSNP!=NULL)
	{
		free(RSDImage->incomingSNP);
		RSDImage->incomingSNP = NULL;
	}	
	
	if(RSDImage->data!=NULL)
	{
		free(RSDImage->data);
		RSDImage->data=NULL;
	}
	
	if(RSDImage->dataT!=NULL)
	{
		free(RSDImage->dataT);
		RSDImage->dataT=NULL;
	}
	
	if(RSDImage->rowSortScore!=NULL)
	{
		free(RSDImage->rowSortScore);
		RSDImage->rowSortScore=NULL;
	}
	
	if(RSDImage->colSortScore!=NULL)
	{
		free(RSDImage->colSortScore);
		RSDImage->colSortScore=NULL;
	}
	
	RSDSort_free (RSDImage->rowSorter);
	RSDSort_free (RSDImage->colSorter);
	
	if(RSDImage->nextSNPDistance!=NULL)
	{
		free(RSDImage->nextSNPDistance);
		RSDImage->nextSNPDistance=NULL;
	}
	
	if(RSDImage->byteBuffer!=NULL)
	{
		free(RSDImage->byteBuffer);
		RSDImage->byteBuffer=NULL;
	}	
	
	if(RSDImage->longIntBuffer!=NULL)
	{
		free(RSDImage->longIntBuffer);
		RSDImage->longIntBuffer=NULL;
	}
	
	if(RSDImage->derivedAlleleFrequency!=NULL)
	{
		free(RSDImage->derivedAlleleFrequency);
		RSDImage->derivedAlleleFrequency=NULL;
	}	
	
	free(RSDImage);
	RSDImage = NULL;
}

void RSDImage_setRemainingSetImages (RSDImage_t * RSDImage, RSDChunk_t * RSDChunk, RSDCommandLine_t * RSDCommandLine)
{
	assert(RSDImage!=NULL);
	assert(RSDChunk!=NULL);
	assert(RSDCommandLine!=NULL);
	
	if(RSDChunk->chunkID==0)
		RSDImage->remainingSetImages = RSDCommandLine->imagesPerSimulation;
}


void RSDImage_resetRemainingSetImages (RSDImage_t * RSDImage, RSDChunk_t * RSDChunk, RSDCommandLine_t * RSDCommandLine)
{
	assert(RSDImage!=NULL);
	assert(RSDChunk!=NULL);
	assert(RSDCommandLine!=NULL);
	
	RSDImage->remainingSetImages = RSDCommandLine->imagesPerSimulation;
}

void RSDImage_setRange (RSDImage_t * RSDImage, RSDChunk_t * RSDChunk, int64_t firstSNPIndex, int64_t lastSNPIndex)
{
	assert(RSDImage!=NULL);
	
	RSDImage->firstSNPIndex = firstSNPIndex;
	RSDImage->lastSNPIndex = lastSNPIndex;
	RSDImage->firstSNPPosition = (uint64_t)RSDChunk->sitePosition[RSDImage->firstSNPIndex]; 
	RSDImage->lastSNPPosition = (uint64_t)RSDChunk->sitePosition[RSDImage->lastSNPIndex];	
}

void decompressPattern (uint64_t * pattern, int patternSz, int samples, int8_t * SNP)
{
	assert(pattern!=NULL);
	assert(SNP!=NULL);
	
	int i=-1, j=-1, bit=-1;
	
	uint64_t d = 0;	
	
	for(i=0;i<patternSz-1;i++)
	{
		d = pattern[i];

		for(j=0;j<64;j++)
		{
			bit = (d & 1);
			d=d>>1;
			SNP[(i+1)*64-j-1] = (int8_t)bit;
		}	
	}
	
	d = pattern[i];

	for(j=0;j<samples-(patternSz-1)*64;j++)
	{
		bit = (d & 1);
		d=d>>1;		
		SNP[samples-j-1] = (int8_t)bit;
	}
}

void RSDImage_getData (RSDImage_t * RSDImage, RSDChunk_t * RSDChunk, RSDPatternPool_t * RSDPatternPool, RSDDataset_t * RSDDataset)
{
	assert(RSDImage!=NULL);
	assert(RSDChunk!=NULL);
	assert(RSDPatternPool!=NULL);
	assert(RSDDataset!=NULL);
	assert(RSDImage->firstSNPIndex!=-1);
	assert(RSDImage->lastSNPIndex!=-1);
	
	int i=0, j=0;	
	
	for(i=RSDImage->firstSNPIndex;i<=RSDImage->lastSNPIndex;i++)
	{
		decompressPattern (&(RSDPatternPool->poolData[RSDChunk->patternID[i]*RSDPatternPool->patternSize]), RSDPatternPool->patternSize, RSDDataset->setSamples, RSDImage->incomingSNP);
		
		for(j=0;j<RSDImage->height;j++)
		{
			RSDImage->data[j*RSDImage->width+i-RSDImage->firstSNPIndex] = RSDImage->incomingSNP[j];
			//printf(" %d ", RSDImage->data[j*RSDImage->width+i-RSDImage->firstSNPIndex]);
		}
		//printf("\n");
	}
}

int RSDImage_savePNG (RSDImage_t * RSDImage, RSDChunk_t * RSDChunk, RSDDataset_t * RSDDataset, RSDCommandLine_t * RSDCommandLine, int imgIndex, int8_t * data, char * destinationPath, float muVar, float muSfs, int scoreIndex, int * gridPointSize, FILE * fpOut)
{
	assert(RSDImage!=NULL);
	assert(RSDDataset!=NULL);
	assert(imgIndex>=0);
	assert(data!=NULL);
	assert(destinationPath!=NULL);
	assert(RSDCommandLine!=NULL);
	assert(fpOut);
	
	char imgPath[STRING_SIZE], tstring [STRING_SIZE];
	
	if(gridPointSize!=NULL)
		(*gridPointSize)++;
		
	muSfs=muSfs; // TODO: remove
	
	//TODO-AI-3 image generation
	//TODO-AI-4 threads
	
	
	//printf("Doing %s and %u\n",destinationPath, RSDImage->totalGeneratedImages);
	
	int threads = 1;
	
	if(RSDCommandLine->opCode==OP_USE_CNN)
	{
		//char imgPath[STRING_SIZE], tstring [STRING_SIZE];
		strncpy(imgPath, destinationPath, STRING_SIZE-1);
		
		if(threads>1)
		{
			sprintf(tstring, "%d", (int)((RSDImage->totalGeneratedImages)%threads));
			
			strcat(imgPath, "T");
			strcat(imgPath, tstring);
			strcat(imgPath, "/");
		
		}
		strcat(imgPath, RSDDataset->setID);
		strcat(imgPath, "_");
		//sprintf(tstring, "%d", (int)RSDChunk->chunkID);
		//strcat(imgPath, tstring);
		//strcat(imgPath, "_");	
		sprintf(tstring, "%d", (int)scoreIndex);
		strcat(imgPath, tstring);
		strcat(imgPath, "_");	
		sprintf(tstring, "%d", (*gridPointSize)-1);
		strcat(imgPath, tstring);
		strcat(imgPath, ".png");
		
		//printf("FINAL: %s\n",imgPath);
	}	
	else
	{
		//char imgPath[STRING_SIZE], tstring [STRING_SIZE];
		strncpy(imgPath, destinationPath, STRING_SIZE-1);
		strcat(imgPath, RSDDataset->setID);
		strcat(imgPath, "_");
		sprintf(tstring, "%d", (int)RSDChunk->chunkID);
		strcat(imgPath, tstring);
		strcat(imgPath, "_");	
		sprintf(tstring, "%d", (int)scoreIndex);
		strcat(imgPath, tstring);
		strcat(imgPath, "_");	
		sprintf(tstring, "%d", imgIndex);
		strcat(imgPath, tstring);
		strcat(imgPath, ".png");
	}
	
	bitmap_t bitmap;
	uint64_t x;
	uint64_t y;

	// Create an image. 

	bitmap.width = RSDImage->width;
	bitmap.height = RSDImage->height;

	bitmap.pixels = calloc (bitmap.width * bitmap.height, sizeof (pixel_t));

	if (bitmap.pixels==NULL) {
	return 0;
	}



	//if(RSDCommandLine->enTF==1)
	for (y = 0; y < bitmap.height; y++) 
	{
		for (x = 0; x < bitmap.width; x++) 
		{
			pixel_t * pixel = getPixel(& bitmap, x, y);
			
			switch(RSDCommandLine->imgDataType)
			{
				case IMG_DATA_RAW:
					pixel->red = data[y*RSDImage->width+x]*255; 
					pixel->green = data[y*RSDImage->width+x]*255; 
					pixel->blue = data[y*RSDImage->width+x]*255;
					
					break;
					
				case IMG_DATA_PAIRWISE_DISTANCE:
					double r_val = round(255*(RSDImage->nextSNPDistance[x]/200.0));
					if (r_val > 255.0) {
						pixel->red = 255;
					} else{
						pixel->red = r_val;
					}
					if (r_val > 255.0) {
						pixel->green = 255;
					} else{
						pixel->green = r_val;
					}
					pixel->blue = (data[y*RSDImage->width+x])*255;
									
					break;
					
				case IMG_DATA_MUVAR_SCALED:
					pixel->red = (data[y*RSDImage->width+x])*255; 
				
					if(muVar<=1.29122021f)
						pixel->green = (data[y*RSDImage->width+x])*255;
					else
						pixel->green = (data[y*RSDImage->width+x])*255/muVar;
			
					if(muVar<=1.29122021f)
						pixel->blue = (data[y*RSDImage->width+x])*255;
					else
						pixel->blue = (data[y*RSDImage->width+x])*255/muVar;
										
					break;
					
				case IMG_DATA_EXPERIMENTAL:
				
					assert(0);
					
					break;
					
				default:
					pixel->red = data[y*RSDImage->width+x]*255; 
					pixel->green = data[y*RSDImage->width+x]*255; 
					pixel->blue = data[y*RSDImage->width+x]*255;
					
					fprintf(fpOut, "\nERROR: Invalid data-type code (-typ %d) in image format (PNG)\n\n",RSDCommandLine->imgDataType);
					fprintf(stderr, "\nERROR: Invalid data-type code (-typ %d) in image format (PNG)\n\n",RSDCommandLine->imgDataType);
					exit(0);
					
				break;			
			}
			
			/*if(RSDCommandLine->imgType==IMG_TYPE_RAW)
			{
				
			
			}

			if(RSDCommandLine->enTF==1) // png - raw - SweepNet - TF and PT - x = 0
			{
				// Raw data - all channels the same
				pixel->red = data[y*RSDImage->width+x]*255; 
				pixel->green = data[y*RSDImage->width+x]*255; 
				pixel->blue = data[y*RSDImage->width+x]*255;
			}
			else
			{
				// Experimental 1
				pixel->red = (data[y*RSDImage->width+x])*255; 
				
				if(muVar<=1.29122021f)
					pixel->green = (data[y*RSDImage->width+x])*255;// *muSfs; 
				else
					pixel->green = (data[y*RSDImage->width+x])*255/muVar;// *muSfs; 
		
				if(muVar<=1.29122021f)
					pixel->blue = (data[y*RSDImage->width+x])*255;// *muSfs; 
				else
					pixel->blue = (data[y*RSDImage->width+x])*255/muVar;// *muSfs;		
				****
				
				
				// Experimental 2
				double r_val = round(255*(RSDImage->nextSNPDistance[x]/200.0));
				if (r_val > 255.0) {
					pixel->red = 255;
				} else{
					pixel->red = r_val;
				}
				if (r_val > 255.0) {
					pixel->green = 255;
				} else{
					pixel->green = r_val;
				}
				pixel->blue = (data[y*RSDImage->width+x])*255;		
				////////////////////////////////////
				
				
				// Experimental 3 - NOT WORKING
				double g = round(255.0*RSDImage->nextSNPDistance[x]*128.0/2.0/(RSDImage->avgImageWidthBP/RSDImage->totalGeneratedImages+1));
				
				pixel->green = g;
				
				if(g>255.0)
					pixel->green = 255;
				
					
				double r = g;//round(255.0*RSDImage->nextSNPDistance[x]/200.0);

				pixel->red = r;
				if(r>255)
					pixel->red = 255;
					
				pixel->blue = data[y*RSDImage->width+x]*255;
				////////////////////////////////	
				****/
			//}
		}
	}
	/*else // Pytorch
	for (y = 0; y < bitmap.height; y++) 
	{
		for (x = 0; x < bitmap.width; x++) 
		{
			pixel_t * pixel = getPixel(& bitmap, x, y);
			
			
//			pixel->red = data[y*RSDImage->width+x]*256*muVar; 
//			pixel->green = data[y*RSDImage->width+x]*256; 
//			pixel->blue = data[y*RSDImage->width+x]*256;




			// TODO: this should be outside the y loop
			
			
			double g = round(255.0*RSDImage->nextSNPDistance[x]*128.0/2.0/(RSDImage->avgImageWidthBP/RSDImage->totalGeneratedImages+1));
			
			pixel->green = g;
			
			if(g>255)
				pixel->green = 255;
			
		//	pixel->green = data[y*RSDImage->width+x]*255;
			
			
			double r = round(255.0*RSDImage->nextSNPDistance[x]/200.0);

			pixel->red = r;//data[y*RSDImage->width+x]*255;
			if(r>255)
				pixel->red = 255;
				
			//pixel->red = data[y*RSDImage->width+x]*255;
			
			
			//printf("%f %d\n", RSDImage->avgImageWidthBP, RSDImage->totalGeneratedImages+1);
		///	fflush(stdout);
		//	assert(RSDImage->totalGeneratedImages+1<5);
			
			
				
			
			 
		
			pixel->blue = data[y*RSDImage->width+x]*255;



			

			//pixel->red = (data[y*RSDImage->width+x])*255; 
			
			//if(muVar<=1.29122021f)
			//	pixel->green = (data[y*RSDImage->width+x])*255;// *muSfs; 
			//else
			//	pixel->green = (data[y*RSDImage->width+x])*255/muVar;// *muSfs; 
	
			//if(muVar<=1.29122021f)
			//	pixel->blue = (data[y*RSDImage->width+x])*255;// *muSfs; 
			//else
			//	pixel->blue = (data[y*RSDImage->width+x])*255/muVar;// *muSfs;		

			
			//pixel->green = (data[y*RSDImage->width+x])*255;
			//pixel->blue = (data[y*RSDImage->width+x])*255;

		}
	}*/

	if (save_png_to_file (& bitmap, imgPath)) 
	{
		fprintf (stderr, "Error writing file.\n");
		assert(0);
	}
	
	free (bitmap.pixels);

	return 1;	
}



int RSDImage_saveBIN (RSDImage_t * RSDImage, RSDChunk_t * RSDChunk, RSDDataset_t * RSDDataset, RSDCommandLine_t * RSDCommandLine, int imgIndex, int8_t * data, char * destinationPath, float muVar, float muSfs, int scoreIndex, int * gridPointSize, double targetPos, FILE * fpOut)
{
	assert(RSDImage!=NULL);
	assert(RSDDataset!=NULL);
	assert(imgIndex>=0);
	assert(data!=NULL);
	assert(destinationPath!=NULL);
	assert(RSDCommandLine!=NULL);
	assert(fpOut);
	
	assert(muVar>=-0.001);
	
	char imgPath[STRING_SIZE], snpPath[STRING_SIZE], tstring [STRING_SIZE];
	
	if(gridPointSize!=NULL)
		(*gridPointSize)++;
		
	muSfs=muSfs; // TODO: remove
	
	//TODO-AI-3 image generation
	//TODO-AI-4 threads
	
	
	//printf("Doing %s and %u\n",destinationPath, RSDImage->totalGeneratedImages);
	
	int threads = 1;
	
	if(RSDCommandLine->opCode==OP_USE_CNN)
	{
		//char imgPath[STRING_SIZE], tstring [STRING_SIZE];
		strncpy(imgPath, destinationPath, STRING_SIZE-1);
		
		if(threads>1)
		{
			sprintf(tstring, "%d", (int)((RSDImage->totalGeneratedImages)%threads));
			
			strcat(imgPath, "T");
			strcat(imgPath, tstring);
			strcat(imgPath, "/");
		
		}
		strcat(imgPath, RSDDataset->setID);
		strcat(imgPath, "_");
		//sprintf(tstring, "%d", (int)RSDChunk->chunkID);
		//strcat(imgPath, tstring);
		//strcat(imgPath, "_");	
		sprintf(tstring, "%d", (int)scoreIndex);
		strcat(imgPath, tstring);
		strcat(imgPath, "_");	
		sprintf(tstring, "%d", (*gridPointSize)-1);
		strcat(imgPath, tstring);
		
		strncpy(snpPath, imgPath, STRING_SIZE-1);
		strcat(snpPath, ".snp");
		
		strcat(imgPath, ".png");
	}	
	else
	{
		strncpy(imgPath, destinationPath, STRING_SIZE-1);
		strcat(imgPath, RSDDataset->setID);
		strcat(imgPath, "_");
		sprintf(tstring, "%d", (int)RSDChunk->chunkID);
		strcat(imgPath, tstring);
		strcat(imgPath, "_");	
		sprintf(tstring, "%d", (int)scoreIndex);
		strcat(imgPath, tstring);
		strcat(imgPath, "_");	
		sprintf(tstring, "%d", imgIndex);
		strcat(imgPath, tstring);
		
		strncpy(snpPath, imgPath, STRING_SIZE-1);
		strcat(snpPath, ".snp");
		
		strcat(imgPath, ".png");		
	}
	
	//bitmap_t bitmap;
	//uint64_t x;
	//uint64_t y;

	/*// Create an image. 

	bitmap.width = RSDImage->width;
	bitmap.height = RSDImage->height;

	bitmap.pixels = calloc (bitmap.width * bitmap.height, sizeof (pixel_t));

	if (bitmap.pixels==NULL) {
	return 0;
	}



	if(RSDCommandLine->enTF==1)
	for (y = 0; y < bitmap.height; y++) 
	{
		for (x = 0; x < bitmap.width; x++) 
		{
			pixel_t * pixel = getPixel(& bitmap, x, y);
			
			
//			pixel->red = data[y*RSDImage->width+x]*256*muVar; 
//			pixel->green = data[y*RSDImage->width+x]*256; 
//			pixel->blue = data[y*RSDImage->width+x]*256;

			 

			pixel->red = (data[y*RSDImage->width+x])*255; 
			
			if(muVar<=1.29122021f)
				pixel->green = (data[y*RSDImage->width+x])*255;// *muSfs; 
			else
				pixel->green = (data[y*RSDImage->width+x])*255/muVar;// *muSfs; 
	
			if(muVar<=1.29122021f)
				pixel->blue = (data[y*RSDImage->width+x])*255;// *muSfs; 
			else
				pixel->blue = (data[y*RSDImage->width+x])*255/muVar;// *muSfs;		

			
			//pixel->green = (data[y*RSDImage->width+x])*255;
			//pixel->blue = (data[y*RSDImage->width+x])*255;

		}
	}
	else // Pytorch
	for (y = 0; y < bitmap.height; y++) 
	{
		for (x = 0; x < bitmap.width; x++) 
		{
			pixel_t * pixel = getPixel(& bitmap, x, y);
			
			
//			pixel->red = data[y*RSDImage->width+x]*256*muVar; 
//			pixel->green = data[y*RSDImage->width+x]*256; 
//			pixel->blue = data[y*RSDImage->width+x]*256;




			// TODO: this should be outside the y loop
			
			
			double g = round(255.0*RSDImage->nextSNPDistance[x]*128.0/2.0/(RSDImage->avgImageWidthBP/RSDImage->totalGeneratedImages+1));
			
			pixel->green = g;
			
			if(g>255)
				pixel->green = 255;
			
		//	pixel->green = data[y*RSDImage->width+x]*255;
			
			
			double r = round(255.0*RSDImage->nextSNPDistance[x]/200.0);

			pixel->red = r;//data[y*RSDImage->width+x]*255;
			if(r>255)
				pixel->red = 255;
				
			//pixel->red = data[y*RSDImage->width+x]*255;
			
			
			//printf("%f %d\n", RSDImage->avgImageWidthBP, RSDImage->totalGeneratedImages+1);
		///	fflush(stdout);
		//	assert(RSDImage->totalGeneratedImages+1<5);
			
			
				
			
			 
		
			pixel->blue = data[y*RSDImage->width+x]*255;



			

			//pixel->red = (data[y*RSDImage->width+x])*255; 
			
			//if(muVar<=1.29122021f)
			//	pixel->green = (data[y*RSDImage->width+x])*255;// *muSfs; 
			//else
			//	pixel->green = (data[y*RSDImage->width+x])*255/muVar;// *muSfs; 
	
			//if(muVar<=1.29122021f)
			//	pixel->blue = (data[y*RSDImage->width+x])*255;// *muSfs; 
			//else
			//	pixel->blue = (data[y*RSDImage->width+x])*255/muVar;// *muSfs;		

			
			//pixel->green = (data[y*RSDImage->width+x])*255;
			//pixel->blue = (data[y*RSDImage->width+x])*255;

		}
	}

	if (save_png_to_file (& bitmap, imgPath)) 
	{
		fprintf (stderr, "Error writing file.\n");
		assert(0);
	}
	
	free (bitmap.pixels);
	*/
	
	FILE * fp1 = NULL;
	fp1 = fopen(snpPath, "wb");
	assert(fp1!=NULL);
	
	switch (RSDCommandLine->imgDataType)
	{
		case BIN_DATA_RAW:

			assert(RSDImage->height<=524280);

			uint16_t num_ints = (uint16_t)RSDImage->snpLengthInBytes; //(RSDImage->height + (8 - 1)) / 8; 
			//printf("%d vs %d\n", num_ints, RSDImage->snpLengthInBytes);
			
			//uint16_t num_ints_ref = (RSDImage->height + (8 - 1)) / 8;
			
			//assert(num_ints==num_ints_ref);
			
			uint8_t * data_as_ints = NULL;
			
			if(RSDImage->byteBuffer==NULL)
			{
				RSDImage->byteBuffer = (uint8_t*)rsd_malloc(sizeof(uint8_t)*num_ints);
				assert(RSDImage->byteBuffer!=NULL);
			}
			
			data_as_ints = RSDImage->byteBuffer;
			assert(data_as_ints!=NULL);					

			//fwrite(&num_ints, 1, 2, fp1);			
			fwrite(&(RSDImage->height), 1, 4, fp1);
			//printf("height %d\n", RSDImage->height);
			//fflush(stdout);
			//assert(0);

			fwrite(&(RSDImage->width), 1, 4, fp1);
			fwrite(&(targetPos), 1, 8, fp1);

			for (uint64_t x = 0; x < (uint64_t)RSDImage->width; x++) 
			{
				// number of 64 bit uints to use with rounding up
				//uint8_t data_as_ints[num_ints];
				for (uint16_t i = 0; i < num_ints; i++){
					data_as_ints[i] = 0;
				}

				for (uint64_t y = 0; y < (uint64_t)RSDImage->height; y++){
					uint16_t int_index = y/8;
					uint8_t bit_pos = y%8;
					data_as_ints[int_index] = data_as_ints[int_index] | (data[y*RSDImage->width+x] << (7-bit_pos));
					// printf("%lu\n", int_index);
					// printf("%lu\n", bit_pos);
					//printf("val: %d pos %u: %u\n", data[y*RSDImage->width+x], int_index, data_as_ints[int_index]);
					
				}
				fwrite(data_as_ints, sizeof data_as_ints[0], num_ints, fp1);

			}
			fwrite(RSDImage->nextSNPDistance, sizeof(RSDImage->nextSNPDistance[0]), RSDImage->width, fp1);

			//printf("f size %d \n ", (int) sizeof RSDImage->nextSNPDistance[0] );
			
			//assert(0);
			data_as_ints = NULL;
		break;
		
		case BIN_DATA_ALLELE_COUNT:
			if(RSDImage->longIntBuffer==NULL)
			{
				RSDImage->longIntBuffer = (uint64_t*)rsd_malloc(sizeof(uint64_t)*RSDImage->width);
				assert(RSDImage->longIntBuffer!=NULL);
			}
			
			if(RSDImage->derivedAlleleFrequency==NULL)
			{
				RSDImage->derivedAlleleFrequency = (float*)rsd_malloc(sizeof(float)*RSDImage->width);
				assert(RSDImage->derivedAlleleFrequency!=NULL);
			}
			
			for(uint64_t x=0;x<(uint64_t)RSDImage->width;x++)
			{
				RSDImage->longIntBuffer[x] = (uint64_t)RSDChunk->derivedAlleleCount[RSDImage->firstSNPIndex+x];
				
				RSDImage->derivedAlleleFrequency[x] = ((float)RSDChunk->derivedAlleleCount[RSDImage->firstSNPIndex+x]) / ((float)RSDImage->height);
				
				// RSDImage->longIntBuffer[x] = 0ull;
				
				//for(uint64_t y=0;y<(uint64_t)RSDImage->height;y++)
				//{
				//	RSDImage->longIntBuffer[x] += (uint64_t)data[y*RSDImage->width+x];
				//}
				
				//RSDImage->derivedAlleleFrequency[x] = ((float)RSDImage->longIntBuffer[x]) / ((float)RSDImage->height);  

				//assert(0);
								
				//assert(RSDImage->longIntBuffer[x]==(uint64_t)RSDChunk->derivedAlleleCount[RSDImage->firstSNPIndex+x]);
				//printf("prev %lu vs new %lu\n", (uint64_t)RSDChunk->derivedAlleleCount[RSDImage->firstSNPIndex+x], RSDImage->longIntBuffer[x]);
				
				
				//printf("deralf1 %lu\n", RSDImage->longIntBuffer[x]);
				//printf("firstsnpindex: %d\tderalf2 %lu\n", RSDImage->firstSNPIndex, (uint64_t)RSDChunk->derivedAlleleCount[RSDImage->firstSNPIndex+x]);
				
				
				assert(RSDImage->derivedAlleleFrequency[x]>=0.0 && RSDImage->derivedAlleleFrequency[x]<=1.001);
				
				assert(RSDImage->longIntBuffer[x]<=(uint64_t)RSDImage->height);
				assert((uint64_t)RSDChunk->derivedAlleleCount[RSDImage->firstSNPIndex+x]==RSDImage->longIntBuffer[x] || RSDImage->height-(uint64_t)RSDChunk->derivedAlleleCount[RSDImage->firstSNPIndex+x]==RSDImage->longIntBuffer[x]);
			
			}
			
			fwrite(&(RSDImage->width), 1, 4, fp1);
			fwrite(&(targetPos), 1, 8, fp1);
			//fwrite(RSDImage->longIntBuffer, sizeof(RSDImage->longIntBuffer[0]), RSDImage->width, fp1);
			fwrite(RSDImage->derivedAlleleFrequency, sizeof(RSDImage->derivedAlleleFrequency[0]), RSDImage->width, fp1);
			fwrite(RSDImage->nextSNPDistance, sizeof(RSDImage->nextSNPDistance[0]), RSDImage->width, fp1);
		break;
		
		default:
			fprintf(fpOut, "\nERROR: Invalid data-type code (-typ %d) in binary format (-bin)\n\n",RSDCommandLine->imgDataType);
			fprintf(stderr, "\nERROR: Invalid data-type code (-typ %d) in binary format (-bin)\n\n",RSDCommandLine->imgDataType);
			exit(0);
		break;
	}
	
	fclose(fp1);
	
	//assert(0);
	
	return 1;	
}

void RSDImage_rankRows (RSDImage_t * RSDImage)
{
	assert(RSDImage!=NULL);
	
	int i=-1;
	
	for(i=0;i<RSDImage->height;i++)
		RSDImage->rowSortScore[i] = sampleBitCount (RSDImage, RSDImage->data, i);	
		
}

void RSDImage_rankColumns (RSDImage_t * RSDImage)
{
	assert(RSDImage!=NULL);
	
	int i=-1;
	
	for(i=0;i<RSDImage->width;i++)
		RSDImage->colSortScore[i] = snpBitCount (RSDImage, RSDImage->data, i);	
		
}

void RSDImage_rearrangeRowData (RSDImage_t * RSDImage, RSDSort_t * RSDSort)
{
	assert(RSDImage!=NULL);
	assert(RSDSort!=NULL);
	
	int i=-1;
	
	for(i=0;i<RSDImage->height;i++)
		memcpy(&(RSDImage->dataT[i*RSDImage->width]), &(RSDImage->data[RSDSort->indexList[i]*RSDImage->width]), RSDImage->width); 
	
	memcpy(RSDImage->data, RSDImage->dataT, RSDImage->height*RSDImage->width);
}

void RSDImage_rearrangeColumnData (RSDImage_t * RSDImage, RSDSort_t * RSDSort)
{
	assert(RSDImage!=NULL);
	assert(RSDSort!=NULL);
	
	int i=-1, j=-1;
	
	for(i=0;i<RSDImage->width;i++)
		for(j=0;j<RSDImage->height;j++)
			RSDImage->dataT[j*RSDImage->width+i] = RSDImage->data[j*RSDImage->width+RSDSort->indexList[i]]; 
	
	memcpy(RSDImage->data, RSDImage->dataT, RSDImage->height*RSDImage->width);
}


void RSDImage_rearrangeRows (RSDImage_t * RSDImage)
{
	assert(RSDImage!=NULL);
	
	RSDImage_rankRows (RSDImage);
	
	RSDSort_appendScores (RSDImage->rowSorter, (void*)RSDImage, SORT_ROWS);
	
	RSDImage_rearrangeRowData (RSDImage, RSDImage->rowSorter);
}

void RSDImage_rearrangeColumns (RSDImage_t * RSDImage) 
{
	assert(RSDImage!=NULL);
	
	RSDImage_rankColumns (RSDImage);
	
	RSDSort_appendScores (RSDImage->colSorter, (void*)RSDImage, SORT_COLUMNS);
	
	RSDImage_rearrangeColumnData (RSDImage, RSDImage->colSorter);
}

void RSDImage_reorderData (RSDImage_t * RSDImage, RSDCommandLine_t * RSDCommandLine)
{
	assert(RSDImage!=NULL);
	assert(RSDCommandLine!=NULL);
	
	if(RSDCommandLine->imageReorderOpt==PIXEL_REORDERING_DISABLED)
		return;
		
	RSDImage_rearrangeRows (RSDImage); 
	RSDImage_rearrangeColumns (RSDImage); 
}

RSDGridPoint_t * RSDImage_createImagesFlex (RSDImage_t * RSDImage, RSDMuStat_t * RSDMuStat, RSDChunk_t * RSDChunk, RSDPatternPool_t * RSDPatternPool, RSDDataset_t * RSDDataset, RSDCommandLine_t * RSDCommandLine, FILE * fpOut, double targetPos, char * destinationPath, int scoreIndex)
{
	assert(RSDImage!=NULL);
	assert(RSDMuStat!=NULL);
	assert(RSDChunk!=NULL);
	assert(RSDPatternPool!=NULL);
	assert(RSDDataset!=NULL);	
	assert(RSDCommandLine!=NULL);
	assert(fpOut!=NULL);
	
	assert(RSDCommandLine->imagesPerSimulation>=1);
	assert(RSDCommandLine->imageWindowStep>=1);
	
	// Check needed in case chunk boundaries are crossed.
	if(RSDImage->remainingSetImages==0) 
		return NULL;	
	
	int i=-1, j=-1, size = (int)RSDChunk->chunkSize;
	
	assert(size>=RSDMuStat->windowSize); 
	
	// Check if target position is in the current chunk
	if(!(RSDChunk->sitePosition[0]<=targetPos && targetPos <= RSDChunk->sitePosition[size-1]))
		return NULL;		
	
	// Set targetSNPIndex - SNP closest to the target position
	double curDist=-1.0, prvDist = fabs(targetPos - RSDChunk->sitePosition[RSDMuStat->windowSize/2-1]);

	for(i=RSDMuStat->windowSize/2;i<=size-1-RSDMuStat->windowSize/2;i++)
	{
		curDist = fabs(targetPos-RSDChunk->sitePosition[i]);

		//printf("cur %f prv %f\n", curDist, prvDist);
		
		if(curDist<=prvDist)// && curDist>=0)
		{
			prvDist=curDist;
		}
		else
			 break;
	}
	
	int targetSNPIndex = i-1;
	
	
	
	//if(targetSNPIndex<)
	
	
		
	/*****************				
		
	// Set targetSNPIndex - SNP closest to the target position 			
	// Find target SNP (first SNP on the right of target)
	for(i=startSNP;i<=size-1-RSDMuStat->windowSize/2;i++) // -1 because its index. for w=50, the middle snp is index 24.
		if(RSDChunk->sitePosition[i]>=targetPos) 
			break;
			
	int targetSNPIndex = i>size-1-RSDMuStat->windowSize/2?size-1-RSDMuStat->windowSize/2:i; // conditional assignment needed in case previous loop does not break
	assert(targetSNPIndex+RSDMuStat->windowSize/2<=size-1); // make sure targetSNPIndex is valid
	
	double targetSNPDistance = RSDChunk->sitePosition[targetSNPIndex]-targetPos; // assumes targetSNP on the right of target position
	
	if(targetSNPDistance>=0) // 
	{
		if(targetPos-RSDChunk->sitePosition[targetSNPIndex-1]<=targetSNPDistance)
			targetSNPIndex--;
	}
	else
	{
		targetSNPDistance = targetPos-RSDChunk->sitePosition[targetSNPIndex-1];
		
		if(targetPos-RSDChunk->sitePosition[targetSNPIndex-1]<=targetSNPDistance)
			targetSNPIndex--;
		
		
	}
	
	// Find target SNP (closest to target, either before or after target)
	if(targetPos-RSDChunk->sitePosition[targetSNPIndex-1]<=targetSNPDistance)
		targetSNPIndex--;

	if(abs(targetPos-RSDChunk->sitePosition[targetSNPIndex])>abs(targetPos-RSDChunk->sitePosition[targetSNPIndex+1]))
	{
		printf("targetPos: %f targetSNPindex %d sitepos[targetSNPindex] %f sitepos[targetSNPindex+1] %f\n", targetPos, targetSNPIndex, RSDChunk->sitePosition[targetSNPIndex], RSDChunk->sitePosition[targetSNPIndex+1]);
	
	}
	
	assert(0);
	
	*/
	//assert(0);
	/*if(fabs(targetPos-RSDChunk->sitePosition[targetSNPIndex])>fabs(targetPos-RSDChunk->sitePosition[targetSNPIndex+1]))
	{
		printf("targetPos: %f targetSNPindex %d sitepos[targetSNPindex] %f sitepos[targetSNPindex+1] %f [%d %d max %d]\n", targetPos, targetSNPIndex, RSDChunk->sitePosition[targetSNPIndex], RSDChunk->sitePosition[targetSNPIndex+1], targetSNPIndex - RSDMuStat->windowSize/2 + 1, targetSNPIndex + RSDMuStat->windowSize/2, size);
		}
	printf("%.10f %.10f vs %.10f\n", targetPos-RSDChunk->sitePosition[targetSNPIndex], fabs(targetPos-RSDChunk->sitePosition[targetSNPIndex]), fabs(targetPos-RSDChunk->sitePosition[targetSNPIndex+1]));
	*/
	
	if(RSDChunk->sitePosition[targetSNPIndex]>=targetPos && RSDChunk->sitePosition[targetSNPIndex-1]<targetPos)	
		assert(fabs(targetPos-RSDChunk->sitePosition[targetSNPIndex])<=fabs(targetPos-RSDChunk->sitePosition[targetSNPIndex+1]));
	
	if(RSDChunk->sitePosition[targetSNPIndex]<targetPos && RSDChunk->sitePosition[targetSNPIndex+1]>=targetPos)
		assert(fabs(targetPos-RSDChunk->sitePosition[targetSNPIndex])<=fabs(targetPos-RSDChunk->sitePosition[targetSNPIndex-1]));
	
		
	/* snp-centered window */
	int snpf = targetSNPIndex - RSDMuStat->windowSize/2 + 1;
	int snpl = targetSNPIndex + (RSDMuStat->windowSize/2);
	assert(snpf>=0);
	assert(snpl<=size-1);	
	assert(snpl-snpf+1==RSDMuStat->windowSize);
	
	//printf("SNP-centered: size %d tgt %d pos %f p %f a %f [W center %f]\n", size, targetSNPIndex, RSDChunk->sitePosition[targetSNPIndex], RSDChunk->sitePosition[targetSNPIndex-1], RSDChunk->sitePosition[targetSNPIndex+1], (RSDChunk->sitePosition[snpf]+RSDChunk->sitePosition[snpl])/2.0);
	
	/* bp-centered window */
	// Center first window/image at targetPos
	int snpff = snpf;
	int snpll = snpl;
		
	// while(snpll-snpff<RSDMuStat->windowSize-1)
	// {
	// 	if(snpff>=0 && snpll<=size-1)
	// 	{
	// 		if((RSDChunk->sitePosition[snpll]-targetPos)>targetPos-RSDChunk->sitePosition[snpff])
	// 			snpff--;
	// 		else
	// 			snpll++;
	// 	}
	// 	else
	// 		break;	
	// }
	
	// if(snpff<0)
	// {
	// 	snpff=0;
	// 	snpll=RSDMuStat->windowSize-1;
	// }
	
	// if(snpll>size-1)
	// {
	// 	snpll = size-1;
	// 	snpff = snpll-RSDMuStat->windowSize+1;
	// }

	assert(snpff>=0);
	assert(snpll<=size-1);		
	assert(snpll-snpff+1==RSDMuStat->windowSize);
		
	int newtargetSNPIndex = snpff + (RSDMuStat->windowSize/2)-1;	
	
	if(RSDCommandLine->imagePositionCenteredEn==1)	
		targetSNPIndex = newtargetSNPIndex;
		
	//printf("POS-centered: size %d tgt %d pos %f p %f a %f [W center %f -%d %d-]\n\n", size, targetSNPIndex, RSDChunk->sitePosition[targetSNPIndex], RSDChunk->sitePosition[targetSNPIndex-1], RSDChunk->sitePosition[targetSNPIndex+1], (RSDChunk->sitePosition[snpff]+RSDChunk->sitePosition[snpll])/2.0, snpff, snpll);
		

	/********************************/		
	
	// Each step generates a different image
	int stepsTotal = RSDCommandLine->imagesPerSimulation; 
	
	int stepsLeft = stepsTotal / 2;
	int stepsRight = stepsTotal - stepsLeft;
	
	int gridPointSize = 0;  
	
	// Boundary conditions for targetSNPIndex, shift first/last grid points to make sure we get results in case they are too close to chunk borders
	if((targetSNPIndex-stepsLeft*RSDCommandLine->imageWindowStep - RSDMuStat->windowSize/2 + 1)<0)
		targetSNPIndex = stepsLeft*RSDCommandLine->imageWindowStep + RSDMuStat->windowSize/2 - 1;	
	
	if((targetSNPIndex+stepsRight*RSDCommandLine->imageWindowStep + (RSDMuStat->windowSize/2))>size-1)
		targetSNPIndex = size-stepsRight*RSDCommandLine->imageWindowStep - RSDMuStat->windowSize/2-1;
	
	RSDGridPoint_t * RSDGridPoint = NULL;
	
	if(RSDCommandLine->opCode == OP_USE_CNN)
		RSDGridPoint = RSDGridPoint_new();	
		
	//TODO-AI-5
	// TODO: Consider alternative algos here. Only max score. Average all. DP like OmegaPlus for best combination.
	for(i=targetSNPIndex-stepsLeft*RSDCommandLine->imageWindowStep;i<=targetSNPIndex+stepsRight*RSDCommandLine->imageWindowStep+10;i=i+RSDCommandLine->imageWindowStep)
	{
		// SNP window range (this is different than the default sliding-window mu-statistic implementation)
		int snpf = i - RSDMuStat->windowSize/2 + 1;
		int snpl = i + (RSDMuStat->windowSize/2);
		assert(snpl-snpf+1==RSDMuStat->windowSize);
		
		if(snpf>=0 && snpl<=size-1)
		{	
			// Window center (bp) - Setting grid point location
			double windowCenter = targetPos; //round((RSDChunk->sitePosition[snpf] + RSDChunk->sitePosition[snpl]) / 2.0); // TODO: change this if we we want to estimate the region used?
			//double windowCenter = round((RSDChunk->sitePosition[snpf] + RSDChunk->sitePosition[snpl]) / 2.0); // TODO: change this if we we want to estimate the region used?
			RSDGridPoint_addNewPosition (RSDGridPoint, windowCenter, RSDCommandLine->networkArchitecture); 			
			
			/****************************************************************/
			// Pairwise SNP distances			
			for(j=0;j<RSDMuStat->windowSize-1;j++)
			{
				double d = RSDChunk->sitePosition[snpf+j+1]-RSDChunk->sitePosition[snpf+j];
				assert(d>=0.0);
				RSDImage->nextSNPDistance[j] = d; 
			}
			
			// Average total image width (division takes places later)
			RSDImage->avgImageWidthBP +=  RSDChunk->sitePosition[snpl] - RSDChunk->sitePosition[snpf];
			
			
			/***************************************************************/			
			//if(RSDCommandLine->opCode == OP_USE_CNN) // TODO: CLEAN THIS UP... NOT EVERYTHING NEEDED
			//{		
				int winlsnpf = snpf;
				int winlsnpl = (int)(winlsnpf + RSDMuStat->windowSize/2 - 1);

				int winrsnpf = winlsnpl + 1;
				int winrsnpl = snpl;	
				
					
				double windowStart = RSDChunk->sitePosition[snpf];
				double windowEnd = RSDChunk->sitePosition[snpl];

				float isValid = 1.0;		

				for(int j=0;j<RSDMuStat->excludeRegionsTotal;j++)
					if((windowEnd>=RSDMuStat->excludeRegionStart[j]) && (windowStart<=RSDMuStat->excludeRegionStop[j]))
						isValid = 0.0;

				// Mu_Var
				float muVar = (float)(RSDChunk->sitePosition[snpl] - RSDChunk->sitePosition[snpf]);
				muVar /= RSDDataset->setRegionLength;
				muVar /= RSDMuStat->windowSize;
				//muVar *= RSDDataset->setSNPs;
				muVar *= RSDDataset->preLoadedsetSNPs; // v2.4
				
			if(RSDCommandLine->opCode == OP_USE_CNN) // TODO: CLEAN THIS UP... NOT EVERYTHING NEEDED
			{	
				RSDGridPoint->muVarScores[RSDGridPoint->size-1] = muVar;
				
				
				// Mu_Sfs
				int dCnt1 = 0, dCntN = 0;
				for(int j=snpf;j<=snpl;j++)
				{
					dCnt1 += (RSDChunk->derivedAlleleCount[j]<=RSDCommandLine->sfsSlack);
					dCntN += (RSDChunk->derivedAlleleCount[j]>=RSDDataset->setSamples-RSDCommandLine->sfsSlack);
				}
				
				float facN = 1.0;//(float)RSDChunk->derAll1CntTotal/(float)RSDChunk->derAllNCntTotal;
				float muSfs = (float)dCnt1 + (float)dCntN*facN; 

				if(dCnt1+dCntN==0) 
					muSfs = 0.0000000001f;

				muSfs *= RSDDataset->muVarDenom; 
				muSfs /= (float)RSDMuStat->windowSize;
			
				RSDGridPoint->muSfsScores[RSDGridPoint->size-1] = muSfs;
				
				
				// Mu_Ld
				int pcntl = 0, pcntr = 0, pcntexll=0, pcntexlr=0;
				
				float tempTest = getPatternCounts (WIN_MODE_FXD, RSDMuStat, (int)RSDMuStat->windowSize, (int)RSDMuStat->windowSize, RSDChunk->patternID, winlsnpf, winlsnpl, winrsnpf, winrsnpl, &pcntl, &pcntr, &pcntexll, &pcntexlr);
				float muLd = tempTest;

				if(pcntexll + pcntexlr==0) 
				{
					muLd = 0.0000000001f;
				}
				else
				{
					muLd = (((float)pcntexll)+((float)pcntexlr)) / ((float)(pcntl * pcntr));
				}
			
				RSDGridPoint->muLdScores[RSDGridPoint->size-1] = muLd;
				
				// Mu
				float mu =  powf(muVar, RSDCommandLine->muVarExp) * powf(muSfs, RSDCommandLine->muSfsExp) * powf(muLd, RSDCommandLine->muLdExp) * isValid;
				
				if(isinf(mu)==1)
				{
					fprintf(fpOut, "\n\nERROR: infinite mu-statistic score was found. Restart the run with smaller exponents. \n\n");
					fprintf(stderr, "\n\nERROR: infinite mu-statistic score was found. Restart the run with smaller exponents.\n\n");
					exit(0);
				}
			
				RSDGridPoint->muScores[RSDGridPoint->size-1] = mu;
			}
			
			
			/*******************************/
		
			RSDImage_setRange (RSDImage, RSDChunk, (int64_t) snpf, (int64_t)snpl);
			RSDImage_getData (RSDImage, RSDChunk, RSDPatternPool, RSDDataset);
			
			RSDImage_reorderData (RSDImage, RSDCommandLine);
							
			if(RSDCommandLine->enBinFormat==0)
				RSDImage_savePNG (RSDImage, RSDChunk, RSDDataset, RSDCommandLine, i, RSDImage->data, destinationPath, muVar, 1.0, scoreIndex, &gridPointSize, fpOut);
			else // pytorch
				RSDImage_saveBIN (RSDImage, RSDChunk, RSDDataset, RSDCommandLine, i, RSDImage->data, destinationPath, 1.0, 1.0, scoreIndex, &gridPointSize, windowCenter, fpOut);
			
			RSDImage->remainingSetImages--;
			RSDImage->generatedSetImages++;
			RSDImage->totalGeneratedImages++;
			
			if(RSDImage->remainingSetImages==0)
				break;		
		}
	}
	
	if(RSDCommandLine->opCode == OP_USE_CNN)
		assert (gridPointSize==RSDGridPoint->size);
	
	return RSDGridPoint;	
}

#endif

