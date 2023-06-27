#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <stdbool.h>
#include <errno.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>

/*
  Project spec:
  https://web.cs.ucla.edu/classes/fall19/cs111/labs/project0.html

  Implemented relevant features for my own learning.
  Previous course teaching of Com Sci 111 at UCLA Project 0.

  Core dump option omitted. Note: setrlimit(2)
*/

#define DUP_ARG 2
#define BAD_ARG 3
#define SIG_CATCH 4
#define UNEXP 5

/* minimum is 20 */
#define STR_MAX 30

_Noreturn void
die(uint8_t status, char *str, char *prog)
{
  fprintf(stderr, str, prog);
  fprintf(stderr, "usage: %s "
		  "[--input=file] [--output=file] [--segfault] [--catch]\n"
		  , prog);
  exit(status);
}

void seg_handler(int sig)
{
  fprintf(stderr, "caught signal SIGSEGV\n");
  exit(SIG_CATCH);
}


bool
ref_same_file(int fd1, int fd2)
{
  struct stat stat1, stat2;
  if (!fstat(fd1, &stat1) && !fstat(fd1, &stat2))
	return (stat1.st_dev == stat2.st_dev)
	  && (stat1.st_ino == stat2.st_ino);
  perror("fstat");
  exit(errno);
}

int
main(int argc, char **argv)
{
  
  bool segfault = false;
  bool catch = false;
  bool dump = false;
  char *input_file = NULL;
  char *output_file = NULL;
  char *str = malloc(STR_MAX);
  
  if (str == NULL)
	{
	  perror("malloc");
	  exit(errno);
	}
  str[0] = '\0';

  int optc;
  
  static struct option long_options[] =
	{
	  { "input", required_argument, NULL, 'i' },
	  { "output", required_argument, NULL, 'o' },
	  { "segfault", no_argument, NULL, 's' },
	  { "catch", no_argument, NULL, 'c' },
	  { NULL       , 0                , NULL, 0 },
	};

  while ((optc = getopt_long(argc, argv, "-:scdi:o:", long_options, NULL))
		   != -1)
	switch (optc)
	  {
	  case 's':
		segfault = true;
		break;
		
	  case 'c':
		catch = true;
		break;

	  case 'i':
		if (input_file)
		  die(DUP_ARG, "multiple '-i' options specified\n", argv[0]);
		input_file = optarg;
		break;

	  case 'o':
		if (output_file)
		  die(DUP_ARG, "multiple '-o' options specified\n", argv[0]);
		output_file = optarg;
		break;

	  case '?':
		if (optopt == 0)
		  snprintf(str, STR_MAX, "unknown option: '%.*s'\n", STR_MAX - 20, argv[optind - 1]);
		else
		  snprintf(str, STR_MAX, "unknown option: '-%c'\n", optopt);		  
		die(BAD_ARG, str, argv[0]);
		
	  case ':':
		// fprintf(stderr, "%s\n", argv[optind - 1]); // ** covers all cases **
		snprintf(str, STR_MAX, "missing arg for '-%c'\n", optopt);
		die(BAD_ARG, str, argv[0]);
		
	  case 1:
		str = "non-option argument not allowed\n";
		die(BAD_ARG, str, argv[0]);
				
	  default:
		die(UNEXP, str, argv[0]);
	  }

  free(str);
  
  if (catch)
	  signal(SIGSEGV, seg_handler);
  
  if (segfault)
	{
	  if (-1 == raise(SIGSEGV))
		{
		  int *i = NULL;
		  *i = 0;
		  exit(UNEXP);
		}
	}

  if (input_file)
	{
	  int fd = open(input_file, O_RDONLY);
	  if (-1 == fd)
		{
		  perror("open");
		  exit(errno);
		}
	  
	  if (-1 == dup2(fd, STDIN_FILENO))
		{
		  perror("dup2");
		  exit(errno);
		}
	}

  if (output_file)
	{
	  int fd = open(output_file, O_WRONLY|O_CREAT|O_TRUNC,
					S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
	  if (-1 == fd)
		{
		  perror("open");
		  exit(errno);
		}
	  if (-1 == dup2(fd, STDOUT_FILENO))
		{
		  perror("dup2");
		  exit(errno);
		}
	}


  if (output_file && input_file)
	{
	  if (ref_same_file(STDIN_FILENO, STDOUT_FILENO))
		{
		  fprintf(stderr, "input and output file cannot be the same\n");
		  exit(EXIT_FAILURE);
		}
	}


  size_t const page_size = sysconf(_SC_PAGESIZE);
  char buf[page_size];
  ssize_t bytes_read;
  ssize_t bytes_written;
  
  while ( (bytes_read = read(STDIN_FILENO, buf, page_size))
		  != 0)
	{
	  if (-1 == bytes_read)
		{
		  perror("read");
		  exit(errno);
		}
	  
	  bytes_written = write(STDOUT_FILENO, buf, bytes_read);
	  if (bytes_written == -1)
		{
		  perror("write");
		  exit(errno);
		}
	  else if (bytes_written != bytes_read)
		{
		  fprintf(stderr, "unsuccessful write\n");
		  exit(EXIT_FAILURE);
		}
	}

  if (close(STDOUT_FILENO) || fclose(stderr))
	{
	  perror("close");
	  exit(errno);
	}
  
  return 0;
}

  
