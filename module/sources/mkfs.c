#include <endian.h>
#include <ctype.h>
#include <fcntl.h>
#include <getopt.h>
#include <linux/fs.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>

#include "trfs/super.h"

#ifndef __linux__
  #error mkfs.trfs has only been tested on Linux so far.
#endif

#define LF "\n"
#define LFLF LF LF

#define MKFS_DEFAULT_BLOCK_SIZE_BITS 12u // 4096
#define MKFS_DEFAULT_BLOCK_SIZE (1u << MKFS_DEFAULT_BLOCK_SIZE_BITS)

#define eprintf(format, ...) fprintf(stderr, format, ##__VA_ARGS__)
#define MKFS_INFO(format, ...) printf(format "\n", ##__VA_ARGS__)
#define MKFS_ERROR(format, ...) eprintf("Error: " format "\n", ##__VA_ARGS__)
#define MKFS_WARNING(format, ...) eprintf("Warning: " format "\n", ##__VA_ARGS__)

struct mkfs_options {
  char const* device;
  uint32_t block_size;
  uint32_t blocks;
  bool verbose;
};

struct device_stats {
  uint64_t block_size;
  uint64_t size;
  int fd;
};

// ╦ ╦┌─┐┌─┐┌─┐┌─┐
// ║ ║└─┐├─┤│ ┬├┤
// ╚═╝└─┘┴ ┴└─┘└─┘

///
/// Returns the filename of the given path.
///
/// @pre path is null-terminated string.
///
static char const* filename(char const* path) {
  char const* filename = path;

  while (*path != 0x0) {
    if (*path == '/' && *(path + 1) != '/') {
      filename = path + 1;
    }

    ++path;
  }

  return filename;
}

///
/// Prints mkfs.trfs usage and then exit.
///
/// @pre argv0 may be NULL.
///
static void mkfs_usage(
  char const* const argv0, int const error
) {
  fprintf(
    error ? stderr : stdout,

    "Usage: %s [OPTIONS] DEVICE" LFLF
    "Description:" LFLF
    "  Create a TRFS file system in the given DEVICE." LFLF
    "  DEVICE can be a disk partition or a file." LFLF
    "Options:" LFLF
    "  -b, --block-size [BYTES]" LF
    "    File system's block size." LFLF
    "  -s, --blocks [N]" LF
    "    Number of blocks." LFLF
    "  -v, --verbose" LF
    "    Produce verbose ouput." LFLF
    "  -h, --help" LF
    "    Display help text and exit." LFLF
    "  BYTES may be followed by the following multiplicative suffixes:" LF
    "    K=1024, M=1024*1024, G=1024*1024*1024 (uppercase or lowercase)" LF

    // __FILE_NAME__ is not standard.
    // https://gcc.gnu.org/onlinedocs/cpp/Common-Predefined-Macros.html
    // https://gcc.gnu.org/onlinedocs/cpp/Standard-Predefined-Macros.html
    , argv0 ? filename(argv0) : __FILE_NAME__
  );

  exit(error);
}

// ╔═╗┌─┐┬─┐┌─┐┌─┐
// ╠═╝├─┤├┬┘└─┐├┤
// ╩  ┴ ┴┴└─└─┘└─┘

///
/// Given string may be followed by the following multiplicative suffixes:
///   - K = 1024
///   - M = 1024 * 1024
///   - G = 1024 * 1024 * 1024.
///
/// @pre string != NULL
///
static uint32_t mkfs_parse_number(char const* const string) {
  uint32_t number = 0u;

  for (char const* character = string; *character != 0x0; ++character) {
    if (isdigit(*character)) {
      number = (number * 10u) + (uint32_t) (*character - '0');
      continue;
    }

    switch (toupper(*character)) {
      case 'K': number *= 1024u; break;
      case 'M': number *= 1024u * 1024u; break;
      case 'G': number *= 1024u * 1024u * 1024u; break;
      default: {
        MKFS_WARNING(
          "Unrecognized character \'%c\' for \"%s\".",
          *character, string
        );
      };
    }

    break;
  }

  return number;
}

///
/// Parses command line arguments into the given `mkfs_options` structure.
///
/// @return False when an unexpected option is encountered.
///
/// @pre argv != NULL
/// @pre mkfs_options != NULL
///
static bool parse_mkfs_options(
  int const argc, char* const argv[],
  struct mkfs_options* const mkfs_options
) {
  int option = 0;
  static struct option options[] = {
    { "help", no_argument, NULL, 'h' },
    { "verbose", no_argument, NULL, 'v' },
    { "block-size", required_argument, NULL, 'b' },
    { "blocks", required_argument, NULL, 's' },
    { NULL, 0, NULL, 0 },
  };

  while (option >= 0) {
    option = getopt_long(argc, argv, "hvb:s:", options, NULL);

    if (option <= -1) {
      break;
    }

    switch (option) {
      // Verbose.
      case 'h': {
        mkfs_usage(argv[0], EXIT_SUCCESS);
        break;
      }

      // Verbose.
      case 'v': {
        mkfs_options->verbose = true;
        break;
      }

      // Block size.
      case 'b': {
        mkfs_options->block_size = mkfs_parse_number(optarg);
        break;
      }

      // Blocks.
      case 's': {
        mkfs_options->blocks = mkfs_parse_number(optarg);
        break;
      }

      // Unrecognized option.
      case '?': {
        // opterr is by default non-zero.
        // getopt_long() print an error on stderr.
        return false;
      }

      case ':': {
        // Should not happen.
        return false;
      }

      default: {
        // Should not happen.
        return false;
      }
    }
  }

  // Device.
  if (optind < argc) {
    mkfs_options->device = argv[optind];
  }

  return true;
}

// ╔═╗┬ ┬┌─┐┌─┐┬┌─
// ║  ├─┤├┤ │  ├┴┐
// ╚═╝┴ ┴└─┘└─┘┴ ┴

///
/// Returns true if the given value is a power of 2, false otherwise.
///
/// (Note that zero is not considered a power of two.)
///
static inline bool is_power_of_2(uint32_t n) {
  // #include <linux/log2.h>
  // https://graphics.stanford.edu/~seander/bithacks.html#DetermineIfPowerOf2
  return (n != 0 && ((n & (n - 1)) == 0));
}

///
/// Returns the multiplication between two values.
///
/// Exit if a multiplication overflow is detected.
///
static inline uint32_t checked_multiplication(uint32_t a, uint32_t b) {
  uint32_t n = a * b;

  // Is -fwrapv set by default?
  // If the compiler going to optimize things?
  if (a != 0u && n / a != b) {
    MKFS_ERROR("Multiplication overflow detected (%u x %u).", a, b);
    exit(EXIT_FAILURE);
  }

  return n;
}

///
/// Checks if command line options are valid.
///
/// On error, prints on stderr and returns false.
///
/// @pre options != NULL
/// @pre device != NULL
///
static bool check_mkfs_options(
  struct mkfs_options const* const options,
  struct device_stats* const device
) {
  int fd;
  struct stat stats;

  uint64_t device_size = 0u;
  uint64_t device_block_size = 0u;

  // 1. Check device.
  if (!options->device) {
    MKFS_ERROR("Device is missing.");
    return false;
  }

  if ((fd = open(options->device, O_RDWR)) <= -1) {
    perror("Error"); // Prints on stderr.
    return false;
  }

  if (fstat(fd, &stats) <= -1) {
    perror("Error fstat()");
    goto close_fd;
  }

  switch (stats.st_mode & S_IFMT) {
    case S_IFREG: {
      // File block size.
      if (stats.st_blksize >= 0) {
        // st_blksize is logical, i.e. = page size
        device_block_size = (uint64_t) stats.st_blksize;
      }

      // File size.
      if (stats.st_size >= 0) {
        device_size = (uint64_t) stats.st_size;
      }

      break;
    }

    case S_IFBLK: {
      // Device block size.
      // [/usr/include/linux/fs.h]:
      //   #define BLKPBSZGET _IO(0x12, 123)
      // [linux/source/block/ioctl.c]:
      //  case BLKPBSZGET: /* get block device physical block size */
      //    return put_uint(argp, bdev_physical_block_size(bdev));
      unsigned int local_block_size;
      if (ioctl(fd, BLKPBSZGET, &local_block_size) <= -1) {
        perror("Error ioctl(BLKPBSZGET)");
        goto close_fd;
      }

      // Safe copy.
      device_block_size = local_block_size;

      // Device size.
      // [/usr/include/linux/fs.h]:
      //   /* return device size in bytes (u64 *arg) */
      //   #define BLKGETSIZE64 _IOR(0x12, 114, size_t)
      if (ioctl(fd, BLKGETSIZE64, &device_size) <= -1) {
        perror("Error ioctl(BLKGETSIZE64)");
        goto close_fd;
      }

      break;
    }

    default: {
      MKFS_ERROR("Device is not a block device or a regular file.");
      goto close_fd;
    }
  }

  // 2. Check block size.
  if (options->block_size < 512) {
    MKFS_ERROR(
      "Block size (%u) cannot be smaller than 512 bytes.",
      options->block_size
    );
    goto close_fd;
  }

  long page_size = sysconf(_SC_PAGESIZE); // getpagesize()
  if (options->block_size > page_size) {
    MKFS_ERROR(
      "Block size (%u) cannot be greater than the page size (%ld).",
      options->block_size, page_size
    );
    goto close_fd;
  }

  if (!is_power_of_2(options->block_size)) {
    MKFS_ERROR(
      "Block size (%u) is not a power of 2.",
      options->block_size
    );
    goto close_fd;
  }

  if (options->block_size < device_block_size) {
    MKFS_ERROR(
      "Block size (%u) cannot be smaller than the device block size (%ld).",
      options->block_size, device_block_size
    );
    goto close_fd;
  }

  // 3. Check number of blocks.
  if (options->blocks < 2) {
    // Block 0: boot block.
    // Block 1: superblock.
    MKFS_ERROR(
      "Number of blocks (%u) cannot be smaller than 2.",
      options->blocks
    );
    goto close_fd;
  }

  if (checked_multiplication(options->blocks, options->block_size) >= device_size) {
    MKFS_ERROR(
      "Total number of blocks (%u x %u) exceeds the device size (%lu).",
      options->blocks, options->block_size, device_size
    );
    goto close_fd;
  }

  // TODO: Check is a filesystem is already mounted on device.
  // realpath(3), getline(3), /proc/mounts

  // 4. Return
  if (device != NULL) {
    device->block_size = device_block_size;
    device->size = device_size;
    device->fd = fd;
  }
  else if (close(fd) <= -1) {
    perror("Error close()");
  }

  return true;

close_fd:
  if (close(fd) <= -1) {
    perror("Error close()");
  }

  return false;
}

// ╔╦╗┬─┐┌─┐┌─┐
//  ║ ├┬┘├┤ └─┐
//  ╩ ┴└─└  └─┘

///
/// Writes given buffer at given offset.
///
/// @returns false if one system call fails.
///
/// @pre options != NULL
/// @pre device != NULL
/// @pre buffer != NULL
/// @pre device->fd >= 0
///
bool seek_and_write(
  struct mkfs_options const* const options,
  struct device_stats const* const device,
  void const* const buffer, size_t const size,
  off_t offset
) {
  if (options->verbose) {
    MKFS_INFO("lseek(offset = %lu)", offset);
  }

  if (lseek(device->fd, offset, SEEK_SET) <= -1) {
    perror("Error lseek(offset)");
    return false;
  }

  if (options->verbose) {
    int length = 0;
    #define LENGTH 64
    char hex_buffer[LENGTH] = { 0 };

    for (int i = 0; (size_t) i < size && length + 3 < LENGTH; ++i) {
      unsigned char byte = *(i + ((unsigned char*) buffer));
      #define HEX(x) (x) <= 9 ? (x) + '0' : (x) - 10 + 'A'
      hex_buffer[length++] = HEX((char) (byte / 16));
      hex_buffer[length++] = HEX((char) (byte % 16));
      hex_buffer[length++] = 0x20;
      #undef HEX
    }

    hex_buffer[length <= 0 ? 0 : length - 1] = 0x0;
    MKFS_INFO("write(size = %ld, buffer = %s...)", size, hex_buffer);
  }

  if (write(device->fd, buffer, size) <= -1) {
    perror("Error write(buffer)");
    return false;
  }

  return true;
}

///
/// Writes the superblock.
///
/// TODO: inodes table, inode bitmap
///
/// @pre options != NULL
/// @pre device != NULL
/// @pre is_power_of_two(options->block_size)
/// @pre device->fd >= 0
///
static bool make_file_system(
  struct mkfs_options const* const options,
  struct device_stats const* const device
) {
  struct trfs_super_block_info super_block = {
    .magic_number = TRFS_MAGIC_NUMBER,

    // Convert to big-endian for readability.
    .block_size = htobe32(options->block_size),
    .blocks = htobe32(options->blocks),
  };

  if (options->verbose) {
    printf(
      LF "Superblock:" LF
      "  Magic number: %.*s" LF
      "  Block size: %u" LF
      "  Blocks: %u" LFLF
      , TRFS_MAGIC_NUMBER_LENGTH
      , super_block.magic_number
      , be32toh(super_block.block_size)
      , be32toh(super_block.blocks)
    );
  }

  // 1. Skip first block.
  for (uint32_t offset = 512; offset < options->block_size; offset <<=1) {
    // As the block size is configurable, the superblock will have to be found
    // between 512 and PAGE_SIZE bytes. To avoid false positives when searching
    // for the magic number, we explicitly reset it on potential locations.
    uint8_t magic_number[TRFS_MAGIC_NUMBER_LENGTH] = "Continue";
    bool success = seek_and_write(
      options, device,
      &magic_number, sizeof(magic_number),
      offset
    );

    if (!success) {
      return false;
    }
  }

  // 2. Write superblock.
  bool success = seek_and_write(
    options, device,
    &super_block, sizeof(super_block),
    options->block_size
  );

  if (!success) {
    return false;
  }

  MKFS_INFO("Done.");

  return true;
}

// ╔╦╗┌─┐┬┌┐┌
// ║║║├─┤││││
// ╩ ╩┴ ┴┴┘└┘

int main(int const argc, char* const argv[]) {
  struct device_stats device;
  struct mkfs_options options = {
    .device = NULL,
    .block_size = MKFS_DEFAULT_BLOCK_SIZE,
    .blocks = 0u,
    .verbose = false,
  };

  if (!parse_mkfs_options(argc, argv, &options)) {
    mkfs_usage(argv[0], EXIT_FAILURE);
  }

  if (!check_mkfs_options(&options, &device)) {
    mkfs_usage(argv[0], EXIT_FAILURE);
  }

  MKFS_INFO("Device: %s", options.device);

  if (options.verbose) {
    MKFS_INFO("Device size: %lu", device.size);
    MKFS_INFO("Device block size: %lu", device.block_size);
  }

  MKFS_INFO("Filesystem block size: %u", options.block_size);
  MKFS_INFO("Filesystem number of block: %u", options.blocks);

  make_file_system(&options, &device);

  if (close(device.fd) <= -1) {
    perror("Error close()");
  }

  return EXIT_SUCCESS;
}
