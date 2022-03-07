/*
 * profile.h
 *
 *  Created on: 5 Mar 2022
 *      Author: le
 */

#ifndef INC_PROFILE_H_
#define INC_PROFILE_H_

typedef struct{

	float vMax; //Maximum velocity
	float aMax; //Maximum acceleration

	float accTime;
	float constTime;
	float decTime;

	float disp;
	float vel;
	float acc;

	float currDisp;
	float currVel;
	float currAcc;

	float accDisp;
	float constDisp;
	float decDisp;

} Profile;

float trapezoidal(Profile *profile, float totalDisp, float aMax, float vMax);
void currProfile(Profile *profile, float currTime);
float calculateDisp(float acc, float vel, float time);
float invCalculateDisp(float acc, float vel, float disp);

#endif /* INC_PROFILE_H_ */
