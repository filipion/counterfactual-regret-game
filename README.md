# The story
I used like to go around and teach people this neat little card game. It's pretty fun, as the game depends on predicting your friends' moves, as well as on keeping yourself unpredictable. Later I was reading about advances in AI like FAIR's [ReBeL](https://ai.facebook.com/blog/rebel-a-general-game-playing-ai-bot-that-excels-at-poker-and-more/), a poker-playing neural net, and I realized those techniques are probably powerful enough to solve one of my favourite childhood card games.  

# Rules
This project implements counterfactual regret minimization for a smaller version of Psychological Jiujitsu (which I will abbreviate to Psycho). Here is how to play the game. (Rules quoted from https://gamerules.com/rules/psychological-jujitsu-card-game/)
> Psychological Jujutsu is a game entirely based on psychology. The game is purely strategy with limited amounts of chance involved in influencing the outcome of the game. Outsmarting or having quick reflexes, however, are not tools which will help you win. In order to win the game, you must know what your opponent is thinking.
> ### Deal
> A single suit is removed from the deck and shuffled. After, it is placed on the table, face-down. Players receive an entire set of cards, from Ace to King. Each of these cards represent numbers 1-13; Ace being equal to one, two equal to two, and so on, ending with King equal to thirteen.
The game of psychological Jujitsu has a total of 13 rounds.
> ### Bidding
> At the start of each new round, the top card from the pile containing the extra suit is auctioned off to players. Players pick a card from their hand, secretly, and on the count of three simultaneously reveal their card’s. These cards are bids for the top card of the extra suit. Highest ranking card, which translates to the highest bid, wins the card. All cards are worth their numerical value, either printed or assigned to them.
In the event of a tie, the card being auctioned is discarded as are all cards played that round.
However, there is a variation to this rule. Players may also discard only the two tied cards and award the bid to the next highest ranking card.
> ### Post-Bid & Scoring
> Once a player wins a card, the card must be placed in front of them, face-up.
Since there are 13 rounds, and each player has 13 cards, players play only one card per round.
After each hand is completed, player’s score their hand and add it to their cumulative score. Each hand has 91 points total. Players earn points from winning cards in the bid.
Keep track of the cards your opponent’s play so you can play your hand more strategically.

In order to keep the state space manageable without neural networks, this first project focuses on solving a simpler version of the game (6-7 cards in hand and two players instead of three). As in the case of tabular reinforcement learning, this algorithm will not scale to larger games like No-Limit Texas Hold'Em without additional techniques and engineering effort.

# Solving the game: counterfactual regret minimization
### Nash equilibria
How do we program a bot for this game? The problem seems different from, say, chess, where there exists a theoretical optimum strategy. In our game (like in poker and like in rock-paper-scissors) it is important not only to extract value efficiently throughout the game, but also to do so in an unpredictable manner (otherwise a human opponent playing against our AI could just predict our next bid and bid exactly one point higher).

For example, in rock-paper-scissors it is best to pick your move uniformly at random, otherwise your opponent can exploit you. However, if for example winning (or losing) with scissors doubled the payoff (which can be negative), then that optimal strategy changes, and you should pick your move like this: 40% of times go Rock, 40% of times go Paper and 20% of times go Scissors.

These are examples of Nash equilibria in those games. Playing to approximate a Nash equilibrium is an "unexploitable" strategy, i.e. you cannot beat it. (You can equal it: If both players know the Nash equilibrium, the game amounts to a coin flip). In two player symmetric zero-sum games a Nash equilibrium is guaranteed to exist. This property is what has been used to build bots that surpass human performance at poker.

### How it works (high level sketch)
To represent the equilibrium strategy, we will maintain an exhaustive table of every information state and the probability distribution for the actions our bot will take. (As the state space grows larger, the table would naturally be replaced by a policy-network) Initially this strategy is random. The algorithm iteratively approximates a perfect solution as follows:

* Play the game according to the *current strategy profile*. Compute "counterfactual regret" for the actions that we did not take.
* Accumulate regret values for each infostate-action pair.
* The new *strategy profile* becomes: pick actions with probability proportional to their cumulated regret.
* (Important!) We are almost done. However, the *strategy profile* which we have computed does **not** converge to the solution. Instead it will "oscillate erratically". Our final strategy will be an average of all of the *strategy profiles* computed so far. This strategy is proven to converge to equilibrium.


# Configuring the game
In the file `psycho.cpp` there is a macro parameter `GAME_SIZE` that determines the number of cards in each player's hand as well as the number of rounds. The number of representations of a game is about `N_REPR ~ GAME_SIZE * (GAME_SIZE ^ 8)`, so I recommend setting it to at most 6-7. For a `GAME_SIZE` of 1-2 the game is trivial. Another parameter that can be changed is the number of iterations which should be larger than `Factorial(GAME_SIZE)`.

By default you can play the game of size 4. To do so just compile and run the file `psycho.cpp`. The program will first compute strategy tables with CFR for a couple of minutes then it will offer a few games.

# References
- Todd W. Neller and Marc Lancto. An Introduction to Counterfactual Regret Minimization (more detailed explanation of the algorithm)
- Martin Zinkevich, Michael Johanson, Michael Bowling, and Carmelo Piccione. Regret minimization
in games with incomplete information (for mathematical proof of correctness)
- https://gamerules.com/rules/psychological-jujitsu-card-game/ 
