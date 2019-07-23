// F ile
// U ser
// I nterface
#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/dir.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <signal.h>
// Ncurses
#include <ncurses.h>

// Curses windows used in this application:
// The window used to display the address bar
#define ADDRESS_BAR_WINDOW_WIDTH COLS 
#define ADDRESS_BAR_WINDOW_HEIGHT 3
#define ADDRESS_BAR_WINDOW_STARTX 0 
#define ADDRESS_BAR_WINDOW_STARTY 0
WINDOW* gAddressBarWindow = NULL;
// The window used to display the directory entries
#define DIRECTORY_ENTRIES_WINDOW_WIDTH COLS
#define DIRECTORY_ENTRIES_WINDOW_HEIGHT (LINES - 2)
#define DIRECTORY_ENTRIES_WINDOW_STARTX 0
#define DIRECTORY_ENTRIES_WINDOW_STARTY 2 
WINDOW* gDirectoryEntriesWindow = NULL;

// Directory entries are displayed in a grid format.
// The grid is defined by how many cells each entry should occupy horizontally
#define DIRECTORY_ENTRY_TITLE_WIDTH 23
// The number of cells to be used for horzontal padding on both sides
#define DIRECTORY_ENTRY_TITLE_PADDING 1

// We define a cursor which may hover over any one of the 
// currently displayed cursor entries.
bool gIsCursorEnabled = false;
unsigned int gDirectoryEntryGridWidth;
unsigned int gDirectoryEntryGridHeight;
unsigned int gCursorX = 0;
unsigned int gCursorY = 0;

// Array of pointers to directory entries contained by the current working dir
// This variable is set by the system call scandir. Items are to be free'd
struct dirent** gCurrentDirectoryEntries = NULL;
unsigned int gItemsInCurrentDirectory = 0;  // The array length

// Performs clean up operations before calling exit with the specified status
// code.
void FuiExit( int status );
// Given a valid path name open the directory and display its formatted
// contents
void ShowDirectory( const char* pathname );
// Apply the fui border theme to a window
void FuiBorder( WINDOW* win );
// Draw the directory entries to the corresponding window
void DrawDirectoryEntries(); 
// Find the correct handler for user interaction through the keyboard
// Returns false if the program should exit
bool HandleInput( int input );
// Move the cursor based on key input
void MoveCursor( int input );
// Draw a directory path to the address bar window
// After calling this function, doupdate must be called to display the content.
void DrawAddressBar( const char* address );
void DetermineDirectoryEntryGrid(); 

void OnTerminalResize() {
  DetermineDirectoryEntryGrid();
  ShowDirectory(".");
}

int main( int argc, char** argv ) {
  char* directory = ".";

  if( argc == 2 ) {
    directory = argv[1];
  } else if ( argc > 2 ) {
    printf("Useage: %s <directory>\n", argv[0]);
    FuiExit(1);
  }

  // Init ncurses
  initscr();
  // Allow the inputting of arrow keys and other special characters
  keypad(stdscr, TRUE);
  // Get input immediatly from the user (dont wait for newline)
  cbreak();
  // Dont print out the characters the user types
  noecho();
  // Dont display the terminal cursor
  curs_set(0);
  // When the terminal is resized we want to 
  signal(SIGWINCH, OnTerminalResize);
  // Give the screen a pretty border
  FuiBorder(stdscr);
  // Initilize the windows used in this program
  gAddressBarWindow = newwin(ADDRESS_BAR_WINDOW_HEIGHT, 
                             ADDRESS_BAR_WINDOW_WIDTH,
                             ADDRESS_BAR_WINDOW_STARTY,
			     ADDRESS_BAR_WINDOW_STARTX);

  gDirectoryEntriesWindow = newwin(DIRECTORY_ENTRIES_WINDOW_HEIGHT,
                                   DIRECTORY_ENTRIES_WINDOW_WIDTH,
  				   DIRECTORY_ENTRIES_WINDOW_STARTY,
  				   DIRECTORY_ENTRIES_WINDOW_STARTX);
  // Refresh needs to be called after windows have been created in order to use
  // wrefresh to draw individual windows.
  refresh();
  // Add the same border to the address bar and directory entry windows
  FuiBorder(gAddressBarWindow);
  FuiBorder(gDirectoryEntriesWindow);
  // Determine the scale of the directory entries grid in respect to the
  // current size of the terminal.
  DetermineDirectoryEntryGrid();
  // Display the contents of the directory
  ShowDirectory(directory);
  
  // Provide the input handler with user input
  while( HandleInput( getch() ) );

  FuiExit(0);
}

void ClearDirectoryEntries() {
  for( unsigned int entryIndex = 0; entryIndex < gItemsInCurrentDirectory;
       entryIndex++ ) {
    free( gCurrentDirectoryEntries[entryIndex] );
  }
}

void FuiBorder( WINDOW* win ) {
  wborder(win, '|', '|', '-', '-', '*', '*', '*', '*');
}

// Given a valid path name open the directory and display its formatted
// contents
void ShowDirectory( const char* pathname ) {
  // Resolve the given path into a full path
  char* fullPathName = realpath(pathname, NULL);
  
  // Update the address bar
  // Clear the existing contents of this window
  werase(gAddressBarWindow);
  // After erasing the contents of this window, the border must be reapplied.
  FuiBorder(gAddressBarWindow);
  // Write the current directory path to the address bar window
  DrawAddressBar(fullPathName);

  // Set the current directory to be the directory we are displaying
  // This ensures that subsequent uses of relative paths are correct
  chdir(fullPathName);
  
  // Free the contents of the previous scandir, if they exist.
  if( gCurrentDirectoryEntries != NULL ) {
    ClearDirectoryEntries(); 
  }
  
  // Scan the current directory using no filter and sorting alphabetically
  gItemsInCurrentDirectory = scandir(fullPathName, &gCurrentDirectoryEntries,
                                     NULL, alphasort);
  
  // Reset the grid cursor
  gCursorX = 0;
  gCursorY = 0;
  gIsCursorEnabled = false;

  // Display the contents of the directory, ensuring that the previous
  // contents have been cleared. After clearing, the border must bee reapplied.
  werase(gDirectoryEntriesWindow);
  FuiBorder(gDirectoryEntriesWindow);
  DrawDirectoryEntries();
  
  // Display the changes that have been made to the screen
  doupdate();
}

// Calculates various values needed to arrange directory entries in a grid.
// This function should be called every time the terminal resizes.
void DetermineDirectoryEntryGrid() {
  // Determine the number of entries which can be displayed on a single line
  gDirectoryEntryGridWidth = (DIRECTORY_ENTRIES_WINDOW_WIDTH - 2)
    / ( DIRECTORY_ENTRY_TITLE_WIDTH + (2*DIRECTORY_ENTRY_TITLE_PADDING));
  // Determine the number of lines of entries which can be displayed in
  // the window
  gDirectoryEntryGridHeight = (DIRECTORY_ENTRIES_WINDOW_HEIGHT - 2) / 2;
}

// Draw a directory path to the address bar window
// After calling this function, doupdate must be called to display the content.
void DrawAddressBar( const char* address ) {
  static char* sAddressBarPrompt = "Directory listing of: ";
  // Calculate where to begin writing the string such that it is centered in
  // the address bar.
  unsigned int addressStartX = (ADDRESS_BAR_WINDOW_WIDTH - strlen(address)
		                - strlen(sAddressBarPrompt) ) / 2;
  // Write the pathname
  mvwprintw(gAddressBarWindow, 1, addressStartX, "%s %s", sAddressBarPrompt,
            address);
  // Commit the changed window buffer
  wnoutrefresh(gAddressBarWindow);
}

// Draw the directory entries to the corresponding window
// Refreshes the directory entries output without updating the screen.
// After calling this function, doupdate must be called to display the content.
void DrawDirectoryEntries() {
  for( unsigned int entryIndex = 0; 
       entryIndex < gItemsInCurrentDirectory;
       entryIndex++ ) {
    
    // Calculate the grid coordinates corresponding to the entry index
    unsigned int xCoord = entryIndex % gDirectoryEntryGridWidth;
    unsigned int yCoord = entryIndex / gDirectoryEntryGridWidth;

    unsigned int xpos = xCoord * (DIRECTORY_ENTRY_TITLE_WIDTH
                        + 2*DIRECTORY_ENTRY_TITLE_PADDING)
                        + DIRECTORY_ENTRY_TITLE_PADDING + 1;
    // If the directory entry being drawn is under the cursor
    // Apply some style to it
    bool currentEntryIsHovered = gIsCursorEnabled && gCursorX == xCoord &&
                                 gCursorY == yCoord; 
    if( currentEntryIsHovered ) {
      // Write the following in bold
      wattr_on( gDirectoryEntriesWindow, WA_STANDOUT, NULL );
    }

    mvwprintw(gDirectoryEntriesWindow, yCoord * 2 + 1, xpos, "%s",
              gCurrentDirectoryEntries[entryIndex]->d_name);

    if( currentEntryIsHovered ) {
      wattr_off( gDirectoryEntriesWindow, WA_STANDOUT, NULL );
    }
  }
  wnoutrefresh(gDirectoryEntriesWindow);
}

// Find the correct handler for user interaction through the keyboard
// Returns false if the program should exit
bool HandleInput( int input ) {
  // Switch on the character, as opposed to it's attributes which are stored
  // in the chtype structure.
  switch( input ) {
    case KEY_UP:
    case KEY_LEFT:
    case KEY_DOWN:
    case KEY_RIGHT:
      MoveCursor(input);
      break;
    case '\n':
      {
        // Get the index into the directory entries array from the grid coords
        unsigned int directoryEntryIndex = (gCursorY * gDirectoryEntryGridWidth)
                                         + gCursorX;
        ShowDirectory(gCurrentDirectoryEntries[directoryEntryIndex]->d_name);
        break;
      }
    // Ignore keyboard interactions if they dont fall into one of the above
    default:
      break;
  }
  return true;
}

// Move the cursor based on key input
void MoveCursor( int input ) {
  gIsCursorEnabled = true;
  switch( input ) {
    case KEY_UP:
      gCursorY -= gCursorY == 0 ? 0 : 1;
      break;
    case KEY_LEFT:
      gCursorX -= gCursorX == 0 ? 0 : 1;
      break;
    case KEY_DOWN:
      // If the cursor were to move down, would it still be in range?
      if( (gCursorY + 1) * gDirectoryEntryGridWidth + gCursorX 
          < gItemsInCurrentDirectory ) {
        // If so we can safely move the cursor down
        gCursorY += 1;
      }
      // Otherwise we cannot move the cursor down and we ignore this keypress.
      break;
    case KEY_RIGHT:
      // If the cursor were to move right, would it still be in range?
      if( gCursorY * gDirectoryEntryGridWidth + gCursorX + 1 
          < gItemsInCurrentDirectory && gCursorX + 1 < gDirectoryEntryGridWidth) {
        gCursorX += 1;
      }
      break;
  }
  DrawDirectoryEntries();
  // Update the display
  doupdate();
}

// Highlight the specified 
void FuiExit( int status ) {
  // Remove the windows used within the display
  if( gAddressBarWindow != NULL ) {
    delwin(gAddressBarWindow);
  }
  if( gDirectoryEntriesWindow != NULL ) {
    delwin(gDirectoryEntriesWindow);
  }
  // Reset the terminal after entering curses mode
  endwin();
  exit( status );
}
