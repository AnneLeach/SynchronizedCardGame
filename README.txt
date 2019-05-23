> HOW TO RUN
	
	> EXAMPLE

		gcc prj_2.c -o prj_2 -lpthread -std=c99
		./prj_2 8

		gcc <srcFileName> -o <exFileName> -lpthread -std=c99
		./<exFileName> <integer seed>

	> INFO

		The -std=c99 flag may not be necessary, but was required to run on TXSTATE's eros server.  The integer value is to plant the seed for randomization.

> ABOUT

	Author: Angela Leach
	Instructor: Mina Guirguis
	Class: Operating Systems
	Date: 4/26/19

	The purpose of this program was to practice synchronization of threads through a simple card game, wherein 3 players take turns drawing from a deck. A matching pair wins the round.

	Although the dealer and player threads do not carry out heavy computation, the writing of this program provided a good introduction to the synchronization of threads, which was the original purpose.

> PROGRAM DESIGN

	This program runs a card game between 3 players, administered by a dealer, each inhabting their own respective thread.  Dealer and player threads are synchronized thusly:

		> ACTION: Players and dealer prep for round.  They prepare their hand and initialize round variables.  

		> SYNCH: Threads wait on remaining table. Once all threads are ready they can continue their execution.  Controlled by "table" mutex and conditional variable.

		> ACTION: Dealer deals to player's hands.  No action by players.

		> SYNCH: Players wait for dealer to deal. Controlled by "table" mutex and conditional variable.

		> ACTION: Players enter drawing mode. No action by dealer.

		> SYNCH:  Players take turns drawing cards in drawing mode.  Controlled by "draw" mutex and conditional variable.

		> ACTION: After a player wins, all players return cards to deck. No action by dealer.

		> SYNCH: At the end of the round, the players wait for the dealer to increment the round counter, thus allowing all threads to continue to next round.  Controlled by "round" mutex and conditional variable.

	Additional controls worth noting are the "file" and "deck" mutexes, which protect the shared data of the deck (a linked list), and the file handle.