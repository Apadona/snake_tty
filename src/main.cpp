#include <iostream>
#include <cstdlib>
#include <ctime>
#include <cstring>

#include <signal.h>

#include <unistd.h>
#include <fcntl.h>
#include <termios.h>

bool console_cursor_is_hidden = false;

void HideConsoleCursor( bool hide )
{
    if( hide )
    {
        if( !console_cursor_is_hidden )
        {
            std::cout << "\e[?25l" << std::flush;
            console_cursor_is_hidden = true;
        }
    }

    else
    {
        if( console_cursor_is_hidden )
        {
            std::cout << "\e[?25h" << std::flush;
            console_cursor_is_hidden = false;
        }
    }
}

void ClearScreen()
{
    system("clear");
}

termios original_terminal_interface, modified_terminal_interface;

void HandleApplicationExit()
{
    tcsetattr(STDIN_FILENO,TCSANOW,&original_terminal_interface);
    HideConsoleCursor(false);
    ClearScreen();
    exit(EXIT_SUCCESS);
}

void HandleInterruptSignal( int signal )
{
    ClearScreen();
    std::cout << "\ninterrupt has been generated!";
    HandleApplicationExit();
}

const uint8_t max_allowed_name_length = 20;
char user_name[max_allowed_name_length + 1];

void InitializeApplication( int argc, char** argv, char** env )
{
    #ifdef DEBUG_MODE
        std::cout << "application is starting in debug mode!\n" << std::flush;
    #endif

    std::memset(user_name,'\0',max_allowed_name_length);

    signal(SIGINT,HandleInterruptSignal);

    tcgetattr(STDIN_FILENO,&original_terminal_interface);

    // so that reading from STDIN would be non blocking.
    int stdin_flags = fcntl(STDIN_FILENO,F_GETFL,0);
    fcntl(STDIN_FILENO,F_SETFL,stdin_flags | O_NONBLOCK);

// this should be done since this code messes with the same terminal states that our debugger
// is attached to.
#ifndef DEBUG_MODE
    tcgetattr(STDIN_FILENO,&modified_terminal_interface);
    modified_terminal_interface.c_lflag &= ~ICANON; // so that input is sent to STDIN by character, not by line.
    modified_terminal_interface.c_lflag &= ~ECHO; // so that input sent, is not seen.
    modified_terminal_interface.c_lflag &= ~ECHOE;
    modified_terminal_interface.c_cc[VMIN] = 1; // sending input to STDIN by 1 character at a time.
    modified_terminal_interface.c_cc[VTIME] = 0; // do not have timing intervals between sending characters to STDIN.
    tcsetattr(STDIN_FILENO,TCSANOW,&modified_terminal_interface);
    HideConsoleCursor(true);
#endif

}

// ascii values for keystrokes
#define KEY_NONE                            0
#define KEY_BACKSPACE                       127
#define KEY_ENTER                           10
#define KEY_ESCAPE                          27
#define KEY_UP                              72
#define KEY_DOWN                            80
#define KEY_LEFT                            75
#define KEY_RIGHT                           77
#define KEY_W_UPPERCASE                     87
#define KEY_W_LOWERCASE                     119
#define KEY_A_UPPERCASE                     65
#define KEY_A_LOWERCASE                     97
#define KEY_S_UPPERCASE                     83
#define KEY_S_LOWERCASE                     115
#define KEY_D_UPPERCASE                     68
#define KEY_D_LOWERCASE                     100
#define KEY_Z_UPPERCASE                     90
#define KEY_Z_LOWERCASE                     122

#define APPLICATION_STATE_OPTIONS           0
#define APPLICATION_STATE_ENTER_NAME        1
#define APPLICATION_STATE_SNAKEGAME         2
#define APPLICATION_STATE_SCOREBOARD        3

#define MENU_STATUS_NEW_GAME                0
#define MENU_STATUS_SCOREBOARD              1
#define MENU_STATUS_EXIT                    2

int8_t menu_status = MENU_STATUS_NEW_GAME;

void PrintMenu()
{
    ClearScreen();

    std::cout << "\t\t\t\t\t\t\t\twelcome to the snake game.\n"
              << "\t\t\t\t\t\t   please choose the desired option from the menu below.\n"
              << "\t\t\t\t\t\t\t\t\t" << ((menu_status == MENU_STATUS_NEW_GAME)? "* " : " ") << "new game\n"
              << "\t\t\t\t\t\t\t\t\t" << ((menu_status == MENU_STATUS_SCOREBOARD)? "* " : " ") << "scores\n"
              << "\t\t\t\t\t\t\t\t\t" << ((menu_status == MENU_STATUS_EXIT)? "* " : " ") << "exit game\n";
}

// it would be reading data in non blocking mode, since we changed STDIN behaviour by fcntl.
unsigned char ReadInputFromSTDIN()
{
    int result;
    unsigned char c;
    if ( ( result = read(0,&c,sizeof(c)) ) < 0 )
        return 0;

    else
        return c;
}

char snake_game[145] =
"############"
"#          #"
"#          #"
"#          #"
"#          #"
"#          #"
"#          #"
"#          #"
"#          #"
"#          #"
"#          #"
"############";

void StartSnakeGame()
{

}

void ResetSnakeGame()
{
    
}

void HandleSnakeGame()
{
    
}

struct GameRecords
{
    char* name;
    uint16_t score;
};

void DrawScoreBoard()
{

}

void ReadRecordsFromFile( const char* directory, GameRecords& records )
{

}

void WriteRecordsToFile( const char* directory, GameRecords& records )
{

}

uint8_t application_status = 0;
uint8_t word_entered_count = 0;

void HandleApplicationUpdate()
{
    char _input = ReadInputFromSTDIN();

    switch( application_status )
    {
        case APPLICATION_STATE_OPTIONS:
            HideConsoleCursor(true);
            
            switch( _input )
            {
                case KEY_NONE:
                break;

                case KEY_W_UPPERCASE:
                case KEY_W_LOWERCASE:
                    if( menu_status == MENU_STATUS_NEW_GAME )
                        menu_status = MENU_STATUS_EXIT;
                    else
                        --menu_status;
                break;

                case KEY_S_UPPERCASE:
                case KEY_S_LOWERCASE:
                    if( menu_status == MENU_STATUS_EXIT )
                        menu_status = MENU_STATUS_NEW_GAME;
                    else
                        ++menu_status;
                break;

                case KEY_ENTER:
                    if( menu_status == MENU_STATUS_NEW_GAME )
                        application_status = APPLICATION_STATE_ENTER_NAME;

                    else if( menu_status == MENU_STATUS_SCOREBOARD )
                        application_status = APPLICATION_STATE_SCOREBOARD;

                    else if( menu_status == MENU_STATUS_EXIT )
                        HandleApplicationExit();
                break;
            }

            PrintMenu();
        break;

        case APPLICATION_STATE_ENTER_NAME:
            ClearScreen();
            HideConsoleCursor(false);

            std::cout << "please enter your name (max 20 characters):" << user_name << std::flush;

            while( ( _input = ReadInputFromSTDIN() ) == 0 )
                usleep(1000);

            if( _input == KEY_NONE )
                break;

            if( _input == KEY_ESCAPE )
            {
                application_status = 0;
                std::memset(user_name,'\0',max_allowed_name_length);
            }
            
            else if( _input == KEY_ENTER )
            {
                if( word_entered_count != 0 )
                    application_status = 2;
            }

            else if( _input == KEY_BACKSPACE )
            {
                if( word_entered_count != 0 )
                {
                    user_name[word_entered_count - 1] = '\0';
                    --word_entered_count;
                }
            }

            else if( ( _input >= KEY_A_UPPERCASE && _input <= KEY_Z_UPPERCASE ) ||
                     ( _input >= KEY_A_LOWERCASE && _input <= KEY_Z_LOWERCASE ) )
            {
                if( word_entered_count >= 20 )
                        break;

                user_name[word_entered_count] = _input;
                ++word_entered_count;
            }
        break;

        case APPLICATION_STATE_SNAKEGAME:
            ClearScreen();
            HideConsoleCursor(true);

            switch( _input )
            {
                case KEY_ESCAPE:
                    application_status = 0;
                break;
            }

        case APPLICATION_STATE_SCOREBOARD:
            ClearScreen();
            HideConsoleCursor(true);

            switch( _input )
            {
                case KEY_ESCAPE:
                    application_status = 0;
                break;
            }
        break;
    }
}

int main( int argc, char** argv, char** env )
{
    InitializeApplication(argc,argv,env);

    while( true )
    {
        HandleApplicationUpdate();
        usleep(50000);
    }

    return 0;
}