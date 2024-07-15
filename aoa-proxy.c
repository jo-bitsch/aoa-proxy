/**
 * @file aoa-proxy.c
 * @author Jó Ágila Bitsch (jgilab@gmail.com)
 * @brief Interact with Android devices using the Android Open Accessory
 * Protocol.
 */

#define _GNU_SOURCE
#include <argp.h>
#include <libusb-1.0/libusb.h>
#include <poll.h>
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#if __has_include(<b64/cdecode.h>) || ! defined (NO_HID)
  #include <b64/cdecode.h>
  #define HAS_HID 1
#endif
#include <sys/param.h>
#include <sys/socket.h>
#include <netdb.h>

#if __has_include("version.h")
#include "version.h"
#else
#ifndef GIT_VERSION
#define GIT_VERSION "unknown"
#endif
#endif

#define PORT_NUMBERS_LEN 8

const char *argp_program_version = "aoa-proxy " GIT_VERSION;
const char *argp_program_bug_address = "https://github.com/jo-bitsch/aoa-proxy/issues";
static char doc[] =
    "Interact with Android devices using the Android Open Accessory protocol"
    "\vThis program aims to make working with Android Open Accessory (in "
    "particular v1) as easy as possible. "
    "It has 2 modes, depending on the state of the attached Android device.\n"
    "  (1) Announce mode: When the Android device is not yet in AOA mode, this "
    "program announces its identity "
    " to the Android device and tries to put it into AOA mode.\n"
    "  (2) Forwarding mode: Forward all input from stdin to the AOA device and "
    "all output from AOA to stdout."
    "\n\n"
    "This is best called automatically from udev to react to the hotplugging "
    "of devices. "
    "Once AOA mode is triggered, the USB device reattaches itself, so the "
    "device reappears on the bus using a different devnum, but the same portnum. "
    "Because of this reattachment, this program needs to be started again."
    "\n\n"
    "For the AOA protocol specification, check out: "
    "https://source.android.com/devices/accessories/protocol"
    ;

static struct argp_option options[] = {
    {0, 0, 0, 0, "Device selection options", 0},
    {"port", 'p', "BUSNUM-PORTNUMS", 0, "Connect to this USB device. e.g. \"2-2\"", 0},
    {0, 0, 0, 0, "Possible actions", 0},
    {"announce", 'a', 0, 0,
     "Announce AOA to the device.", 0},
    {"forward", 'f', 0, 0,
     "Forward stdin to AOA device and forward AOA device to stdout.", 0},
#ifdef HAS_HID
    {"hid",'y', 0, 0, 
     "send HID events instead (first line: base64 encoded descriptor, next lines: base64 encoded events", 0},
#endif
    {0, 0, 0, 0, "Announce options", 0},
    {"audio", 'A', 0, 0,
     "enable audio interface for AOAv2. (default: false)", 0},
    {"manufacturer", 'm', "MANUFACTURER", 0,
     "Who produced the accessory. (default: aoa-proxy)", 0},
    {"model", 'M', "MODEL", 0,
     "What accessory model ist this? (default: generic-device)", 0},
    {"model-version", 'v', "VERSION", 0,
     "What version is this accessory? (default: 0.1)", 0},
    {"serial", 's', "SERIAL", 0,
     "What's the serial number of this accesory? (default: \"\"", 0},
    {"description", 'd', "DESCRIPTION", 0,
     "Display string for the default Android UI. (default: \"\")", 0},
    {"url", 'u', "URL", 0, "Where to get more information? (default: \"\")", 0},
    {0, 0, 0, 0, "Forwarding only options", 0},
    {"wait", 'w', 0, 0,
     "Wait for first byte from AOA device before forwarding input from stdin. "
     "(default: false)", 0},
    {"connect", 'c', "PORT", 0,
     "Connect to a tcp port on localhost and forward AOA traffic via network instead of stdio. "
     "(default: \"\")", 0},
    {0, 0, 0, 0, "Forwarding/HID options", 0},
    {"reset-on-exit", 'r', 0, 0,
     "leave AOA mode on exit from forwarding."
     "(default: false)", 0},
    {0}};

struct arguments {
  int busnum;
  uint8_t portnums[PORT_NUMBERS_LEN];
  char *manufacturer, *model, *version, *serial, *description, *url;
  bool audio;
  bool wait;
  bool reset;
  bool announce;
  bool forward;
  char *connect;
#ifdef HAS_HID
  bool hid;
#endif
};

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
  struct arguments *arguments = state->input;
  char *p;
  switch (key) {
  case 'p':
    p=arg;
    arguments->busnum = atoi(p);
    if (arguments->busnum < 0 || arguments->busnum > 255) {
      argp_error(state, "only values between 0 and 255 are allowed for busnum");
    }
    p=strchr(p, '-')+1;
    if(p==(char*)1){
      argp_error(state, "follow the format BUSNUM-PORTNUMS");
    }
    for(int i = 0; p!=(char*)1; p=strchr(p, '.')+1){
      if(i>=7 || i>=PORT_NUMBERS_LEN){
        argp_error(state, "portnums current maximum depth per USB3.0 specs is 7");
        break;
      }
      int num = atoi(p);
      if (num < 1 || num > 255) {
        argp_error(state, "only values between 1 and 255 are allowed for portnums");
        break;
      }

      arguments->portnums[i] = num;
      i+=1;
    }
    break;

  case 'm':
    if(strlen(arg)>255){
      argp_error(state,"manufacturer string is too long");
    }
    arguments->manufacturer = arg;
    break;
  case 'M':
    if(strlen(arg)>255){
      argp_error(state,"model string is too long");
    }
    arguments->model = arg;
    break;
  case 'v':
      if(strlen(arg)>255){
      argp_error(state,"version string is too long");
    }
    arguments->version = arg;
    break;
  case 's':
    if(strlen(arg)>255){
      argp_error(state,"serial string is too long");
    }
    arguments->serial = arg;
    break;
  case 'd':
    if(strlen(arg)>255){
      argp_error(state,"description string is too long");
    }
    arguments->description = arg;
    break;
  case 'u':
    if(strlen(arg)>255){
      argp_error(state,"url string is too long");
    }
    arguments->url = arg;
    break;
  case 'A':
    arguments->audio = true;
    break;
  case 'a':
    arguments->announce = true;
    break;
  case 'f':
    arguments->forward = true;
    break;
  case 'c':
    arguments->connect = arg;
    break;
  case 'r':
    arguments->reset = true;
    break;
  case 'w':
    arguments->wait = true;
    break;
#ifdef HAS_HID
  case 'y':
    arguments->hid = true;
    break;
#endif

  case ARGP_KEY_END:
    if (arguments->busnum == -1 || arguments->portnums[0] == 0) {
      argp_error(state, "port is required");
    }
    break;

  default:
    return ARGP_ERR_UNKNOWN;
  }
  return 0;
}

static struct argp argp = {options, parse_opt, 0, doc, NULL, NULL, NULL};

static libusb_device_handle *get_usb_device(uint8_t busnum, uint8_t* portnums) {
  libusb_device **devs;
  libusb_device *dev;
  libusb_device_handle *ret;
  ssize_t cnt;

  cnt = libusb_get_device_list(NULL, &devs);
  if (cnt < 0) {
    exit(cnt);
  }

  for (int i = 0; (dev = devs[i]) != NULL; i++) {
    if (busnum == libusb_get_bus_number(dev)) {
        uint8_t dev_portnums[PORT_NUMBERS_LEN];
        memset(dev_portnums, 0, PORT_NUMBERS_LEN);
        libusb_get_port_numbers(dev, dev_portnums, PORT_NUMBERS_LEN);
        bool same = true;
        for(uint8_t j=0; j<PORT_NUMBERS_LEN; j++){
          if(portnums[j]!=dev_portnums[j]){
            same = false;
            break;
          }
          if(portnums[j] == 0){
            // end of comparison
            break;
          }
        }
        if(same){
            // we found the device!
            break;
        }
    }
    dev = NULL;
  }

  if (dev == NULL) {
    fprintf(stderr, "device not found\n");
    libusb_free_device_list(devs, 1);
    libusb_exit(NULL);
    exit(ENOENT);
  }

  int r = libusb_open(dev, &ret);
  if (r != 0) {
    fprintf(stderr, "error opening the device: %s\n", libusb_error_name(r));
    libusb_free_device_list(devs, 1);
    libusb_exit(NULL);

    if (r==LIBUSB_ERROR_ACCESS) {
      // on Ubuntu, this package has the relevant udev rules: android-sdk-platform-tools-common
      fprintf(stderr, "Try running as root, or install udev rules, that make the device accessible to you.\n");
    }

    exit(EXIT_FAILURE);
  }

  libusb_free_device_list(devs, 1);

  return ret;
}

static bool is_device_in_AOA_mode(libusb_device_handle *dev) {
  struct libusb_device_descriptor desc;
  libusb_get_device_descriptor(libusb_get_device(dev), &desc);
  bool ret = false;
  // fprintf(stderr, "idVendor: %04x, idProduct: %04x\n", desc.idVendor, desc.idProduct);

  if (desc.idVendor != 0x18d1 || (desc.idProduct != 0x2d00 &&
      desc.idProduct != 0x2d01 && desc.idProduct != 0x2d02 &&
      desc.idProduct != 0x2d03 && desc.idProduct != 0x2d04 &&
      desc.idProduct != 0x2d05)) {
    ret = false;
  } else {
    ret = true;
  }

  return ret;
}

static void aoa_announce(libusb_device_handle *device,
                         struct arguments *arguments) {
  uint8_t buffer[256];
  uint16_t aoa_version = 0;
  int r = 0;
  buffer[sizeof(buffer)-1] = 0;


  r = libusb_control_transfer(device,
                          LIBUSB_REQUEST_TYPE_VENDOR |
                              LIBUSB_TRANSFER_TYPE_CONTROL |
                              LIBUSB_ENDPOINT_IN,
                          51, 0, 0, buffer, 2, 1000);
  if(r==LIBUSB_ERROR_PIPE){
    fprintf(stderr, "device does not support AOA mode (control request was not supported by the device)\n");
    return;
  }else if(r<0){
    fprintf(stderr, "device responded to AOA version request control transfer with an error(%d): %s\n", r, libusb_error_name(r));
    return;
  }
  aoa_version = buffer[0] + (buffer[1]<<8);
  if (aoa_version != 1 && aoa_version != 2) {
    fprintf(stderr, "device does not support AOA mode (version returned should be in [1, 2], but is: %d)\n", aoa_version);
    return;
  }
  fprintf(stderr, "device supports AOAv%d\n", aoa_version);
  if (strnlen(arguments->manufacturer, sizeof(buffer)-1)!=0 && strnlen(arguments->model, sizeof(buffer)-1)!=0){
    strncpy((char *)buffer, arguments->manufacturer, sizeof(buffer) - 1);
    libusb_control_transfer(device,
                            LIBUSB_REQUEST_TYPE_VENDOR |
                                LIBUSB_TRANSFER_TYPE_CONTROL |
                                LIBUSB_ENDPOINT_OUT,
                            52, 0, 0, buffer, strlen((char *)buffer) + 1, 0);

    strncpy((char *)buffer, arguments->model, sizeof(buffer) - 1);
    libusb_control_transfer(device,
                            LIBUSB_REQUEST_TYPE_VENDOR |
                                LIBUSB_TRANSFER_TYPE_CONTROL |
                                LIBUSB_ENDPOINT_OUT,
                            52, 0, 1, buffer, strlen((char *)buffer) + 1, 0);

    strncpy((char *)buffer, arguments->description, sizeof(buffer) - 1);
    libusb_control_transfer(device,
                            LIBUSB_REQUEST_TYPE_VENDOR |
                                LIBUSB_TRANSFER_TYPE_CONTROL |
                                LIBUSB_ENDPOINT_OUT,
                            52, 0, 2, buffer, strlen((char *)buffer) + 1, 0);

    strncpy((char *)buffer, arguments->version, sizeof(buffer) - 1);
    libusb_control_transfer(device,
                            LIBUSB_REQUEST_TYPE_VENDOR |
                                LIBUSB_TRANSFER_TYPE_CONTROL |
                                LIBUSB_ENDPOINT_OUT,
                            52, 0, 3, buffer, strlen((char *)buffer) + 1, 0);

    strncpy((char *)buffer, arguments->url, sizeof(buffer) - 1);
    libusb_control_transfer(device,
                            LIBUSB_REQUEST_TYPE_VENDOR |
                                LIBUSB_TRANSFER_TYPE_CONTROL |
                                LIBUSB_ENDPOINT_OUT,
                            52, 0, 4, buffer, strlen((char *)buffer) + 1, 0);


    strncpy((char *)buffer, arguments->serial, sizeof(buffer)-1);
    libusb_control_transfer(device,
                            LIBUSB_REQUEST_TYPE_VENDOR |
                                LIBUSB_TRANSFER_TYPE_CONTROL |
                                LIBUSB_ENDPOINT_OUT,
                            52, 0, 5, buffer, strlen((char *)buffer) + 1, 0);
  }

  if(aoa_version==2 && arguments->audio){
    libusb_control_transfer(device,
                            LIBUSB_REQUEST_TYPE_VENDOR |
                                LIBUSB_TRANSFER_TYPE_CONTROL |
                                LIBUSB_ENDPOINT_OUT,
                            58, 1, 0, NULL, 0, 0);
  }

  libusb_control_transfer(device,
                          LIBUSB_REQUEST_TYPE_VENDOR |
                              LIBUSB_TRANSFER_TYPE_CONTROL |
                              LIBUSB_ENDPOINT_OUT,
                          53, 0, 0, NULL, 0, 0);
}

static void stdin_to_aoa_cb(struct libusb_transfer *transfer) {
  if (transfer->user_data != NULL) {
    *((ssize_t *)transfer->user_data) = 0;
  }
}
static void aoa_to_stdout_cb(struct libusb_transfer *transfer) {
  if (transfer->user_data != NULL) {
    *((ssize_t *)transfer->user_data) = transfer->actual_length;
  }
}

static void signal_handler(__attribute__ ((unused)) int sig) {}

static void aoa_cat(libusb_device_handle *device, struct arguments *arguments) {
  libusb_device *dev = libusb_get_device(device);

  int r = libusb_set_auto_detach_kernel_driver(device, 1);
  if (r != 0) {
    fprintf(stderr,
            "error setting auto_detach for kernel driver for the device: %s\n",
            libusb_error_name(r));
    libusb_exit(NULL);
    exit(EXIT_FAILURE);
  }
  r = libusb_claim_interface(device, 0);
  if (r != 0) {
    fprintf(stderr, "error claiming the interface of the device: %s\n",
            libusb_error_name(r));
    libusb_exit(NULL);
    exit(EXIT_FAILURE);
  }

  struct libusb_config_descriptor *config = NULL;
  libusb_get_active_config_descriptor(dev, &config);
  uint16_t max_packet_size =
      config->interface[0].altsetting[0].endpoint[0].wMaxPacketSize;

  struct libusb_transfer *AOA_to_stdout = libusb_alloc_transfer(0);
  struct libusb_transfer *stdin_to_AOA = libusb_alloc_transfer(0);

  uint8_t buffer_from_stdin[max_packet_size];
  ssize_t buffer_from_stdin_len = 0;

  uint8_t buffer_from_aoain[max_packet_size];
  ssize_t buffer_from_aoain_len = 0;

  libusb_fill_bulk_transfer(AOA_to_stdout, device, 0x81, buffer_from_aoain,
                            max_packet_size, aoa_to_stdout_cb,
                            &buffer_from_aoain_len, 0);
  libusb_submit_transfer(AOA_to_stdout);

  bool aoa_sent_already = !(arguments->wait);

  int fd_in = STDIN_FILENO;
  int fd_out = STDOUT_FILENO;

  if (strlen(arguments->connect)>0)
  {
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int s, sfd;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = 0;
    hints.ai_protocol = 0;

    s = getaddrinfo("localhost", arguments->connect, &hints, &result);
    if (s !=0 ) {
      fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
      libusb_exit(NULL);
      exit(EXIT_FAILURE);
    }

    for (rp = result; rp != NULL; rp = rp->ai_next) {
      sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);

      if (sfd == -1)
        continue;
      if (connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1)
        break;  // Success
      
      close(sfd);
    }
    if (rp == NULL) {
      fprintf(stderr, "Could not connect\n");
      libusb_exit(NULL);
      exit(EXIT_FAILURE);
    }

    freeaddrinfo(result);

    // sfd contains the open socket file descriptor
    fd_in = sfd;
    fd_out = sfd;
  }

  while (1) {
    if (buffer_from_stdin_len < 0 || buffer_from_aoain_len < 0) {
      goto exiting;
    }

    // fill pollfd list
    const struct libusb_pollfd **usb_fds = libusb_get_pollfds(NULL);
    size_t num_pollfd = 0;

    for (int i = 0; usb_fds[i] != NULL; i++) {
      num_pollfd++;
    }
    if (aoa_sent_already && buffer_from_stdin_len == 0) {
      num_pollfd++;
    }
    if (buffer_from_aoain_len > 0) {
      num_pollfd++;
    }

    struct pollfd fds[num_pollfd];

    size_t j = 0;

    for (int i = 0; usb_fds[i] != NULL; i++) {
      fds[j].fd = usb_fds[i]->fd;
      fds[j].events = usb_fds[i]->events;
      j++;
    }
    libusb_free_pollfds(usb_fds);

    if (aoa_sent_already && buffer_from_stdin_len == 0) {
      fds[j].fd = fd_in;
      fds[j].events = POLLIN;
      j++;
    }

    if (buffer_from_aoain_len > 0) {
      fds[j].fd = fd_out;
      fds[j].events = POLLOUT;
      j++;
    }

    struct timeval timeout;
    if (!libusb_get_next_timeout(NULL, &timeout)) {
      // no pending timeouts, call with 1 sec instead.
      timeout.tv_sec = 1;
      timeout.tv_usec = 0;
    }
    struct timespec tmo;
    tmo.tv_sec = timeout.tv_sec;
    tmo.tv_nsec = timeout.tv_usec * 1000000;

    sigset_t emptyset, blockset;
    struct sigaction sa;
    sigemptyset(&blockset);
    sigaddset(&blockset, SIGINT);
    sigaddset(&blockset, SIGTERM);
    sigprocmask(SIG_BLOCK, &blockset, NULL);

    sa.sa_handler = signal_handler;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);

    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    sigemptyset(&emptyset);

    // poll
    int r = ppoll(fds, num_pollfd, &tmo, &emptyset);
    if (r < 0) {
      if (errno = EINTR) {
        // a signal (SIGINT or SIGTERM)
        fprintf(stderr, "SIGINT or SIGTERM received: terminating\n");
      } else {
        // an error occured
        fprintf(stderr, "an error occured %d: %s\n", errno, strerror(errno));
      }
      goto exiting;
    }

    if (r == 0) {
      // timeout
      struct timeval zero_tv = {0, 0};
      libusb_handle_events_timeout(NULL, &zero_tv);
      continue;
    }

    // check which one fired
    for (size_t i = 0; i < num_pollfd; i++) {
      if (fds[i].fd == fd_in && fds[i].revents & POLLIN) {
        // reading from stdin possible
        buffer_from_stdin_len =
            read(fd_in, buffer_from_stdin, max_packet_size);
        if (buffer_from_stdin_len <= 0) {
          goto exiting;
        }
        // fprintf(stderr, "read %ld bytes from stdin\n",
        // buffer_from_stdin_len);
        libusb_fill_bulk_transfer(stdin_to_AOA, device, 0x1, buffer_from_stdin,
                                  buffer_from_stdin_len, stdin_to_aoa_cb,
                                  &buffer_from_stdin_len, 0);
        libusb_submit_transfer(stdin_to_AOA);
      } else if (fds[i].fd == fd_out && fds[i].revents & POLLOUT) {
        // writing to stdout possible
        // fprintf(stderr, "read %ld bytes from aoa\n", buffer_from_aoain_len);
        ssize_t b =
            write(fd_out, buffer_from_aoain, buffer_from_aoain_len);
        if (b != buffer_from_aoain_len) {
          fprintf(stderr, "could not write out the complete AOA buffer to "
                          "stdout. Exiting...\n");
          goto exiting;
        }
        buffer_from_aoain_len = 0;
        aoa_sent_already = true;

        libusb_fill_bulk_transfer(AOA_to_stdout, device, 0x81,
                                  buffer_from_aoain, max_packet_size,
                                  aoa_to_stdout_cb, &buffer_from_aoain_len, 0);
        libusb_submit_transfer(AOA_to_stdout);
      } else {
        // handle usb events
        struct timeval zero_tv = {0, 0};
        libusb_handle_events_timeout(NULL, &zero_tv);
      }
    }
  }

exiting:
  if (fd_in == fd_out) {
    close(fd_in);
  }
}

#ifdef HAS_HID
static void aoa_hid(libusb_device_handle *device, __attribute__ ((unused)) struct arguments *arguments) {
  char *line = NULL;
  size_t len = 0;
  ssize_t nread = 0;


  uint8_t hid_index = 0;
  int r = 0;
  
  struct libusb_device_descriptor dev_desc;
  libusb_get_device_descriptor(libusb_get_device(device), &dev_desc);
  uint16_t max_packet_size = dev_desc.bMaxPacketSize0;
//  fprintf(stderr, "wMaxPacketSize: %d\n", max_packet_size);

  base64_decodestate state;

  nread = getline(&line, &len, stdin);
  ssize_t max_allowed_line_length = nread;
  char binary_line[max_allowed_line_length + 2];  // this ensures, that there is always enough space for the decoded string,
                                                  // even in the degenerate case of very short malicious encoded lines.

  base64_init_decodestate(&state);
  ssize_t binary_len = base64_decode_block(line, nread, binary_line, &state);

  if( binary_len > UINT16_MAX) {
    free(line);
    fprintf(stderr, "HID descriptor too big for AOA (length: %ld, max allowed size: %d)\n", binary_len, UINT16_MAX);
    return;
  }

  libusb_control_transfer(device,
                            LIBUSB_REQUEST_TYPE_VENDOR |
                                LIBUSB_TRANSFER_TYPE_CONTROL |
                                LIBUSB_ENDPOINT_OUT,
                            54, hid_index, binary_len, (unsigned char*)binary_line, 0, 0);

  for(ssize_t offset=0; offset < binary_len; offset += max_packet_size){
    libusb_control_transfer(device,
                              LIBUSB_REQUEST_TYPE_VENDOR |
                                  LIBUSB_TRANSFER_TYPE_CONTROL |
                                  LIBUSB_ENDPOINT_OUT,
                              56, hid_index, offset, (unsigned char*)binary_line + offset, MIN(binary_len - offset, max_packet_size), 0);
  }
  
  fprintf(stderr, "registered HID device (len=%ld)\n", binary_len);
  fflush(stderr);

  usleep(100000);

  while( 0 <= (nread = getline(&line, &len, stdin))){
    if(nread>max_allowed_line_length){
      fprintf(stderr, "base64 encoded event size too long (length: %ld, max_allowed_line_length: %ld)\n", nread, max_allowed_line_length);
      break;
    }
    base64_init_decodestate(&state);
    binary_len = base64_decode_block(line, nread, binary_line, &state);
    if(binary_len > max_packet_size){
      fprintf(stderr, "event size too big for AOA (length: %ld, max_packet_size: %d)\n", binary_len, max_packet_size);
      break;
    }
    r = libusb_control_transfer(device,
                              LIBUSB_REQUEST_TYPE_VENDOR |
                                  LIBUSB_TRANSFER_TYPE_CONTROL |
                                  LIBUSB_ENDPOINT_OUT,
                              57, hid_index, 0, (unsigned char*)binary_line, binary_len, 0);

     if(r<0){
      fprintf(stderr, "error: %s\n", libusb_error_name(r));
    }

  }

  libusb_control_transfer(device,
                            LIBUSB_REQUEST_TYPE_VENDOR |
                                LIBUSB_TRANSFER_TYPE_CONTROL |
                                LIBUSB_ENDPOINT_OUT,
                            55, hid_index, 0, (unsigned char*)binary_line, 0, 0);

  free(line);
}
#endif  // HAS_HID

static void aoa_reset(libusb_device_handle *device,
                      __attribute__ ((unused)) struct arguments *arguments) {
  int r = libusb_reset_device(device);
  if (r != 0 && r != LIBUSB_ERROR_NOT_FOUND) {
    fprintf(stderr, "error resetting the device: %s\n", libusb_error_name(r));
  }
}

int main(int argc, char *argv[]) {
  struct arguments arguments;

  arguments.busnum = -1;
  memset(arguments.portnums, 0, PORT_NUMBERS_LEN);
  arguments.manufacturer = "aoa-proxy";
  arguments.model = "generic-device";
  arguments.version = "0.1";
  arguments.serial = "";
  arguments.url = "";
  arguments.description = "";
  arguments.reset = false;
  arguments.wait = false;
  arguments.audio = false;
#ifdef HAS_HID
  arguments.hid = false;
#endif
  arguments.announce = false;
  arguments.forward = false;
  arguments.connect = "";

  argp_parse(&argp, argc, argv, 0, 0, &arguments);

  if (0 > libusb_init(NULL)) {
    fprintf(stderr, "libusb_init failed\n");
    exit(-1);
  }

  libusb_device_handle *dev =
      get_usb_device((uint8_t)arguments.busnum, arguments.portnums);

  if (!is_device_in_AOA_mode(dev) && arguments.announce) {
    aoa_announce(dev, &arguments);
  } else {
    if(arguments.announce){
      fprintf(stderr, "device already in AOA mode\n");
    }
    if(arguments.forward){
      aoa_cat(dev, &arguments);
    } else {
#ifdef HAS_HID
      if(arguments.hid){
        aoa_hid(dev, &arguments);
      }
#endif
      if (arguments.reset) {
        aoa_reset(dev, &arguments);
      }
    }
  }
  libusb_close(dev);

  libusb_exit(NULL);
  return EXIT_SUCCESS;
}
