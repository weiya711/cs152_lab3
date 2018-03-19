// my_predictor.h
// This file contains a sample my_predictor class.
// It is a simple 32,768-entry gshare with a history length of 15.
// Note that this predictor doesn't use the whole 32 kilobytes available
// for the CBP-2 contest; it is just an example.

class my_update : public branch_update {
public:
	unsigned int index;
};

class my_predictor : public branch_predictor {
public:
#define HISTORY_LENGTH	30              // Increased length of the history from 15 to 30
                                        // Global history length (keeps up to HISTORY_LENGTH branches with h1 = taken, 0 = not taken)
#define TABLE_BITS	30              // Increased length of tab from 15 to 30
                                        // Local history table (number of entries in table)
// My code
#define SHORT_HISTORY_LENGTH 2         // Keep a short history length to see if it works better for some branches and some for longer 
#define MEDIUM_HISTORY_LENGTH 12        // Keep a medium history length to see if it works better for some branches
#define ALPHA 0.6                        // Weight of current taken value
	my_update u;
	branch_info bi;
	unsigned int history, short_history, medium_history;    // Add short_history to keep track of short history
	unsigned char tab[1<<TABLE_BITS];
        // My code
        unsigned int counter[1<<TABLE_BITS];    // table that keeps track of if local history is better or if local and global history is
        unsigned int index;             // Index of the last branch in counter
        bool GHvLH[2];   // Initialize table to keep value of local history prediction and global history prediction for last branch
        bool SvL[3];     // Initialize table to keep value of short history prediction or medium or long history prediction
        unsigned int shortOrLong[1<<TABLE_BITS];        // Table that keeps track if short or long history is better
        unsigned int prevTake[1<<TABLE_BITS];           // Table that keeps track of if previous same branch performed better with short (1), medium (2), or long (0)

	my_predictor (void) : history(0) { 
		memset (tab, 0, sizeof (tab));
	}

	branch_update *predict (branch_info & b) {
		bi = b;
                int g, gs, gm, l;       // g = global history estimated index, l = local history estimated index
		unsigned int h; // h = history (use either short or long history)
                if (b.br_flags & BR_CONDITIONAL) {
			/*
                        u.index = 
				  (history << (TABLE_BITS - HISTORY_LENGTH)) 
				^ (b.address & ((1<<TABLE_BITS)-1));
                        */
                        
                        // My code
                        g = history << (TABLE_BITS - HISTORY_LENGTH);
                        gs = short_history << (TABLE_BITS - SHORT_HISTORY_LENGTH);
                        gm = medium_history << (TABLE_BITS - MEDIUM_HISTORY_LENGTH);
                        l = b.address & ((1<<TABLE_BITS)-1);
                        
                        // Saves value of g and gs as history
                        SvL[0] = tab[g^l] >> 2; // Saves long history length prediction
                        SvL[1] = tab[gm^l] >> 2;        // Saves short history length prediction
                        SvL[2] = tab[gs^l] >> 2;        // Saves medium history length prediction
                        
                        index = g^l;
                        /* 
                        if (shortOrLong[index] == 2) {  // If = 1, take short history
                                g = gs;
                        }
                        if (shortOrLong[index] == 1) {  // If = 2, take medium history
                                g = gm;
                        } */
                        int take = (ALPHA)*(shortOrLong[index]) + (1-ALPHA)*(prevTake[index]);
                        if (take >= 2*ALPHA + (1-ALPHA)*1  ) {  // If = 2, take short history
                                g = gs;
                        }
                        if (take < 2*ALPHA + (1-ALPHA)*1  && take >= ALPHA*1+(1-ALPHA)*1) {  // If between 2 and 4, take short history
                                g = gm;
                        }
                        // If counter = 0 (i.e. local history prediction was more accurate), use only global history to determine predicted branch. Otherwise use both global and local
                        if (counter[index] == 0) {                      // If counter at index is 0, only take local history
                                u.index = l;
                        } else {                                        // If counter at index is 1, take global and local
                                u.index = g^l;
                        }
                        GHvLH[0] = tab[g] >> 2;                         // Update the global history value to be tab[g] >> 0
                        GHvLH[1] = tab[g^l] >> 2;                       // Update the local history to tab[g^l] >> 2

			u.direction_prediction (tab[u.index] >> 2);     // shift by 2 to accomodate for increase in taken or not taken size
		} else {
			u.direction_prediction (true);
		}
		u.target_prediction (0);
		return &u;
	}

	void update (branch_update *u, bool taken, unsigned int target) {
		if (bi.br_flags & BR_CONDITIONAL) {
			unsigned char *c = &tab[((my_update*)u)->index];
			if (taken) {
				if (*c < 8) (*c)+=2;     // Keep track of 3 bit counter instead of 2, increase stride size to 2
			} else {
				if (*c > 0) (*c)-=2;
			}
			history <<= 1;
			history |= taken;
			history &= (1<<HISTORY_LENGTH)-1;
		        
                        // My code
                        short_history <<= 1;
                        short_history |= taken;
                        short_history &= (1<<SHORT_HISTORY_LENGTH)-1;

                        medium_history <<= 1;
                        medium_history |= taken;
                        medium_history &= (1<<MEDIUM_HISTORY_LENGTH)-1;

                        if (GHvLH[0] == taken) {        // If the local history is more accurate, next time take just local history
                                counter[index] = 0;
                        }
                        if (GHvLH[1] == taken)  {       // If the combination local and global history is more accurate, take combination and local and global history
                                counter[index] = 1;
                        }
                        
                        prevTake[index] = shortOrLong[index];
                        if (SvL[1] == taken) {          // If medium prediction is accurate, set to take medium
                                shortOrLong[index] = 1;
                        }
                        if (SvL[2] == taken) {          // If short prediction is accurate, set to short
                                 shortOrLong[index] = 2; 
                        }
                        if (SvL[0] == taken) {          // If long prediction is accurate, set to take long
                                 shortOrLong[index] = 0;
                        }
                }
	}
};
