/*
	Portions Copyright (C) 2010 Apple Inc. All Rights Reserved.
	
	Derived from code from the book "Applied Control Theory for Embedded Systems" by Tim Wescott.
*/

#include "PIDUtils.h"

//===========================================================================================================================
//	PIDInit
//===========================================================================================================================

void	PIDInit( PIDContext *inPID, double pGain, double iGain, double dGain, double dPole, double iMin, double iMax )
{
	inPID->iState	= 0.0;
	inPID->iMax		= iMax;
	inPID->iMin		= iMin;
	inPID->iGain	= iGain;
	
	inPID->dState	= 0.0;
	inPID->dpGain	= 1.0 - dPole;
	inPID->dGain	= dGain * ( 1.0 - dPole );
	
	inPID->pGain	= pGain;
}

//===========================================================================================================================
//	PIDUpdate
//===========================================================================================================================

double	PIDUpdate( PIDContext *inPID, double input )
{
	double		output;
	double		dTemp;
	double		pTerm;
	
	pTerm = input * inPID->pGain; // Proportional.
	
	// Find the integrator state and limit it.
	
	inPID->iState = inPID->iState + ( inPID->iGain * input );
	output = inPID->iState + pTerm;
	if( output > inPID->iMax )
	{
		inPID->iState = inPID->iMax - pTerm;
		output = inPID->iMax;
	}
	else if( output < inPID->iMin )
	{
		inPID->iState = inPID->iMin - pTerm;
		output = inPID->iMin;
	}
	
	// Calculate the differentiator state.
	
	dTemp = input - inPID->dState;
	inPID->dState += inPID->dpGain * dTemp;
	output += dTemp * inPID->dGain;
	return( output );
}
