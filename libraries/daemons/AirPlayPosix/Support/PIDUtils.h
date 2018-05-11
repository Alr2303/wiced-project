/*
	Portions Copyright (C) 2010 Apple Inc. All Rights Reserved.
	
	Derived from code from the book "Applied Control Theory for Embedded Systems" by Tim Wescott.
*/

#ifndef	__PIDUtils_h__
#define	__PIDUtils_h__

#ifdef	__cplusplus
extern "C" {
#endif

//---------------------------------------------------------------------------------------------------------------------------
/*!	@group		Proportional-Integral-Derivative (PID) Controller
	@abstract	Structure and API for a generic PID controller.
*/

typedef struct
{
	double		pGain;		// Proportional Gain.
	double		iState;		// Integrator state.
	double		iMin, iMax;	// Integrator limits.
	double		iGain;		// Integrator gain (always less than 1).
	double		dState;		// Differentiator state.
	double		dpGain;		// Differentiator filter gain = 1 - pole.
	double		dGain;		// Derivative gain.
	
}	PIDContext;

void	PIDInit( PIDContext *inPID, double pGain, double iGain, double dGain, double dPole, double iMin, double iMax );
#define PIDReset( CONTEXT )		do { (CONTEXT)->iState = 0; (CONTEXT)->dState = 0; } while( 0 )
double	PIDUpdate( PIDContext *inPID, double input );

#ifdef __cplusplus
}
#endif

#endif	// __PIDUtils_h__
