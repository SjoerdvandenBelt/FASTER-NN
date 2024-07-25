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

RSDResults_t * RSDResults_new (RSDCommandLine_t * RSDCommandLine)
{
	if(RSDCommandLine->opCode==OP_DEF || RSDCommandLine->opCode==OP_CREATE_IMAGES)
		return NULL;
	
	RSDResults_t * RSDResults = (RSDResults_t *)rsd_malloc(sizeof(RSDResults_t));
	assert(RSDResults!=NULL);
	
	RSDResults->setsTotal = 0;
	RSDResults->setID = NULL;
	RSDResults->setGridSize = -1; 
	RSDResults->gridPointSize = NULL;
	RSDResults->gridPointData = NULL;
	RSDResults->nnMax = 0.0f;
	RSDResults->nnMaxLoc = 0.0;
	RSDResults->compnnMax = 0.0f;
	RSDResults->compnnMaxLoc = 0.0;
		
	return RSDResults;
}

void RSDResults_setGridSize (RSDResults_t * RSDResults, RSDCommandLine_t * RSDCommandLine)
{
	if(RSDResults==NULL)
		return;
		
	assert(RSDCommandLine!=NULL);
	
	RSDResults->setGridSize = RSDCommandLine->gridSize;
	
	assert(RSDResults->setGridSize>=1);
}

void RSDResults_incrementSetCounter (RSDResults_t * RSDResults)
{
	if(RSDResults==NULL)
		return;
	
	RSDResults->setsTotal++;
	
	RSDResults->setID = rsd_realloc (RSDResults->setID, sizeof(char*)*((unsigned long)RSDResults->setsTotal));
	RSDResults->setID[RSDResults->setsTotal-1] = rsd_malloc (sizeof(char)*STRING_SIZE);
	RSDResults->gridPointSize = rsd_realloc(RSDResults->gridPointSize, sizeof(int64_t)*((size_t)RSDResults->setsTotal*RSDResults->setGridSize));
	RSDResults->gridPointData = rsd_realloc(RSDResults->gridPointData, sizeof(RSDGridPoint_t*)*((unsigned long)RSDResults->setsTotal*RSDResults->setGridSize));
	
	int64_t i=-1;
	int64_t sta = (RSDResults->setsTotal-1)*RSDResults->setGridSize;
	int64_t sto = sta + RSDResults->setGridSize - 1; 
	
	for(i=sta;i<=sto;i++)
	{
		RSDResults->gridPointSize[i]=-1;
		RSDResults->gridPointData[i]=NULL;
	}			
}

void RSDResults_free (RSDResults_t * RSDResults)
{
	if(RSDResults==NULL)
		return;
	
	int i;
	
	if(RSDResults->setID!=NULL)
	{
		for(i=0;i<RSDResults->setsTotal;i++)
		{
			if(RSDResults->setID[i]!=NULL)
			{
				free(RSDResults->setID[i]);
				RSDResults->setID[i] = NULL;
			}
		}
		
		free(RSDResults->setID);
		RSDResults->setID=NULL;
	}
	
	if(RSDResults->gridPointSize!=NULL)
	{
		free(RSDResults->gridPointSize);
		RSDResults->gridPointSize=NULL;
	}	
	
	if(RSDResults->gridPointData!=NULL)
	{
		for(i=0;i<RSDResults->setsTotal*RSDResults->setGridSize;i++)
		{
			RSDGridPoint_free(RSDResults->gridPointData[i]);
		}
		free(RSDResults->gridPointData);
		RSDResults->gridPointData=NULL; 
	}		

	free(RSDResults);
	RSDResults = NULL;
}

void RSDResults_saveSetID (RSDResults_t * RSDResults, RSDDataset_t * RSDDataset)
{
	if(RSDResults==NULL)
		return;
		
	assert(RSDDataset!=NULL);
	
	strcpy(RSDResults->setID[RSDResults->setsTotal-1], RSDDataset->setID);
}

void RSDResults_setGridPointSize (RSDResults_t * RSDResults, int64_t gridPointIndex, int64_t gridPointSize)
{
	if(RSDResults==NULL)
		return;
		
	assert(RSDResults->gridPointSize!=NULL);
	assert(RSDResults->setsTotal*RSDResults->setGridSize>gridPointIndex);
	assert(gridPointIndex>=0);
	
	assert(RSDResults->gridPointSize[(RSDResults->setsTotal-1)*RSDResults->setGridSize+gridPointIndex]==-1);

	RSDResults->gridPointSize[(RSDResults->setsTotal-1)*RSDResults->setGridSize+gridPointIndex] = gridPointSize;		
}

void RSDResults_setGridPoint (RSDResults_t * RSDResults, int64_t gridPointIndex, RSDGridPoint_t * RSDGridPoint)
{
	if(RSDResults==NULL)
		return;
		
	assert(RSDResults->gridPointSize!=NULL);
	assert(RSDResults->gridPointData!=NULL);
	assert(RSDResults->setsTotal*RSDResults->setGridSize>gridPointIndex);
	assert(gridPointIndex>=0);
	
	assert(RSDResults->gridPointSize[(RSDResults->setsTotal-1)*RSDResults->setGridSize+gridPointIndex]==-1);
	assert(RSDResults->gridPointData[(RSDResults->setsTotal-1)*RSDResults->setGridSize+gridPointIndex]==NULL);

	RSDResults->gridPointSize[(RSDResults->setsTotal-1)*RSDResults->setGridSize+gridPointIndex] = RSDGridPoint->size;
	RSDResults->gridPointData[(RSDResults->setsTotal-1)*RSDResults->setGridSize+gridPointIndex] = RSDGridPoint;		
}

void RSDResults_load (RSDResults_t * RSDResults, RSDCommandLine_t * RSDCommandLine)
{
	if(RSDResults==NULL)
		return;
		
	assert(RSDCommandLine!=NULL);
	
	char reportPath[STRING_SIZE], rline[STRING_SIZE]; 
	
	//int i=-1, ensembleSize = 1;
	//char nn_i [STRING_SIZE];
	
	//for(i=0;i<ensembleSize;i++)
	{
		//sprintf(reportPath, "tempOutputFolder%d/PredResults.txt", i);
		strncpy(reportPath, "tempOutputFolder/PredResults.txt", STRING_SIZE);
		
		FILE * fp = fopen(reportPath, "r");
		assert(fp!=NULL);
		
		// TODO: The class number should be a parameter here.
		
		while(fgets(rline, STRING_SIZE, fp)!=NULL) 
		{
	    		//printf("%s: ", rline);
	    		
	    		char * imgName = strtok(rline, " ");
	    		char * imgClass = strtok(NULL, " ");
	    		assert(imgClass!=NULL);
	    		
	    		char * imgProb0 = strtok(NULL, " ");
	    		assert(imgProb0!=NULL);
	    		char * imgProb1 = strtok(NULL, " ");
	    		
	    		
	    		
	    		int setIndex=-1, gridPointIndex=-1, gridPointDataIndex=-1;
	    		
	    		getIndicesFromImageName(imgName, &setIndex, &gridPointIndex, &gridPointDataIndex);
	    		
	    		//if(i==0)
	    		{
		    		if(RSDCommandLine->positiveClassIndex[0]==0)
			       		RSDResults->gridPointData[setIndex*RSDResults->setGridSize+gridPointIndex]->nnScores0[gridPointDataIndex] = (float)atof(imgProb0);
			       	else
			    		if(RSDCommandLine->positiveClassIndex[0]==1)
				       		RSDResults->gridPointData[setIndex*RSDResults->setGridSize+gridPointIndex]->nnScores0[gridPointDataIndex] = (float)atof(imgProb1);
			       		else
			       			assert(0);
			       			
	       		}
	       		//else
	       		//{
	       		//	if(RSDCommandLine->positiveClassIndex==0)
			 //      		RSDResults->gridPointData[setIndex*RSDResults->setGridSize+gridPointIndex]->nnScores[gridPointDataIndex] += (float)atof(imgProb0);
			//       	else
			//    		if(RSDCommandLine->positiveClassIndex==1)
			//	       		RSDResults->gridPointData[setIndex*RSDResults->setGridSize+gridPointIndex]->nnScores[gridPointDataIndex] += (float)atof(imgProb1);
			////       		else
			//       			assert(0);
	       		//
	       		//}
	       		//assert(RSDResults->gridPointData[setIndex*RSDResults->setGridSize+gridPointIndex]->nnScores[gridPointDataIndex]>=-0.0001);
	       		//assert(RSDResults->gridPointData[setIndex*RSDResults->setGridSize+gridPointIndex]->nnScores[gridPointDataIndex]<=1.0001);
		}
		
		fclose(fp);
	}
	
	exec_command ("rm -r tempOutputFolder");
}

void RSDResults_load_2x2 (RSDResults_t * RSDResults, RSDCommandLine_t * RSDCommandLine)
{
	if(RSDResults==NULL)
		return;
		
	assert(RSDCommandLine!=NULL);
	
	char reportPath[STRING_SIZE], rline[STRING_SIZE];
	char * imgProb[CLA_SWEEPNETRECOMB]; 
	
	//int i=-1, ensembleSize = 1;
	//char nn_i [STRING_SIZE];
	
	int i=-1;
	
	//XXXXXXXXXX need TO FIX THE PRINTING OF VAR^nn IN THE CASE OF SWEEPNET RECOMB. IT IS NOT VAR^NN. full testing all AND ADD MORE CLASSES?
	
	//for(i=0;i<ensembleSize;i++)
	{
		//sprintf(reportPath, "tempOutputFolder%d/PredResults.txt", i);
		strncpy(reportPath, "tempOutputFolder/PredResults.txt", STRING_SIZE);
		//strncpy(reportPath, "tempOutputFolderNEW/PredResults.txt", STRING_SIZE);
		
		FILE * fp = fopen(reportPath, "r");
		assert(fp!=NULL);
		
		while(fgets(rline, STRING_SIZE, fp)!=NULL) 
		{
	    		//printf("%s: ", rline);
	    		
	    		char * imgName = strtok(rline, " ");	    		
	    		
	    		//printf("%s - %s: ", imgName, rline);
	    		//fflush(stdout);
	    		
	    		char * imgClassRows = strtok(NULL, " ");
	    		assert(imgClassRows!=NULL);

	    		//printf("%s - %s: ", imgName, rline);
	    		//fflush(stdout);	    		
	    			    		
	    		char * imgClassCols = strtok(NULL, " ");
	    		assert(imgClassCols!=NULL);
	    		
	    		for(i=0;i<CLA_SWEEPNETRECOMB;i++)
	    		{
	    			imgProb[i] = strtok(NULL, " ");
		    		assert(imgProb[i]!=NULL); 
		    		
		    		//printf(" %d=%s ", i, imgProb[i]);   		
	    		
	    		}
	    		
	    		
	    			    		
	    	/*	
//		    		char * imgProb00 = strtok(NULL, " ");
//		    		assert(imgProb00!=NULL);

		    		imgProb[0] = strtok(NULL, " ");
		    		assert(imgProb[0]!=NULL);

		    		
				printf("a %s ", imgProb[0]);
		    		
		    		char * imgProb10 = strtok(NULL, " ");
		    		assert(imgProb10!=NULL);
		    		
printf("b %s ", imgProb10);
		    		
		    		char * imgProb01 = strtok(NULL, " ");
		    		assert(imgProb01!=NULL);
		    		
printf("%s ", imgProb01);
		    		
		    		char * imgProb11 = strtok(NULL, " ");
		    		assert(imgProb11!=NULL);	    		
		    			    		
printf("%s ", imgProb11);
	    		
	    		//}    
	    	*/
	    		
	    		int setIndex=-1, gridPointIndex=-1, gridPointDataIndex=-1;
	    		getIndicesFromImageName(imgName, &setIndex, &gridPointIndex, &gridPointDataIndex);
	    		    		
	    		
		    	RSDResults->gridPointData[setIndex*RSDResults->setGridSize+gridPointIndex]->nnScores0[gridPointDataIndex] = (float)atof(imgProb[RSDCommandLine->positiveClassIndex[0]]);	    					
		   	RSDResults->gridPointData[setIndex*RSDResults->setGridSize+gridPointIndex]->nnScores1[gridPointDataIndex] = (float)atof(imgProb[RSDCommandLine->positiveClassIndex[1]]);
	    		
//		    	RSDResults->gridPointData[setIndex*RSDResults->setGridSize+gridPointIndex]->nnScores0[gridPointDataIndex] = (float)atof(imgProb[0]);	    					
//		    	RSDResults->gridPointData[setIndex*RSDResults->setGridSize+gridPointIndex]->nnScores1[gridPointDataIndex] = (float)atof(imgProb[1]);
//		    	RSDResults->gridPointData[setIndex*RSDResults->setGridSize+gridPointIndex]->nnScores2[gridPointDataIndex] = (float)atof(imgProb[2]);	    		
//		    	RSDResults->gridPointData[setIndex*RSDResults->setGridSize+gridPointIndex]->nnScores3[gridPointDataIndex] = (float)atof(imgProb[3]);
		    				    				    		
		    				    				    		
	    		//printf("%d - %f %f\n", setIndex, (float)atof(imgProb10), (float)atof(imgProb11)); 
	    		    		

	    		
	    		
	    		/*{
		    		if(RSDCommandLine->positiveClassIndex==0)
			       		RSDResults->gridPointData[setIndex*RSDResults->setGridSize+gridPointIndex]->nnScores[gridPointDataIndex] = (float)atof(imgProb00);
			       	else
			    		if(RSDCommandLine->positiveClassIndex==1)
				       		RSDResults->gridPointData[setIndex*RSDResults->setGridSize+gridPointIndex]->nnScores[gridPointDataIndex] = (float)atof(imgProb10);
			       		else
			       			assert(0);			       			
	       		}*/
		}
		
		fclose(fp);
	}
	
	exec_command ("rm -r tempOutputFolder");
}



/*
void RSDResults_process (RSDResults_t * RSDResults)
{
	if(RSDResults==NULL)
		return;
	
	for(int i=0;i<RSDResults->setsTotal;i++)
	{
		for(int j=0;j<RSDResults->setGridSize;j++)
		{
			//printf("GridPoint %d Size %d\n", j, (int)RSDResults->gridPointSize[i*RSDResults->setGridSize+j]);
			
			//printf("GridPoint %d Size %d\n", j, (int)RSDResults->gridPointData[i*RSDResults->setGridSize+j]->size);
			//
			
			float gridPointFinalScore = 0.0;
			double gridPointPosition = (RSDResults->gridPointData[i*RSDResults->setGridSize+j]->positions[(int)RSDResults->gridPointData[i*RSDResults->setGridSize+j]->size-1]+RSDResults->gridPointData[i*RSDResults->setGridSize+j]->positions[0])/2.0;							
			
			for (int k = 0; k<(int)RSDResults->gridPointData[i*RSDResults->setGridSize+j]->size;k++)
			{
				gridPointFinalScore += RSDResults->gridPointData[i*RSDResults->setGridSize+j]->nnScores[k];
				
				//printf("k=0: %f %f\n",RSDResults->gridPointData[i*RSDResults->setGridSize+j]->positions[k], RSDResults->gridPointData[i*RSDResults->setGridSize+j]->scores[k]);
			}
			gridPointFinalScore /= (float)RSDResults->gridPointData[i*RSDResults->setGridSize+j]->size;
			RSDResults->gridPointData[i*RSDResults->setGridSize+j]->positions[0] = gridPointPosition;
			RSDResults->gridPointData[i*RSDResults->setGridSize+j]->nnScores[0] = gridPointFinalScore;
		}

	}
}
*/

void RSDResults_resetScores (RSDResults_t * RSDResults)
{
	assert(RSDResults!=NULL);
	
	RSDResults->nnMax = 0.0f; 
	RSDResults->nnMaxLoc = 0.0;
		
	RSDResults->compnnMax = 0.0f;
	RSDResults->compnnMaxLoc = 0.0;
}

void RSDResults_processSet (RSDResults_t * RSDResults, RSDNeuralNetwork_t * RSDNeuralNetwork, RSDCommandLine_t * RSDCommandLine, RSDMuStat_t * RSDMuStat, int setIndex)
{
	assert(RSDResults!=NULL);
	assert(RSDCommandLine!=NULL);
	assert(RSDMuStat!=NULL);

	RSDMuStat_resetScores (RSDMuStat);
	RSDResults_resetScores (RSDResults);
	
	RSDMuStat->currentScoreIndex=-1;
	
	int i = setIndex;
	
	for(int j=0;j<RSDResults->setGridSize;j++)
	{
		RSDGridPoint_t * RSDGridPoint = RSDResults->gridPointData[i*RSDResults->setGridSize+j];
		RSDGridPoint_reduce (RSDGridPoint, 1, RSDNeuralNetwork->networkArchitecture); //0: avg, 1: max 
		
/*		
		//printf("GridPoint %d Size %d\n", j, (int)RSDResults->gridPointSize[i*RSDResults->setGridSize+j]);				
		//printf("GridPoint %d Size %d\n", j, (int)RSDResults->gridPointData[i*RSDResults->setGridSize+j]->size);
		//							
		
		// SOS TODO
		// TODO median here
		//double mean = 0.0;
		
		double prodScore = 1.0;
		double meanScore = 0.0;
		
		double muVarScore = 1.0;
		double muSfsScore = 1.0;
		double muLdScore = 1.0;
		double muScore = 1.0;
		
		
		
		double nnScore = 0.0;
		
		for (int k = 0; k<(int)RSDResults->gridPointData[i*RSDResults->setGridSize+j]->size;k++)
		{
			//int k = 0;
			//printf("%d: %f %f\n",j, RSDResults->gridPointData[i*RSDResults->setGridSize+j]->positions[k], RSDResults->gridPointData[i*RSDResults->setGridSize+j]->scores[k]);
			
			
			//RSDMuStat_storeOutput (RSDMuStat, (double)RSDResults->gridPointData[i*RSDResults->setGridSize+j]->positions[k], 
			//				  (double)RSDResults->gridPointData[i*RSDResults->setGridSize+j]->positions[k], 
			//				  (double)RSDResults->gridPointData[i*RSDResults->setGridSize+j]->positions[k], 
			//				  (double)RSDResults->gridPointData[i*RSDResults->setGridSize+j]->scores[k], 
			//				  (double)RSDResults->gridPointData[i*RSDResults->setGridSize+j]->scores[k], 
			//				  (double)RSDResults->gridPointData[i*RSDResults->setGridSize+j]->scores[k], 
			//				  (double)RSDResults->gridPointData[i*RSDResults->setGridSize+j]->scores[k]);
			
			
			prodScore *= (double)RSDResults->gridPointData[i*RSDResults->setGridSize+j]->nnScores[k];
			meanScore += (double)RSDResults->gridPointData[i*RSDResults->setGridSize+j]->nnScores[k];
			
			muVarScore *= (double)RSDResults->gridPointData[i*RSDResults->setGridSize+j]->muVarScores[k];
			muSfsScore *= (double)RSDResults->gridPointData[i*RSDResults->setGridSize+j]->muSfsScores[k];
			muLdScore *= (double)RSDResults->gridPointData[i*RSDResults->setGridSize+j]->muLdScores[k];
			muScore += (double)RSDResults->gridPointData[i*RSDResults->setGridSize+j]->muScores[k];
			nnScore += (double)RSDResults->gridPointData[i*RSDResults->setGridSize+j]->nnScores[k];
		}
*/		
		
		
		// Set final grid-point position
		//RSDResults->gridPointData[i*RSDResults->setGridSize+j]->finalRegionCenter = (double)RSDResults->gridPointData[i*RSDResults->setGridSize+j]->positions[0];//
		
		
//				((double)RSDResults->gridPointData[i*RSDResults->setGridSize+j]->positions[(int)RSDResults->gridPointData[i*RSDResults->setGridSize+j]->size-1]+//
//				 (double)RSDResults->gridPointData[i*RSDResults->setGridSize+j]->positions[0])*0.5;//(double)RSDResults->gridPointData[i*RSDResults->setGridSize+j]->positions[0];
		//RSDResults->gridPointData[i*RSDResults->setGridSize+j]->finalRegionScore = 0.0*powf(muScore,nnScore);//prodScore;// muVarScore; // *// (double)RSDResults->gridPointData[i*RSDResults->setGridSize+j]->scores[0];
		
		//https://blogs.sas.com/content/iml/2021/06/01/hampel-filter-robust-outliers.html
		
		RSDMuStat->currentScoreIndex++; 		


		if(!strcmp(RSDNeuralNetwork->networkArchitecture, ARC_SWEEPNETRECOMB))
		{

			RSDMuStat_output2BufferFullExtended (RSDMuStat, RSDGridPoint->positionFinal, 
							     -1.0, 
							     -1.0,
							     RSDGridPoint->muVarScoreFinal, //powf(nnScore,muVarScore)*1.0, //muVarScore*1.0,//meanScore/RSDResults->gridPointData[i*RSDResults->setGridSize+j]->size, //3
							     RSDGridPoint->muSfsScoreFinal, // muScore*1.0,//RSDResults->gridPointData[i*RSDResults->setGridSize+j]->finalRegionScore, //4
							     RSDGridPoint->muLdScoreFinal, 
							     RSDGridPoint->muScoreFinal, //(double)nnScore/1, //5
							     RSDGridPoint->nnScore0Final,
							     //RSDGridPoint->nnScoreFinal*RSDGridPoint->muVarScoreFinal);//powf(muVarScore,nnScore)*1.0);// muScore*nnScore);	//6			
							     RSDGridPoint->nnScore1Final,
							     RSDGridPoint->nnScore2Final,
							     RSDGridPoint->nnScore3Final,
							     CLA_SWEEPNETRECOMB);//powf(RSDGridPoint->muVarScoreFinal, RSDGridPoint->nnScoreFinal));//powf(muVarScore,nnScore)*1.0);// muScore*nnScore);	//6
		}
		else
		{
			RSDMuStat_output2BufferFullExtended (RSDMuStat, RSDGridPoint->positionFinal, 
							     -1.0, 
							     -1.0,
							     RSDGridPoint->muVarScoreFinal, //powf(nnScore,muVarScore)*1.0, //muVarScore*1.0,//meanScore/RSDResults->gridPointData[i*RSDResults->setGridSize+j]->size, //3
							     RSDGridPoint->muSfsScoreFinal, // muScore*1.0,//RSDResults->gridPointData[i*RSDResults->setGridSize+j]->finalRegionScore, //4
							     RSDGridPoint->muLdScoreFinal, 
							     RSDGridPoint->muScoreFinal, //(double)nnScore/1, //5
							     RSDGridPoint->nnScore0Final,
							     //RSDGridPoint->nnScoreFinal*RSDGridPoint->muVarScoreFinal);//powf(muVarScore,nnScore)*1.0);// muScore*nnScore);	//6			
							     powf(RSDGridPoint->muVarScoreFinal, RSDGridPoint->nnScore0Final),
							     -1.0, 
							     -1.0, 
							     CLA_SWEEPNET);//powf(muVarScore,nnScore)*1.0);// muScore*nnScore);	//6
						   
		}			

	}
	
	
	//RSDMuStat_removeOutliers (RSDMuStat, RSDCommandLine);
	
	//TODO: Final combination should be here.. after having removed outliers
	
	/*for(int j=0;j<RSDCommandLine->gridSize;j++)
	{
		//RSDMuStat->buffer6Data[j] = powf(RSDMuStat->buffer4Data[j],RSDMuStat->buffer5Data[j]);
		RSDMuStat->buffer6Data[j] = RSDMuStat->buffer4Data[j]*RSDMuStat->buffer5Data[j];

	}
	*/
	//RSDMuStat_removeOutliers (RSDMuStat, RSDCommandLine);


	
	
	for(int j=0;j<RSDCommandLine->gridSize;j++)
	{
		// MuVar Max
		if (RSDMuStat->buffer3Data[j] > RSDMuStat->muVarMax)
		{
			RSDMuStat->muVarMax = RSDMuStat->buffer3Data[j] ;
			RSDMuStat->muVarMaxLoc = RSDMuStat->buffer0Data[j];
		}
		
		// MuSfs Max
		if (RSDMuStat->buffer4Data[j] > RSDMuStat->muSfsMax)
		{
			RSDMuStat->muSfsMax = RSDMuStat->buffer4Data[j] ;
			RSDMuStat->muSfsMaxLoc = RSDMuStat->buffer0Data[j];
		}
		
		// MuLd Max	
		if (RSDMuStat->buffer5Data[j] > RSDMuStat->muLdMax)
		{
			RSDMuStat->muLdMax = RSDMuStat->buffer5Data[j] ;
			RSDMuStat->muLdMaxLoc = RSDMuStat->buffer0Data[j];
		}
		
		// Mu Max
		if (RSDMuStat->buffer6Data[j] > RSDMuStat->muMax)
		{
			RSDMuStat->muMax = RSDMuStat->buffer6Data[j] ;
			RSDMuStat->muMaxLoc = RSDMuStat->buffer0Data[j];
		}
				
		// NN Max
		if(RSDMuStat->buffer7Data[j]>RSDResults->nnMax)
		{
			RSDResults->nnMax = RSDMuStat->buffer7Data[j] ;
			RSDResults->nnMaxLoc = RSDMuStat->buffer0Data[j];
		}
		
		// Comb Max pow(mUVar, nn)
		if(RSDMuStat->buffer8Data[j]>RSDResults->compnnMax)
		{
			RSDResults->compnnMax = RSDMuStat->buffer8Data[j] ;
			RSDResults->compnnMaxLoc = RSDMuStat->buffer0Data[j];
		}
		
		
		
	/*	
	//	if (muLdScore > RSDMuStat->muLdMax)
	//	{
	//		RSDMuStat->muLdMax = muLdScore;
	//		RSDMuStat->muLdMaxLoc = RSDResults->gridPointData[i*RSDResults->setGridSize+j]->finalRegionCenter;
	//	}
		
	
		
		//printf("IIN %f max %f at %f\n", RSDMuStat->buffer5Data[j], RSDResults->nnMax, RSDResults->nnMaxLoc);
		// NN Max
		if (RSDMuStat->buffer5Data[j] > RSDResults->nnMax)
		{
			RSDResults->nnMax = (float)RSDMuStat->buffer5Data[j] ;
			RSDResults->nnMaxLoc = RSDMuStat->buffer0Data[j];
			
		//	printf("in %f max %f at %f\n", RSDMuStat->buffer5Data[j], RSDResults->nnMax, RSDResults->nnMaxLoc);
		}

		// Mu Max
		if(RSDMuStat->buffer6Data[j]>RSDResults->maxGridPointScore)
		{
			RSDResults->maxGridPointScore = RSDMuStat->buffer6Data[j] ;
			RSDResults->maxGridPointPosition = RSDMuStat->buffer0Data[j];
		}
		
		// NN Max
		if(RSDMuStat->buffer7Data[j]>RSDResults->maxGridPointScore)
		{
			RSDResults->maxGridPointScore = RSDMuStat->buffer6Data[j] ;
			RSDResults->maxGridPointPosition = RSDMuStat->buffer0Data[j];
		}
		*/				
	}
	

}

void RSDResults_process (RSDResults_t * RSDResults, RSDNeuralNetwork_t * RSDNeuralNetwork, RSDCommandLine_t * RSDCommandLine, RSDDataset_t * RSDDataset, RSDMuStat_t * RSDMuStat, RSDCommonOutliers_t * RSDCommonOutliers)
{
	assert(RSDResults!=NULL);
	assert(RSDCommandLine!=NULL);
	assert(RSDDataset!=NULL);
	assert(RSDMuStat!=NULL);
	assert(RSDCommonOutliers!=NULL);
	
	static int slen[13]={0};
	
	char colHeader1[STRING_SIZE], colHeader2[STRING_SIZE];	
	
	for(int i=0;i<RSDResults->setsTotal;i++)
	{
		//RSDMuStat_resetScores (RSDMuStat);
		
		//RSDResults->nnMax = 0.0f; 
		//RSDResults->nnMaxLoc = 0.0;
		
		//RSDResults->maxGridPointScore = 0.0f;
		//RSDResults->maxGridPointPosition = -1.0;

		strncpy(RSDDataset->setID, RSDResults->setID[i], STRING_SIZE-1);
		
		RSDMuStat_setReportNamePerSet (RSDMuStat, RSDCommandLine, RAiSD_Info_FP, RSDDataset, RSDCommonOutliers);

		assert(RSDMuStat->reportFP!=NULL);

		if(RSDCommandLine->setSeparator)
			fprintf(RSDMuStat->reportFP, "// %s\n", RSDResults->setID[i]);
			
			
		RSDResults_processSet (RSDResults, RSDNeuralNetwork, RSDCommandLine, RSDMuStat, i);
		
		//RSDMuStat->currentScoreIndex=-1;

		/*for(int j=0;j<RSDResults->setGridSize;j++)
		{
			//printf("GridPoint %d Size %d\n", j, (int)RSDResults->gridPointSize[i*RSDResults->setGridSize+j]);				
			//printf("GridPoint %d Size %d\n", j, (int)RSDResults->gridPointData[i*RSDResults->setGridSize+j]->size);
			//							
			
			// SOS TODO
			// TODO median here
			//double mean = 0.0;
			
			double prodScore = 1.0;
			double meanScore = 0.0;
			
			double muVarScore = 1.0;
			double muSfsScore = 1.0;
			double muLdScore = 1.0;
			double muScore = 1.0;
			double nnScore = 0.0;
			
			for (int k = 0; k<(int)RSDResults->gridPointData[i*RSDResults->setGridSize+j]->size;k++)
			{
				//int k = 0;
				//printf("%d: %f %f\n",j, RSDResults->gridPointData[i*RSDResults->setGridSize+j]->positions[k], RSDResults->gridPointData[i*RSDResults->setGridSize+j]->scores[k]);
				
				
				//RSDMuStat_storeOutput (RSDMuStat, (double)RSDResults->gridPointData[i*RSDResults->setGridSize+j]->positions[k], 
				//				  (double)RSDResults->gridPointData[i*RSDResults->setGridSize+j]->positions[k], 
				//				  (double)RSDResults->gridPointData[i*RSDResults->setGridSize+j]->positions[k], 
				//				  (double)RSDResults->gridPointData[i*RSDResults->setGridSize+j]->scores[k], 
				//				  (double)RSDResults->gridPointData[i*RSDResults->setGridSize+j]->scores[k], 
				//				  (double)RSDResults->gridPointData[i*RSDResults->setGridSize+j]->scores[k], 
				//				  (double)RSDResults->gridPointData[i*RSDResults->setGridSize+j]->scores[k]);
				
				
				prodScore *= (double)RSDResults->gridPointData[i*RSDResults->setGridSize+j]->nnScores[k];
				meanScore += (double)RSDResults->gridPointData[i*RSDResults->setGridSize+j]->nnScores[k];
				
				muVarScore *= (double)RSDResults->gridPointData[i*RSDResults->setGridSize+j]->muVarScores[k];
				muSfsScore *= (double)RSDResults->gridPointData[i*RSDResults->setGridSize+j]->muSfsScores[k];
				muLdScore *= (double)RSDResults->gridPointData[i*RSDResults->setGridSize+j]->muLdScores[k];
				muScore += (double)RSDResults->gridPointData[i*RSDResults->setGridSize+j]->muScores[k];
				nnScore += (double)RSDResults->gridPointData[i*RSDResults->setGridSize+j]->nnScores[k];
			}
			
			
			
			// Calculate final grid position
			RSDResults->gridPointData[i*RSDResults->setGridSize+j]->finalRegionCenter = (double)RSDResults->gridPointData[i*RSDResults->setGridSize+j]->positions[0];//
			
			
	//				((double)RSDResults->gridPointData[i*RSDResults->setGridSize+j]->positions[(int)RSDResults->gridPointData[i*RSDResults->setGridSize+j]->size-1]+//
	//				 (double)RSDResults->gridPointData[i*RSDResults->setGridSize+j]->positions[0])*0.5;//(double)RSDResults->gridPointData[i*RSDResults->setGridSize+j]->positions[0];
			RSDResults->gridPointData[i*RSDResults->setGridSize+j]->finalRegionScore = 0.0*powf(muScore,nnScore);//prodScore;// muVarScore; // / (double)RSDResults->gridPointData[i*RSDResults->setGridSize+j]->scores[0];
			
			//https://blogs.sas.com/content/iml/2021/06/01/hampel-filter-robust-outliers.html
			
			RSDMuStat->currentScoreIndex++; 
			
			//printf("set %d grid %d score %f pos %f\n", i, j, nnScore, RSDResults->gridPointData[i*RSDResults->setGridSize+j]->finalRegionCenter);
			
			RSDMuStat_storeOutput (RSDMuStat, RSDResults->gridPointData[i*RSDResults->setGridSize+j]->finalRegionCenter, 
							  RSDResults->gridPointData[i*RSDResults->setGridSize+j]->positions[0]*0, 
							  RSDResults->gridPointData[i*RSDResults->setGridSize+j]->positions[(int)RSDResults->gridPointData[i*RSDResults->setGridSize+j]->size-1]*0,
							  powf(nnScore,muVarScore)*1.0, //muVarScore*1.0,//meanScore/RSDResults->gridPointData[i*RSDResults->setGridSize+j]->size, //3
							  muScore*1.0,//RSDResults->gridPointData[i*RSDResults->setGridSize+j]->finalRegionScore, //4 
							  (double)nnScore/1, //5
							 powf(muVarScore,nnScore)*1.0);// muScore*nnScore);	//6			
		}
		
		
		//RSDMuStat_removeOutliers (RSDMuStat, RSDCommandLine);
		
		//TODO: Final combination should be here.. after having removed outliers
		
		//for(int j=0;j<RSDCommandLine->gridSize;j++)
//		{
//			//RSDMuStat->buffer6Data[j] = powf(RSDMuStat->buffer4Data[j],RSDMuStat->buffer5Data[j]);
//			RSDMuStat->buffer6Data[j] = RSDMuStat->buffer4Data[j]*RSDMuStat->buffer5Data[j];
//
		//}
		//
		//RSDMuStat_removeOutliers (RSDMuStat, RSDCommandLine);


		
		
		for(int j=0;j<RSDCommandLine->gridSize;j++)
		{
			// MuVar Max
			if (RSDMuStat->buffer3Data[j] > RSDMuStat->muVarMax)
			{
				RSDMuStat->muVarMax = RSDMuStat->buffer3Data[j] ;
				RSDMuStat->muVarMaxLoc = RSDMuStat->buffer0Data[j];
			}
			
			// MuSfs Max
			//if (muSfsScore > RSDMuStat->muSfsMax)
			//{
			//	RSDMuStat->muSfsMax = muSfsScore;
		//		RSDMuStat->muSfsMaxLoc = RSDResults->gridPointData[i*RSDResults->setGridSize+j]->finalRegionCenter;
		//	}
			
		//	// MuLd Max
		//	if (muLdScore > RSDMuStat->muLdMax)
		//	{
		//		RSDMuStat->muLdMax = muLdScore;
		//		RSDMuStat->muLdMaxLoc = RSDResults->gridPointData[i*RSDResults->setGridSize+j]->finalRegionCenter;
		//	}
			
			// Mu Max
			if (RSDMuStat->buffer4Data[j] > RSDMuStat->muMax)
			{
				RSDMuStat->muMax = RSDMuStat->buffer4Data[j] ;
				RSDMuStat->muMaxLoc = RSDMuStat->buffer0Data[j];
			}
			
			//printf("IIN %f max %f at %f\n", RSDMuStat->buffer5Data[j], RSDResults->nnMax, RSDResults->nnMaxLoc);
			// NN Max
			if (RSDMuStat->buffer5Data[j] > RSDResults->nnMax)
			{
				RSDResults->nnMax = (float)RSDMuStat->buffer5Data[j] ;
				RSDResults->nnMaxLoc = RSDMuStat->buffer0Data[j];
				
			//	printf("in %f max %f at %f\n", RSDMuStat->buffer5Data[j], RSDResults->nnMax, RSDResults->nnMaxLoc);
			}

			// Mu * NN Max
			if(RSDMuStat->buffer6Data[j]>RSDResults->maxGridPointScore)
			{
				RSDResults->maxGridPointScore = RSDMuStat->buffer6Data[j] ;
				RSDResults->maxGridPointPosition = RSDMuStat->buffer0Data[j];
			}				
		}
		
		
	*/		
			
			
		
		
		//HampelFilter(vec, tvec, 10);
		
		
		
		// Dist and Succ
		if(selectionTarget!=0ull)
		{
			MuVar_Accum += DIST (RSDMuStat->muVarMaxLoc, (double)selectionTarget);
			if(selectionTargetDThreshold!=0ull)
			{
				if(DIST (RSDMuStat->muVarMaxLoc, (double)selectionTarget)<=selectionTargetDThreshold)
					MuVar_Success += 1.0;
			}

			MuSfs_Accum += DIST (RSDMuStat->muSfsMaxLoc, (double)selectionTarget);
			if(selectionTargetDThreshold!=0ull)
			{
				if(DIST (RSDMuStat->muSfsMaxLoc, (double)selectionTarget)<=selectionTargetDThreshold)
					MuSfs_Success += 1.0;
			}

			MuLd_Accum += DIST (RSDMuStat->muLdMaxLoc, (double)selectionTarget);
			if(selectionTargetDThreshold!=0ull)
			{
				if(DIST (RSDMuStat->muLdMaxLoc, (double)selectionTarget)<=selectionTargetDThreshold)
					MuLd_Success += 1.0;
			}

			Mu_Accum += DIST (RSDMuStat->muMaxLoc, (double)selectionTarget);
			if(selectionTargetDThreshold!=0ull)
			{
				if(DIST (RSDMuStat->muMaxLoc, (double)selectionTarget)<=selectionTargetDThreshold)
					Mu_Success += 1.0;
			}
			
			NN_Accum += DIST (RSDResults->nnMaxLoc, (double)selectionTarget);
			if(selectionTargetDThreshold!=0ull)
			{
				if(DIST (RSDResults->nnMaxLoc, (double)selectionTarget)<=selectionTargetDThreshold)
					NN_Success += 1.0;					
			}
			
			compNN_Accum += DIST (RSDResults->compnnMaxLoc, (double)selectionTarget); 
			if(selectionTargetDThreshold!=0ull)
			{
				if(DIST (RSDResults->compnnMaxLoc, (double)selectionTarget)<=selectionTargetDThreshold)
					compNN_Success += 1.0;
			}
		}

			
			
						
		
		
		
		
		// SOS TODO 	xxxx check all conditions I put earlier that did not mess up anything
		
		//RSDMuStat_writeBuffer2File (RSDMuStat, RSDCommandLine); //TODO: is this executed in CNN mode? it should not be. a new version of it should be implemented. one that stores after removing outliers.

		
		RSDMuStat_writeBuffer2FileNoInterp (RSDMuStat, RSDCommandLine);	
		
		
		//printf("class is %s\n", RSDNeuralNetwork->classLabel[RSDCommandLine->positiveClassIndex]);	

		if(RSDCommandLine->createPlot==1)
			RSDPlot_createPlot (RSDCommandLine, RSDDataset, RSDMuStat, RSDCommonOutliers, RSDPLOT_BASIC_MU, (void*)RSDNeuralNetwork);
			
			
		get_print_slen 	(&slen[4], (void *) (&RSDMuStat->muVarMaxLoc), 2);
		get_print_slen 	(&slen[5], (void *) (&RSDMuStat->muVarMax), 3);
		get_print_slen 	(&slen[6], (void *) (&RSDMuStat->muSfsMaxLoc), 2);
		get_print_slen 	(&slen[7], (void *) (&RSDMuStat->muSfsMax), 3);
		get_print_slen 	(&slen[8], (void *) (&RSDMuStat->muLdMaxLoc), 2);
		get_print_slen 	(&slen[9], (void *) (&RSDMuStat->muLdMax), 3);
		get_print_slen 	(&slen[10], (void *) (&RSDMuStat->muMaxLoc), 2);
		get_print_slen 	(&slen[11], (void *) (&RSDMuStat->muMax), 3);
		get_print_slen 	(&slen[0], (void *) (&RSDResults->nnMaxLoc), 2);
		get_print_slen 	(&slen[1], (void *) (&RSDResults->nnMax), 3);
		get_print_slen 	(&slen[2], (void *) (&RSDResults->compnnMaxLoc), 2);
		get_print_slen 	(&slen[3], (void *) (&RSDResults->compnnMax), 3);
		get_print_slen 	(&slen[12], (void *) RSDDataset->setID, 1);		

		
		RSDNeuralNetwork_getColumnHeaders (RSDNeuralNetwork, RSDCommandLine, colHeader1, colHeader2);
		
							
				
		if(RSDCommandLine->displayProgress==1) 
		{
			fprintf(stdout, " %d: Set %*s - muvar %*.0f %*.3e | musfs %*.0f %*.3e | muld %*.0f %*.3e | mustat %*.0f %*.3e | %s %*.0f %*.3e | %s %*.0f %*.3e \n", 
					i, 
					slen[12], RSDDataset->setID, 
					slen[4], (double)RSDMuStat->muVarMaxLoc, 
					slen[5], (double)RSDMuStat->muVarMax, 
					slen[6], (double)RSDMuStat->muSfsMaxLoc, 
					slen[7], (double)RSDMuStat->muSfsMax, 
					slen[8], (double)RSDMuStat->muLdMaxLoc, 
					slen[9], (double)RSDMuStat->muLdMax, 
					slen[10], (double)RSDMuStat->muMaxLoc, 
					slen[11], (double)RSDMuStat->muMax,
					colHeader1, 
					slen[0], (double)RSDResults->nnMaxLoc, 
					slen[1], (double)RSDResults->nnMax,
					colHeader2,
					slen[2], (double)RSDResults->compnnMaxLoc, 
					slen[3], (double)RSDResults->compnnMax);
		}
		fflush(stdout);					
		
		fprintf(RAiSD_Info_FP, " %d: Set %*s - muvar %*.0f %*.3e | musfs %*.0f %*.3e | muld %*.0f %*.3e | mustat %*.0f %*.3e | %s %*.0f %*.3e | %s %*.0f %*.3e \n", 
					i, 
					slen[12], RSDDataset->setID, 
					slen[4], (double)RSDMuStat->muVarMaxLoc, 
					slen[5], (double)RSDMuStat->muVarMax, 
					slen[6], (double)RSDMuStat->muSfsMaxLoc, 
					slen[7], (double)RSDMuStat->muSfsMax, 
					slen[8], (double)RSDMuStat->muLdMaxLoc, 
					slen[9], (double)RSDMuStat->muLdMax, 
					slen[10], (double)RSDMuStat->muMaxLoc, 
					slen[11], (double)RSDMuStat->muMax,
					colHeader1, 
					slen[0], (double)RSDResults->nnMaxLoc, 
					slen[1], (double)RSDResults->nnMax,
					colHeader2,
					slen[2], (double)RSDResults->compnnMaxLoc, 
					slen[3], (double)RSDResults->compnnMax);

		fflush(RAiSD_Info_FP);
	}
}


#endif
