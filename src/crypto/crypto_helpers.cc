#include <gea/API.h>

#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>



#include "crypto_helpers.h"

using namespace gea;
using namespace std;


bool readKeyFromFile(const char* filename, char newkey[16]) {

    int fd, ret;
    char buf[128];
    //    char newkey[16];

    fd = open(filename, O_RDONLY);
    if (fd < 0) {
	GEA.dbg() << "error while opening key file '" << filename
		  << "':" << strerror(fd) <<  std::endl;
    }
    //try to read 32 characters, which are the hex representation of the key
    ret = read(fd, buf, 128);
    buf[ret]='\0'; // terminate string for printf debugging.

    if (ret < 16) {
	GEA.dbg() << "no valid key information found in "
		  << filename << std::endl;
    }

    bool parse_error = false;
    if (ret >= 32) {
	// it may be the hex key.
	// now we must parse the array.

	const char *parse_pos=buf;

	for (int i = 0; i < 16; ++i) {
	    unsigned  v;
	    int err;
	    while( *parse_pos == ' ')
		parse_pos++;

	    //	    printf("parse string is '%s'\n", parse_pos);

	    err = sscanf(parse_pos ,"%2x", &v);
	    parse_pos +=2;
	    //printf("read value %x\n", v);
	    newkey[i] = static_cast<char>(v);
	    if (err != 1) {
		//printf("PARSE ERROR!\n");
		parse_error = true;
		break;
	    }
	}

    }
    if (parse_error ||  (ret < 32) ) {
	GEA.dbg() << "assuming key information in "
		  << filename << " being the key in binary format" << std::endl;
	memcpy(newkey, buf, 16);
    }

    close(fd);

    GEA.dbg() << "using key ";
    for (int i = 0; i < 16; ++i) {
	GEA.dbg() << std::hex << ((unsigned)(unsigned char)newkey[i]) << std::dec;
    }
    GEA.dbg() << std::endl;

    return true;
}


void getRandomByte(void *dest, size_t num) {
    int ret;
    int fd;

    char * cdest = static_cast<char *>(dest);
    fd = open("/dev/urandom", O_RDONLY);
    assert(fd >= 0);

    while (num) {
	do {
	    ret = read(fd, cdest, num);
	} while ( (ret == -1) && (errno == EINTR));

	assert(ret >= 0);
	num -= ret;
	cdest += num;
    }
    close(fd);

}

/* This stuff is for emacs
 * Local variables:
 * mode:c++
 * c-basic-offset: 4
 * End:
 */
