#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <ctime>
#define MAX 200000005
using namespace std;

#define NUM_ACTIONS 3

int cumulativeRegret[NUM_ACTIONS];
float strategy[NUM_ACTIONS];
float strategySum[NUM_ACTIONS];
float oppStrategy[NUM_ACTIONS] = {0.33, 0.33, 0.43};
int RPS_payoff[NUM_ACTIONS][NUM_ACTIONS] = {{0, -1, 2}, {1, 0, -2}, {-2, 2, 0}};


int getAction(float strategy[]) {
	double r = (float)(rand()) / RAND_MAX;
	int a = 0;
	float cumulativeProbability = 0;
	while (a < NUM_ACTIONS - 1) {
		cumulativeProbability += strategy[a];
		if (r < cumulativeProbability)
			break;
		a++;
	}
	return a;
}

float* RMStrategy(){
	float* myStrategy = new float[NUM_ACTIONS];
	int totalRegret = 0;

	for(int i = 0; i < NUM_ACTIONS; ++i)
		if(cumulativeRegret[i] > 0)
			totalRegret += cumulativeRegret[i];

	if(totalRegret > 0)
		for(int i = 0; i < NUM_ACTIONS; ++i){
			myStrategy[i] = cumulativeRegret[i] > 0 ? (float)cumulativeRegret[i] / totalRegret : 0;
			strategySum[i] += myStrategy[i];
		}
	else
		for(int i = 0; i < NUM_ACTIONS; ++i){
			myStrategy[i] = (float)1 / NUM_ACTIONS;
			strategySum[i] += myStrategy[i];
		}

	return myStrategy;

}


void train(int iterations) {
	float actionUtility[NUM_ACTIONS];
	for (int i = 0; i < iterations; i++) {
		float* myStrategy = RMStrategy();
		int myAction = getAction(myStrategy);
		int otherAction = getAction(myStrategy);

		for(int j = 0; j < NUM_ACTIONS; ++j){
			cumulativeRegret[j] += (RPS_payoff[j][otherAction] - RPS_payoff[myAction][otherAction]);
		}
	}
}

float* getAverageStrategy() {
	float* avgStrategy = new float[NUM_ACTIONS];
	float normalizingSum = 0;
	for (int a = 0; a < NUM_ACTIONS; a++)
		normalizingSum += strategySum[a];
	for (int a = 0; a < NUM_ACTIONS; a++)
		if (normalizingSum > 0)
			avgStrategy[a] = strategySum[a] / normalizingSum;
		else
			avgStrategy[a] = 1.0 / NUM_ACTIONS;
	return avgStrategy;
}


int main(){
	srand(time(0));
	train(1e6);
	float* avgStrategy = getAverageStrategy();
	for(int a = 0; a < NUM_ACTIONS; ++a)
		cout << avgStrategy[a] << " ";
}