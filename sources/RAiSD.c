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

#include "RAiSD.h"

void RSD_init (void);

uint64_t selectionTarget;
double MuVar_Accum;
double MuSfs_Accum;
double MuLd_Accum;
double Mu_Accum;
uint64_t selectionTargetDThreshold;
double MuVar_Success;
double MuSfs_Success;
double MuLd_Success;
double Mu_Success;
double fpr_loc;
int scr_svec_sz;
float * scr_svec;
double tpr_thres;
double tpr_scr;
int setIndexValid;
double StartTime;
double FinishTime;
double MemoryFootprint;
FILE * RAiSD_Info_FP;
FILE * RAiSD_SiteReport_FP;
FILE * RAiSD_ReportList_FP;
struct timespec requestStart;
struct timespec requestEnd;

#ifdef _RSDAI
double NN_Accum;
double NN_Success;
double compNN_Accum;
double compNN_Success;
#endif

#ifdef _PTIMES
struct timespec requestStartMu;
struct timespec requestEndMu;
double TotalMuTime;
#endif

void RSD_header (FILE * fpOut)
{
	if(fpOut==NULL)
		return;

	printRAiSD (fpOut);

	fprintf(fpOut, "\n");
#ifdef _RSDAI	
	fprintf(fpOut, " RAiSD-AI, Raised Accuracy in Sweep Detection using AI (version %d.%d, released in %s %d)\n", MAJOR_VERSION, MINOR_VERSION, RELEASE_MONTH, RELEASE_YEAR);
#else
	fprintf(fpOut, " RAiSD, Raised Accuracy in Sweep Detection (version %d.%d, released in %s %d)\n", MAJOR_VERSION, MINOR_VERSION, RELEASE_MONTH, RELEASE_YEAR);
#endif
	//fprintf(fpOut, " This is version %d.%d (released in %s %d)\n\n", MAJOR_VERSION, MINOR_VERSION, RELEASE_MONTH, RELEASE_YEAR);

	fprintf(fpOut, " Copyright (C) 2017-2024, and GNU GPL'd, by Nikolaos Alachiotis and Pavlos Pavlidis\n");
	fprintf(fpOut, " Contact: n.alachiotis@utwente.nl and pavlidisp@gmail.com\n");
#ifdef _RSDAI
	fprintf(fpOut, " Code contributions: Sjoerd van den Belt and Hanqing Zhao\n");
#endif

	fprintf(fpOut, "\n \t\t\t\t    * * * * *\n\n");
}

void RSD_init (void)
{
	clock_gettime(CLOCK_REALTIME, &requestStart);
	//StartTime = gettime();

	FinishTime = 0.0;
	MemoryFootprint = 0.0;

	RAiSD_Info_FP = NULL;
	RAiSD_SiteReport_FP = NULL;
	RAiSD_ReportList_FP = NULL;

	/*Testing*/
	selectionTarget = 0ull;
	selectionTargetDThreshold = 0ull;
	MuVar_Accum = 0.0;
 	MuSfs_Accum = 0.0;
 	MuLd_Accum = 0.0;
 	Mu_Accum = 0.0;
 	MuVar_Success = 0.0;
	MuSfs_Success = 0.0;
	MuLd_Success = 0.0;
	Mu_Success = 0.0;
	fpr_loc = 0.0;
	scr_svec_sz = 0;
	scr_svec = NULL;
	tpr_thres = 0.0;
	tpr_scr = 0.0;
	setIndexValid = -1;
	
#ifdef _RSDAI
	NN_Accum = 0.0;
	NN_Success = 0.0;
	compNN_Accum = 0.0;
	compNN_Success = 0.0;
#endif

	/**/

#ifndef _INTRINSIC_POPCOUNT
	popcount_u64_init();
#endif	

	srand((unsigned int)time(NULL)); // if no seed given

#ifdef _PTIMES
	TotalOoCTime = 0.0;
	TotalMuTime = 0.0;
#endif

}

int main (int argc, char ** argv)
{
	RSD_init();

	RSDCommandLine_t * RSDCommandLine = RSDCommandLine_new();
	RSDCommandLine_init(RSDCommandLine);
	RSDCommandLine_load(RSDCommandLine, argc, argv);
	
	RSD_header(stdout);
	RSD_header(RAiSD_Info_FP);
	RSD_header(RAiSD_SiteReport_FP);

	RSDCommandLine_print(argc, argv, stdout);
	RSDCommandLine_print(argc, argv, RAiSD_Info_FP);
	RSDCommandLine_print(argc, argv, RAiSD_SiteReport_FP);
	RSDCommandLine_printInfo(RSDCommandLine, stdout);
	RSDCommandLine_printInfo(RSDCommandLine, RAiSD_Info_FP);
		
#ifdef _RSDAI
	RSDNeuralNetwork_t * RSDNeuralNetwork = RSDNeuralNetwork_new (RSDCommandLine);
	RSDNeuralNetwork_init (RSDNeuralNetwork, RSDCommandLine, RAiSD_Info_FP);
	RSDNeuralNetwork_train (RSDNeuralNetwork, RSDCommandLine, RAiSD_Info_FP);
	RSDNeuralNetwork_test (RSDNeuralNetwork, RSDCommandLine, RAiSD_Info_FP);
	
	switch(RSDCommandLine->opCode)
	{
		case OP_TRAIN_CNN:
		
			if(RSDNeuralNetwork_modelExists(RSDNeuralNetwork->modelPath)!=1)
			{
				char trainCommand[STRING_SIZE];
				RSDNeuralNetwork_createTrainCommand (RSDNeuralNetwork, RSDCommandLine, trainCommand, 1);
	
				fprintf(RAiSD_Info_FP, "\nERROR: Model generation failed. The CNN did not train correctly.\n\nUse this command to see Python errors: %s\n\n", trainCommand);
				fprintf(stderr, "\nERROR: Model generation failed. The CNN did not train correctly.\n\nUse this command to see Python errors: %s\n\n", trainCommand);
				exit(0);
			}
			
		
			fprintf(stdout, "\n");
			fprintf(stdout, " Output directory    :\t%s\n", RSDNeuralNetwork->modelPath);
					
			fprintf(RAiSD_Info_FP, "\n");
			fprintf(RAiSD_Info_FP, " Output directory: %s\n", RSDNeuralNetwork->modelPath);
					
			break;
		default:
			break;
	}		
	
	if(RSDCommandLine->opCode==OP_TRAIN_CNN || RSDCommandLine->opCode==OP_TEST_CNN)
	{
		RSD_printTime(stdout, RAiSD_Info_FP);
		RSD_printMemory(stdout, RAiSD_Info_FP);
		fclose(RAiSD_Info_FP);
		
		RSDCommandLine_free(RSDCommandLine);
		RSDNeuralNetwork_free(RSDNeuralNetwork);
		
		return 0;
	}	
#endif	

	RSDPlot_generateRscript(RSDCommandLine, RSDPLOT_BASIC_MU);

	RSDCommonOutliers_t * RSDCommonOutliers = RSDCommonOutliers_new ();
	RSDCommonOutliers_init (RSDCommonOutliers, RSDCommandLine);
	RSDCommonOutliers_process (RSDCommonOutliers, RSDCommandLine);

	if(strcmp(RSDCommonOutliers->reportFilenameRAiSD, "\0"))
	{
		RSDCommonOutliers_free (RSDCommonOutliers);
		RSDCommandLine_free (RSDCommandLine);

		RSD_printTime(stdout, RAiSD_Info_FP);
		RSD_printMemory(stdout, RAiSD_Info_FP);

		fclose(RAiSD_Info_FP);
		
		return 0;
	}	

	RSD_printSiteReportLegend(RAiSD_SiteReport_FP, RSDCommandLine->imputePerSNP, RSDCommandLine->createPatternPoolMask);

	RSDDataset_t * RSDDataset = RSDDataset_new();
	RSDDataset_init(RSDDataset, RSDCommandLine, RAiSD_Info_FP); 
	RSDDataset_print(RSDDataset, RSDCommandLine, stdout);
	RSDDataset_print(RSDDataset, RSDCommandLine, RAiSD_Info_FP);
	
	RSDCommandLine_printExponents (RSDCommandLine, stdout);
	RSDCommandLine_printExponents (RSDCommandLine, RAiSD_Info_FP);

	RSDPlot_printRscriptVersion (RSDCommandLine, stdout);
	RSDPlot_printRscriptVersion (RSDCommandLine, RAiSD_Info_FP);
	
#ifdef _RSDAI
	RSDImage_t * RSDImage = RSDImage_new (RSDCommandLine);	
	RSDImage_makeDirectory (RSDImage, RSDCommandLine);
	
	RSDImage_print (RSDImage, RSDCommandLine, stdout);
	RSDImage_print (RSDImage, RSDCommandLine, RAiSD_Info_FP); 
		
	RSDGrid_t * RSDGrid = RSDGrid_new (RSDCommandLine);
	RSDGrid_makeDirectory (RSDGrid, RSDCommandLine, RSDImage); 
	
	RSDResults_t * RSDResults = RSDResults_new (RSDCommandLine);
	RSDResults_setGridSize (RSDResults, RSDCommandLine);
#endif	

	RSDCommandLine_printWarnings (RSDCommandLine, argc, argv, (void*)RSDDataset, stdout);
	RSDCommandLine_printWarnings (RSDCommandLine, argc, argv, (void*)RSDDataset, RAiSD_Info_FP);
	
	RSDPatternPool_t * RSDPatternPool = RSDPatternPool_new(RSDCommandLine); // RSDCommandLine here only for vcf2ms conversion
	RSDPatternPool_init(RSDPatternPool, RSDCommandLine, RSDDataset->numberOfSamples);
	RSDPatternPool_print(RSDPatternPool, stdout);
	RSDPatternPool_print(RSDPatternPool, RAiSD_Info_FP);

	RSDChunk_t * RSDChunk = RSDChunk_new();
	RSDChunk_init(RSDChunk, RSDCommandLine, RSDDataset->numberOfSamples);
	
	RSDMuStat_t * RSDMuStat = RSDMuStat_new();	
	RSDMuStat_setReportName (RSDMuStat, RSDCommandLine, RAiSD_Info_FP);	
	RSDMuStat_loadExcludeTable (RSDMuStat, RSDCommandLine);	
	
	RSDVcf2ms_t * RSDVcf2ms = RSDVcf2ms_new (RSDCommandLine);	

	int setIndex = -1, setDone = 0, setsProcessedTotal=0;

	int slen0=1, slen1=1, slen2=1, slen3=1; 
	
#ifdef _RSDAI
	int warningMsgEn=1;
	char colHeader1[STRING_SIZE], colHeader2[STRING_SIZE];
#endif	
	
	// Set processing
	while(RSDDataset_goToNextSet(RSDDataset)!=EOF) 
	{
		RSDDataset_setPosition (RSDDataset, &setIndex);

		RSDMuStat->currentScoreIndex=-1;

		if(setIndexValid!=-1 && setIndex!=setIndexValid)
		{
			char tchar = (char)fgetc(RSDDataset->inputFilePtr); // Note: this might generate a segfault with MakefileZLIB
			tchar = tchar - tchar;
			assert(tchar==0);
		}

		if(setIndexValid==-1 || setIndex == setIndexValid)
		{
		
#if _RSDAI
			RSDResults_incrementSetCounter (RSDResults);
			RSDResults_saveSetID (RSDResults, RSDDataset);
						
			if(RSDCommandLine->opCode==OP_DEF)
			{
				RSDMuStat_setReportNamePerSet (RSDMuStat, RSDCommandLine, RAiSD_Info_FP, RSDDataset, RSDCommonOutliers);
			
				if(RSDCommandLine->setSeparator)
					fprintf(RSDMuStat->reportFP, "// %s\n", RSDDataset->setID);
			
			}
#else
			RSDMuStat_setReportNamePerSet (RSDMuStat, RSDCommandLine, RAiSD_Info_FP, RSDDataset, RSDCommonOutliers);
			
			if(RSDCommandLine->setSeparator)
				fprintf(RSDMuStat->reportFP, "// %s\n", RSDDataset->setID);
#endif				

			RSDMuStat_init (RSDMuStat, RSDCommandLine);
			RSDMuStat_excludeRegion (RSDMuStat, RSDDataset);

			setDone = 0;

			RSDChunk->chunkID = -1;	
			RSDChunk_reset(RSDChunk, RSDCommandLine);

			RSDPatternPool_reset(RSDPatternPool, RSDDataset->numberOfSamples, RSDDataset->setSamples, RSDChunk, RSDCommandLine);	

			setDone = RSDDataset_getFirstSNP(RSDDataset, RSDPatternPool, RSDChunk, RSDCommandLine, RSDCommandLine->regionLength, RSDCommandLine->maf, RAiSD_Info_FP);
#if _RSDAI
			if(RSDCommandLine->opCode==OP_USE_CNN && RSDNeuralNetwork->imageHeight!=RSDDataset->numberOfSamples)
			{
				
				if(RSDNeuralNetwork->dataFormat==1 && RSDNeuralNetwork->dataType==1 && warningMsgEn==1)
				{
					warningMsgEn = 0;
					
					fprintf(RAiSD_Info_FP, "\nWARNING: Mismatch between the sample size (%d) and the trained model (height %d)! Classification accuracy might be negatively affected! \n\n", RSDDataset->numberOfSamples, RSDNeuralNetwork->imageHeight);		
					fprintf(stderr, "\nWARNING: Mismatch between the sample size (%d) and the trained model (height %d)! Classification accuracy might be negatively affected!\n\n", RSDDataset->numberOfSamples, RSDNeuralNetwork->imageHeight);
				}
				else
				{
					if(!(RSDNeuralNetwork->dataFormat==1 && RSDNeuralNetwork->dataType==1))
					{
						fprintf(RAiSD_Info_FP, "\nERROR: Set %s sample size (%d) is incompatible with the trained model (expected %d)!\n\n",RSDDataset->setID, RSDDataset->numberOfSamples, RSDNeuralNetwork->imageHeight);		
						fprintf(stderr, "\nERROR: Set %s sample size (%d) is incompatible with the trained model (expected %d)!\n\n",RSDDataset->setID, RSDDataset->numberOfSamples, RSDNeuralNetwork->imageHeight);
		
						exit(0);
					}
				}
			}	
#endif			
			if(setDone)
			{
				get_print_slen 	(&slen0, (void *) RSDDataset->setID, 1); 
				get_print_slen 	(&slen1, (void *) (&RSDDataset->setSize), 0); 
				get_print_slen 	(&slen2, (void *) (&RSDDataset->setSNPs), 0); 
				
				if(RSDCommandLine->displayProgress==1)
				fprintf(stdout, " %d: Set %*s | sites %*d | snps %*d | region %lu - skipped\n", setIndex, slen0, RSDDataset->setID, 
															slen1, (int)RSDDataset->setSize, 
															slen2, (int)RSDDataset->setSNPs, 
															RSDDataset->setRegionLength);
															
				fprintf(RAiSD_Info_FP, " %d: Set %*s | sites %*d | snps %*d | region %lu - skipped\n", setIndex, slen0, RSDDataset->setID, 
															      slen1, (int)RSDDataset->setSize, 
															      slen2, (int)RSDDataset->setSNPs, 
															      RSDDataset->setRegionLength);

				if(RSDCommandLine->displayDiscardedReport==1)
					RSDDataset_printSiteReport (RSDDataset, RAiSD_SiteReport_FP, setIndex, RSDCommandLine->imputePerSNP, RSDCommandLine->createPatternPoolMask);

				RSDDataset_resetSiteCounters (RSDDataset);	

				continue;
			}
			RSDPatternPool_resize (RSDPatternPool, RSDDataset->setSamples, RAiSD_Info_FP);
#ifdef _HM
			RSDHashMap_free(RSDPatternPool->hashMap);
			RSDPatternPool->hashMap = RSDHashMap_new();
			RSDHashMap_init (RSDPatternPool->hashMap, RSDDataset->setSamples, RSDPatternPool->maxSize, RSDPatternPool->patternSize);
#endif
#ifdef _LM
			RSDLutMap_free(RSDPatternPool->lutMap);
			RSDPatternPool->lutMap =  RSDLutMap_new();
			RSDLutMap_init (RSDPatternPool->lutMap, RSDDataset->setSamples);
#endif
			RSDPatternPool_pushSNP (RSDPatternPool, RSDChunk, RSDDataset->setSamples, RSDCommandLine, (void*)RSDVcf2ms);

			//int sitesloaded = 0;
			//int patternsloaded = 0;
			
			// Chunk processing
			while(!setDone) 
			{
				RSDChunk->chunkID++;
				
				//printf("Chunk %ld\n", RSDChunk->chunkID);
				//fflush(stdout);

				if(RSDCommandLine->vcf2msExtra == VCF2MS_CONVERT && RSDChunk->chunkID!=0)
				{
					fprintf(stderr, "\nERROR: Insufficient memory size provided through -Q for VCF-to-ms conversion!\n\n");
					exit(0);
				}

				int poolFull = 0;

				// SNP processing
				while(!poolFull && !setDone) 
				{
					setDone = RSDDataset_getNextSNP(RSDDataset, RSDPatternPool, RSDChunk, RSDCommandLine, RSDDataset->setRegionLength, RSDCommandLine->maf, RAiSD_Info_FP);
					poolFull = RSDPatternPool_pushSNP (RSDPatternPool, RSDChunk, RSDDataset->setSamples, RSDCommandLine, (void*)RSDVcf2ms); 
				}
	
				RSDPatternPool_assessMissing (RSDPatternPool, RSDDataset->setSamples);

				/*// version 2.4, for ms
				if(!strcmp(RSDDataset->inputFileFormat, "ms"))
				{
					//printf("%d - %d vs %d\n", RSDChunk->chunkID, (int)RSDDataset->setSNPs, (int)RSDDataset->preLoadedsetSNPs);
					//fflush(stdout);

					assert((uint64_t)RSDDataset->setSNPs==RSDDataset->preLoadedsetSNPs); // TODO: fails when ms contains monomorphic sites
				}*/

#ifdef _PTIMES
				clock_gettime(CLOCK_REALTIME, &requestStartMu);
#endif

#ifdef _RSDAI
				switch(RSDCommandLine->opCode)
				{
					case OP_CREATE_IMAGES:								
						RSDImage_init (RSDImage, RSDDataset, RSDMuStat, RSDPatternPool, RSDCommandLine, RSDChunk, setIndex, RAiSD_Info_FP);
						if(RSDCommandLine->trnObjDetection==0)
						{
							RSDImage_createImagesFlex (RSDImage, RSDMuStat, RSDChunk, RSDPatternPool, RSDDataset, RSDCommandLine, 
										   RAiSD_Info_FP, RSDCommandLine->imageTargetSite, 
										   RSDImage->destinationPath, 0);
						}
						else
						{
							//assert(RSDCommandLine->enBinFormat==1);
							assert(RSDCommandLine->trnObjDetection==1);
							
							RSDGrid_init (RSDGrid, RSDChunk, RSDMuStat, RSDCommandLine, setDone);
							RSDGrid_process (RSDGrid, RSDImage, RSDMuStat, RSDChunk, RSDPatternPool, RSDDataset, RSDCommandLine, RSDResults);						
						}
						break;
												
					case OP_USE_CNN:
						RSDImage_init (RSDImage, RSDDataset, RSDMuStat, RSDPatternPool, RSDCommandLine, RSDChunk, setIndex, RAiSD_Info_FP);					
						RSDGrid_init (RSDGrid, RSDChunk, RSDMuStat, RSDCommandLine, setDone);
						RSDGrid_process (RSDGrid, RSDImage, RSDMuStat, RSDChunk, RSDPatternPool, RSDDataset, RSDCommandLine, RSDResults);
						break;
					
					default: // OP_DEF opcode
						RSDMuStat_scanChunk (RSDMuStat, RSDChunk, RSDPatternPool, RSDDataset, RSDCommandLine, RAiSD_Info_FP);
						break;
				}
#else
				// Compute Mu statistic
				RSDMuStat_scanChunk (RSDMuStat, RSDChunk, RSDPatternPool, RSDDataset, RSDCommandLine, RAiSD_Info_FP);
#endif			
#ifdef _PTIMES
				clock_gettime(CLOCK_REALTIME, &requestEndMu);
				double MuTime = (requestEndMu.tv_sec-requestStartMu.tv_sec)+ (requestEndMu.tv_nsec-requestStartMu.tv_nsec) / BILLION;
				TotalMuTime += MuTime;
#endif
				//sitesloaded+=RSDChunk->chunkSize;
				//patternsloaded += RSDPatternPool->dataSize;

				RSDChunk_reset(RSDChunk, RSDCommandLine);
				RSDPatternPool_reset(RSDPatternPool, RSDDataset->numberOfSamples, RSDDataset->setSamples, RSDChunk, RSDCommandLine);
			}

#ifdef _RSDAI
			if(RSDCommandLine->opCode==OP_DEF)
			{
#endif
				if(selectionTarget!=0ull)// Dist and Succ
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
				}


				RSDMuStat_writeBuffer2File (RSDMuStat, RSDCommandLine);

				if(RSDCommandLine->createPlot==1 && RSDCommandLine->opCode!=OP_USE_CNN)
						RSDPlot_createPlot (RSDCommandLine, RSDDataset, RSDMuStat, RSDCommonOutliers, RSDPLOT_BASIC_MU, NULL);

#ifdef _RSDAI				
			}
#endif
			setsProcessedTotal++;
			
			get_print_slen 	(&slen0, (void *) RSDDataset->setID, 1); 
			get_print_slen 	(&slen1, (void *) (&RSDDataset->setSize), 0); 
			get_print_slen 	(&slen2, (void *) (&RSDDataset->setSNPs), 0); 			

#ifdef _RSDAI				
			if(RSDCommandLine->displayProgress==1)
			{
				switch(RSDCommandLine->opCode)
				{
					case OP_CREATE_IMAGES:						
						fprintf(stdout, " %d: Set %*s | sites %*d | snps %*d | region %lu - images %lu\n", setIndex, slen0, RSDDataset->setID, 
																	     slen1, (int)RSDDataset->setSize, 
																	     slen2, (int)RSDDataset->setSNPs, 
																	     RSDDataset->setRegionLength, 
																	     RSDImage->generatedSetImages);
						break;
					case OP_USE_CNN:
						fprintf(stdout, " %d: Set %*s | sites %*d | snps %*d | region %lu - images %lu\n", setIndex, slen0, RSDDataset->setID, 
																	  slen1, (int)RSDDataset->setSize, 
																	  slen2, (int)RSDDataset->setSNPs, 
																	  RSDDataset->setRegionLength, 
																	  RSDImage->generatedSetImages);
						break;
					default: 
						fprintf(stdout, " %d: Set %*s | sites %*d | snps %*d | region %lu - muvar %.0f %.3e | musfs %.0f %.3e | muld %.0f %.3e | mustat %.0f %.3e\n", setIndex,
								 slen0, RSDDataset->setID, 
								 slen1, (int)RSDDataset->setSize, 
								 slen2, (int)RSDDataset->setSNPs, 
								 RSDDataset->setRegionLength, 
								 (double)RSDMuStat->muVarMaxLoc, 
								 (double)RSDMuStat->muVarMax, 
								 (double)RSDMuStat->muSfsMaxLoc, 
								 (double)RSDMuStat->muSfsMax, 
								 (double)RSDMuStat->muLdMaxLoc, 
								 (double)RSDMuStat->muLdMax, 
								 (double)RSDMuStat->muMaxLoc, 
								 (double)RSDMuStat->muMax);
						break;
				}
			}
#else
			if(RSDCommandLine->displayProgress==1)
				fprintf(stdout, " %d: Set %*s | sites %*d | snps %*d | region %lu - muvar %.0f %.3e | musfs %.0f %.3e | muld %.0f %.3e | mustat %.0f %.3e\n", setIndex, 
																slen0, RSDDataset->setID, 
																slen1, (int)RSDDataset->setSize, 
																slen2, (int)RSDDataset->setSNPs, 
																RSDDataset->setRegionLength, 
																(double)RSDMuStat->muVarMaxLoc, 
																(double)RSDMuStat->muVarMax, 
																(double)RSDMuStat->muSfsMaxLoc, 
																(double)RSDMuStat->muSfsMax, 
																(double)RSDMuStat->muLdMaxLoc, 
																(double)RSDMuStat->muLdMax, 
																(double)RSDMuStat->muMaxLoc, 
																(double)RSDMuStat->muMax);
#endif				
			fflush(stdout);

			if(RSDCommandLine->displayDiscardedReport==1)
				RSDDataset_printSiteReport (RSDDataset, RAiSD_SiteReport_FP, setIndex, RSDCommandLine->imputePerSNP, RSDCommandLine->createPatternPoolMask);

			RSDDataset_resetSiteCounters (RSDDataset);			

#ifdef _RSDAI
			switch(RSDCommandLine->opCode)
			{
				case OP_CREATE_IMAGES:
					fprintf(RAiSD_Info_FP, " %d: Set %*s | sites %*d | snps %*d | region %lu - images %lu\n", setIndex, slen0, RSDDataset->setID, 
																	    slen1, (int)RSDDataset->setSize, 
																	    slen2, (int)RSDDataset->setSNPs, 
																	    RSDDataset->setRegionLength, 
																	    RSDImage->generatedSetImages);
					break;
				case OP_USE_CNN:
					fprintf(RAiSD_Info_FP, " %d: Set %*s | sites %*d | snps %*d | region %lu - images %lu\n", setIndex, slen0, RSDDataset->setID, 
																	    slen1, (int)RSDDataset->setSize, 
																	    slen2, (int)RSDDataset->setSNPs, 
																	    RSDDataset->setRegionLength, 
																	    RSDImage->generatedSetImages);
					break;
				default:
					fprintf(RAiSD_Info_FP, " %d: Set %*s | sites %*d | snps %*d | region %lu - muvar %.0f %.3e | musfs %.0f %.3e | muld %.0f %.3e | mustat %.0f %.3e\n", setIndex, 
																		slen0, RSDDataset->setID, 
																		slen1, (int)RSDDataset->setSize, 
																		slen2, (int)RSDDataset->setSNPs, 
																		RSDDataset->setRegionLength, 
																		(double)RSDMuStat->muVarMaxLoc, 
																		(double)RSDMuStat->muVarMax, 
																		(double)RSDMuStat->muSfsMaxLoc, 
																		(double)RSDMuStat->muSfsMax, 
																		(double)RSDMuStat->muLdMaxLoc, 
																		(double)RSDMuStat->muLdMax, 
																		(double)RSDMuStat->muMaxLoc, 
																		(double)RSDMuStat->muMax);
					break;
			}
#else				
			fprintf(RAiSD_Info_FP, " %d: Set %*s | sites %*d | snps %*d | region %lu - muvar %.0f %.3e | musfs %.0f %.3e | muld %.0f %.3e | mustat %.0f %.3e\n", setIndex, 
																		slen0, RSDDataset->setID, 
																		slen1, (int)RSDDataset->setSize, 
																		slen2, (int)RSDDataset->setSNPs, 
																		RSDDataset->setRegionLength, 
																		(double)RSDMuStat->muVarMaxLoc, 
																		(double)RSDMuStat->muVarMax, 
																		(double)RSDMuStat->muSfsMaxLoc, 
																		(double)RSDMuStat->muSfsMax, 
																		(double)RSDMuStat->muLdMaxLoc, 
																		(double)RSDMuStat->muLdMax, 
																		(double)RSDMuStat->muMaxLoc, 
																		(double)RSDMuStat->muMax);
#endif
			fflush(RAiSD_Info_FP);

			// FPR/TPR
			if(fpr_loc>0.0)
				scr_svec = putInSortVector(&scr_svec_sz, scr_svec, RSDMuStat->muMax);

			if(tpr_thres>0.0)
				if(RSDMuStat->muMax>=(float)tpr_thres)
					tpr_scr += 1.0;

			// VCF2MS
			if(RSDCommandLine->vcf2msExtra == VCF2MS_CONVERT)
			{
				if(RSDVcf2ms->status==0)
				{
					RSDVcf2ms->status=1;
					RSDVcf2ms_printHeader (RSDVcf2ms);
				}

				RSDVcf2ms_printSegsitesAndPositions (RSDVcf2ms);
				RSDVcf2ms_printSNPData (RSDVcf2ms);

				RSDVcf2ms_reset (RSDVcf2ms);
			}

			if(setIndex == setIndexValid)
				break;
		}		
	}	
			
#ifdef _RSDAI
	if(RSDCommandLine->opCode==OP_USE_CNN)
	{
		RSDNeutralNetwork_run (RSDNeuralNetwork, RSDCommandLine, (void*)RSDGrid, RAiSD_Info_FP);
		
		if(!strcmp(RSDNeuralNetwork->networkArchitecture, ARC_SWEEPNETRECOMB))
		{//assert(0);
			RSDResults_load_2x2 (RSDResults, RSDCommandLine);
		}
		else
		{
			//RSDResults_load_2x2 (RSDResults, RSDCommandLine);
			RSDResults_load (RSDResults, RSDCommandLine);
		}
		
		RSDResults_process (RSDResults, RSDNeuralNetwork, RSDCommandLine, RSDDataset, RSDMuStat, RSDCommonOutliers);
		
		// TODO-AI-1
		
		//RSDResults_process (RSDResults);
		//RSDResults_processSet (RSDResults, RSDMuStat) TODO?
		
		
		/*
		for(int i=0;i<RSDResults->setsTotal;i++)
		{
			RSDMuStat->muVarMax = 0.0f; 
			RSDMuStat->muVarMaxLoc = 0.0;

			RSDMuStat->muSfsMax = 0.0f; 
			RSDMuStat->muSfsMaxLoc = 0.0;

			RSDMuStat->muLdMax = 0.0f;
			RSDMuStat->muLdMaxLoc = 0.0;

			RSDMuStat->muMax = 0.0f; 
			RSDMuStat->muMaxLoc = 0.0;
			
			RSDResults->nnMax = 0.0f; 
			RSDResults->nnMaxLoc = 0.0;
			
			RSDResults->maxGridPointScore = 0.0f;
			RSDResults->maxGridPointPosition = -1.0;
		
			strncpy(RSDDataset->setID, RSDResults->setID[i], STRING_SIZE-1);
			
			RSDMuStat_setReportNamePerSet (RSDMuStat, RSDCommandLine, RAiSD_Info_FP, RSDDataset, RSDCommonOutliers);

			assert(RSDMuStat->reportFP!=NULL);

			if(RSDCommandLine->setSeparator)
				fprintf(RSDMuStat->reportFP, "// %s\n", RSDResults->setID[i]);
			
			RSDMuStat->currentScoreIndex=-1;
		
			for(int j=0;j<RSDResults->setGridSize;j++)
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
				RSDResults->gridPointData[i*RSDResults->setGridSize+j]->finalRegionScore = 0.0*powf(muScore,nnScore);//prodScore;// muVarScore; // //// (double)RSDResults->gridPointData[i*RSDResults->setGridSize+j]->scores[0];
				
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
			//{
			//	//RSDMuStat->buffer6Data[j] = powf(RSDMuStat->buffer4Data[j],RSDMuStat->buffer5Data[j]);
			//	RSDMuStat->buffer6Data[j] = RSDMuStat->buffer4Data[j]*RSDMuStat->buffer5Data[j];
//
//			}
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
				
				MuNN_Accum += DIST (RSDResults->maxGridPointPosition, (double)selectionTarget); 
				if(selectionTargetDThreshold!=0ull)
				{
					if(DIST (RSDResults->maxGridPointPosition, (double)selectionTarget)<=selectionTargetDThreshold)
						MuNN_Success += 1.0;
				}
			}

				
				
							
			
			
			
			
			// SOS TODO 	xxxx check all conditions I put earlier that did not mess up anything
			
			//RSDMuStat_writeBuffer2File (RSDMuStat, RSDCommandLine); //TODO: is this executed in CNN mode? it should not be. a new version of it should be implemented. one that stores after removing outliers.

			
			RSDMuStat_writeBuffer2FileNoInterp (RSDMuStat, RSDCommandLine);
			
			

			if(RSDCommandLine->createPlot==1)
				RSDPlot_createPlot (RSDCommandLine, RSDDataset, RSDMuStat, RSDCommonOutliers, RSDPLOT_BASIC_MU);					
					
			if(RSDCommandLine->displayProgress==1) // TODO: Fix print here
			{
				//fprintf(stdout, " %d: Set %s - position %.0f score %.3f\n", i, RSDDataset->setID, RSDResults->maxGridPointPosition, RSDResults->maxGridPointScore );
				fprintf(stdout, " %d: Set %s - position %.0f score %.3e (NN)\n", i, RSDDataset->setID, RSDResults->nnMaxLoc, RSDResults->nnMax);
				fprintf(stdout, " %d: Set %s - position %.0f score %.3f (MU)\n", i, RSDDataset->setID, RSDMuStat->muMaxLoc, RSDMuStat->muMax);
				fprintf(stdout, " %d: Set %s - position %.0f score %.3f (COMB)\n\n", i, RSDDataset->setID, RSDResults->maxGridPointPosition, RSDResults->maxGridPointScore );

			}
			fflush(stdout);					
					// TODO: Fix print here
			fprintf(RAiSD_Info_FP, " %d: Set %s | sites %d | snps %d | region %lu - Var %.0f %.3e | SFS %.0f %.3e | LD %.0f %.3e | MuStat %.0f %.3e\n", i, RSDDataset->setID, (int)RSDDataset->setSize, (int)RSDDataset->setSNPs, RSDDataset->setRegionLength, (double)RSDMuStat->muVarMaxLoc, (double)RSDMuStat->muVarMax, (double)RSDMuStat->muSfsMaxLoc, (double)RSDMuStat->muSfsMax, (double)RSDMuStat->muLdMaxLoc, (double)RSDMuStat->muLdMax, (double)RSDMuStat->muMaxLoc, (double)RSDMuStat->muMax);

			fflush(RAiSD_Info_FP);					

		}
		
		
		*/	
	}
#endif	

	fprintf(stdout, "\n");
	fprintf(stdout, " Sets (total)        :\t%d\n", setIndex+1);
	fprintf(stdout, " Sets (processed)    :\t%d\n", setsProcessedTotal);
	fprintf(stdout, " Sets (skipped)      :\t%d\n", setIndex+1-setsProcessedTotal);

	fprintf(RAiSD_Info_FP, "\n");
	fprintf(RAiSD_Info_FP, " Sets (total)        :\t%d\n", setIndex+1);
	fprintf(RAiSD_Info_FP, " Sets (processed)    :\t%d\n", setsProcessedTotal);
	fprintf(RAiSD_Info_FP, " Sets (skipped)      :\t%d\n", setIndex+1-setsProcessedTotal);
	
#ifdef _RSDAI
	switch(RSDCommandLine->opCode)
	{
		case OP_CREATE_IMAGES:
		
			fprintf(stdout, "\n");
			fprintf(stdout, " Output directory    :\t%s\n", RSDImage->destinationPath);
			fprintf(stdout, " Data information    :\tRAiSD_Images.%s/info.txt\n", RSDCommandLine->runName); 
			fprintf(stdout, " Images (total)      :\t%lu\n", RSDImage->totalGeneratedImages);
			
			fprintf(RAiSD_Info_FP, "\n");
			fprintf(RAiSD_Info_FP, " Output directory: %s\n", RSDImage->destinationPath);
			fprintf(RAiSD_Info_FP, " Data information: RAiSD_Images.%s/info.txt\n", RSDCommandLine->runName);
			fprintf(RAiSD_Info_FP, " Images (total):   %lu\n", RSDImage->totalGeneratedImages);
			
			break;
		default:
			break;
	}		
#endif

	fflush(stdout);
	fflush(RAiSD_Info_FP);

#ifdef _RSDAI	
	if(selectionTarget!=0ull)
	{
		RSDNeuralNetwork_getColumnHeaders (RSDNeuralNetwork, RSDCommandLine, colHeader1, colHeader2);
		
		slen3 = 6;
		get_print_slen 	(&slen3, (void *) (colHeader1), 1);
		get_print_slen 	(&slen3, (void *) (colHeader2), 1);
	
		fprintf(stdout, "\n");
		fprintf(stdout, " AVERAGE DISTANCE (Target %lu)\n %-*s\t%.3f\n %-*s\t%.3f\n %-*s\t%.3f\n %-*s\t%.3f\n %-*s\t%.3f\n %-*s\t%.3f\n", 
						selectionTarget, 
						slen3, "muvar", MuVar_Accum/setsProcessedTotal, 
						slen3, "musfs", MuSfs_Accum/setsProcessedTotal, 
						slen3, "muld", MuLd_Accum/setsProcessedTotal, 
						slen3, "mustat", Mu_Accum/setsProcessedTotal, 
						slen3, colHeader1, NN_Accum/setsProcessedTotal, 
						slen3, colHeader2, compNN_Accum/setsProcessedTotal);

		if(selectionTargetDThreshold!=0ull)
		{
			fprintf(stdout, "\n");
			fprintf(stdout, " SUCCESS RATE (Distance %lu)\n %-*s\t%.3f\n %-*s\t%.3f\n %-*s\t%.3f\n %-*s\t%.3f\n %-*s\t%.3f\n %-*s\t%.3f\n", 
						selectionTargetDThreshold, 
						slen3, "muvar", MuVar_Success/setsProcessedTotal, 
						slen3, "musfs", MuSfs_Success/setsProcessedTotal, 
						slen3, "muld", MuLd_Success/setsProcessedTotal, 
						slen3, "mustat", Mu_Success/setsProcessedTotal, 
						slen3, colHeader1, NN_Success/setsProcessedTotal, 
						slen3, colHeader2, compNN_Success/setsProcessedTotal);
		}

		fprintf(RAiSD_Info_FP, "\n");
		fprintf(RAiSD_Info_FP, " AVERAGE DISTANCE (Target %lu)\n %-*s\t%.3f\n %-*s\t%.3f\n %-*s\t%.3f\n %-*s\t%.3f\n %-*s\t%.3f\n %-*s\t%.3f\n", 
						selectionTarget, 
						slen3, "muvar", MuVar_Accum/setsProcessedTotal, 
						slen3, "musfs", MuSfs_Accum/setsProcessedTotal, 
						slen3, "muld", MuLd_Accum/setsProcessedTotal, 
						slen3, "mustat", Mu_Accum/setsProcessedTotal, 
						slen3, colHeader1, NN_Accum/setsProcessedTotal, 
						slen3, colHeader2, compNN_Accum/setsProcessedTotal);
						
		if(selectionTargetDThreshold!=0ull)
		{
			fprintf(RAiSD_Info_FP, "\n");
			fprintf(RAiSD_Info_FP, " SUCCESS RATE (Distance %lu)\n %-*s\t%.3f\n %-*s\t%.3f\n %-*s\t%.3f\n %-*s\t%.3f\n %-*s\t%.3f\n %-*s\t%.3f\n", 
						selectionTargetDThreshold, 
						slen3, "muvar", MuVar_Success/setsProcessedTotal, 
						slen3, "musfs", MuSfs_Success/setsProcessedTotal, 
						slen3, "muld", MuLd_Success/setsProcessedTotal, 
						slen3, "mustat", Mu_Success/setsProcessedTotal, 
						slen3, colHeader1, NN_Success/setsProcessedTotal, 
						slen3, colHeader2, compNN_Success/setsProcessedTotal);
		
		
		}
	}
#else
	if(selectionTarget!=0ull)
	{
		fprintf(stdout, "\n");
		fprintf(stdout, " AVERAGE DISTANCE (Target %lu)\n muvar \t%.3f\n musfs \t%.3f\n muld  \t%.3f\n mustat\t%.3f\n", 
						selectionTarget, 
						MuVar_Accum/setsProcessedTotal, 
						MuSfs_Accum/setsProcessedTotal, 
						MuLd_Accum/setsProcessedTotal, 
						Mu_Accum/setsProcessedTotal);

		if(selectionTargetDThreshold!=0ull)
		{
			fprintf(stdout, "\n");
			fprintf(stdout, " SUCCESS RATE (Distance %lu)\n muvar \t%.3f\n musfs \t%.3f\n muld  \t%.3f\n mustat\t%.3f\n", 
						selectionTargetDThreshold, 
						MuVar_Success/setsProcessedTotal, 
						MuSfs_Success/setsProcessedTotal, 
						MuLd_Success/setsProcessedTotal, 
						Mu_Success/setsProcessedTotal);
		}

		fprintf(RAiSD_Info_FP, "\n");
		fprintf(RAiSD_Info_FP, " AVERAGE DISTANCE (Target %lu)\n muvar \t%.3f\n musfs \t%.3f\n muld  \t%.3f\n mustat\t%.3f\n", 
						selectionTarget, 
						MuVar_Accum/setsProcessedTotal, 
						MuSfs_Accum/setsProcessedTotal, 
						MuLd_Accum/setsProcessedTotal, 
						Mu_Accum/setsProcessedTotal);

		if(selectionTargetDThreshold!=0ull)
		{
			fprintf(RAiSD_Info_FP, "\n");
			fprintf(RAiSD_Info_FP, " SUCCESS RATE (Distance %lu)\n muvar \t%.3f\n musfs \t%.3f\n muld  \t%.3f\n mustat\t%.3f\n", 
						selectionTargetDThreshold, 
						MuVar_Success/setsProcessedTotal, 
						MuSfs_Success/setsProcessedTotal, 
						MuLd_Success/setsProcessedTotal, 
						Mu_Success/setsProcessedTotal);
		}
	}
#endif
	if(fpr_loc>0.0)
	{	
		fprintf(stdout, "\n");
		fprintf(stdout, " SORTED DATA (FPR %f)\n Size\t\t\t%d\n Highest Score\t\t%.9f\n Lowest Score\t\t%.9f\n FPR Threshold\t\t%.9f\n Threshold Location\t%d\n", fpr_loc, scr_svec_sz, (double)scr_svec[0],(double)scr_svec[scr_svec_sz-1], (double)scr_svec[(int)(scr_svec_sz*fpr_loc)], (int)(scr_svec_sz*fpr_loc));

		fprintf(RAiSD_Info_FP, "\n");
		fprintf(RAiSD_Info_FP, " SORTED DATA (FPR %f)\n Size\t\t\t%d\n Highest Score\t\t%.9f\n Lowest Score\t\t%.9f\n FPR Threshold\t\t%.9f\n Threshold Location\t%d\n", fpr_loc, scr_svec_sz, (double)scr_svec[0], (double)scr_svec[scr_svec_sz-1], (double)scr_svec[(int)(scr_svec_sz*fpr_loc)], (int)(scr_svec_sz*fpr_loc));
	}

	if(tpr_thres>0.0)
	{
		fprintf(stdout, "\n");
		fprintf(stdout, " SCORE COUNT (Threshold %f)\n TPR\t%f\n", tpr_thres, tpr_scr/setsProcessedTotal);

		fprintf(RAiSD_Info_FP, "\n");
		fprintf(RAiSD_Info_FP, " SCORE COUNT (Threshold %f)\n TPR\t%f\n", tpr_thres, tpr_scr/setsProcessedTotal);
	}

	fflush(stdout);
	fflush(RAiSD_Info_FP);

	RSDVcf2ms_free (RSDVcf2ms, RSDCommandLine);

	RSDPlot_removeRscript(RSDCommandLine, RSDPLOT_BASIC_MU);

	if(RSDCommandLine->createMPlot==1)
	{
		RSDPlot_generateRscript(RSDCommandLine, RSDPLOT_MANHATTAN);
		RSDPlot_createPlot (RSDCommandLine, RSDDataset, RSDMuStat, RSDCommonOutliers, RSDPLOT_MANHATTAN, NULL);		
		RSDPlot_removeRscript(RSDCommandLine, RSDPLOT_MANHATTAN);
	}

	if(RSDCommandLine->createCOPlot==1)
	{
		RSDCommonOutliers_process (RSDCommonOutliers, RSDCommandLine);
	}
	
#ifdef _RSDAI
	RSDImage_free (RSDImage);
	RSDNeuralNetwork_free(RSDNeuralNetwork);
	RSDGrid_free (RSDGrid);
	RSDResults_free (RSDResults);
#endif

	RSDCommonOutliers_free (RSDCommonOutliers);
	RSDCommandLine_free(RSDCommandLine);
	RSDPatternPool_free(RSDPatternPool);
	RSDChunk_free(RSDChunk);
	RSDDataset_free(RSDDataset);
	RSDMuStat_free(RSDMuStat);

	if(scr_svec!=NULL)
		free(scr_svec);	
	
	RSD_printTime(stdout, RAiSD_Info_FP);
	RSD_printMemory(stdout, RAiSD_Info_FP);

	fclose(RAiSD_Info_FP);

	if(RAiSD_SiteReport_FP!=NULL)
		fclose(RAiSD_SiteReport_FP);	

	return 0;
}
