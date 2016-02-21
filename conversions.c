#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "cards.h"

// The following function takes the value of the previous hand and calculates it anew when given a new card. hands are 2 element arrays with the value of the hand and the number of aces worth 11 in the hand(which should be 0 or 1 at all points except sometimes when a card is being added).
int newhandval(hand_t *hand, int newcard)  {
	
	if(newcard < 0 || newcard > 51)
		return -1;
	hand->contents[hand->num_cards]=newcard;
	hand->num_cards++;
	hand->value += cards[newcard].value;
	if(cards[newcard].value == 11) // New card is an ace
		hand->num_aces++;
			// If we bust and have aces worth 11, downgrade them to being worth 1 until we either run out of aces or our hand value is no longer > 21 
	while(hand->value > 21 && hand->num_aces > 0)  {
		hand->value -= 10;
		hand->num_aces--;
	}
	return 0;
}

int score(hand_t *hands)   {
	// This function scores the game and returns the number of the
	// winning player. When more players per game are added, it will
	// return an array of winning players. If 0 is returned, no players
	// have won. If 2 is returned, the game was a draw.

	int i;

	for(i = 1; i < MAX_PLAYERS_PER_GAME; i++)	{
		//basic comparison
		if(hands[i].value > 21)
			return 0;	
		else if(hands[0].value > 21)
			return 1;	
		else if(hands[i].value < hands[0].value)
			return 0;	
		else if(hands[i].value > hands[0].value)
			return 1;
		else  { 
			// values are equal. 
			// Additional calculations needed in the case of a 21 because
			// if only one of the player or a dealer has a blackjack
			// (A + 10), that player wins; otherwise, they draw.
			if(hands[i].value != 21)
				return 2;
				// if players have 21s; check which, if any, have a blackjack. Rework maybe?
			else {
				int player_blackjack = 0, dealer_blackjack = 0;
				if(hands[i].num_cards == 2)
					player_blackjack = 1;
				if(hands[0].num_cards == 2)
					dealer_blackjack = 1;
				if(player_blackjack == dealer_blackjack)
					return 2;
				else if(player_blackjack > dealer_blackjack)
					return 1;
				else
					return 0;
			}
		}
	}
	return -1;
}
