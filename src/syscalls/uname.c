#include <uname.h>
#include <version.h>
#include <string.h>
#include <memory.h>

char * uname_sysname = "LemonOS";
char * uname_nodename = "lemonos";
char * uname_release;
char * uname_version = "v2";
char * uname_machine = "i386";
char * uname_domainname = "(none)";

static void strcat_dot(char * d, char * s) {
	strcat(d, s);
	strcat(d, ".");
}

void uname(uname_t * name) {
	strcpy(name->sysname, uname_sysname);
	strcpy(name->nodename, uname_nodename);
	strcpy(name->release, uname_release);
	strcpy(name->version, uname_version);
	strcpy(name->machine, uname_machine);
	strcpy(name->domainname, uname_domainname);
}

// lol what
void uname_init() {
	char edition_buffer[64];
	char major_buffer[64];
	char minor_buffer[64];
	char patch_buffer[64];
	size_t required_size = 0;
	ulldtoa(ver_edition, edition_buffer, 10);
	ulldtoa(ver_major, major_buffer, 10);
	ulldtoa(ver_minor, minor_buffer, 10);
	ulldtoa(ver_patch, patch_buffer, 10);
	required_size += strlen(os_name);
	required_size += strlen(edition_buffer);
	required_size += strlen(major_buffer);
	required_size += strlen(minor_buffer);
	required_size += strlen(patch_buffer);
	required_size += 8;
	uname_release = malloc(required_size);
	*uname_release = 'v';
	strcat_dot(uname_release + 1, edition_buffer); // add these with a dot at the end
	strcat_dot(uname_release, major_buffer);
	strcat_dot(uname_release, minor_buffer);
	strcat(uname_release, patch_buffer);
	strcat(uname_release, " (");
	strcat(uname_release, os_name);
	strcat(uname_release, ")");
	// uname_release should now contain "vX.X.X.X (Name)"
}
