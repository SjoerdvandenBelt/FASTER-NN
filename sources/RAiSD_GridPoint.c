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

RSDGridPoint_t * RSDGridPoint_new (void)
{
	RSDGridPoint_t * RSDGridPoint = (RSDGridPoint_t *) rsd_malloc (sizeof(RSDGridPoint_t));
	assert(RSDGridPoint!=NULL);
	
	RSDGridPoint->size=0;
	RSDGridPoint->positions = NULL;
	
	RSDGridPoint->nnScores0 = NULL;
	RSDGridPoint->nnScores1 = NULL;
	RSDGridPoint->nnScores2 = NULL;
	RSDGridPoint->nnScores3 = NULL;
	RSDGridPoint->muVarScores = NULL;
	RSDGridPoint->muSfsScores = NULL;
	RSDGridPoint->muLdScores = NULL;
	RSDGridPoint->muScores = NULL;
	
	RSDGridPoint->nnScore0Final = 0.0;
	RSDGridPoint->nnScore1Final = 0.0;
	RSDGridPoint->nnScore2Final = 0.0;
	RSDGridPoint->nnScore3Final = 0.0;
	RSDGridPoint->muVarScoreFinal = 0.0;
	RSDGridPoint->muSfsScoreFinal = 0.0;
	RSDGridPoint->muLdScoreFinal = 0.0;
	RSDGridPoint->muScoreFinal = 0.0;
	
	RSDGridPoint->positionFinal = -1.0;

	RSDGridPoint->finalRegionCenter = -1.0;
	RSDGridPoint->finalRegionStart = -1.0;
	RSDGridPoint->finalRegionEnd = -1.0;
	RSDGridPoint->finalRegionScore = 0.0f;
	
	return RSDGridPoint;		
}

void RSDGridPoint_addNewPosition (RSDGridPoint_t * RSDGridPoint, double pos, char * networkArchitecture)
{
	if(RSDGridPoint==NULL)
		return;
		
	assert(pos>=0.0);
	
	RSDGridPoint->size++;
	
	RSDGridPoint->positions = rsd_realloc (RSDGridPoint->positions, sizeof(RSDGridPoint->positions)*RSDGridPoint->size);
	assert(RSDGridPoint->positions!=NULL);
	
	RSDGridPoint->nnScores0 = rsd_realloc (RSDGridPoint->nnScores0, sizeof(RSDGridPoint->nnScores0)*RSDGridPoint->size);
	assert(RSDGridPoint->nnScores0!=NULL);
	
	RSDGridPoint->nnScores1 = rsd_realloc (RSDGridPoint->nnScores1, sizeof(RSDGridPoint->nnScores1)*RSDGridPoint->size);
	assert(RSDGridPoint->nnScores1!=NULL);
	
	if(!strcmp(networkArchitecture, ARC_SWEEPNETRECOMB))
	{
		RSDGridPoint->nnScores2 = rsd_realloc (RSDGridPoint->nnScores2, sizeof(RSDGridPoint->nnScores0)*RSDGridPoint->size);
		assert(RSDGridPoint->nnScores2!=NULL);
	
		RSDGridPoint->nnScores3 = rsd_realloc (RSDGridPoint->nnScores3, sizeof(RSDGridPoint->nnScores1)*RSDGridPoint->size);
		assert(RSDGridPoint->nnScores3!=NULL);
	}
	
	RSDGridPoint->muVarScores = rsd_realloc (RSDGridPoint->muVarScores, sizeof(RSDGridPoint->muVarScores)*RSDGridPoint->size);
	assert(RSDGridPoint->muVarScores!=NULL);
	
	RSDGridPoint->muSfsScores = rsd_realloc (RSDGridPoint->muSfsScores, sizeof(RSDGridPoint->muSfsScores)*RSDGridPoint->size);
	assert(RSDGridPoint->muSfsScores!=NULL);
	
	RSDGridPoint->muLdScores = rsd_realloc (RSDGridPoint->muLdScores, sizeof(RSDGridPoint->muLdScores)*RSDGridPoint->size);
	assert(RSDGridPoint->muLdScores!=NULL);
	
	RSDGridPoint->muScores = rsd_realloc (RSDGridPoint->muScores, sizeof(RSDGridPoint->muScores)*RSDGridPoint->size);
	assert(RSDGridPoint->muScores!=NULL);	
	
	RSDGridPoint->positions[RSDGridPoint->size-1] = pos;
	RSDGridPoint->nnScores0[RSDGridPoint->size-1] = 0.0;
	RSDGridPoint->nnScores1[RSDGridPoint->size-1] = 0.0;
	if(!strcmp(networkArchitecture, ARC_SWEEPNETRECOMB))
	{
		RSDGridPoint->nnScores2[RSDGridPoint->size-1] = 0.0;
		RSDGridPoint->nnScores3[RSDGridPoint->size-1] = 0.0;
	}
	RSDGridPoint->muVarScores[RSDGridPoint->size-1] = 0.0;
	RSDGridPoint->muSfsScores[RSDGridPoint->size-1] = 0.0;
	RSDGridPoint->muLdScores[RSDGridPoint->size-1] = 0.0;
	RSDGridPoint->muScores[RSDGridPoint->size-1] = 0.0;
}

void RSDGridPoint_free (RSDGridPoint_t * RSDGridPoint)
{
	if(RSDGridPoint==NULL)
		return;
	
	if(RSDGridPoint->positions!=NULL)
	{
		free(RSDGridPoint->positions);
		RSDGridPoint->positions=NULL;
	}
	
	if(RSDGridPoint->nnScores0!=NULL)
	{
		free(RSDGridPoint->nnScores0);
		RSDGridPoint->nnScores0=NULL;
	}
	
	if(RSDGridPoint->nnScores1!=NULL)
	{
		free(RSDGridPoint->nnScores1);
		RSDGridPoint->nnScores1=NULL;
	}
	
	if(RSDGridPoint->nnScores2!=NULL)
	{
		free(RSDGridPoint->nnScores2);
		RSDGridPoint->nnScores2=NULL;
	}
	
	if(RSDGridPoint->nnScores3!=NULL)
	{
		free(RSDGridPoint->nnScores3);
		RSDGridPoint->nnScores3=NULL;
	}	
	
	if(RSDGridPoint->muVarScores!=NULL)
	{
		free(RSDGridPoint->muVarScores);
		RSDGridPoint->muVarScores=NULL;
	}
	
	if(RSDGridPoint->muSfsScores!=NULL)
	{
		free(RSDGridPoint->muSfsScores);
		RSDGridPoint->muSfsScores=NULL;
	}
	
	if(RSDGridPoint->muLdScores!=NULL)
	{
		free(RSDGridPoint->muLdScores);
		RSDGridPoint->muLdScores=NULL;
	}
	
	if(RSDGridPoint->muScores!=NULL)
	{
		free(RSDGridPoint->muScores);
		RSDGridPoint->muScores=NULL;
	}
	
	free(RSDGridPoint);
	RSDGridPoint = NULL;
}

void RSDGridPoint_resetFinalScores (RSDGridPoint_t * RSDGridPoint) 
{
	assert(RSDGridPoint!=NULL);

	RSDGridPoint->nnScore0Final = 0.0;
	RSDGridPoint->nnScore1Final = 0.0;
	RSDGridPoint->nnScore2Final = 0.0;
	RSDGridPoint->nnScore3Final = 0.0;
	RSDGridPoint->muVarScoreFinal = 0.0;
	RSDGridPoint->muSfsScoreFinal = 0.0;
	RSDGridPoint->muLdScoreFinal = 0.0;
	RSDGridPoint->muScoreFinal = 0.0;		
}

double maxd (double a, double b)
{
	if(a>b)
		return a;
		
	return b;
}

void RSDGridPoint_reduce (RSDGridPoint_t * RSDGridPoint, int op, char * networkArchitecture) // op=0->avg, op=1->max
{
	assert(RSDGridPoint!=NULL);
	
	RSDGridPoint_resetFinalScores (RSDGridPoint);
	
	if(op==0)
	{
		RSDGridPoint->positionFinal = (double)RSDGridPoint->positions[0];
		
		for (int k = 0; k<(int)RSDGridPoint->size;k++)
		{
			assert(RSDGridPoint->positionFinal==(double)RSDGridPoint->positions[k]);
			
			RSDGridPoint->nnScore0Final += (double)RSDGridPoint->nnScores0[k];
			RSDGridPoint->nnScore1Final += (double)RSDGridPoint->nnScores1[k];
			
			if(!strcmp(networkArchitecture, ARC_SWEEPNETRECOMB))	
			{
				RSDGridPoint->nnScore2Final += (double)RSDGridPoint->nnScores2[k];			
				RSDGridPoint->nnScore3Final += (double)RSDGridPoint->nnScores3[k];
			}
			
			RSDGridPoint->muVarScoreFinal += (double)RSDGridPoint->muVarScores[k];
			RSDGridPoint->muSfsScoreFinal += (double)RSDGridPoint->muSfsScores[k];
			RSDGridPoint->muLdScoreFinal += (double)RSDGridPoint->muLdScores[k];
			RSDGridPoint->muScoreFinal += (double)RSDGridPoint->muScores[k];
		}		

		RSDGridPoint->nnScore0Final /= (double)RSDGridPoint->size;
		RSDGridPoint->nnScore1Final /= (double)RSDGridPoint->size;
		if(!strcmp(networkArchitecture, ARC_SWEEPNETRECOMB))	
		{
			RSDGridPoint->nnScore2Final /= (double)RSDGridPoint->size;
			RSDGridPoint->nnScore3Final /= (double)RSDGridPoint->size;
		}
		RSDGridPoint->muVarScoreFinal /= (double)RSDGridPoint->size;
		RSDGridPoint->muSfsScoreFinal /= (double)RSDGridPoint->size;
		RSDGridPoint->muLdScoreFinal /= (double)RSDGridPoint->size;
		RSDGridPoint->muScoreFinal /= (double)RSDGridPoint->size;
	}
	else
	{
		// remove outliers?
		
		RSDGridPoint->positionFinal = (double)RSDGridPoint->positions[0];
		
		for (int k = 0; k<(int)RSDGridPoint->size;k++)
		{
			assert(RSDGridPoint->positionFinal==(double)RSDGridPoint->positions[k]);
			
			RSDGridPoint->nnScore0Final = maxd (RSDGridPoint->nnScore0Final, (double)RSDGridPoint->nnScores0[k]);
			RSDGridPoint->nnScore1Final = maxd (RSDGridPoint->nnScore1Final, (double)RSDGridPoint->nnScores1[k]);
			if(!strcmp(networkArchitecture, ARC_SWEEPNETRECOMB))	
			{
				RSDGridPoint->nnScore2Final = maxd (RSDGridPoint->nnScore2Final, (double)RSDGridPoint->nnScores2[k]);
				RSDGridPoint->nnScore3Final = maxd (RSDGridPoint->nnScore3Final, (double)RSDGridPoint->nnScores3[k]);
			}
			RSDGridPoint->muVarScoreFinal = maxd (RSDGridPoint->muVarScoreFinal, (double)RSDGridPoint->muVarScores[k]);
			RSDGridPoint->muSfsScoreFinal = maxd (RSDGridPoint->muSfsScoreFinal, (double)RSDGridPoint->muSfsScores[k]);
			RSDGridPoint->muLdScoreFinal = maxd (RSDGridPoint->muLdScoreFinal, (double)RSDGridPoint->muLdScores[k]);
			RSDGridPoint->muScoreFinal = maxd (RSDGridPoint->muScoreFinal, (double)RSDGridPoint->muScores[k]);
		}
	}

}

#endif
