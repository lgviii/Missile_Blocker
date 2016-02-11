//=====================================================================
// File: MissileBlocker.c
//=====================================================================
// Programmer: Lynn Greene VIII
// Project: MissileBlocker.c
//
// Version: 1.1
// Updated: 02-08-2009
// Start Date: 12-11-2007
//
// Description:
//	This program is a simple game where missles drop from the 
//	top of the screen and the user tries to block them from
//	hitting the bottom of the screen using a paddle.
//
// Note: To compile this program using gcc use:
//	gcc -o executable MissileBlocker.c -lpthread -lncurses	
//
//=====================================================================

// include files
#include    <unistd.h>
#include    <stdlib.h>
#include    <string.h>
#include    <curses.h>
#include    <pthread.h>
#include    <time.h>

// define statements
#define TRUE 1
#define FALSE 0
#define LEFT 1
#define RIGHT 0

// structures
typedef    struct
{
    int    colCoord;
    int    rowCoord;
    int    missleInterval;
} MissileInfo;

// funtion prototypes
void    HandleKeystrokes(void);
void    DrawPaddle(int  direction);
void     InitApp(void);
void*   LaunchMissiles(void  *ptr);
void*   DropMissile(void  *ptr);
int     GetFreeXCoord(void);
int     GetMissileSleep(int  lo, int  hi);

// global data
int	g_PaddleDrawStartPosition = 0;
int	g_MissileCounter = 0;
int	g_FucntionWorking = FALSE;
int g_Missle_Cols[200] = {0};
int g_NumOfMissles = 4;
int g_minTime = 250;
int g_maxTime = 500;
int g_MissileLaunchTime = 1500;
pthread_mutex_t    myMutex = PTHREAD_MUTEX_INITIALIZER;

// ==== main ==================================================================
//
// ============================================================================

int  main(int  argc, char  **argv)
{
    // variables
    pthread_t	  launchThread; 

    // initialize the program and screen display
    InitApp();

    // start the missile launches
    pthread_create(&launchThread, NULL, LaunchMissiles, NULL);

    // user attempts to block missiles
    HandleKeystrokes();

    // restore the console's original text colors
    endwin();

    return  0;

}  // end of "main"

//=====================================================================
// Function Name: DrawPaddle 
//=====================================================================
// Description:
//	This function redraws the paddle as the user moves it.
//			
// Input:
//		direction -- int 
//			Description: The paddle's movement direction.			
//
// Output:
//		NONE
//=====================================================================

void    DrawPaddle(int  direction)
{
	// all threads locked
	pthread_mutex_lock(&myMutex);
	
	//variable
	auto	int	counter;

    //  change to original colors
	attron(COLOR_PAIR(2));

	// if the left arrow in entered move paddle left
	if (direction == 1 && g_PaddleDrawStartPosition > 0)
	{
		move(LINES-1, g_PaddleDrawStartPosition);
		for ( counter = 0 ; counter < (COLS/8) ; counter++ )
		{
			addstr(" ");
		}
		move(LINES-1, --g_PaddleDrawStartPosition);
		for ( counter = 0 ; counter < (COLS/8) ; counter++ )
		{
			addstr(" ");
		}
	}
	// if the right arrow in entered move paddle right
	else if (direction == 0 && g_PaddleDrawStartPosition < (COLS - (COLS/8)))
	{
		move(LINES-1, g_PaddleDrawStartPosition);
		for ( counter = 0 ; counter < (COLS/8) ; counter++ )
		{
			addstr(" ");
		}
		move(LINES-1, ++g_PaddleDrawStartPosition);
		for ( counter = 0 ; counter < (COLS/8) ; counter++ )
		{
			addstr(" ");
		}
	}
	// otherwise redraw paddle
	else
	{
		move(LINES-1, g_PaddleDrawStartPosition);
		attron(COLOR_PAIR(3));
		for ( counter = 0 ; counter < (COLS/8) ; counter++ )
		{
			addstr(" ");
		}
		attron(COLOR_PAIR(2));	
		refresh();
	}
	// threads unlocked
	pthread_mutex_unlock(&myMutex);

}  // end of "DrawPaddle"

//=====================================================================
// Function Name: DropMissile 
//=====================================================================
// Description:
//	This function is a thread funtion for a missile. The missile
//	starting point is determined and it drops until it either
//	hits the paddle or the bottom of the screen.
//			
// Input:
//		ptr -- void* 
//			Description: Pointer to a missile.			
//
// Output:
//		NONE
//=====================================================================

void*   DropMissile(void  *ptr)
{
    // variables
	auto	int	counter;

	// typecase ptr to a MissleInfo pointer
    MissileInfo*    thisMissilesInfo = (MissileInfo*)ptr;
    // set the interval
    thisMissilesInfo->missleInterval = GetMissileSleep(g_minTime, g_maxTime);
    //  set the COL with funtion
    thisMissilesInfo->colCoord = GetFreeXCoord();
    thisMissilesInfo->rowCoord = 0;

	// lock the missile's COL from other threads
	g_Missle_Cols[thisMissilesInfo->colCoord] = -1;

    // draw missle until it hits the paddle or goes out of range
    while (thisMissilesInfo->rowCoord < (LINES))
	{
		// mutex locked
		pthread_mutex_lock(&myMutex);
		// one space updated on missile's course
		attron(COLOR_PAIR(3));
		move(thisMissilesInfo->rowCoord++, thisMissilesInfo->colCoord);
		addstr(" ");
		refresh();
		attron(COLOR_PAIR(2));
		// if the missile hits the paddle 
		if ( counter == (LINES - 1) && (thisMissilesInfo->colCoord >= g_PaddleDrawStartPosition && thisMissilesInfo->colCoord <= (g_PaddleDrawStartPosition + (COLS/8))) )
		{
			free(thisMissilesInfo);
			pthread_mutex_unlock(&myMutex);
			break;
		}
		// mutex unlocked
		pthread_mutex_unlock(&myMutex);
		napms(thisMissilesInfo->missleInterval);
	}
	// deduct one from the total number of missiles
    g_MissileCounter--;

	// mutex locked
    pthread_mutex_lock(&myMutex);
    // erase the missile's path
    for ( counter = 0 ; counter < (LINES + 1) ; counter++ )
	{
		if ( counter == (LINES - 1) && (thisMissilesInfo->colCoord >= g_PaddleDrawStartPosition && thisMissilesInfo->colCoord <= (g_PaddleDrawStartPosition + (COLS/8))) )
		{
			free(thisMissilesInfo);
			break;
		}
		else if (counter == LINES)
		{
			flash();
			beep();
		}
		move(counter, thisMissilesInfo->colCoord);
		addstr(" ");
		refresh();
	}
	//mutex unlocked
	pthread_mutex_unlock(&myMutex);
    // reset the COL to available
	g_Missle_Cols[thisMissilesInfo->colCoord] = 0;
}  // end of "DropMissile"

//=====================================================================
// Function Name: GetFreeXCoord
//=====================================================================
// Description:
//	This function returns an available column for a new missile.
//			
// Input:
//		NONE			
//
// Output:
//		randCol -- int
//			Description: Random, non-occupied column.
//=====================================================================

int     GetFreeXCoord(void)
{
    // variables
    auto	int	randCol;
    // mutex locked
    pthread_mutex_lock(&myMutex);
    // keeps looking for a random number between the bounds
    do 
	{
		randCol = (rand() % ( COLS - 1));
	} while ( g_Missle_Cols[randCol] != 0);
	// mutex unlocked
	pthread_mutex_unlock(&myMutex);
    return randCol;
}  // end of "GetFreeXCoord"

//=====================================================================
// Function Name: GetMissileSleep 
//=====================================================================
// Description:
//	This function finds a random missile sleep interval.
//			
// Input:
//		hi -- int 
//			Description: Hi interval boundary.	
//		lo -- int 
//			Description: Low interval boundary.			
//
// Output:
//		randValue -- int
//			Description: A random sleep interval.
//=====================================================================

int     GetMissileSleep(int  lo, int  hi)
{
	// variables
    auto	int	randValue;
    // keeps looking for a random number between the bounds
    do
	{
		randValue = ( rand() % hi);
	} while ( randValue < lo );
	return randValue;
}  // end of "GetMissileSleep"

//=====================================================================
// Function Name: HandleKeystrokes
//=====================================================================
// Description:
//	This function handles the user's input and either quits,
//	moves the paddle or does nothing.
//			
// Input:
//		NONE			
//
// Output:
//		NONE
//=====================================================================

void    HandleKeystrokes(void)
{
	// mutex locked
    auto    int ch;
    do 
    {
        // set default paddle position
		DrawPaddle(-1);
		// start random function
		srand (time(NULL));
        // the user's input is collected
		ch = getch();
		// process the input
		switch (ch) 
		{ 
			case KEY_LEFT: 
				DrawPaddle(1);
				break; 
			case KEY_RIGHT: 
				DrawPaddle(0);
				break;  
			case 'q': 
				// quit message is displayed
				erase();
				attron(COLOR_PAIR(3));
				bkgd(COLOR_PAIR(3));
				move(15, 20); 
				addstr("Quit program.\n"); 
				pthread_mutex_lock(&myMutex);
				refresh();
				sleep(3);
			default:
				break;
		};
		refresh();
	} while (ch != 'q');
}  // end of "HandleKeystrokes"

//=====================================================================
// Function Name: InitApp
//=====================================================================
// Description:
//	This function sets up the initial state of the visual display.
//			
// Input:
//		NONE			
//
// Output:
//		NONE
//=====================================================================

void     InitApp(void)
{   
    // start screen and colors
    initscr();
    pthread_mutex_init ( &myMutex, NULL);
    start_color();
    init_pair(2, COLOR_GREEN, COLOR_BLACK);
    init_pair(3, COLOR_BLACK, COLOR_GREEN);
    init_pair(4, COLOR_MAGENTA, COLOR_CYAN);
    curs_set(0);

    // a quit information message is displayed
    bkgd(COLOR_PAIR(2));
    attron(COLOR_PAIR(4));
    move(LINES/2, 0);
    addstr(" TO QUIT at any time during gameplay, press q ");
    refresh();
    sleep(3);
    erase();

    // screen and input is prepared
    attron(COLOR_PAIR(2));
    bkgd(COLOR_PAIR(2));
    noecho();
    cbreak();    
    move(LINES-1, 0);
    keypad(stdscr, TRUE);
    start_color();
    init_pair(2, COLOR_RED, COLOR_BLACK);
    init_pair(3, COLOR_BLUE, COLOR_GREEN);
    attron(COLOR_PAIR(2));
    bkgd(COLOR_PAIR(2));

	// draw default paddle
    DrawPaddle(-1);
}  // end of "InitApp"

//=====================================================================
// Function Name: LaunchMissiles
//=====================================================================
// Description:
//	This function launches missiles while the maximum number
//	of missiles is not being untilized.
//			
// Input:
//		ptr -- void*
//			Description: ptr required by the thread function.			
//
// Output:
//		NONE
//=====================================================================

void*   LaunchMissiles(void  *ptr)
{
	// variables
	pthread_t	  aMissileThread; 
	MissileInfo* 	currentMissile;	 

	while (1)
	{
		napms((rand() % g_MissileLaunchTime));
		// if there are avilable missiles, one is launched
		if (g_MissileCounter < g_NumOfMissles)
		{
			// memory is allocated
			currentMissile = malloc(sizeof(MissileInfo));
			// missile is created
			pthread_create(&aMissileThread, NULL, DropMissile, currentMissile);
			// number of missiles is incremented
			g_MissileCounter++;
		}	
	}
}  // end of "LaunchMissiles"
