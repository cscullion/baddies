#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <assert.h>
#include <time.h>

#define DEBUG 0

#define MAX_X 25
#define MAX_Y 25
#define MAX_BADDIES (MAX_X * MAX_Y / 2)
#define MAX_TRAPS (MAX_X * MAX_Y / 2)
#define LEVEL_ADD_BADDIES 2
#define LEVEL_ADD_TRAPS 2
#define TYPE_PLAYER 1
#define TYPE_BADDIE 2
#define TYPE_TRAP 3

typedef struct {
	int x;
	int y;
} POSITION;

typedef struct {
	char view;
	unsigned char type;
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
int num_zaps;
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
	printf("+-");
	for (y = 0; y < MAX_Y; y++) {
		printf("--");
	}
	printf("+\n");

	// print board
	for (y = 0; y < MAX_Y; y++) {
		printf("| ");
		for (x = 0; x < MAX_X; x++) {
			// if (board[x][y] == player.view) printf("\033[7m");
			printf("%c", board[x][y]);
			// if (board[x][y] == player.view) printf("\033[0m");
			printf(" ");
		}
		printf("|\n");
	}

	// print bottom border
	printf("+-");
	for (y = 0; y < MAX_Y; y++) {
		printf("--");
	}
	printf("+\n");
	printf("u k i  level: %d  baddies: %d\n", level, baddies_count());
	printf("h   l  q=quit\n");
	printf("n j m  t=teleport [%d] z=zap [%d]", num_teleports, num_zaps);
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

void players_init(int n_baddies, int n_traps, int n_teleports, int n_zaps) {
	int randx, randy;

	// player
	player.view = '@';
	player.pos.x = (int)(MAX_X / 2);
	player.pos.y = (int)(MAX_Y / 2);
	player.type = TYPE_PLAYER;

	// baddies
	num_baddies = 0;
	while (num_baddies < n_baddies) {
		baddies[num_baddies].dead = 0;
		baddies[num_baddies].view = 'B';
		baddies[num_baddies].type = TYPE_BADDIE;
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
		traps[num_traps].type = TYPE_TRAP;
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

	//zaps
	num_zaps = n_zaps;
}

void trap_new(int x, int y) {
	num_traps++;
	traps[num_traps - 1].view = '%';
	traps[num_traps - 1].type = TYPE_TRAP;
	traps[num_traps - 1].pos.x = x;
	traps[num_traps - 1].pos.y = y;
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

void player_zap(void) {
	int i;

	for (i = 0; i < num_baddies; i++) {
		if (!baddies[i].dead) {
			if (abs(baddies[i].pos.x - player.pos.x) <= 1) {
				if (abs(baddies[i].pos.y - player.pos.y) <= 1) {
					baddies[i].dead = 1;
				}
			}
		}
	}
}

unsigned char player_move(char key) {
	unsigned char handled = 1;

	switch (key) {
		case ' ': break;  // valid move == stand still
		case 'h': if (player.pos.x > 0) player.pos.x--; break;
		case 'j': if (player.pos.y < MAX_Y - 1) player.pos.y++; break;
		case 'k': if (player.pos.y > 0) player.pos.y--; break;
		case 'l': if (player.pos.x < MAX_X - 1) player.pos.x++; break;
		case 'u': handled = player_move('h'); handled = player_move('k'); break;
		case 'i': handled = player_move('l'); handled = player_move('k'); break;
		case 'n': handled = player_move('h'); handled = player_move('j'); break;
		case 'm': handled = player_move('l'); handled = player_move('j'); break;
		case 't': 
			if (num_teleports > 0) {
				player_teleport();
				num_teleports--;
			}
			else {
				handled = 0;
			}
			break;
		case 'z':
			if (num_zaps > 0) {
				player_zap();
				num_zaps--;
			}
			else {
				handled = 0;
			}
			break;
		default: handled = 0; break;
	}
	return(handled);
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
	unsigned char move_baddies = 0;

	// seed the random number generator
	srand(time(NULL));

	level = 1;
	board_clear();
	board_draw();
	players_init(3, 4, 2, 2);
	board_update();
	board_draw();
	while (1) {
		input_key = (char)getch();
		if (input_key == 'q') return(1);
		move_baddies = player_move(input_key);
		if (move_baddies) {
			baddies_move();
			collision_detect();
		}
		board_update();
		board_draw();

		if (victory()) {
			// level up
			level++;
			board_clear();
			board_draw();
			players_init(num_baddies + LEVEL_ADD_BADDIES, num_traps + LEVEL_ADD_TRAPS, level, 2);
			board_update();
			board_draw();
		}
	}
	return(0);
}
