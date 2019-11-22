#define _XOPEN_SOURCE 500
#include <syslog.h>
#include <sys/resource.h>
#include <libgen.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>
#include <ftw.h>
#include <fstream>

static const int MAX_PATH = 100;
static const char* PID_PATH = "/var/run/daemon.pid";
static const char* TOTAL_LOG = "/total.log";

char abs_config_path[MAX_PATH];
char abs_folder1[MAX_PATH];
char abs_folder2[MAX_PATH];
char total_log_path[MAX_PATH];
int interval_sec = -1;

void get_config() {
	if (access(abs_config_path, F_OK)) {
		syslog(LOG_ERR, "Can't find config file %s", abs_config_path);
		exit(EXIT_FAILURE);
	}

	FILE* conf = fopen(abs_config_path, "r");
	if (conf == nullptr) {
		syslog(LOG_ERR, "Failed to open config file. Error %d", errno);
		exit(EXIT_FAILURE);
	}
	if (fscanf(conf, "%s", abs_folder1) < 0 ||
			fscanf(conf, "%s", abs_folder2) < 0 ||
			fscanf(conf, "%d", &interval_sec) < 0) {
		syslog(LOG_ERR, "Failed to read config file. Error %d", errno);
		fclose(conf);
		exit(EXIT_FAILURE);
	}
	fclose(conf);

	if (!strcmp(abs_folder1, abs_folder2)) {
		syslog(LOG_ERR, "Directories' paths are equal");
		exit(EXIT_FAILURE);
	}

	if (interval_sec <= 0) {
		syslog(LOG_ERR, "Incorrect interval %d", interval_sec);
		exit(EXIT_FAILURE);
	}

	strcpy(total_log_path, abs_folder2);
	strcat(total_log_path, TOTAL_LOG);
}

bool file_is_log(char* fpath) {
	char* dot = strrchr(fpath, '.');
	return dot && !strcmp(dot, ".log");
}

int copy_and_remove_log(const char* source_path, const struct stat* st,
												int typeflag, struct FTW* ftwbuf) {
	if (typeflag) {
		return 0;
	}

	char filename[MAX_PATH];
	strcpy(filename, basename(strdup(source_path)));

	if (!file_is_log(filename)) {
		return 0;
	}

	std::ifstream source(source_path);
	if (!source) {
		syslog(LOG_ERR, "Can't open %s", source_path);
		return 0;
	}

	std::ofstream target(total_log_path, std::fstream::app);

	if (target.tellp() != target.beg) {
		target << std::endl << std::endl;
	}

	target << filename << std::endl << std::endl;
	target << source.rdbuf();

	source.close();
	target.close();

	if (remove(source_path)) {
		syslog(LOG_ERR, "Can't remove %s", source_path);
		return 0;
	}

	return 0;
}

void proc() {
	DIR* dir1 = opendir(abs_folder1);

	if (dir1 == nullptr) {
		syslog(LOG_ERR, "Can't open source directory %s", abs_folder1);
		exit(EXIT_FAILURE);
	} else {
		closedir(dir1);
	}

	DIR* dir2 = opendir(abs_folder2);
	if (dir2 == nullptr) {
		mkdir(abs_folder2, 0755);
	} else {
		closedir(dir2);
	}

	nftw(abs_folder1, copy_and_remove_log, 64, FTW_DEPTH | FTW_PHYS);
}

void sig_handler(int signo) {
	if (signo == SIGTERM) {
		syslog(LOG_INFO, "SIGTERM. Stopping daemon...");
		FILE* pid_file = fopen(PID_PATH, "w");
		if (pid_file != nullptr) {
			fclose(pid_file);
		}
		exit(EXIT_SUCCESS);
	}

	if (signo == SIGHUP) {
		syslog(LOG_INFO, "SIGHUP. Updating config file...");
		get_config();
	}
}

void handle_signals() {
	if (signal(SIGTERM, sig_handler) == SIG_ERR) {
		syslog(LOG_ERR, "Error! Can't catch SIGTERM");
		exit(EXIT_FAILURE);
	}
	if (signal(SIGHUP, sig_handler) == SIG_ERR) {
		syslog(LOG_ERR, "Error! Can't catch SIGHUP");
		exit(EXIT_FAILURE);
	}
}

int kill_daemon() {
	if (access(PID_PATH, F_OK)) {
		return EXIT_FAILURE;
	}

	FILE* pid_file = fopen(PID_PATH, "r");
	if (pid_file == nullptr) {
		return EXIT_FAILURE;
	}

	pid_t pid;
	fscanf(pid_file, "%d", &pid);
	fclose(pid_file);

	char proc_path[15];
	sprintf(proc_path, "/proc/%d", pid);

	struct stat st;
	if (stat(proc_path, &st) || !S_ISDIR(st.st_mode)) {
		return EXIT_FAILURE;
	}

	if (kill(pid, SIGTERM)) {
		syslog(LOG_ERR, "Unable to kill daemon %d", pid);
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

void daemonize() {
	// First fork
	pid_t pid = fork();
	if (pid < 0) {
		exit(EXIT_FAILURE);
	} else if (pid > 0) {
		exit(EXIT_SUCCESS);
	}

	// Create a new session
	pid_t sid = setsid();
	if (sid < 0) {
		exit(EXIT_FAILURE);
	}

	// Second fork
	pid = fork();
	if (pid < 0) {
		exit(EXIT_FAILURE);
	} else if (pid > 0) {
		exit(EXIT_SUCCESS);
	}

	pid = getpid();

	// Change working directory to root directory
	chdir("/");

	// Grant all permisions for all files and directories created by the daemon
	umask(0);

	// Redirect std IO
	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);

	kill_daemon();

	// Rewrite pid file
	FILE* pid_fp = fopen(PID_PATH, "w");
	if (pid_fp == nullptr) {
		syslog(LOG_ERR, "Failed to open pid file");
		exit(EXIT_FAILURE);
	}
	fprintf(pid_fp, "%d", pid);
	fclose(pid_fp);
}

int main(int argc, char* argv[]) {
	if (argc < 2) {
		printf("Too few arguments");
		return 0;
	}

	openlog("daemon_log", LOG_NOWAIT | LOG_PID, LOG_USER);

	if (strcmp(argv[1], "stop") == 0) {
		kill_daemon();
	} else if (strcmp(argv[1], "start") == 0) {
		if (argc < 3) {
			printf("Config path is missing\n");
			return 0;
		}

		char* config_path = argv[2];
		realpath(config_path, abs_config_path);
		get_config();

		daemonize();
		syslog(LOG_INFO, "Daemon started");

		handle_signals();

		while (true) {
			proc();
			sleep(interval_sec);
		}
	} else {
		printf("Incorrect argument \"%s\"\n", argv[1]);
	}

	return 0;
}
