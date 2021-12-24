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

#include <sys/types.h>
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
static int sigfd;

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

/** gpcron tables location */
char *crontabDir = GPIOD_CRON_TABLE_DIRECTORY;

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
    {"tabdir",          required_argument,  0,                  'd'},
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
        "-d, --tabdir=DIR           tabs location directory[default=%s]\n" \
        "-h, --help                 prints this message\n",
        pidFile,
        configFile,
        crontabDir
        );
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

    while((c = getopt_long(argc, argv, "Vvl:nkHp:c:d:h", long_options, &option_index)) != -1) {
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
            case 'd':
                crontabDir = optarg;
                printf("Reading tabs from: %s\n", crontabDir);
                break;
            case 'h':
                PrintUsage(argc, argv);
                exit(EXIT_SUCCESS);
                break;
            default:
                break;
        }
    }

    pid_t pid = read_pid_file(pidFile);

    if(pid != 0) {
        ret = kill(pid, 0);
        if(ret == -1) {
            fprintf(stderr, "%s : %s pid file exists, but the process doesn't!\n", PACKAGE_NAME, pidFile);

            if(kill_flag || hup_flag)
                goto quit;

            unlink(pidFile);
        } else {
            /** check if -k (kill) passed*/
            if(kill_flag)
            {
                kill(pid, SIGTERM);
                goto quit;
            }

            /** check if -h (hup) passed*/
            if(hup_flag)
            {
                kill(pid, SIGHUP);
                goto quit;
            }
        }
    }

    if(daemon_flag) {
        daemonize("/", 0);
        pid = create_pid_file(pidFile);
    } else
        openlog(PACKAGE_NAME, LOG_PERROR, LOG_DAEMON);

    /** setup signals */
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTERM);
    sigaddset(&mask, SIGQUIT);
    sigaddset(&mask, SIGHUP);
    sigaddset(&mask, SIGPIPE);
    sigaddset(&mask, SIGCHLD);

    if (sigprocmask(SIG_BLOCK, &mask, NULL) == -1) {
        syslog(LOG_ERR, "Could not register signal handlers (%s).", strerror(errno));
        goto unlink_pid;
    }

    /** get sigfd */
    sigfd = signalfd(-1, &mask, SFD_CLOEXEC);

    /** set log level */
    setlogmask(LOG_UPTO(log_level));

    INIT_LIST_HEAD(&(gp_list));

    ret = loadConfig(configFile);
    if(ret) {
        errsv = errno;
        syslog(LOG_ERR, "Could not initialize sysfs (%s).", strerror(errsv));
        goto unlink_pid;
    }

    ret = loadTabs(crontabDir);
    if(ret) {
        errsv = errno;
        syslog(LOG_ERR, "Failed loading tabs (%s).", strerror(errsv));
    }

    ret = init_gpio_chips();
    if(ret) {
        errsv = errno;
        syslog(LOG_ERR, "Failed initializing gpiochips (%s)", strerror(errsv));
        goto cleanup;
    }

    ret = init_sysfs();
    if(ret) {
        syslog(LOG_ERR, "Could not initialize sysfs (%s).", strerror(errno));
        goto cleanup;
    }

    ret = init_gpio_pins();
    if(ret) {
        syslog(LOG_ERR, "Failed to init gpio_pins (%s).", strerror(errno));
        goto cleanup;
    }

    ret = loop(sigfd);

    cleanup_gpio_pins();

    cleanup:
    free_gpio_pins();

    cleanup_sysfs();

    cleanup_gpio_chips();

    unlink_pid:
    unlink(pidFile);

    quit:
    return ret;
}
