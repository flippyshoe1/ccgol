#include <ncurses.h> // to get the terminal width and height
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>

/* i assume the terminal size wont change during the app runtime
 * i really dont wanna bother with reallocating and just changing all the buffers dynamically :)
 */

// both will be allocated on runtime
char *future = NULL; // state to prepare the new display buffer at
char *display  = NULL; // the state that will be displayed

int cols,rows; // columns and lines

FILE *input = NULL;

/* rule of thumb:
 * '#' means a cell is alive
 * ' ' means a cell is dead
 */

// some globals
#define DEBUG_SPEED 100000

// helpful macro in case something screws up
#define ASSERT(_e, ...) if(!(_e)) { fprintf(stderr, __VA_ARGS__); exit(1); }

void cleanup(){
  // close the file
  if(input) { fclose(input); }
  
  // free up allocated memory
  if(future) { free(future); }
  if(display)  { free(display);  }

  // close ncurses
  if(stdscr && !(isendwin())) { endwin(); }
}

bool isCellAlive(int index){
  // the amount of alive neighbors
  int count = 0;
  
  // get the "coordinates" of the cell
  int y=index/cols;
  int x=index%cols;

  // get the rectangle coordinates that represent the area of neighboring cells
  int sy=(y-1<0)?0:y-1;
  int fy=(y+1>=rows)?rows-1:y+1;
  
  int sx=(x-1<0)?0:x-1;
  int fx=(x+1>=cols)?cols-1:x+1;

  for(int ty=sy;ty<=fy;ty+=1){
    int offset = ty*cols;
    for(int tx=sx;tx<=fx;tx+=1){
      // if its our cell, skip
      if(tx==x&&ty==y) continue;
      // if its not and its alive, add to count
      if(display[offset+tx]=='#') count+=1;
    }
  }

  // if our cell is alive and has 2/3 neighbors, it lives
  if(display[(y*cols)+x]=='#' && (count==2 || count==3)) return true;
  // if our cell is dead and has 3 neighbors, its revived
  if(display[(y*cols)+x]==' ' && count==3) return true;
  // otherwise our cell is either invalid, underpopulated or overpopulated
  // also known as dead
  return false;
}

int main(int argc, char **argv){
  ASSERT(argc>=2, "Insufficient amount of arguements (2 minimum required)\n");

  // clean up when we exit
  atexit(cleanup);

  // try to open argv[1]
  char *f = argv[1];
  ASSERT((input = fopen(f, "r"))!=NULL,
	 "failed to open %s: %s\n",
	 f, strerror(errno));
  
  initscr(); // everything will be done with stdscr

  // incase the terminal window changes sizes
  cols = COLS; 
  rows = LINES;
  int buffsize = cols*rows;

  // allocate space for both buffers (or at least try to)
  future = malloc(buffsize*sizeof(char));
  display = malloc(buffsize*sizeof(char));
  memset(future, ' ', buffsize*sizeof(char));
  memset(display, ' ', buffsize*sizeof(char));
  
  ASSERT(future && display,
	 "[main.c > main] Failed to allocate memory for either buffers: %s\n",
	 strerror(errno));

  /* read a portion of the input file (the size of the buffer) and make it the input buffer
   * act like every cell in the buffer is a bit, flip them accoding to the input file
   */
  size_t count=0; //how many bits in are we
  uint8_t b=0;

  // checking we dont overflow and if there are still any bytes in the file
  while(count<buffsize*sizeof(char)){
    // how many bits are we gonna test, make sure we dont overflow
    (void)fread(&b, sizeof(char), 1, input);
    int r = 8;

    for(int i=0;i<r;i++){
      display[count++] = ( b & (1<<i) ) ? '#' : ' '; // if the bit is set, make the cell alive and vice versa
    }
  }
    
  // keep running forever
  while(true){    
    // clean the screen, display the updated field
    clear();
    printw("%s", display);
    refresh();

    // prepare the updates display and wait
    for(size_t c=0;c<buffsize*sizeof(char);c++)
      future[c] = (isCellAlive(c))?'#':' ';  // update the display
    
    (void)memcpy(display, future, buffsize*sizeof(char));
    
    usleep(DEBUG_SPEED);    
  }
  
  exit(0);
}

