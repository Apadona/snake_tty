#include <iostream>
#include <fstream>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <csignal>
#include <ctime>

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <termios.h>

// it would be reading data in non blocking mode, since we change STDIN behaviour by fcntl.
// must not return char since some keystrokes return multiple bytes into STDIN instead of 1 byte ( such as arrow keys )
int32_t ReadKeyStrokeFromSTDIN()
{
    int32_t key = 0;
    
    int result = read(0,&key,sizeof(key));
    if ( result < 0 )
        return 0;

    return key;
}

bool console_cursor_is_hidden = false;
bool console_is_maximized = false;
uint8_t console_character_width;
uint8_t console_character_height;

void ClearConsoleScreen()
{
    std::cout << "\x1b[H\x1b[2J\x1b[3J" << std::flush;
}

void SetConsoleCursorPosition( uint8_t pos_x, uint8_t pos_y )
{
    char ascci_escape_command[11] = {0};
    sprintf(ascci_escape_command,"\x1b[%d;%df",pos_y,pos_x);
    std::cout << ascci_escape_command << std::flush;
}

void SetConsoleSize( uint8_t size_x, uint8_t size_y )
{
    char ascci_escape_command[15] = {0};
    sprintf(ascci_escape_command,"\x1b[8;%d;%dt",size_y,size_x);
    std::cout << ascci_escape_command << std::flush;
}

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
        std::cerr << "could not perform ioctl operation to get terminal/console character size.\n";
        return;
    }

    console_character_width = ws.ws_col;
    console_character_height = ws.ws_row;
    close(fd);
}

// on passing false, it restores the console line width and height to it's original form.
void MaximizeWindow( bool maximize )
{
    if( maximize )
    {
        if( !console_is_maximized )
        {
            SetConsoleSize(150,43);
            console_is_maximized = true;
        }
    }

    else
    {
        if( console_is_maximized )
        {
            SetConsoleSize(console_character_width,console_character_height);
            
            console_is_maximized = false;
        }
    }
}

termios original_terminal_interface, modified_terminal_interface;
bool application_recieved_interrupt = false;
bool app_is_running = false;

void Sleep( uint32_t macro_seconds )
{
    usleep(macro_seconds);
}

void SleepIfNotInterrupted( uint32_t macro_seconds )
{
    if( app_is_running )
        Sleep(macro_seconds);
}

void HandleApplicationTermination()
{
    tcsetattr(STDIN_FILENO,TCSANOW,&original_terminal_interface);
    HideConsoleCursor(false);
    MaximizeWindow(false);
    ClearConsoleScreen();
    app_is_running = false;
    //exit(EXIT_SUCCESS);
}

void HandleInterruptSignal( int signal )
{
    //#ifndef DEBUG_MODE
        ClearConsoleScreen();
        std::cout << "\ninterrupt has been generated!";
        HandleApplicationTermination();
    //#else // on debug mode, we use interrupt signal to restore terminal i/o mode, so we can debug.
    //    HideConsoleCursor(false);
    //    tcsetattr(STDIN_FILENO,TCSANOW,&original_terminal_interface);
    //#endif
}

void InitializeApplication( int argc, char** argv, char** env )
{
    std::signal(SIGINT,HandleInterruptSignal);
    std::atexit(HandleApplicationTermination);

    tcgetattr(STDIN_FILENO,&original_terminal_interface);
    GetConsoleCharacterSize();
    MaximizeWindow(true);

    // so that reading from STDIN would be non blocking.
    int stdin_flags = fcntl(STDIN_FILENO,F_GETFL,0);
    fcntl(STDIN_FILENO,F_SETFL,stdin_flags | O_NONBLOCK);

// this should be done since this code messes with the same terminal states that our debugger
// is attached to.
//#ifndef DEBUG_MODE
    tcgetattr(STDIN_FILENO,&modified_terminal_interface);
    modified_terminal_interface.c_lflag &= ~ICANON; // so that input is sent to STDIN by character, not by line.
    modified_terminal_interface.c_lflag &= ~ECHO; // so that input sent, is not seen.
    modified_terminal_interface.c_cc[VMIN] = 1; // sending input to STDIN by 1 character at a time.
    modified_terminal_interface.c_cc[VTIME] = 0; // do not have timing intervals between sending characters to STDIN.
    modified_terminal_interface.c_lflag |= ISIG;
    tcsetattr(STDIN_FILENO,TCSANOW,&modified_terminal_interface);
    HideConsoleCursor(true);
//#endif
    
    app_is_running = true;
}

bool ApplicationShouldClose()
{
    if( app_is_running )
        return true;
    else
        return false;
}

// ascii values for keystrokes
#define KEY_NONE                                0

#define KEY_ENTER                               10
#define KEY_ESCAPE                              27
#define KEY_SPACE                               32
#define KEY_BACKSPACE                           127

#define KEY_W_UPPERCASE                         87
#define KEY_W_LOWERCASE                         119
#define KEY_A_UPPERCASE                         65
#define KEY_A_LOWERCASE                         97
#define KEY_S_UPPERCASE                         83
#define KEY_S_LOWERCASE                         115
#define KEY_D_UPPERCASE                         68
#define KEY_D_LOWERCASE                         100
#define KEY_Z_UPPERCASE                         90
#define KEY_Z_LOWERCASE                         122

#define KEY_0                                   48
#define KEY_1                                   49
#define KEY_2                                   50

// in case of arrow keys,3 characters are inserted into STDIN when we press them, when stored
// inside a 32 bit int, they have these values.
#define KEY_UP                                  4283163
#define KEY_LEFT                                4479771
#define KEY_RIGHT                               4414235
#define KEY_DOWN                                4348699

#define APPLICATION_STATE_MAIN_MENU             0
#define APPLICATION_STATE_ENTER_NAME            1
#define APPLICATION_STATE_ENTER_DIFFICULTY      2
#define APPLICATION_STATE_SNAKE_GAME            3
#define APPLICATION_STATE_OPTIONS               4
#define APPLICATION_STATE_SCOREBOARD            5

#define MENU_STATUS_NEW_GAME                    0
#define MENU_STATUS_OPTIONS                     1
#define MENU_STATUS_SCOREBOARD                  2
#define MENU_STATUS_EXIT                        3

#define OPTION_STATUS_ALLOW_SNAKE_CUT_ITSELF    0
#define OPTION_STATUS_ALLOW_SNAKE_PASS_BORDERS  1
#define OPTION_STATUS_BACK                      2

bool options_read = false;
bool options_changed = false;
bool can_read_options = true;
uint8_t option_menu_choise = OPTION_STATUS_ALLOW_SNAKE_CUT_ITSELF;
bool snake_can_cut_itself = false;
bool snake_can_pass_border = false;

uint8_t menu_status = MENU_STATUS_NEW_GAME;
uint8_t application_status = APPLICATION_STATE_MAIN_MENU;
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

void DisplayMainMenu()
{
    ClearConsoleScreen();

    SetConsoleCursorPosition(64,1);
    std::cout << "welcome to the snake game.\n";
    SetConsoleCursorPosition(51,2);
    std::cout << "please choose the desired option from the menu below.\n";
    SetConsoleCursorPosition(44,3);
    std::cout << "use 'W' And 'S' or Arrow keys 'Up' and 'Down' for menu navigation.\n";
    SetConsoleCursorPosition(69,4);
    std::cout << ((menu_status == MENU_STATUS_NEW_GAME)? "* " : " ") << "new game\n";
    SetConsoleCursorPosition(69,5);
    std::cout << ((menu_status == MENU_STATUS_OPTIONS)? "* " : " ") << "options\n";
    SetConsoleCursorPosition(69,6);
    std::cout << ((menu_status == MENU_STATUS_SCOREBOARD)? "* " : " ") << "scores\n";
    SetConsoleCursorPosition(69,7);
    std::cout << ((menu_status == MENU_STATUS_EXIT)? "* " : " ") << "exit game\n" << std::flush;
}

void DisplayOptions()
{
    ClearConsoleScreen();

    std::cout << "options:\n( Press arrow keys and W or S for navigation, press space to "
              << "change option, press Enter to save changes and Escape to get back at main menu )\n"
              << ((option_menu_choise == OPTION_STATUS_ALLOW_SNAKE_CUT_ITSELF)? "* " : " ")
              << "ALLOW_SNAKE_CUT_ITSELF\t\t" << (( snake_can_cut_itself )? "ON" : "OFF") << '\n'
              << ((option_menu_choise == OPTION_STATUS_ALLOW_SNAKE_PASS_BORDERS)? "* " : " ")
              << "ALLOW_SNAKE_PASS_BORDERS\t\t" << (( snake_can_pass_border )? "ON" : "OFF") << '\n'
              << ((option_menu_choise == OPTION_STATUS_BACK)? "* " : " " )
              << "Back" << std::endl;
}

#define GAME_DIFFICULTY_NOT_DEFINED             0
#define GAME_DIFFICULTY_EASY                    1
#define GAME_DIFFICULTY_NORMAL                  2
#define GAME_DIFFICULTY_HARD                    3

#define GAME_STATUS_NOT_INITIALIZED             0
#define GAME_STATUS_CAN_BEGIN                   1
#define GAME_STATUS_WON                         2
#define GAME_STATUS_ONGOING                     3
#define GAME_STATUS_LOST                        4

#define SNAKE_DIRECTION_NONE                    0
#define SNAKE_DIRECTION_UP                      1
#define SNAKE_DIRECTION_LEFT                    2
#define SNAKE_DIRECTION_DOWN                    3
#define SNAKE_DIRECTION_RIGHT                   4

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
        std::memset(m_player_name,0,max_allowed_name_length);
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

    bool operator!() const { return bool(*this) == false; }

    operator bool() const { return m_count != 0; }

    int m_count;
    GameRecordNode* head;
};

#define FILE_RECORDS_LIMIT 10
GameRecordQueue records;
bool should_read_from_file = true;

void SortRecordsByDescendingOrder()
{
    // doing it for head seperately.
    if( records.head->m_record.m_player_score < records.head->next->m_record.m_player_score )
    {
        GameRecordNode* temp = records.head;
        records.head = records.head->next;
        records.head->next = temp;
    }

    // we've done the compression of head by hand, now we start from the next.
    GameRecordNode* first = records.head->next;
    GameRecordNode* second = first;
    
    while( second )
    {
        while( first->next )
        {
            if( first->m_record.m_player_score < first->next->m_record.m_player_score )
            {
                GameRecordNode* temp = first;
                first = first->next;
                first->next = temp;
            }

            first = first->next;
        }
        
        second = second->next;
    }
}

void ReadOptionsFromFile()
{
    if( options_read )
        return;

        options_read = true;

    std::ifstream reader("settings.ini", std::ios::in);
    if( !reader )
        return;

    if( can_read_options )
    {
        can_read_options = true;

        char buffer[30];

        for( int i = 0; i < 2; ++i )
        {    
            //std::memset(buffer,0,30);
            std::string buffer;
            reader >> buffer;

            if( reader.fail() || reader.bad() )
            {
                std::cerr << "settings.ini is modified or corrupted!";
                return;
            }

            if( buffer == "snake_can_pass_border" )
            {
                char _operator;
                reader >> _operator;
                if( _operator == '=' )
                    reader >> snake_can_pass_border;

                buffer.clear();
            }

            else if( buffer == "snake_can_cut_itself" )
            {
                char _operator;
                reader >> _operator;
                if( _operator == '=' )
                    reader >> snake_can_cut_itself;

                buffer.clear();
            }
        }

        reader.close();
    }
}

void WriteOptionsToFile()
{
    std::ofstream writer("settings.ini", std::ios::out);
    if( !writer )
    {
        ClearConsoleScreen();
        std::cerr << "Error:could not access settings.ini to write settings. press any key to continue.\n";
        while( ReadKeyStrokeFromSTDIN() == KEY_NONE )
            SleepIfNotInterrupted(3000);

        return;
    }

    writer << "snake_can_cut_itself" << ' ' << '=' << ' ' << snake_can_cut_itself << '\n'
           << "snake_can_pass_border" << ' ' << '=' << ' ' << snake_can_pass_border << std::flush;

    writer.close();

    can_read_options = false;
}

void ReadRecordsFromFile()
{
    if( !should_read_from_file )
        return;

    std::ifstream reader("scores.txt");
    if( !reader )
    {
        std::cerr << "could not access the file for reading!";
        return;
    }

    GameRecordNode** current_record_node = &records.head;

    char player_name[max_allowed_name_length];
    uint16_t player_score = 0;
    std::memset(player_name,0,max_allowed_name_length);
    bool once = false;

    while( !reader.eof() )
    {
        reader >> player_name;
        if( reader.fail() || reader.bad() )
            break;
        
        reader >> player_score;

        *current_record_node = new GameRecordNode(player_name,player_score);
        ++records.m_count;

        if( records.m_count >= FILE_RECORDS_LIMIT )
            break;

        GameRecordNode* node = *current_record_node;
        current_record_node = &node->next;
    }

    //SortRecordsByDescendingOrder();

    should_read_from_file = false;
}

void WriteRecordsToFile()
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
        writer << record_node->m_record.m_player_name << ' ' << record_node->m_record.m_player_score;
        if( record_node->next )
            writer << '\n'; // we don't want to have a new line at the end of last record, it causes some problems with reading them.
        record_node = record_node->next;
    }

    should_read_from_file = true;
}

void SubmitPlayerScore()
{
    ReadRecordsFromFile();

    if( records )
    {
        bool submitted_node = false;

        if( current_user_score > records.head->m_record.m_player_score )
        {
            GameRecordNode* temp = records.head;
            records.head = new GameRecordNode(current_user_name,current_user_score);
            records.head->next = temp;
            submitted_node = true;
            ++records.m_count;
        }

        GameRecordNode* record_node = records.head;
        GameRecordNode* prev = records.head;
        GameRecordNode* current;

        while( record_node && !submitted_node )
        {
            if( current_user_score > record_node->m_record.m_player_score )
            {
                current = new GameRecordNode(current_user_name,current_user_score);
                prev->next = current;
                current->next = record_node;

                ++records.m_count;
                
                submitted_node = true;
                break;
            }

            prev = record_node;
            record_node = record_node->next;
        }

        if( submitted_node && records.m_count > FILE_RECORDS_LIMIT )
        {
            while( record_node->next )
            {
                prev = record_node;
                record_node = record_node->next;
            }

            delete record_node; // delete the last record since we have more than 10 records and they are sorted.
            prev->next = nullptr;
        }

        else if( !submitted_node && records.m_count < FILE_RECORDS_LIMIT )
            prev->next = new GameRecordNode(current_user_name,current_user_score);
    }

    else
    {
        records.head = new GameRecordNode(current_user_name,current_user_score);
        records.m_count++;
    }

    WriteRecordsToFile();
}

void DrawScoreBoard()
{
    ReadRecordsFromFile();
    if( !records )
        std::cout << "no scores have been submitted yet!" << std::endl;

    else
    {
        GameRecordNode* record_node = records.head;
        while( record_node )
        {
            std::cout << record_node->m_record.m_player_name << '\t' << record_node->m_record.m_player_score << std::endl;
            record_node = record_node->next;
        }
    }

    std::cout << "Press Escape to return." << std::endl;
}

struct SnakeBody
{
    SnakeBody() : m_pos_x(0), m_pos_y(0), m_direction(SNAKE_DIRECTION_NONE) {}
    SnakeBody( uint8_t pos_x, uint8_t pos_y, uint8_t direction ) : m_pos_x(pos_x), m_pos_y(pos_y),
                                                                   m_direction(direction) {}

    SnakeBody( const SnakeBody& other )
    {
        *this = other;
    }

    SnakeBody& operator=( const SnakeBody& other )
    {
        if( this == &other )
            return *this;

        m_pos_x = other.m_pos_x;
        m_pos_y = other.m_pos_y;
        m_direction = other.m_direction;

        return *this;
    }

    uint16_t m_pos_x;
    uint16_t m_pos_y;
    uint16_t m_direction;
};

struct SnakePart
{
    SnakePart()
    {
        next = nullptr;
    }

    SnakePart( const SnakeBody& part )
    {
        m_body = part;
        next = nullptr;
    }

    SnakeBody m_body;
    SnakePart* next;
};

struct Snake
{
    Snake() : m_head(nullptr), m_length(0) {}

    SnakePart* m_head;
    uint8_t m_length;
};

std::time_t current_user_time = 0;
std::clock_t begin_time = 0;
std::uint8_t game_size_x;
std::uint8_t game_size_y;
std::int8_t snake_direction_to_move = SNAKE_DIRECTION_NONE;
std::uint8_t food_x;
std::uint8_t food_y;
std::int8_t game_status;
float game_speed;
char game_difficulty_string[7];

Snake* snake = nullptr;

char* snake_field = nullptr;

struct ElapsedTime
{
    ElapsedTime( time_t from_time_point )
    {
        time_t time_elapsed = time(NULL) - from_time_point;

        if( time_elapsed >= 3600 )
        {
            hours = time_elapsed / 3600;
            minutes = time_elapsed - hours * 3600;
            seconds = time_elapsed - ( hours * 3600 + minutes * 60 );
        }

        else if( time_elapsed >= 60 && time_elapsed < 3600 )
        {
            hours = 0;
            minutes = time_elapsed / 60;
            seconds = time_elapsed - minutes * 60;
        }

        else
        {
            hours = minutes = 0;
            seconds = time_elapsed;
        }
    }

    int seconds = 0;
    int minutes = 0;
    int hours = 0;
};

void DisplayGameOnScreen()
{
    ClearConsoleScreen();

    ElapsedTime elapsed_time(current_user_time);

    SetConsoleCursorPosition(17,1);
    std::cout << "difficulty:" << game_difficulty_string;
    SetConsoleCursorPosition(52,1);
    std::cout << "game_score:" << current_user_score;
    SetConsoleCursorPosition(92,1);
    std::cout << "player:" << current_user_name;
    SetConsoleCursorPosition(127,1);
    std::cout << "time:";

    if( elapsed_time.hours )
    {
        std::cout << elapsed_time.hours << "h "; 
        if( elapsed_time.minutes )
            std::cout << elapsed_time.minutes << "m ";
    }

    else if( elapsed_time.minutes )
        std::cout << elapsed_time.minutes << "m " ;

    std::cout << elapsed_time.seconds << 's' << std::endl;

    for( int i = 0; i < game_size_x; ++i )
    {
        SetConsoleCursorPosition(73 - ( (game_size_x - 10) / 2 ),i + 3);
        for( int j = 0; j < game_size_y; ++j )
            std::cout << snake_field[ j * game_size_x + i ];

        std::cout << std::flush;
    }
}

void HandleGameDifficulty()
{
    std::memset(game_difficulty_string,0,7);

    if( game_difficulty == GAME_DIFFICULTY_EASY )
    {
        game_size_x = 10;
        game_size_y = 10;
        game_speed = 1.0f;
        std::memcpy(game_difficulty_string,"Easy",4);
    }

    else if( game_difficulty == GAME_DIFFICULTY_NORMAL )
    {
        game_size_x = 15;
        game_size_y = 15;
        game_speed = 0.8f;
        std::memcpy(game_difficulty_string,"Normal",6);
    }

    else if( game_difficulty == GAME_DIFFICULTY_HARD )
    {
        game_size_x = 20;
        game_size_y = 20;
        game_speed = 0.6f;
        std::memcpy(game_difficulty_string,"Hard",4);
    }
}

void GrowSnake()
{
    if( snake )
    {
        SnakePart* ite = snake->m_head;
        while( ite->next )
            ite = ite->next;

        ite->next = new SnakePart();
        snake->m_length++;
    }
}

void GenerateFood()
{
    do
    {
        food_x = 1 + rand() % ( game_size_x - 1 );
        food_y = 1 + rand() % ( game_size_y - 1 );
     // so that food doesn't spawn on snake body or head or corners.
    } while( 
             (food_x == snake->m_head->m_body.m_pos_x && food_y == snake->m_head->m_body.m_pos_y) ||
             snake_field[ food_x + game_size_y * food_y ] == '#'
           );

    snake_field[ food_x + game_size_y * food_y ] = '@';
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

    snake->m_head = new SnakePart();

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
                snake_field[ i * game_size_x + j ] = '#';
                
            else
            {
                if( ( j == 0 ) || ( j == game_size_y - 1 ) )
                    snake_field[ i * game_size_x + j ] = '#';
            }
        }
    }

    game_status = GAME_STATUS_CAN_BEGIN;

    return true;
}

void StartSnakeGame()
{
    current_user_score = 0;
    current_user_time = time(NULL);
    begin_time = std::clock();

    // clear game inner field.
    for( int i = 1; i < game_size_x - 1; ++i )
    {
        for( int j = 1; j < game_size_y - 1; ++j )
            snake_field[ i + j * game_size_y ] = ' ';
    }

    int16_t snake_pos_x, snake_pos_y;
    do
    {
        snake_pos_x = 2 + std::rand() % ( game_size_x - 2 );
        snake_pos_y = 2 + std::rand() % ( game_size_y - 2 );
    } while (snake_field[snake_pos_x + snake_pos_y * game_size_y] == '#'); // so that snake doesn't spawn at borders.

    int16_t snake_direction_need_to_be_avoided_1 = SNAKE_DIRECTION_NONE;
    int16_t snake_direction_need_to_be_avoided_2 = SNAKE_DIRECTION_NONE;

    if( snake_pos_x == 1 && snake_pos_y == 1 )
    {
        snake_direction_need_to_be_avoided_1 = SNAKE_DIRECTION_UP;
        snake_direction_need_to_be_avoided_2 = SNAKE_DIRECTION_LEFT;
    }

    else if( snake_pos_x == game_size_x - 1 && snake_pos_y == 1 )
    {
        snake_direction_need_to_be_avoided_1 = SNAKE_DIRECTION_UP;
        snake_direction_need_to_be_avoided_2 = SNAKE_DIRECTION_RIGHT;
    }

    else if( snake_pos_x == 1 && snake_pos_y == game_size_y - 1 )
    {
        snake_direction_need_to_be_avoided_1 = SNAKE_DIRECTION_DOWN;
        snake_direction_need_to_be_avoided_2 = SNAKE_DIRECTION_LEFT;
    }

    else if( snake_pos_x == game_size_x - 1 && snake_pos_y == game_size_y - 1 )
    {
        snake_direction_need_to_be_avoided_1 = SNAKE_DIRECTION_DOWN;
        snake_direction_need_to_be_avoided_2 = SNAKE_DIRECTION_RIGHT;
    }

    else if( snake_field[snake_pos_x + 1] == '#' )
    {
        snake_direction_need_to_be_avoided_1 = SNAKE_DIRECTION_RIGHT;
        snake_direction_need_to_be_avoided_2 = SNAKE_DIRECTION_NONE;
    }

    else if( snake_field[snake_pos_x - 1] == '#' )
    {
        snake_direction_need_to_be_avoided_1 = SNAKE_DIRECTION_LEFT;
        snake_direction_need_to_be_avoided_2 = SNAKE_DIRECTION_NONE;
    }

    else if( snake_field[snake_pos_y + 1] == '#' )
    {
        snake_direction_need_to_be_avoided_1 = SNAKE_DIRECTION_DOWN;
        snake_direction_need_to_be_avoided_2 = SNAKE_DIRECTION_NONE;
    }

    else if( snake_field[snake_pos_y - 1] == '#' )
    {
        snake_direction_need_to_be_avoided_1 = SNAKE_DIRECTION_UP;
        snake_direction_need_to_be_avoided_2 = SNAKE_DIRECTION_NONE;
    }

    uint16_t snake_direction = SNAKE_DIRECTION_NONE;

    while( snake_direction == snake_direction_need_to_be_avoided_1 ||
           snake_direction == snake_direction_need_to_be_avoided_2 )
        snake_direction = 1 + rand() % 4;

    snake->m_length = 1;
    snake->m_head->m_body.m_pos_x = snake_pos_x;
    snake->m_head->m_body.m_pos_y = snake_pos_y;
    snake->m_head->m_body.m_direction = snake_direction;

    GenerateFood();

    game_status = GAME_STATUS_ONGOING;
}

void HandleSnakeGameLogic()
{
    bool lost = false, win = false;
    uint16_t& snake_pos_x = snake->m_head->m_body.m_pos_x;
    uint16_t& snake_pos_y = snake->m_head->m_body.m_pos_y;
    uint16_t& snake_direction = snake->m_head->m_body.m_direction;

    if( snake_direction_to_move != SNAKE_DIRECTION_NONE )
    {
        if( snake_direction == SNAKE_DIRECTION_UP && snake_direction_to_move != SNAKE_DIRECTION_DOWN )
            snake_direction = snake_direction_to_move;

        else if( snake_direction == SNAKE_DIRECTION_DOWN && snake_direction_to_move != SNAKE_DIRECTION_UP )
            snake_direction = snake_direction_to_move;

        else if( snake_direction == SNAKE_DIRECTION_LEFT && snake_direction_to_move != SNAKE_DIRECTION_RIGHT )
            snake_direction = snake_direction_to_move;

        else if( snake_direction == SNAKE_DIRECTION_RIGHT && snake_direction_to_move != SNAKE_DIRECTION_LEFT )
            snake_direction = snake_direction_to_move;
    }

    snake_field[ snake_pos_x + snake_pos_y * game_size_y ] = ' ';

    switch( snake_direction )
    {
        case SNAKE_DIRECTION_UP:
            --snake_pos_y;
        break;

        case SNAKE_DIRECTION_LEFT:
            --snake_pos_x;
        break;

        case SNAKE_DIRECTION_DOWN:
            ++snake_pos_y;
        break;

        case SNAKE_DIRECTION_RIGHT:
            ++snake_pos_x;
        break;
    }

    if( snake_field[ snake_pos_x + snake_pos_y * game_size_y ] == '@' )
    {
        snake_field[ snake_pos_x + snake_pos_y * game_size_y ] = 'x';
        current_user_score += 10;
        if( snake->m_length == game_size_x * game_size_y )
            win = true;
        
        else
        {
            GrowSnake();
            GenerateFood();
        }

        if( win )
        {
            SubmitPlayerScore();
            game_status = GAME_STATUS_WON;
        }
    }

    else
    {
        if( snake_field[ snake_pos_x + snake_pos_y * game_size_y ] == '#' ||
            snake_field[ snake_pos_x + snake_pos_y * game_size_y ] == 'o' )
            lost = true;

        if( lost )
        {
            SubmitPlayerScore();
            game_status = GAME_STATUS_LOST;
            return;
        }

        else
        {
            snake_field[ snake_pos_x + snake_pos_y * game_size_y ] = 'x';
        }
    }
}

void HandleApplicationUpdate()
{
    switch( application_status )
    {
        int32_t user_key_input;

        case APPLICATION_STATE_MAIN_MENU:
            HideConsoleCursor(true);
            
            user_key_input = ReadKeyStrokeFromSTDIN();

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
                        application_status = APPLICATION_STATE_ENTER_NAME;

                    else if( menu_status == MENU_STATUS_OPTIONS )
                        application_status = APPLICATION_STATE_OPTIONS;

                    else if( menu_status == MENU_STATUS_SCOREBOARD )
                        application_status = APPLICATION_STATE_SCOREBOARD;

                    else if( menu_status == MENU_STATUS_EXIT )
                        app_is_running = false;
                break;
            }

            DisplayMainMenu();
            SleepIfNotInterrupted(50000);
        break;

        case APPLICATION_STATE_ENTER_NAME:
            ClearConsoleScreen();
            HideConsoleCursor(false);

            std::cout << "please enter your name (max 20 characters):" << current_user_name << std::flush;

            while( ( user_key_input = ReadKeyStrokeFromSTDIN() ) == KEY_NONE )
                SleepIfNotInterrupted(3000);

            if( user_key_input == KEY_ESCAPE )
            {
                application_status = APPLICATION_STATE_MAIN_MENU;
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

        SleepIfNotInterrupted(35000);
        break;

        case APPLICATION_STATE_ENTER_DIFFICULTY:
            ClearConsoleScreen();
            HideConsoleCursor(true);

            std::cout << "please enter difficulty( 0 for easy, 1 for normal, 2 for hard ):" << std::flush;
            game_difficulty = GAME_DIFFICULTY_NOT_DEFINED;

            while( game_difficulty == GAME_DIFFICULTY_NOT_DEFINED )
            {
                user_key_input = ReadKeyStrokeFromSTDIN();
                if( user_key_input == KEY_NONE )
                {
                    SleepIfNotInterrupted(3000);                    
                    continue;
                }

                else if( user_key_input == KEY_ESCAPE )
                {
                    application_status = APPLICATION_STATE_MAIN_MENU;
                    ClearUserName();

                    break;
                }

                else if( user_key_input == KEY_0 )
                    game_difficulty = GAME_DIFFICULTY_EASY;

                else if( user_key_input == KEY_1 )
                    game_difficulty = GAME_DIFFICULTY_NORMAL;
                
                else if( user_key_input == KEY_2 )
                    game_difficulty = GAME_DIFFICULTY_HARD;
            }

            if( application_status != APPLICATION_STATE_MAIN_MENU )
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
                    user_key_input = ReadKeyStrokeFromSTDIN();

                    switch( user_key_input )
                    {
                        case KEY_W_LOWERCASE:
                        case KEY_W_UPPERCASE:
                        case KEY_UP:
                            snake_direction_to_move = SNAKE_DIRECTION_LEFT;
                        break;

                        case KEY_A_LOWERCASE:
                        case KEY_A_UPPERCASE:
                        case KEY_LEFT:
                            snake_direction_to_move = SNAKE_DIRECTION_UP;
                        break;

                        case KEY_S_LOWERCASE:
                        case KEY_S_UPPERCASE:
                        case KEY_DOWN:
                            snake_direction_to_move = SNAKE_DIRECTION_RIGHT;
                        break;

                        case KEY_D_LOWERCASE:
                        case KEY_D_UPPERCASE:
                        case KEY_RIGHT:
                            snake_direction_to_move = SNAKE_DIRECTION_DOWN;
                        break;

                        case KEY_ESCAPE:
                            application_status = APPLICATION_STATE_MAIN_MENU;
                            game_status = GAME_STATUS_NOT_INITIALIZED;
                        break;
                    }

                    // if user hasn't pressed escape while playing the game
                    if( application_status == APPLICATION_STATE_SNAKE_GAME )
                    {
                        if( static_cast<float>(( std::clock() - begin_time )) / CLOCKS_PER_SEC >= 0.8f * game_speed )
                        {
                            HandleSnakeGameLogic();
                            DisplayGameOnScreen();

                            begin_time = clock();
                        }
                    }

                    else
                        ClearUserName();
                break;

                case GAME_STATUS_LOST:
                    ClearConsoleScreen();
                    std::cout << "you lost the game with the score of:" << current_user_score << '\n'
                              << "press Enter to play again, Escape to return to main menu and space "
                              << "to change difficulty." << std::flush;

                    while( ( user_key_input = ReadKeyStrokeFromSTDIN() ) == KEY_NONE )
                        SleepIfNotInterrupted(3000);

                    if( user_key_input == KEY_ENTER )
                        StartSnakeGame();

                    else if( user_key_input == KEY_ESCAPE )
                    {
                        game_status = GAME_STATUS_NOT_INITIALIZED;
                        application_status = APPLICATION_STATE_MAIN_MENU;
                        ClearUserName();
                    }

                    else if( user_key_input == KEY_SPACE )
                        application_status = APPLICATION_STATE_ENTER_DIFFICULTY;
                break;

                case GAME_STATUS_WON:
                    ClearConsoleScreen();
                    std::cout << "congratulations!you won the game with the score of:" << current_user_score << '\n'
                              << "Press Escape to go to main menu. if you want, you can check your score "
                              << "by selecting scores option from main menu." << std::endl;

                    while( ( user_key_input = ReadKeyStrokeFromSTDIN() ) == KEY_NONE )
                        SleepIfNotInterrupted(3000);

                    if( user_key_input == KEY_ESCAPE )
                    {
                        game_status = GAME_STATUS_NOT_INITIALIZED;
                        application_status = APPLICATION_STATE_MAIN_MENU;
                        ClearUserName();
                    }
                break;
            }
        break;

        case APPLICATION_STATE_OPTIONS:
            ClearConsoleScreen();
            HideConsoleCursor(false);

            ReadOptionsFromFile();

            DisplayOptions();

            user_key_input = ReadKeyStrokeFromSTDIN();
            switch( user_key_input )
            {
                case KEY_ESCAPE:
                    if( options_changed )
                    {
                        options_changed = false;
                        WriteOptionsToFile();
                    }

                    application_status = APPLICATION_STATE_MAIN_MENU;
                break;

                case KEY_SPACE:
                    if( option_menu_choise == OPTION_STATUS_ALLOW_SNAKE_CUT_ITSELF )
                    {
                        snake_can_cut_itself = !snake_can_cut_itself;
                        options_changed = true;
                    }

                    else if( option_menu_choise == OPTION_STATUS_ALLOW_SNAKE_PASS_BORDERS )
                    {
                        snake_can_pass_border = !snake_can_pass_border;
                        options_changed = true;
                    }

                break;

                case KEY_ENTER:
                    if( option_menu_choise == OPTION_STATUS_BACK )
                    {
                        //if( options_changed )
                        //{
                            WriteOptionsToFile();
                            options_changed = false;
                            application_status = APPLICATION_STATE_MAIN_MENU;
                        //}
                    }
                break;

                case KEY_W_UPPERCASE:
                case KEY_W_LOWERCASE:
                case KEY_UP:
                    if( option_menu_choise == OPTION_STATUS_ALLOW_SNAKE_CUT_ITSELF )
                        option_menu_choise = OPTION_STATUS_BACK;

                    else
                        --option_menu_choise;
                break;

                case KEY_S_LOWERCASE:
                case KEY_S_UPPERCASE:
                case KEY_DOWN:
                    if( option_menu_choise == OPTION_STATUS_BACK )
                        option_menu_choise = OPTION_STATUS_ALLOW_SNAKE_CUT_ITSELF;

                    else
                        ++option_menu_choise;
                break;
            }

            SleepIfNotInterrupted(50000);
        break;

        case APPLICATION_STATE_SCOREBOARD:
            ClearConsoleScreen();
            HideConsoleCursor(true);

            DrawScoreBoard();

            user_key_input = ReadKeyStrokeFromSTDIN();

            switch( user_key_input )
            {
                case KEY_ESCAPE:
                    application_status = APPLICATION_STATE_MAIN_MENU;
                break;
            }

            SleepIfNotInterrupted(50000);
        break;
    }
}

int main( int argc, char** argv, char** env )
{
    InitializeApplication(argc,argv,env);

    while( ApplicationShouldClose() )
        HandleApplicationUpdate();

    return 0;
}