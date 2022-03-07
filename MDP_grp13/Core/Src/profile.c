/*
 * profile.c
 *
 *  Created on: 5 Mar 2022
 *      Author: le
 */

#include "profile.h"
#include "stdbool.h"

void Profile_Init(Profile *profile){


	profile->accTime=0;
	profile->constTime=0;
	profile->decTime=0;

	profile->disp=0;
	profile->vel=0;
	profile->acc=0;

	profile->currDisp=0;
	profile->currVel=0;
	profile->currAcc=0;

	profile->accDisp=0;
	profile->constDisp=0;
	profile->decDisp=0;

}

/* Calculates the timing and displacement for each 3 stages of the motion profile */
float trapezoidal(Profile *profile, float totalDisp, float aMax, float vMax){

	profile->aMax = aMax;
	profile->vMax = vMax;

	bool negInput = (totalDisp<0)?true:false;
	totalDisp = fabs(totalDisp);
	float halfDisp = totalDisp/2.0f;

	profile->acc = profile->aMax;

	/* Compute acceleration period */
	profile->accTime = profile->vMax/profile->aMax;
	profile->accDisp = calculateDisp(profile->aMax,0,profile->accTime);
	if(profile->accDisp>halfDisp){
		profile->accDisp = halfDisp;
		profile->accTime = invCalculateDisp(profile->aMax, 0.0, profile->accDisp);
	}

	/* Final Acceleration */


	/* Final velocity */
	profile->vel = profile->accTime * profile->acc;

	/* Compute constant-velocity period */
	profile->constDisp = fmax(totalDisp - (2.0 * profile->accDisp), 0.0);
	profile->constTime = profile->accTime + (profile->constDisp>0?(profile->constDisp/profile->vel):0.0); //no constant velocity if profile is only acceleration then decceleration
	profile->constDisp = profile->constDisp + profile->accDisp;

	/*Compute decceleration period */
	profile->decDisp = totalDisp;
	profile->decTime = profile->accTime + profile->constTime;

	if(negInput){
		profile->vel	   = -(profile->vel);
		profile->acc 	   = -(profile->aMax);
		profile->accDisp   = -(profile->accDisp);
		profile->constDisp = -(profile->constDisp);
		profile->decDisp   = -(profile->decDisp);
	}

	return profile->decTime;
}

/* Set the displacement and time according to the current stage */
void currProfile(Profile *profile, float currTime){

	float currVel;
	float stageTime;
	float off;

	if(currTime>=profile->decTime){
		profile->decDisp = profile->decDisp;
		profile->currVel = 0;
		profile->currAcc = 0;
	}

	if(currTime>=profile->constTime){
		currVel = profile->vel;
		profile->currAcc = -(profile->acc);
		stageTime = currTime - profile->constTime;
		off = profile->constDisp;
	}
	else if(currTime>=profile->accTime){
		currVel = profile->vel;
		profile->currAcc = 0;
		stageTime = currTime - profile->accTime;
		off = profile->accDisp;
	}
	else{
		currVel = 0;
		profile->currAcc = profile->acc;
		stageTime = currTime;
		off = 0;
	}

	/* Output displacement and time */
	profile->currDisp = calculateDisp(profile->currAcc, currVel, stageTime) + off;
	profile->currVel = profile->currAcc * stageTime + currVel;
}


/*Calculates the displacement of object given constant acceleration and initial velocity for a given time */
float calculateDisp(float acc, float vel, float time){
	return (0.5 * acc * powf(time,2) + (vel * time));
}

/*Calculates the time needed given constant acceleration, initial velocity and displacement*/
float invCalculateDisp(float acc, float vel, float disp){
	float a = 0.5 * acc;
	float b = vel;
	float c = -disp;

	float s1 = (- b + sqrt(pow(b,2) - 4 * a * c))/(2 * a);
	float s2 = (- b - sqrt(pow(b,2) - 4 * a * c))/(2 * a);

	return fmax(s1,s2);
}
