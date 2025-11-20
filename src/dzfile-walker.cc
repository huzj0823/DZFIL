#include "common.h"
#include "dzfile-api.h"
#include "dzfile-intern.h"
#include "threadPool.hh"
#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <getopt.h>
#include <grp.h>
#include <hdf5.h>
#include <iostream>
#include <json/json.h>
#include <pwd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#if 0
/***Policy of converting file***/
// How many seconds the file will be converted after it created
const int MTIME_PERIOD = 300;
// How many seconds to start next scan
const int CHECK_INTERVAL = 30;
// The file will be converted only it is bigger than the size (default 1MB)
const size_t LOW_SIZE = 1048576;
// The following file extension will not converted
std::string blacklist = "gz;*.jpg;*.zst;*.zip;*.bz2;*.rar";
#endif
// Thread pool size
const size_t THREAD_POOL_SIZE = 4;

struct Policy
{
  int mtime_period;
  int check_interval;
  int low_size;
  int nbthread;
  string blacklist;
};

struct Configure
{
  string data_dir;
  string log_level;
  string log_file;
};

static Policy policy;
static Configure configure;

static bool
match_extension(const std::string& filename, const std::string& extensions)
{
  std::vector<std::string> extension_list;
  std::string::size_type start = 0, end = 0;
  std::string exts;
  int i = 0;
  // Decompose the extension list into a vector of strings
  while (true) {
    end = extensions.find(';', start);
    if (end != std::string::npos) {
      if (end != start) {
        exts = extensions.substr(start, end - start);
        std::string::size_type pos = exts.rfind('.');
        if (pos != std::string::npos)
          exts = exts.substr(pos + 1);
      }
    } else {
      exts = extensions.substr(start);
      if (exts.empty())
        break;
    }
    // cout << "extends " << i++ << ": " << exts << endl;
    extension_list.push_back(exts);
    if (end == std::string::npos)
      break;
    start = end + 1;
  }
  /*
    if (start < extensions.length()) {
      extension_list.push_back(extensions.substr(start));
    }
  */
  // Decompose the file name into extension and base file name
  std::string::size_type pos = filename.rfind('.');
  if (pos == std::string::npos) {
    return false;
  }

  std::string ext = filename.substr(pos + 1);
  std::string base = filename.substr(0, pos);

  // Check if the file's extension matches any of them in the list
  for (std::vector<std::string>::const_iterator it = extension_list.begin();
       it != extension_list.end();
       ++it) {
    if (*it == ext) {
      return true;
    }
  }
  return false;
}

static void
process_file(const string& filename)
{
  // Process the file here
  struct stat stbuf;
  int ret;
  bool ismatch;
  if ((match_sysdir(filename.c_str()) == 1) ||
      (match_hiddenfile(filename.c_str()) == 1))
    return;
  ret = stat(filename.c_str(), &stbuf);
  if (ret < 0)
    return;
  if (stbuf.st_size < policy.low_size)
    return;
  ret = findhiddenFile(filename.c_str());
  if (ret == 1)
    return;
  ismatch = match_extension(filename, policy.blacklist);
  if (ismatch)
    return;
  cout << "  Processing file: " << filename << endl;
  ret = convert2h5(filename.c_str(), DZ_ALG_ZSTD);
  if (ret < 0) {
    if (errno == EEXIST) {
      dz_msg(DZ_LOG_ERROR, "hiddenfile is already exist\n");
    } else {
      dz_msg(DZ_LOG_ERROR, "conver h5 file failed\n");
    }
  }
  dz_msg(DZ_LOG_DEBUG, "file %s is converted successfully\n", filename.c_str());
  return;
}

static int
parseJsonFile(const string& fileName, Policy& policy, Configure& configure)
{
  ifstream ifs(fileName);
  if (!ifs.is_open()) {
    cerr << "Failed to open configure file '" << fileName
         << "': " << strerror(errno) << endl;
    return -1;
  }

  Json::Value root;
  ifs >> root;

  Json::Value policyNode = root["policy"];
  policy.mtime_period = policyNode.get("mtime_period", 0).asInt();
  policy.check_interval = policyNode.get("check_interval", 0).asInt();
  policy.low_size = policyNode.get("low_size", 0).asInt();
  policy.nbthread = policyNode.get("nbthread", 0).asInt();
  policy.blacklist = policyNode.get("blacklist", "").asString();

  Json::Value configureNode = root["configure"];
  configure.data_dir = configureNode.get("data_dir", "").asString();
  configure.log_level = configureNode.get("log_level", "").asString();
  configure.log_file = configureNode.get("log_file", "").asString();

  ifs.close();
  return 0;
}

int
main(int argc, char* argv[])
{
  int helpflg = 0;
  int verbose = 0;
  int errflg = 0;
  int bgflg = 0;
  static struct option longopts[] = { { "help", no_argument, &helpflg, 1 },
                                      { "verbose", no_argument, &verbose, 1 },
                                      { 0, 0, 0, 0 } };
  char* filename = NULL;
  const char* data_path = NULL;
  int c;
  int ret;
  hid_t h5fd;
  struct stat stbuf;
  std::string conf_file;
  while ((c = getopt_long(argc, argv, "Vvd:hc:d", longopts, NULL)) != EOF) {
    switch (c) {
      case 'h':
        helpflg = 1;
        break;
      case 'v':
        verbose = 1;
        break;
      case 'c':
        conf_file = optarg;
        break;
      case 'd':
        bgflg = 1;
        break;
      case '?':
        errflg++;
        break;
      default:
        break;
    }
  }
  if (errflg || helpflg) {
    printf("usage: %s -c configure_file [default: /etc/dzfile.conf]\n",
           argv[0]);
    printf("\tfor example %s\n", argv[0]);
    return -1;
  }

  if (conf_file.empty()) {
    conf_file = "/etc/dzfile.conf";
  }
  ret = parseJsonFile(conf_file, policy, configure);
  if (ret < 0) {
    return -1;
  }
  if (verbose) {
    cout << "configure file:\t\t" << conf_file << endl;
    cout << "policy.mtime_period:\t" << policy.mtime_period << endl;
    cout << "policy.check_interval:\t" << policy.check_interval << endl;
    cout << "policy.low_size:\t" << policy.low_size << endl;
    cout << "policy.nbthread:\t" << policy.nbthread << endl;
    cout << "policy.blacklist:\t" << policy.blacklist << endl;
    cout << "configure.data_dir:\t" << configure.data_dir << endl;
    cout << "configure.log_level:\t" << configure.log_level << endl;
    cout << "configure.log_file:\t" << configure.log_file << endl;
  }
  data_path = configure.data_dir.c_str();
  
  // If it is specified to run in backgroud
  if (bgflg) {
    pid_t pid = fork(); // Create child process
    if (pid < 0) {
      printf("Failed to fork\n");
      exit(1);
    }
    if (pid > 0) { // Parent process exits
      exit(0);
    }
    umask(0); // Set file mode to 0
    if (setsid() < 0) { // Create new session, become session group leader
      exit(1);
    }
    if (chdir("/") < 0) { // Set working directory to root directory
      exit(1);
    }
    close(STDIN_FILENO);  // Close standard input
    close(STDOUT_FILENO); // Close standard output
    close(STDERR_FILENO); // Close standard error output
  } //end of if (bgflg)
  
  ret = dzfile_init(data_path, NULL);
  if (ret < 0) {
    fprintf(stderr, "init dzfile failed\n");
  }
  // Create the thread pool
  ThreadPool pool(policy.nbthread);
  while (1) {
    DIR* dir = opendir(data_path); // Open specified directory
    if (!dir) {
      fprintf(stderr, "Failed to open directory %s\n", data_path);
      if (bgflg)
      dz_msg(DZ_LOG_ERROR, "Failed to open directory %s\n", data_path);
      exit(1);
    }

    struct dirent* entry;
    struct stat file_stat;

    while ((entry = readdir(dir)) != NULL) { // Traverse files in the directory
      char file_path[1024];
      sprintf(file_path, "%s/%s", data_path, entry->d_name);

      if (stat(file_path, &file_stat) < 0) { // Get file status
        printf("Failed to get file stat\n");
        exit(1);
      }

      time_t now = time(NULL);                 // Get current time
      time_t create_time = file_stat.st_mtime; // Get file creation time

      if ((now - create_time) >
          policy.mtime_period) { // Determine if the file meets the conditions
                                 // Add the file to the thread pool
        pool.enqueue(process_file, file_path);
      }
    }

    closedir(dir);                // Close directory
    sleep(policy.check_interval); // Sleep for a period of time and scan again
  } //end of while(1)

  return 0;
}
