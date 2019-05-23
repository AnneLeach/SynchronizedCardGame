## Synchronized Card Game
This program is an exercise to properly synchronize threads as they play a card game.  There are three player threads and one dealer thread.  At the beginning of each round, the dealer deals a single card to each player.  Then, taking synchronized turns, each player draws another card from the deck.  

If the drawn card matches the hand card, the player wins the round.  In this event, all players return their cards to the deck, wherein the dealer shuffles them and begins the next round.  If the drawn card does not match, the player returns one of the two cards at random to the deck, and the next player may draw.

The results of each round is printed to the screen.  "sampleRuns.txt" provides examples of 3 rounds that were printed to screen.  A more detailed log of each event as it occurs is recorded to log.txt.  

## How to Run
The -std=c99 flag may not be necessary for all environments, but was required to run on TXSTATE's eros server (which the grader used).  The integer value is to plant the seed for randomization.

		gcc <srcFileName> -o <exFileName> -lpthread -std=c99
		./<exFileName> <integer seed>
		
For example:

		gcc prj_2.c -o prj_2 -lpthread -std=c99
		./prj_2 8

## Design
Each card game is adminstered by the dealer thread to the three player threads.  They are synchronized thusly:
- ACTION: Players and dealer prep for round.  They prepare their hand and initialize round variables.  
- SYNCH: Threads wait on remaining table. Once all threads are ready they can continue their execution.  Controlled by "table" mutex and conditional variable.
- ACTION: Dealer deals to player's hands.  No action by players.
- SYNCH: Players wait for dealer to deal. Controlled by "table" mutex and conditional variable.
- ACTION: Players enter drawing mode. No action by dealer.
- SYNCH:  Players take turns drawing cards in drawing mode.  Controlled by "draw" mutex and conditional variable.
- ACTION: After a player wins, all players return cards to deck. No action by dealer.
- SYNCH: At the end of the round, the players wait for the dealer to increment the round counter, thus allowing all threads to continue to next round.  Controlled by "round" mutex and conditional variable.

Additional controls worth noting are the "file" and "deck" mutexes, which protect the shared data of the deck (a linked list), and the file handle.

## Meta

- Author: Angela Leach
- Instructor: Mina Guirguis
- Class: Operating Systems
- Spring 2019 
