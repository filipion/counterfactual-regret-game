#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <cassert>
#include <algorithm>

#define MAX 200000005
#define GAME_N 3
#define NUM_ACTIONS GAME_N
#define N_REPR (GAME_N * GAME_N) * (1 << GAME_N * 3)

using namespace std;

float cumulativeRegret[N_REPR][NUM_ACTIONS];
float cumulativeStrategy[N_REPR][NUM_ACTIONS];

int PLAYER_1 = 0, PLAYER_2 = 1, BANK = 2;
float null_strat[NUM_ACTIONS];

int getAction(float strategy[]);

class Gamestate{
public:
	int repr; //compact representation of the history
	int infostate; //tracks the representation of the public information that feeds into CFR
	vector<int> history; //unrolled representation of the history of the state. Allows us to know the score
	vector<int> bank_hand; //order of the cards in the bank deck. Shuffled at the beginning of a CFR iteration
	int hist_len = 0;

	Gamestate(int r): repr{r}{
		vector<int> bank(GAME_N);
		for(int i = 0; i < GAME_N; ++i)
			bank[i] = i;
		random_shuffle(bank.begin(), bank.end());
		bank_hand = bank;
	}

	Gamestate update(int player, int card){
		Gamestate next_node(0);
		next_node = *this;

		int play_bit = 1 << (card + player * GAME_N);
		assert((play_bit & next_node.repr) == 0); //check the card was not already played
		next_node.repr += play_bit;


		if(player == BANK){
			next_node.repr = ((card + 2) << (3 * GAME_N)) + (next_node.repr % ((1 << (3 * GAME_N))));
			next_node.infostate = next_node.repr; //whenever a card is drawn from the bank that means public info can be updated
		}

		next_node.history.push_back(card);
		++next_node.hist_len;

		return next_node;
	}

	Gamestate sample_bank(){
		assert(hist_len % 3 == 0);
		int draw = bank_hand[hist_len / 3];
		return this->update(BANK, draw);
	}

	int utility(int player){
		int score_1 = 0;
		int score_2 = 0;
		assert(hist_len == 3 * GAME_N);
		for(int i = 0; i < 3 * GAME_N; i += 3){
			if(history[i + 1] > history[i + 2])
				score_1 += history[i] + 2;
			if(history[i + 1] < history[i + 2])
				score_2 += history[i] + 2;
			if(history[i + 1] == history[i + 2])
				continue;
		}
		int P1_utility = 0;
		if(score_1 > score_2)
			P1_utility = 1;
		else if(score_1 < score_2) 
			P1_utility = -1;

		if(player == PLAYER_1)
			return P1_utility;
		else
			return -P1_utility;
	}

};

string print_repr(int state){
	int reprcopy = state;
	string answer;
	answer += "P1's hand: ";
	for(int i = 0; i < GAME_N; ++i){
		if(reprcopy % 2 == 0){
			answer += to_string(i);
			answer += " ";
		}
		reprcopy /= 2;
	}

	answer += "\nP2's hand: ";
	for(int i = 0; i < GAME_N; ++i){
		if(reprcopy % 2 == 0){
			answer += to_string(i);
			answer += " ";
		}
		reprcopy /= 2;
	}

	answer += "\nThe bank's hand: ";
	for(int i = 0; i < GAME_N; ++i){
		if(reprcopy % 2 == 0){
			answer += to_string(i);
			answer += " ";
		}
		reprcopy /= 2;
	}

	answer += "\nNext bounty: ";
	answer += to_string(reprcopy);

	return answer;
}

vector<int> legal_moves(int state, int player) {
	vector<int> ans;

	if(player == PLAYER_2)
		state = (state >> GAME_N);

	for (int a = 0; a < NUM_ACTIONS; a++){
	 	if(state % 2 == 0)
	 		ans.push_back(a);
	 	state /= 2;
	}

	return ans;
}

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

float* getStrategy(int state, int player, float realizationWeight){
	float* myStrategy = new float[NUM_ACTIONS];
	float totalRegret = 0;
	vector<int> legal = legal_moves(state, player);
	int normalization = legal.size();

	for(auto i: legal)
		if(cumulativeRegret[state][i] > 0)
			totalRegret += cumulativeRegret[state][i];


	for(auto i: legal){
		if(totalRegret > 0)
			myStrategy[i] = cumulativeRegret[state][i] > 0 ? (float)cumulativeRegret[state][i] / totalRegret : 0;
		else
			myStrategy[i] = (float)1 / normalization;

		cumulativeStrategy[state][i] += realizationWeight * myStrategy[i];
	}

	return myStrategy;
}

float* getAverageStrategy(int state, int player) {
	float* avgStrategy = new float[NUM_ACTIONS];
	float normalizingSum = 0;
	vector<int> legal = legal_moves(state, player);
	int normalization = legal.size();

	for (auto a: legal)
		normalizingSum += cumulativeStrategy[state][a];
	for (auto a: legal)
		if (normalizingSum > 0)
			avgStrategy[a] = cumulativeStrategy[state][a] / normalizingSum;
		else
			avgStrategy[a] = 1.0 / normalization;
	return avgStrategy;
}


float cfr(Gamestate node, int player, int iteration, float p1, float p2){
	if(node.hist_len == 3 * GAME_N) //game has ended
		return (float)node.utility(player);

	if(node.hist_len % 3 == 0) //a chance event occurs next
		return cfr(node.sample_bank(), player, iteration, p1, p2);
	
	int player_to_move;

	if(node.hist_len % 3 == 1)
		player_to_move = PLAYER_1;
	else if(node.hist_len % 3 == 2)
		player_to_move = PLAYER_2;

	int info = node.infostate;
	//Find the current strategy profile
	vector<int> actions = legal_moves(info, player_to_move);
	float* strategy = getStrategy(info, player_to_move, (player_to_move == PLAYER_1 ? p1 : p2));

	//Recursively compute value and counterfactual value
	float strategy_value = 0;
	vector<float> counterfactual_strategy_value(NUM_ACTIONS);
	
	for(auto a: actions){
		Gamestate next_node(0);
		next_node = node.update(player_to_move, a);

		if(player_to_move == PLAYER_1)
			counterfactual_strategy_value[a] = cfr(next_node, player, iteration, strategy[a] * p1, p2);
		else
			counterfactual_strategy_value[a] = cfr(next_node, player, iteration, p1, strategy[a] * p2);

		strategy_value += strategy[a] * counterfactual_strategy_value[a];
	}

	//Accumulate regret and unnormalized strategy probabilities
	float p_player = (player == PLAYER_1 ? p1 : p2);
	float p_other = (player == PLAYER_1 ? p2 : p1);
	if(player == player_to_move){
		for(auto a: actions){
			cumulativeRegret[info][a] += p_other * (counterfactual_strategy_value[a] - strategy_value);
		}
	}

	return strategy_value;
}

void train(int iterations) {
	Gamestate root(0);
	for (int t = 1; t <= iterations; t++) {
		Gamestate new_root(0);
		root = new_root;

		if(t % 1000 == 0)
			cout << "\nCFR iteration " << t << "/" << iterations;

		cfr(root, PLAYER_1, t, 1.0, 1.0);
	}
}

void ai_vs_ai(){
	Gamestate game(0);
	for(int i = 0; i < GAME_N; ++i){
		game = game.sample_bank();
		int info = game.infostate;

		int p1_move = getAction(getAverageStrategy(info, PLAYER_1));
		int p2_move = getAction(getAverageStrategy(info, PLAYER_2));

		game = game.update(PLAYER_1, p1_move);
		game = game.update(PLAYER_2, p2_move);
		
	}

	for(auto x: game.history)
		cout << x << " ";

	cout << endl << game.utility(0) << endl;
}


int play_vs_ai(){
	Gamestate game(0);
	for(int i = 0; i < GAME_N; ++i){
		game = game.sample_bank();
		int info = game.infostate;

		for(auto x: game.history)
			cout << x << " ";

		bool is_legal = false;
		int p1_move;
		while(!is_legal){
			cout << "\nPlease select a move: ";
			cin >> p1_move;

			for(auto legal_move: legal_moves(info, PLAYER_1))
				if(p1_move == legal_move)
					is_legal = true;

			if(!is_legal)
				cout << "Invalid move.";
		}

		int p2_move = getAction(getAverageStrategy(info, PLAYER_2));

		game = game.update(PLAYER_1, p1_move);
		game = game.update(PLAYER_2, p2_move);

	}

	int human_score = game.utility(0);
	switch(human_score){
		case 0: cout << "Draw.\n"; break;
		case 1: cout << "You won.\n"; break;
		case -1: cout << "You lost.\n"; break;
	}
	return human_score;
}

int main(){
	srand(time(0));
	train(100000);

	int total = 0;
	for(int i = 1; i <=100; ++i){
		cout << "\nGame number " << i << endl;
		total -= play_vs_ai();
		cout << "Current total AI score: " << total << endl;
	}
}	