#include <iostream>
#include <fstream>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <csignal>

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <termios.h>

// it would be reading data in non blocking mode, since we changed STDIN behaviour by fcntl.
// must not return char since some keystrokes return multiple bytes into STDIN instead of 1 byte ( such as arrow keys )
int32_t ReadInputFromSTDIN()
{
    int result;
    int32_t c = 0;
    result = read(0,&c,sizeof(c));
    if ( result < 0 )
        return 0;

    return c;
}

bool console_cursor_is_hidden = false;
bool console_is_maximized = false;
uint16_t console_character_width;
uint16_t console_character_height;

void HideConsoleCursor( bool hide )
{
    if( hide )
    {
        if( !console_cursor_is_hidden )
        {
            std::cout << "\x1b[?25l" << std::flush;
            console_cursor_is_hidden = true;
        }
    }

    else
    {
        if( console_cursor_is_hidden )
        {
            std::cout << "\x1b[?25h" << std::flush;
            console_cursor_is_hidden = false;
        }
    }
}

void GetConsoleCharacterSize()
{
    winsize ws;
    int fd;

    fd = open("/dev/tty", O_RDWR);
    if ( fd < 0 || ioctl(fd, TIOCGWINSZ, &ws) < 0 )
    {
        std::cerr << "could not perform ioctl operation.\n";
        return;
    }

    else
    {
        console_character_width = ws.ws_col;
        console_character_height = ws.ws_row;
        close(fd);
    }
}

// on passing false, it restores the console line width and height to it's original form.
void MaximizeWindow( bool maximize )
{
    if( maximize )
    {
        if( !console_is_maximized )
        {
            std::cout << "\x1b[8;43;150t" << std::flush;
            console_is_maximized = true;
        }
    }

    else
    {
        if( console_is_maximized )
        {
            int length = snprintf(NULL,0,"%d;%dt",console_character_height,console_character_width);
            char* escape_sequence_command = new char[length + strlen("\x1b[8;")];
            sprintf(escape_sequence_command,"\x1b[8;%d;%dt",console_character_height,console_character_width);
            
            std::cout << escape_sequence_command << std::flush;
            
            delete[] escape_sequence_command;

            console_is_maximized = false;
        }
    }
}

void ClearScreen()
{
    std::cout << "\x1b[H\x1b[2J\x1b[3J" << std::flush;
}

void Sleep( uint32_t macro_seconds )
{
    usleep(macro_seconds);
}

termios original_terminal_interface, modified_terminal_interface;

void HandleApplicationTermination()
{
    tcsetattr(STDIN_FILENO,TCSANOW,&original_terminal_interface);
    HideConsoleCursor(false);
    MaximizeWindow(false);
    ClearScreen();
    exit(EXIT_SUCCESS);
}

void HandleInterruptSignal( int signal )
{
    #ifndef DEBUG_MODE
        ClearScreen();
        std::cout << "\ninterrupt has been generated!";
        HandleApplicationTermination();
    #else // on debug mode, we use interrupt signal to restore terminal i/o mode, so we can debug.
        HideConsoleCursor(false);
        tcsetattr(STDIN_FILENO,TCSANOW,&original_terminal_interface);
    #endif
}

void InitializeApplication( int argc, char** argv, char** env )
{
    std::signal(SIGINT,HandleInterruptSignal);

    tcgetattr(STDIN_FILENO,&original_terminal_interface);
    GetConsoleCharacterSize();
    MaximizeWindow(true);

    // so that reading from STDIN would be non blocking.
    int stdin_flags = fcntl(STDIN_FILENO,F_GETFL,0);
    fcntl(STDIN_FILENO,F_SETFL,stdin_flags | O_NONBLOCK);

// this should be done since this code messes with the same terminal states that our debugger
// is attached to.
#ifndef DEBUG_MODE
    tcgetattr(STDIN_FILENO,&modified_terminal_interface);
    modified_terminal_interface.c_lflag &= ~ICANON; // so that input is sent to STDIN by character, not by line.
    modified_terminal_interface.c_lflag &= ~ECHO; // so that input sent, is not seen.
    modified_terminal_interface.c_cc[VMIN] = 1; // sending input to STDIN by 1 character at a time.
    modified_terminal_interface.c_cc[VTIME] = 0; // do not have timing intervals between sending characters to STDIN.
    modified_terminal_interface.c_lflag |= ISIG;
    tcsetattr(STDIN_FILENO,TCSANOW,&modified_terminal_interface);
    HideConsoleCursor(true);
#endif

}

// ascii values for keystrokes
#define KEY_NONE                            0

#define KEY_ENTER                           10
#define KEY_ESCAPE                          27
#define KEY_SPACE                           32
#define KEY_BACKSPACE                       127

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

#define KEY_0                               48
#define KEY_1                               49
#define KEY_2                               50

// in case of arrow keys,3 characters are inserted into STDIN when we press them, when stored inside a 32 bit int, they have these values.
#define KEY_UP                              4283163
#define KEY_LEFT                            4479771
#define KEY_RIGHT                           4414235
#define KEY_DOWN                            4348699

#define APPLIACTION_STATE_MAIN_MENU         0
#define APPLICATION_STATE_ENTER_NAME        1
#define APPLICATION_STATE_ENTER_DIFFICULTY  2
#define APPLICATION_STATE_SNAKE_GAME         3
#define APPLICATION_STATE_SCOREBOARD        4

#define MENU_STATUS_NEW_GAME                0
#define MENU_STATUS_SCOREBOARD              1
#define MENU_STATUS_EXIT                    2

uint8_t menu_status = MENU_STATUS_NEW_GAME;
uint8_t application_status = APPLICATION_STATE_SCOREBOARD;
uint8_t word_entered_count = 0;
uint8_t game_difficulty;
const uint8_t max_allowed_name_length = 20;
char current_user_name[max_allowed_name_length + 1];
int16_t current_user_score = 0;

void ClearUserName()
{
    std::memset(current_user_name,0,max_allowed_name_length);
    word_entered_count = 0;
}

void PrintMainMenu()
{
    ClearScreen();

    std::cout << "\t\t\t\t\t\t\t\twelcome to the snake game.\n"
              << "\t\t\t\t\t\t   please choose the desired option from the menu below.\n"
              << "\t\t\t\t\t\tuse W And S or Arrow keys Up and Down for menu navigation.\n"
              << "\t\t\t\t\t\t\t\t\t" << ((menu_status == MENU_STATUS_NEW_GAME)? "* " : " ") << "new game\n"
              << "\t\t\t\t\t\t\t\t\t" << ((menu_status == MENU_STATUS_SCOREBOARD)? "* " : " ") << "scores\n"
              << "\t\t\t\t\t\t\t\t\t" << ((menu_status == MENU_STATUS_EXIT)? "* " : " ") << "exit game\n"
              << std::flush;
}

#define GAME_DIFFICULTY_NOT_DEFINED         0
#define GAME_DIFFICULTY_EASY                1
#define GAME_DIFFICULTY_NORMAL              2
#define GAME_DIFFICULTY_HARD                3

#define GAME_STATUS_NOT_INITIALIZED         0
#define GAME_STATUS_CAN_BEGIN               1
#define GAME_STATUS_WON                     2
#define GAME_STATUS_ONGOING                 3
#define GAME_STATUS_LOST                    4

#define SNAKE_DIRECTION_UP                  0
#define SNAKE_DIRECTION_LEFT                1
#define SNAKE_DIRECTION_DOWN                2
#define SNAKE_DIRECTION_RIGHT               3

struct GameRecord
{
    GameRecord()
    {
        std::memset(m_player_name,0,max_allowed_name_length);
        m_player_score = 0;
    }

    GameRecord( const GameRecord& other )
    {
        *this = other;
    }

    GameRecord& operator=( const GameRecord& other )
    {
        if( this == &other )
            return *this;

        std::memcpy(m_player_name,other.m_player_name,std::strlen(other.m_player_name));
        m_player_score = other.m_player_score;

        return *this;
    }

    GameRecord( const char* player_name, uint16_t player_score )
    {
        auto count = std::strlen(player_name);
        std::memcpy(m_player_name,player_name,count);
        m_player_score = player_score;
    }

    char m_player_name[20];
    uint16_t m_player_score;
};

struct GameRecordNode
{
    GameRecordNode()
    {
        next = nullptr;
    }

    GameRecordNode( const char* player_name, uint16_t player_score )
    {
        m_record = GameRecord(player_name,player_score);
        next = nullptr;
    }

    GameRecordNode( const GameRecord& record ) : m_record(record), next(nullptr) {}

    GameRecord m_record;
    GameRecordNode* next;
};

struct GameRecordQueue
{
    GameRecordQueue() : m_count(0), head(nullptr) {}

    GameRecordQueue( const GameRecordQueue& other ) : m_count(other.m_count), head(other.head) {}

    ~GameRecordQueue()
    {
        delete head;
    }

    bool operator!() const { return bool(*this); }

    operator bool() const { return m_count != 0; }

    int m_count;
    GameRecordNode* head;
};

GameRecordQueue ReadRecordsFromFile()
{
    std::ifstream reader("scores.txt");
    if( !reader )
    {
        std::cerr << "could not access the file for reading!";
        return {};
    }

    GameRecordQueue records;
    GameRecordNode* record_node;
    record_node = records.head;

    char player_name[max_allowed_name_length];
    uint16_t player_score = 0;
    std::memset(player_name,0,max_allowed_name_length);

    while( !reader.eof() )
    {
        reader >> player_name >> player_score; 
        record_node = new GameRecordNode(player_name,player_score);
        record_node = record_node->next;
    }
    
    //record_node->next = nullptr;

    return records;
}

void WriteRecordsToFile( const GameRecordQueue& records )
{
    std::ofstream writer("scores.txt",std::ios::out);
    if( !writer )
    {
        std::cerr << "could not access the file for writing!";
        return;
    }

    const GameRecordNode* record_node = records.head;

    while( record_node )
    {
        writer << record_node->m_record.m_player_name << ' ' << record_node->m_record.m_player_score << '\n';
        record_node = record_node->next;
    }
}

void SubmitPlayerScore()
{
    GameRecordQueue record_queue = ReadRecordsFromFile();
    if( record_queue )
    {
        GameRecordNode* record_node = record_queue.head;
        while( record_node )
        {
            if( current_user_score > record_node->m_record.m_player_score  )
            {
                std::memcpy(record_node->m_record.m_player_name,current_user_name,strlen(current_user_name));
                record_node->m_record.m_player_score = current_user_score;
                break;
            }

            else
                record_node = record_node->next;
        }

        record_node = new GameRecordNode(current_user_name,current_user_score);
    }

    else
    {
        record_queue.head = new GameRecordNode(current_user_name,current_user_score);
        record_queue.m_count++;
    }

    WriteRecordsToFile(record_queue);
}

void DrawScoreBoard()
{
    GameRecordQueue records = ReadRecordsFromFile();
    if( records )
        std::cout << "no scores have been submitted yet!" << std::endl;

    else
    {
        GameRecordNode* record_node = records.head;
        while( record_node )
        {
            std::cout << record_node->m_record.m_player_name << '\t' << record_node->m_record.m_player_score << std::endl;
            record_node = record_node->next;
        }

        std::cout << "Press Escape to return." << std::endl;
    }
}

struct SnakePart
{
    SnakePart() : m_pos_x(0), m_pos_y(0), m_direction(0) {}
    SnakePart( uint8_t pos_x, uint8_t pos_y, uint8_t direction ) : m_pos_x(pos_x), m_pos_y(pos_y),
                                                                   m_direction(direction) {}

    SnakePart( const SnakePart& other )
    {
        *this = other;
    }

    SnakePart& operator=( const SnakePart& other )
    {
        if( this == &other )
            return *this;

        m_pos_x = other.m_pos_x;
        m_pos_y = other.m_pos_y;
        m_direction = other.m_direction;

        return *this;
    }

    uint8_t m_pos_x;
    uint8_t m_pos_y;
    uint8_t m_direction;
};

struct SnakeBody
{
    SnakeBody()
    {
        next = nullptr;
    }

    SnakeBody( const SnakePart& part )
    {
        m_part = part;
        next = nullptr;
    }

    SnakePart m_part;
    SnakeBody* next;
};

struct Snake
{
    Snake() : m_head(nullptr), m_length(0) {}

    SnakeBody* m_head;
    uint8_t m_length;
};

uint8_t game_size_x;
uint8_t game_size_y;
int8_t snake_direction;
uint8_t food_x;
uint8_t food_y;
int8_t game_status;

Snake* snake = nullptr;

char* snake_field = nullptr;

void DisplayGameOnScreen()
{
    ClearScreen();

    std::cout << "\t\tgame_score:" << current_user_score << "\t\t" << std::endl;

    for( int i = 0; i < game_size_x; ++i )
    {
        for( int j = 0; j < game_size_y; ++j )
        {
            if( i == snake->m_head->m_part.m_pos_x && j == snake->m_head->m_part.m_pos_y )
                std::cout << "x";

            else
                std::cout << snake_field[j + game_size_y * i];
        }

        std::cout << std::endl;
    }
}

void HandleGameDifficulty()
{
    if( game_difficulty == GAME_DIFFICULTY_EASY )
    {
        game_size_x = 8;
        game_size_y = 8;
    }

    else if( game_difficulty == GAME_DIFFICULTY_NORMAL )
    {
        game_size_x = 12;
        game_size_y = 12;
    }

    else if( game_difficulty == GAME_DIFFICULTY_HARD )
    {
        game_size_x = 16;
        game_size_y = 16;
    }
}

void GenerateFood()
{
    do
    {
        food_x = 1 + rand() % game_size_x - 1;
        food_y = 1 + rand() % game_size_y - 1;

     // so that food doesn't spawn on snake body or head or corners.
    } while( 
             (food_x == snake->m_head->m_part.m_pos_x && food_y == snake->m_head->m_part.m_pos_y) ||
             snake_field[food_x + game_size_y * food_y] == '#'
           );

    snake_field[food_x + game_size_y * food_y] = '@';
}

bool InitializeSnakeGame()
{
    if( game_size_x <= 3 || game_size_y <= 3 )
    {
        std::cerr << "snake game width or length are too low, please select higher length."
                  << std::endl;
        return false;
    }

    std::srand(time(NULL));

    if( snake )
        delete snake;

    snake = new Snake();

    snake->m_head = new SnakeBody();
    snake->m_length++;

    if( snake_field )
        delete snake_field;

    snake_field = new char[game_size_x * game_size_y];
    if( !snake_field )
    {
        std::cerr << "could not allocate enough space for game! quiting." << std::endl;
        HandleApplicationTermination();
    }

    for( int i = 0; i < game_size_x; ++i )
    {
        for( int j = 0; j < game_size_y; ++j )
        {
            if( i == 0 || i == game_size_x - 1 )
                snake_field[ i * game_size_y + j ] = '#';
                
            else
            {
                if( ( j == 0 ) || ( j == game_size_y - 1 ) )
                    snake_field[ i * game_size_y + j ] = '#';
            }
        }
    }

    game_status = GAME_STATUS_CAN_BEGIN;

    return true;
}

void StartSnakeGame()
{
    game_status = GAME_STATUS_ONGOING;

    current_user_score = 0;

    // clear game inner field.
    for( int i = 1; i < game_size_x - 1; ++i )
    {
        for( int j = 1; j < game_size_y - 1; ++j )
            snake_field[i + j * game_size_y] = ' ';
    }

    GenerateFood();

    do
    {
        snake->m_head->m_part.m_pos_x = 1 + std::rand() % ( game_size_x - 1 );
        snake->m_head->m_part.m_pos_y = 1 + std::rand() % ( game_size_y - 1 );
    } while
      (
        snake->m_head->m_part.m_pos_x != food_x || snake->m_head->m_part.m_pos_y != food_y ||
        snake_field[snake->m_head->m_part.m_pos_x + snake->m_head->m_part.m_pos_y * game_size_y] == '#'
      ); // so that snake doesn't spawn at food or borders.
      
}

void HandleSnakeGameLogic()
{
    bool lost = false;
    switch( snake_direction )
    {
        case SNAKE_DIRECTION_UP:

        break;

        case SNAKE_DIRECTION_LEFT:
        break;

        case SNAKE_DIRECTION_DOWN:
        break;

        case SNAKE_DIRECTION_RIGHT:
        break;
    }
/*
    if( lost )
    {
        SubmitPlayerScore();
        game_status = GAME_STATUS_LOST;
    }

    else
    {
        SubmitPlayerScore();
        game_status = GAME_STATUS_WON;
    }
*/
}

void HandleApplicationUpdate()
{
    switch( application_status )
    {
        int32_t user_key_input;

        case APPLIACTION_STATE_MAIN_MENU:
            HideConsoleCursor(true);
            
            user_key_input = ReadInputFromSTDIN();

            switch( user_key_input )
            {
                case KEY_NONE:
                break;

                case KEY_W_UPPERCASE:
                case KEY_W_LOWERCASE:
                case KEY_UP:
                    if( menu_status == MENU_STATUS_NEW_GAME )
                        menu_status = MENU_STATUS_EXIT;
                    else
                        --menu_status;
                break;

                case KEY_S_UPPERCASE:
                case KEY_S_LOWERCASE:
                case KEY_DOWN:
                    if( menu_status == MENU_STATUS_EXIT )
                        menu_status = MENU_STATUS_NEW_GAME;
                    else
                        ++menu_status;
                break;

                case KEY_ENTER:
                    if( menu_status == MENU_STATUS_NEW_GAME )
                        #ifndef DEBUG_MODE
                            application_status = APPLICATION_STATE_ENTER_NAME;
                        #else
                            application_status = APPLICATION_STATE_ENTER_DIFFICULTY;
                        #endif

                    else if( menu_status == MENU_STATUS_SCOREBOARD )
                        application_status = APPLICATION_STATE_SCOREBOARD;

                    else if( menu_status == MENU_STATUS_EXIT )
                        HandleApplicationTermination();
                break;
            }

            PrintMainMenu();
        break;

        case APPLICATION_STATE_ENTER_NAME:
            ClearScreen();
            HideConsoleCursor(false);

            std::cout << "please enter your name (max 20 characters):" << current_user_name << std::flush;

            while( ( user_key_input = ReadInputFromSTDIN() ) == KEY_NONE )
                Sleep(1000);

            if( user_key_input == KEY_ESCAPE )
            {
                application_status = APPLIACTION_STATE_MAIN_MENU;
                ClearUserName();
            }
            
            else if( user_key_input == KEY_ENTER )
            {
                if( word_entered_count != 0 )
                    application_status = APPLICATION_STATE_ENTER_DIFFICULTY;
            }

            else if( user_key_input == KEY_BACKSPACE )
            {
                if( word_entered_count != 0 )
                {
                    current_user_name[word_entered_count - 1] = '\0';
                    --word_entered_count;
                }
            }

            else if( ( user_key_input >= KEY_A_UPPERCASE && user_key_input <= KEY_Z_UPPERCASE ) ||
                     ( user_key_input >= KEY_A_LOWERCASE && user_key_input <= KEY_Z_LOWERCASE ) )
            {
                if( word_entered_count >= max_allowed_name_length )
                        break;

                current_user_name[word_entered_count] = user_key_input;
                ++word_entered_count;
            }
        break;

        case APPLICATION_STATE_ENTER_DIFFICULTY:
            ClearScreen();
            HideConsoleCursor(true);

            std::cout << "please enter difficulty( 0 for easy, 1 for normal, 2 for hard ):" << std::flush;
            game_difficulty = GAME_DIFFICULTY_NOT_DEFINED;

            while( game_difficulty == GAME_DIFFICULTY_NOT_DEFINED )
            {
                user_key_input = ReadInputFromSTDIN();
                if( user_key_input == KEY_NONE )
                    continue;

                else if( user_key_input == KEY_ESCAPE )
                {
                    application_status = APPLIACTION_STATE_MAIN_MENU;
                    ClearUserName();
                }

                else if( user_key_input == KEY_0 )
                    game_difficulty = GAME_DIFFICULTY_EASY;

                else if( user_key_input == KEY_1 )
                    game_difficulty = GAME_DIFFICULTY_NORMAL;

                else if( user_key_input == KEY_2 )
                    game_difficulty = GAME_DIFFICULTY_HARD;
            }

            if( game_difficulty != GAME_DIFFICULTY_NOT_DEFINED )
            {
                HandleGameDifficulty();

                application_status = APPLICATION_STATE_SNAKE_GAME;
            }
        break;

        case APPLICATION_STATE_SNAKE_GAME:
            switch( game_status )
            {
                case GAME_STATUS_NOT_INITIALIZED:
                    InitializeSnakeGame();
                break;

                case GAME_STATUS_CAN_BEGIN:
                    StartSnakeGame();
                break;

                case GAME_STATUS_ONGOING:
                    user_key_input = ReadInputFromSTDIN();

                    switch(user_key_input)
                    {
                        case KEY_W_LOWERCASE:
                        case KEY_W_UPPERCASE:
                        case KEY_UP:
                            snake_direction = SNAKE_DIRECTION_UP;
                        break;

                        case KEY_A_LOWERCASE:
                        case KEY_A_UPPERCASE:
                        case KEY_LEFT:
                            snake_direction = SNAKE_DIRECTION_LEFT;
                        break;

                        case KEY_S_LOWERCASE:
                        case KEY_S_UPPERCASE:
                        case KEY_DOWN:
                            snake_direction = SNAKE_DIRECTION_DOWN;
                        break;

                        case KEY_D_LOWERCASE:
                        case KEY_D_UPPERCASE:
                        case KEY_RIGHT:
                            snake_direction = SNAKE_DIRECTION_RIGHT;
                        break;

                        case KEY_ESCAPE:
                            application_status = APPLIACTION_STATE_MAIN_MENU;
                            game_status = GAME_STATUS_NOT_INITIALIZED;
                        break;
                    }

                    // if user hasn't pressed escape while playing the game
                    if( application_status == APPLICATION_STATE_SNAKE_GAME )
                    {
                        HandleSnakeGameLogic();
                        DisplayGameOnScreen();
                    }

                    else
                        ClearUserName();
                break;

                case GAME_STATUS_LOST:
                    ClearScreen();
                    std::cout << "you lost the game with the score of:" << current_user_score << '\n'
                              << "press Enter to play again, Escape to return to main menu and space "
                              << "to change difficulty." << std::flush;

                    while( ( user_key_input = ReadInputFromSTDIN() ) == KEY_NONE )
                        Sleep(1000);

                    if( user_key_input == KEY_ENTER )
                        StartSnakeGame();

                    else if( user_key_input == KEY_ESCAPE )
                    {
                        application_status = APPLIACTION_STATE_MAIN_MENU;
                        ClearUserName();
                    }

                    else if( user_key_input == KEY_SPACE )
                        application_status = APPLICATION_STATE_ENTER_DIFFICULTY;
                break;

                case GAME_STATUS_WON:
                    ClearScreen();
                    std::cout << "congratulations!you won the game with the score of:" << current_user_score << '\n'
                              << "Press Escape to go to main menu. if you want, you can check your score "
                              << "by selecting scores option from main menu." << std::endl;

                    while( ( user_key_input = ReadInputFromSTDIN() ) == KEY_NONE )
                        Sleep(1000);

                    if( user_key_input == KEY_ESCAPE )
                    {
                        application_status = APPLIACTION_STATE_MAIN_MENU;
                        ClearUserName();
                    }
                break;
            }

                Sleep(50000);
        break;

        case APPLICATION_STATE_SCOREBOARD:
            ClearScreen();
            HideConsoleCursor(true);

            DrawScoreBoard();

            user_key_input = ReadInputFromSTDIN();

            switch( user_key_input )
            {
                case KEY_ESCAPE:
                    application_status = APPLIACTION_STATE_MAIN_MENU;
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
        Sleep(50000);
    }

    return 0;
}