#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <assert.h>
#include <time.h>

#define DEBUG 0

#define MAX_X 24
#define MAX_Y 24
#define MAX_BADDIES (MAX_X * MAX_Y / 2)
#define MAX_TRAPS (MAX_X * MAX_Y / 2)
#define LEVEL_ADD_BADDIES 2
#define LEVEL_ADD_TRAPS 2

typedef struct {
	int x;
	int y;
} POSITION;

typedef struct {
	char view;
	unsigned char baddie;
	unsigned char trap;
	POSITION pos;
	unsigned char dead;
} PLAYER;

char board[MAX_X][MAX_Y];
PLAYER player;
PLAYER baddies[MAX_BADDIES];
PLAYER traps[MAX_TRAPS + (MAX_BADDIES / 2)];
int num_baddies;
int num_traps;
int num_teleports;
int level;

int getch(void) {
	int c=0;

	struct termios org_opts, new_opts;
	int res=0;
	  //-----  store old settings -----------
	res=tcgetattr(STDIN_FILENO, &org_opts);
	assert(res==0);
	  //---- set new terminal parms --------
	memcpy(&new_opts, &org_opts, sizeof(new_opts));
	new_opts.c_lflag &= ~(ICANON | ECHO | ECHOE | ECHOK | ECHONL | ECHOPRT | ECHOKE | ICRNL);
	tcsetattr(STDIN_FILENO, TCSANOW, &new_opts);
	c=getchar();
	  //------  restore old settings ---------
	res=tcsetattr(STDIN_FILENO, TCSANOW, &org_opts);
	assert(res==0);
	return(c);
}

void board_clear(void) {
	memset(board, '.', sizeof(board));
}

unsigned char victory(void) {
	int b = 0;

	for (b = 0; b < num_baddies; b++) {
		if (!baddies[b].dead) {
			// still at least one baddie left
			return 0;
		}
	}
	return 1;
}

int baddies_count(void) {
	int result = 0;
	int i = 0;

	for (i = 0; i < num_baddies; i++) {
		if (!baddies[i].dead) {
			result++;
		}
	}
	return result;
}

void board_draw(void) {
	int x, y;

	// clear screen and home cursor
	if (!DEBUG) printf("\033[2J");   // clear
	if (!DEBUG) printf("\033[H");    // home

	// print top border
	printf("\n");
	printf("+");
	for (y = 0; y < MAX_Y; y++) {
		printf("-");
	}
	printf("+\n");

	// print board
	for (y = 0; y < MAX_Y; y++) {
		printf("|");
		for (x = 0; x < MAX_X; x++) {
			printf("%c", board[x][y]);
		}
		printf("|\n");
	}

	// print bottom border
	printf("+");
	for (y = 0; y < MAX_Y; y++) {
		printf("-");
	}
	printf("+\n");
	printf("       level: %d\n", level);
	printf("u k i  left: %d\n", baddies_count());
	printf("h   l  q=quit\n");
	printf("n j m  t=teleport [%d]\n", num_teleports);
	if (player.dead) printf("***** DEAD *****\n");
	if (victory()) printf("***** YOU WON *****\n");
}

void board_update(void) {
	int i = 0;

	board_clear();

	// player
	board[player.pos.x][player.pos.y] = player.view;

	// traps
	for (i = 0; i < num_traps; i++) {
		board[traps[i].pos.x][traps[i].pos.y] = traps[i].view;
	}

	// baddies
	for (i = 0; i < num_baddies; i++) {
		if (!baddies[i].dead) {
			board[baddies[i].pos.x][baddies[i].pos.y] = baddies[i].view;
		}
	}
}

unsigned char occupied(int x, int y) {
	unsigned char result = 0;
	int i = 0;

	if ((player.pos.x == x) && (player.pos.y == y)) {
		//occupied by player
		return 1;
	}

	for (i = 0; i < num_baddies; i++) {
		if ((baddies[i].pos.x == x) && (baddies[i].pos.y == y)) {
			//occupied by a baddie
			return 1;
		}
	}

	for (i = 0; i < num_traps; i++) {
		if ((traps[i].pos.x == x) && (traps[i].pos.y == y)) {
			//occupied by a traps
			return 1;
		}
	}
	
	return result;
}

void players_init(int n_baddies, int n_traps, int n_teleports) {
	int randx, randy;

	// player
	player.view = '@';
	player.baddie = 0;
	player.pos.x = (int)(MAX_X / 2);
	player.pos.y = (int)(MAX_Y / 2);

	// baddies
	num_baddies = 0;
	while (num_baddies < n_baddies) {
		baddies[num_baddies].dead = 0;
		baddies[num_baddies].view = 'B';
		baddies[num_baddies].baddie = 1;
		baddies[num_baddies].trap = 0;
		randx = rand() % MAX_X; randy = rand() % MAX_Y;
		while (occupied(randx, randy)) {
			printf("generating random player positions x=%d, y=%d\r", randx, randy);
			randx = rand() % MAX_X; randy = rand() % MAX_Y;
		}
		baddies[num_baddies].pos.x = randx;
		baddies[num_baddies].pos.y = randy;
		num_baddies++;
	}

	//traps
	num_traps = 0;
	while(num_traps < n_traps) {
		traps[num_traps].view = '%';
		traps[num_traps].baddie = 0;
		traps[num_traps].trap = 1;
		randx = rand() % MAX_X; randy = rand() % MAX_Y;
		while (occupied(randx, randy)) {
			printf("generating random player positions x=%d, y=%d\r", randx, randy);
			randx = rand() % MAX_X; randy = rand() % MAX_Y;
		}
		traps[num_traps].pos.x = randx;
		traps[num_traps].pos.y = randy;
		num_traps++;
	}

	//teleports
	num_teleports = n_teleports;
}

void trap_new(int x, int y) {
	num_traps++;
	traps[num_traps - 1].view = '%';
	traps[num_traps - 1].baddie = 0;
	traps[num_traps - 1].trap = 1;
	traps[num_traps - 1].pos.x = x;
	traps[num_traps - 1].pos.y = y;
}

void player_move(char dir) {
	switch (dir) {
		case 'h': if (player.pos.x > 0) player.pos.x--; break;
		case 'j': if (player.pos.y < MAX_Y - 1) player.pos.y++; break;
		case 'k': if (player.pos.y > 0) player.pos.y--; break;
		case 'l': if (player.pos.x < MAX_X - 1) player.pos.x++; break;
		default: break;
	}
}

void player_teleport(void) {
	int randx, randy;

	randx = rand() % MAX_X;
	randy = rand() % MAX_Y;

	while (occupied(randx, randy)) {
		randx = rand() % MAX_X;
		randy = rand() % MAX_Y;
	}
	player.pos.x = randx;
	player.pos.y = randy;
}

void baddies_move(void) {
	int b = num_baddies;

	for (b = 0; b < num_baddies; b++) {
		if (!baddies[b].dead) {
			if (baddies[b].pos.x > player.pos.x) baddies[b].pos.x--;
			if (baddies[b].pos.x < player.pos.x) baddies[b].pos.x++;
			if (baddies[b].pos.y > player.pos.y) baddies[b].pos.y--;
			if (baddies[b].pos.y < player.pos.y) baddies[b].pos.y++;
		}
	}
}

void collision_detect(void) {
	int b = 0;
	int b2 = 0;

	// check for player collisions with baddies
	for (b = 0; b < num_baddies; b++) {
		if (!baddies[b].dead) {
			if (player.pos.x == baddies[b].pos.x) {
				if (player.pos.y == baddies[b].pos.y) {
					// collision - player dead
					player.dead = 1;
					return;
				}
			}
		}
	}

	// check for player collisions with traps
	for (b = 0; b < num_traps; b++) {
		if (player.pos.x == traps[b].pos.x) {
			if (player.pos.y == traps[b].pos.y) {
				// collision - player dead
				player.dead = 1;
				return;
			}
		}
	}

	// check for baddies collisions
	for (b = 0; b < num_baddies; b++) {
		if (!baddies[b].dead) {
			// check other baddies
			for (b2 = b+1; b2 < num_baddies; b2++) {
				if (!baddies[b2].dead) {
					if (baddies[b].pos.x == baddies[b2].pos.x) {
						if (baddies[b].pos.y == baddies[b2].pos.y) {
							// collision - baddies dead, trap created
							baddies[b].dead = 1;
							baddies[b2].dead = 1;
							// this should be more elegant: don't create a trap if one already exists here
							trap_new(baddies[b].pos.x, baddies[b].pos.y);
							break;
						}
					}
				}
			}
			if (!baddies[b].dead) {
				// check traps
				for (b2 = 0; b2 < num_traps; b2++) {
					if (baddies[b].pos.x == traps[b2].pos.x) {
						if (baddies[b].pos.y == traps[b2].pos.y) {
							// collision with trap
							baddies[b].dead = 1;
							break;
						}
					}
				}
			}
		}
	}
}

int main(int argc, char**argv) {
	char input_key = '\0';

	// seed the random number generator
	srand(time(NULL));

	level = 1;
	board_clear();
	board_draw();
	players_init(3, 4, 2);
	board_update();
	board_draw();
	while (1) {
		input_key = (char)getch();
		switch (input_key) {
			case 'q': return(1); break;
			case 'h':
			case 'j':
			case 'k':
			case 'l':
			case ' ':
				player_move(input_key);
				break;
			case 'u':
				player_move('h'); player_move('k');
				break;
			case 'i':
				player_move('l'); player_move('k');
				break;
			case 'n':
				player_move('h'); player_move('j');
				break;
			case 'm':
				player_move('l'); player_move('j');
				break;
			case 't':
				if (num_teleports > 0) {
					player_teleport();
					num_teleports--;
				}
				break;
			default:
				break;
		}
		baddies_move();
		collision_detect();
		board_update();
		board_draw();

		if (victory()) {
			// level up
			level++;
			board_clear();
			board_draw();
			players_init(num_baddies + LEVEL_ADD_BADDIES, num_traps + LEVEL_ADD_TRAPS, level);
			board_update();
			board_draw();
		}
	}
	return(0);
}
