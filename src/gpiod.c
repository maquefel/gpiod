/** @file gpiod.c
*
* @brief Main file for the gpio daemon
*
* @par
*/

#include "gpiod.h"
#include "daemonize.h"
#include "gpiod-chip.h"
#include "gpiod-config.h"
#include "gpiod-pin.h"
#include "gpiod-event-loop.h"

#include "pins/gpiod-sysfs.h"

#include <syslog.h>
#include <getopt.h>
#include <stdio.h>
#include <signal.h>

#include <sys/signalfd.h>

static int verbose_flag = 0;
static int no_daemon_flag = 0;
static int kill_flag = 0;
static int hup_flag = 0;

/** signal fd */
int sigfd;

/** /dev/null fd*/
int devnullfd;

/** logging */
#ifdef DEBUG
int log_level = LOG_DEBUG;
#else
int log_level = LOG_INFO;
#endif

/** pid file */
static char *pidFile = GPIOD_PID_FILE;

/** config file */
char *configFile = GPIOD_CONFIG_FILE_PATH;

static struct option long_options[] =
{
    {"verbose",         no_argument,        &verbose_flag,      'V'},
    {"version",         no_argument,        0,                  'v'},
    {"log-level",       required_argument,  0,                  'l'},
    {"n",               no_argument,        &no_daemon_flag,    'n'},
    {"kill",            no_argument,        &kill_flag,         'k'},
    {"hup",             no_argument,        &hup_flag,          'H'},
    {"pid",             required_argument,  0,                  'p'},
    {"config",          required_argument,  0,                  'c'},
    {"help",            no_argument,        0,                  'h'},
    {0, 0, 0, 0}
};

/**************************************************************************
*    Function: Print Usage
*    Description:
*        Output the nul terminated string with daemon version.
*    Params:
*    Returns:
*        returns nul terminated string with daemon version
**************************************************************************/
char* daemon_version()
{
    static char version[31] = {0};
    snprintf(version, sizeof(version), "%d.%d.%d\n", MAJOR, MINOR, VERSION_PATCH);
    return version;
}

/**************************************************************************
*    Function: Print Usage
*    Description:
*        Output the command-line options for this daemon.
*    Params:
*        @argc - Standard argument count
*        @argv - Standard argument array
*    Returns:
*        returns void always
**************************************************************************/
void PrintUsage(int argc, char *argv[]) {
    argv = argv;
    if(argc >=1) {
        printf(
        "-v, --version              prints version and exits\n" \
        "-V, --verbose              be more verbose\n" \
        "-l, --log-level            set log level[default=LOG_INFO]\n" \
        "-n                         don\'t fork off as daemon\n" \
        "-k, --kill                 kill old daemon instance in case if present\n" \
        "-H, --hup                  send daemon signal to reload configuration\n" \
        "-p, --pid                  path to pid file[default=%s]\n" \
        "-c, --config=FILE          configuration file[default=%s]\n" \
        "-h, --help                 prints this message\n",
        pidFile,
        configFile);
    }
}

/**************************************************************************
*    Function: main
*    Description:
*        The c standard 'main' entry point function.
*    Params:
*        @argc - count of command line arguments given on command line
*        @argv - array of arguments given on command line
*    Returns:
*        returns integer which is passed back to the parent process
**************************************************************************/
int main(int argc, char ** argv)
{
    int daemon_flag = 1; //daemonizing by default
    int c = -1;
    int option_index = 0;

    int errsv = 0;
    int ret = 0;

    while((c = getopt_long(argc, argv, "Vvl:nkHp:c:h", long_options, &option_index)) != -1) {
                switch(c) {
            case 'v' :
                printf("%s", daemon_version());
                exit(EXIT_SUCCESS);
                break;
            case 'V' :
                verbose_flag = 1;
                break;
            case 'l':
                log_level = strtol(optarg, 0, 10);
                break;
            case 'n' :
                printf("Not daemonizing!\n");
                daemon_flag = 0;
                break;
            case 'k' :
                kill_flag = 1;
                break;
            case 'H' :
                hup_flag = 1;
                break;
            case 'p' :
                pidFile = optarg;
                break;
            case 'c':
                configFile = optarg;
                printf("Using config file: %s\n", configFile);
                break;
            case 'h':
                PrintUsage(argc, argv);
                exit(EXIT_SUCCESS);
                break;
            default:
                break;
        }
    }

    /** check if -k (kill) passed*/
    if(kill_flag)
    {
        goto quit;
    }

    /** check if -h (hup) passed*/
    if(hup_flag)
    {
        goto quit;
    }

    if(daemon_flag)
        daemonize("/", 0);
    else
        openlog(PACKAGE_NAME, LOG_PERROR, LOG_DAEMON);

    /** setup signals */
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTERM);
    sigaddset(&mask, SIGQUIT);
    sigaddset(&mask, SIGHUP);
    sigaddset(&mask, SIGPIPE);

    if (sigprocmask(SIG_BLOCK, &mask, NULL) == -1) {
        syslog(LOG_ERR, "Could not register signal handlers (%s).", strerror(errno));
        goto quit;
    }

    /** get sigfd */
    extern int sigfd;
    sigfd = signalfd(-1, &mask, 0);

    /** set log level */
    setlogmask(LOG_UPTO(log_level));

    INIT_LIST_HEAD(&(gp_list));

    ret = loadConfig(configFile);
    errsv = errno;

    ret = init_gpio_chips();

    ret = init_sysfs();
    if(ret == -1) {
        syslog(LOG_ERR, "Could not initialize sysfs (%s).", strerror(errno));
        goto quit;
    }

    ret = init_gpio_pins();

    ret = loop();

    ret = cleanup_gpio_pins();

    free_gpio_pins();

    cleanup_sysfs();

    cleanup_gpio_chips();

    quit:
    return 0;
}