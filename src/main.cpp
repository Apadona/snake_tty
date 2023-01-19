#include <iostream>
#include <cstdlib>
#include <ctime>

#include <signal.h>

void HandleInterruptSignal( int signal )
{
    std::cout << "\ninterrupt has been generated!\n";
    std::exit(EXIT_SUCCESS);
}

void InitializeApplication( int argc, char** argv, char** env )
{
    signal(SIGINT,HandleInterruptSignal);
}

void ClearScreen()
{
    system("clear");
}

void test()
{
    std::cout << "testing" << std::flush;

    while( true )
    {
        for( int i = 0; i < 3; ++i )
        {
            usleep(333333);
            std::cout << '.' << std::flush;
        }
        
        usleep(333333);
        std::cout << "\b\b\b   \b\b\b" << std::flush;
    }
}

int main( int argc, char** argv, char** env )
{
    InitializeApplication(argc,argv,env);

    test();

    return 0;
}