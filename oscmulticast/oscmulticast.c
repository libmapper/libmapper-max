//
// oscmulticast.c
// a maxmsp and puredata external for handling multicast OSC
// http://www.idmil.org/software/mappingtools
// Joseph Malloch, IDMIL 2010
// License: LGPL
//

// *********************************************************
// -(Includes)----------------------------------------------

#ifdef MAXMSP
	#include "ext.h"			// standard Max include, always required
	#include "ext_obex.h"		// required for new style Max object
	#include "ext_dictionary.h"
	#include "jpatcher_api.h"
#else
	#include "m_pd.h"
    #define A_SYM A_SYMBOL
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>
#include "lo/lo.h"

#define INTERVAL 1
#define MAXSIZE 256

// *********************************************************
// -(object struct)-----------------------------------------
typedef struct _oscmulticast
{
	t_object ob;
    void *outlets[3];
    char *iface_pref;
    char *iface;
    char *group;
    char port[10];
    lo_server servers[2];
    lo_address address;
    void *clock;          // pointer to clock object
	t_atom buffer[MAXSIZE];
} t_oscmulticast;

static char *char_buffer;
static int char_buffer_len = 0;

// *********************************************************
// -(function prototypes)-----------------------------------
static void *oscmulticast_new(t_symbol *s, int argc, t_atom *argv);
static void oscmulticast_free(t_oscmulticast *x);
static void oscmulticast_group(t_oscmulticast *x, t_symbol *s, int argc, t_atom *argv);
static void oscmulticast_port(t_oscmulticast *x, t_symbol *s, int argc, t_atom *argv);
static void oscmulticast_interface(t_oscmulticast *x, t_symbol *s, int argc, t_atom *argv);
static void oscmulticast_anything(t_oscmulticast *x, t_symbol *s, int argc, t_atom *argv);
static void oscmulticast_poll(t_oscmulticast *x);
static int multicast_handler(const char *path, const char *types, lo_arg ** argv,
                             int argc, void *data, void *user_data);
static int reply_handler(const char *path, const char *types, lo_arg ** argv,
                         int argc, void *data, void *user_data);
#ifdef MAXMSP
	static void oscmulticast_assist(t_oscmulticast *x, void *b, long m, long a, char *s);
#endif

static const char *maxpd_atom_get_string(t_atom *a);
static void maxpd_atom_set_string(t_atom *a, const char *string);
static void maxpd_atom_set_int(t_atom *a, int i);
static double maxpd_atom_get_float(t_atom *a);
static void maxpd_atom_set_float(t_atom *a, float d);

// *********************************************************
// -(global class pointer variable)-------------------------
static void *oscmulticast_class;

// *********************************************************
// -(main)--------------------------------------------------
#ifdef MAXMSP
int main(void)
{
    t_class *c;
    c = class_new("oscmulticast", (method)oscmulticast_new, (method)oscmulticast_free,
                  (long)sizeof(t_oscmulticast), 0L, A_GIMME, 0);
    class_addmethod(c, (method)oscmulticast_assist,    "assist",    A_CANT,  0);
    class_addmethod(c, (method)oscmulticast_group,     "group",     A_GIMME, 0);
    class_addmethod(c, (method)oscmulticast_port,      "port",      A_GIMME, 0);
    class_addmethod(c, (method)oscmulticast_interface, "interface", A_GIMME, 0);
    class_addmethod(c, (method)oscmulticast_anything,  "anything",  A_GIMME, 0);
    class_register(CLASS_BOX, c); /* CLASS_NOBOX */
    oscmulticast_class = c;
    return 0;
}
#else
int oscmulticast_setup(void)
{
    t_class *c;
    c = class_new(gensym("oscmulticast"), (t_newmethod)oscmulticast_new,
                  (t_method)oscmulticast_free, (long)sizeof(t_oscmulticast),
                  0L, A_GIMME, 0);
    class_addmethod(c, (t_method)oscmulticast_interface, gensym("interface"),
                    A_GIMME, 0);
    class_addanything(c, (t_method)oscmulticast_anything);
    oscmulticast_class = c;
    return 0;
}
#endif


/* Internal LibLo error handler */
static void handler_error(int num, const char *msg, const char *where)
{
    post("oscmulticast: liblo server error %d in path %s: %s\n", num, where, msg);
}

/*! Local function to get the IP address of a network interface. */
static int get_interface_addr(const char* pref, struct in_addr* addr,
                              char **iface)
{
    struct in_addr zero;
    struct sockaddr_in *sa;

    *(unsigned int *)&zero = inet_addr("0.0.0.0");

    struct ifaddrs *ifaphead;
    struct ifaddrs *ifap;
    struct ifaddrs *iflo=0, *ifchosen=0;

    if (getifaddrs(&ifaphead) != 0)
        return 1;

    ifap = ifaphead;
    while (ifap) {
        sa = (struct sockaddr_in *) ifap->ifa_addr;
        if (!sa) {
            ifap = ifap->ifa_next;
            continue;
        }

        if (sa->sin_family == AF_INET && ifap->ifa_flags & IFF_UP
            && memcmp(&sa->sin_addr, &zero, sizeof(struct in_addr))!=0) {
            ifchosen = ifap;
            if (pref && strcmp(ifap->ifa_name, pref)==0)
                break;
            else if (ifap->ifa_flags & IFF_LOOPBACK)
                iflo = ifap;
        }
        ifap = ifap->ifa_next;
    }

        // Default to loopback address in case user is working locally.
    if (!ifchosen)
        ifchosen = iflo;

    if (ifchosen) {
        if (*iface)
            free(*iface);
        *iface = strdup(ifchosen->ifa_name);
        sa = (struct sockaddr_in *) ifchosen->ifa_addr;
        *addr = sa->sin_addr;
        freeifaddrs(ifaphead);
        return 0;
    }

    freeifaddrs(ifaphead);

    return 2;
}

void startup(t_oscmulticast *x)
{
    if (!x->group || !x->port[0])
        return;

    if (x->address) {
        lo_address_free(x->address);
        x->address = NULL;
    }
    if (x->servers[0]) {
        lo_server_free(x->servers[0]);
        x->servers[0] = NULL;
    }

    /* Initialize interface information. */
    struct in_addr iface_ip;
    if (get_interface_addr(x->iface_pref, &iface_ip, &x->iface)) {
        post("oscmulticast: no interface found!\n");
        return;
    }
    else {
        post("oscmulticast: using interface '%s'.\n", x->iface);
    }

    /* Open address */
    x->address = lo_address_new(x->group, x->port);
    if (!x->address) {
        post("oscmulticast: could not create multicast address.");
        return;
    }

    /* Set TTL for packet to 1 -> local subnet */
    lo_address_set_ttl(x->address, 1);

    /* Specify the interface to use for multicasting */
    lo_address_set_iface(x->address, x->iface, 0);

    x->servers[0] = lo_server_new_multicast_iface(x->group, x->port, x->iface, 0,
                                                 handler_error);

    if (!x->servers[0]) {
        post("oscmulticast: could not create multicast server");
        lo_address_free(x->address);
        x->address = NULL;
        return;
    }

    // Create a server for receiving replies if necessary
    if (!x->servers[1])
        while (!(x->servers[1] = lo_server_new(0, handler_error))) {}

    // Disable liblo message queueing
    lo_server_enable_queue(x->servers[0], 0, 1);
    lo_server_enable_queue(x->servers[1], 0, 1);

    lo_server_add_method(x->servers[0], NULL, NULL, multicast_handler, x);
    lo_server_add_method(x->servers[1], NULL, NULL, reply_handler, x);

    if (!x->clock) {
#ifdef MAXMSP
        x->clock = clock_new(x, (method)oscmulticast_poll);	// Create the timing clock
#else
        x->clock = clock_new(x, (t_method)oscmulticast_poll);
#endif
        clock_delay(x->clock, INTERVAL);  // Set clock to go off after delay
    }
}

// *********************************************************
// -(new)---------------------------------------------------
void *oscmulticast_new(t_symbol *s, int argc, t_atom *argv)
{
	t_oscmulticast *x = NULL;
    int i;

#ifdef MAXMSP
    if ((x = object_alloc(oscmulticast_class))) {
        x->outlets[2] = listout((t_object *)x);
        x->outlets[1] = listout((t_object *)x);
        x->outlets[0] = listout((t_object *)x);
#else
    if (x = (t_oscmulticast *) pd_new(oscmulticast_class)) {
        x->outlets[0] = outlet_new(&x->ob, gensym("list"));
        x->outlets[1] = outlet_new(&x->ob, gensym("list"));
        x->outlets[2] = outlet_new(&x->ob, gensym("list"));
#endif

        x->address = NULL;
        x->servers[0] = NULL;
        x->servers[1] = NULL;
        x->clock = NULL;
        x->group = NULL;
        x->port[0] = '\0';
        x->iface_pref = NULL;
        x->iface = NULL;

        for (i = 0; i < argc; i++) {
            if (strcmp(maxpd_atom_get_string(argv+i), "@group") == 0) {
                if ((argv+i+1)->a_type == A_SYM) {
                    x->group = strdup(maxpd_atom_get_string(argv+i+1));
                    i++;
                }
            }
            else if (strcmp(maxpd_atom_get_string(argv+i), "@port") == 0) {
                if ((argv+i+1)->a_type == A_FLOAT) {
                    snprintf(x->port, 10, "%i", (int)maxpd_atom_get_float(argv+i+1));
                    i++;
                }
#ifdef MAXMSP
                else if ((argv+i+1)->a_type == A_LONG) {
                    snprintf(x->port, 10, "%i", (int)atom_getlong(argv+i+1));
                    i++;
                }
#endif
            }
            else if(strcmp(maxpd_atom_get_string(argv+i), "@interface") == 0) {
                if ((argv+i+1)->a_type == A_SYM) {
                    x->iface_pref = strdup(maxpd_atom_get_string(argv+i+1));
                    i++;
                }
            }
        }
        startup(x);
    }
	return (x);
}

// *********************************************************
// -(free)--------------------------------------------------
void oscmulticast_free(t_oscmulticast *x)
{
    if (x->clock) {
        clock_unset(x->clock);	// Remove clock routine from the scheduler
        clock_free(x->clock);		// Frees memory used by clock
    }
    if (x->servers[0]) {
        lo_server_free(x->servers[0]);
    }
    if (x->servers[1]) {
        lo_server_free(x->servers[1]);
    }
    if (x->address) {
        lo_address_free(x->address);
    }
    if (x->iface_pref) {
        free(x->iface_pref);
    }
    if (x->iface) {
        free(x->iface);
    }
    if (x->group) {
        free(x->group);
    }
}

// *********************************************************
// -(inlet/outlet assist - maxmsp only)---------------------
#ifdef MAXMSP
void oscmulticast_assist(t_oscmulticast *x, void *b, long m, long a, char *s)
{
	if (m == ASSIST_INLET) { // inlet
		sprintf(s, "Message to be sent to multicast group.");
	}
	else {	// outlet
        switch (a) {
            case 0:
                sprintf(s, "Messages from multicast group.");
                break;
            case 1:
                sprintf(s, "Messages sent directly to reply server.");
                break;
            case 2:
                sprintf(s, "URL of message origin.");
                break;
            default:
                sprintf(s, "Outlet %d.", (int)a);
                break;
        }
	}
}
#endif

// *********************************************************
// -(group)-------------------------------------------------
static void oscmulticast_group(t_oscmulticast *x, t_symbol *s, int argc, t_atom *argv)
{
    const char *group;

    if (argc < 1)
        return;
    if (argv->a_type != A_SYM)
        return;

    group = maxpd_atom_get_string(argv);
    if (x->group) {
        if (strcmp(x->group, group)==0)
            return;
        free(x->group);
    }
    x->group = strdup(group);

    startup(x);
}

// *********************************************************
// -(port)--------------------------------------------------
static void oscmulticast_port(t_oscmulticast *x, t_symbol *s, int argc, t_atom *argv)
{
    char port[10];

    if (argc < 1)
        return;
    if ((argv)->a_type == A_FLOAT) {
        snprintf(port, 10, "%i", (int)maxpd_atom_get_float(argv));
    }
#ifdef MAXMSP
    else if ((argv)->a_type == A_LONG) {
        snprintf(port, 10, "%i", (int)atom_getlong(argv));
    }
#endif

    if (x->port) {
        if (strcmp(x->port, port)==0)
            return;
    }
    strncpy(x->port, port, 10);
    
    startup(x);
}

// *********************************************************
// -(interface)---------------------------------------------
static void oscmulticast_interface(t_oscmulticast *x, t_symbol *s, int argc, t_atom *argv)
{
    const char *iface = 0;

    if (argc < 1)
        return;
    if (argv->a_type != A_SYM)
        return;

    iface = maxpd_atom_get_string(argv);
    if (x->iface_pref) {
        if (strcmp(x->iface_pref, iface)==0)
            return;
        free(x->iface_pref);
    }
    x->iface_pref = strdup(iface);

    startup(x);
}

// *********************************************************
// -(anything)----------------------------------------------
void oscmulticast_anything(t_oscmulticast *x, t_symbol *s, int argc, t_atom *argv)
{
    if (!x->address || !x->servers[1])
        return;

    lo_timetag tt;
    lo_timetag_now(&tt);
    lo_bundle b = lo_bundle_new(tt);

    lo_message m = lo_message_new();
    if (!m) {
        post("oscmulticast: error creating message!");
        return;
    }

    int i;
    for (i = 0; i < argc; i++) {
        switch ((argv + i)->a_type) {
			case A_FLOAT:
                lo_message_add_float(m, atom_getfloat(argv + i));
                break;
#ifdef MAXMSP
            case A_LONG:
                lo_message_add_int32(m, (int)atom_getlong(argv + i));
                break;
#endif
            case A_SYM: {
                const char *s = maxpd_atom_get_string(argv + i);
                if (s && 0 == strcmp(s, "True"))
                    lo_message_add_true(m);
                else if (s && 0 == strcmp(s, "False"))
                    lo_message_add_false(m);
                else
                    lo_message_add_string(m, s);
                break;
            }
        }
    }

    lo_bundle_add_message(b, s->s_name, m);
    lo_send_bundle_from(x->address, x->servers[1], b);
    lo_message_free(m);
}

// *********************************************************
// -(poll libmapper)----------------------------------------
void oscmulticast_poll(t_oscmulticast *x)
{
    int count = 0, status[2];

    if (x->servers[0]) {
        while (count < 10 && lo_servers_recv_noblock(x->servers, status, 2, 0)) {
            count++;
        }
    }

	clock_delay(x->clock, INTERVAL);  // Set clock to go off after delay
}

// *********************************************************
// -(OSC handlers)-------------------------------------------
int generic_handler(const char *path, const char *types, lo_arg ** argv,
                    int argc, lo_message msg, void *user_data, int outlet)
{
    t_oscmulticast *x = (t_oscmulticast *)user_data;
    int i, j;
    char my_string[2];

    j=0;

    if (!x->buffer) {
        post("oscmulticast: error receiving message!");
        return 0;
    }

    lo_address address = lo_message_get_source(msg);
    if (address) {
        maxpd_atom_set_string(x->buffer, lo_address_get_url(address));
        outlet_anything(x->outlets[2], gensym("symbol"), 1, x->buffer);
    }

    if (argc > MAXSIZE) {
        post("oscmulticast: truncating received message to %i elements!",
             MAXSIZE);
        argc = MAXSIZE;
    }

    for (i=0; i<argc; i++)
    {
        switch (types[i])
        {
            case 'i':
                maxpd_atom_set_int(x->buffer+j, argv[i]->i);
				++j;
                break;
            case 'h':
                maxpd_atom_set_int(x->buffer+j, argv[i]->h);
				++j;
                break;
            case 'f':
                maxpd_atom_set_float(x->buffer+j, argv[i]->f);
				++j;
                break;
            case 'd':
                maxpd_atom_set_float(x->buffer+j, (float)argv[i]->d);
				++j;
                break;
            case 's':
                maxpd_atom_set_string(x->buffer+j, (const char *)&argv[i]->s);
				++j;
                break;
            case 'S':
                maxpd_atom_set_string(x->buffer+j, (const char *)&argv[i]->s);
				++j;
                break;
            case 'c':
                snprintf(my_string, 2, "%c", argv[i]->c);
                maxpd_atom_set_string(x->buffer+j, (const char *)my_string);
				++j;
                break;
            case 'T':
                maxpd_atom_set_string(x->buffer+j, "True");
                ++j;
                break;
            case 'F':
                maxpd_atom_set_string(x->buffer+j, "False");
                ++j;
                break;
            case 't':
                //output timetag from a second outlet?
                break;
        }
    }

    outlet_anything(x->outlets[outlet], gensym((char *)path), j, x->buffer);
    return 0;
}

int multicast_handler(const char *path, const char *types, lo_arg ** argv,
                      int argc, lo_message msg, void *user_data)
{
    return generic_handler(path, types, argv, argc, msg, user_data, 0);
}

int reply_handler(const char *path, const char *types, lo_arg ** argv,
                  int argc, lo_message msg, void *user_data)
{
    return generic_handler(path, types, argv, argc, msg, user_data, 1);
}

// *********************************************************
// some helper functions for abtracting differences
// between maxmsp and puredata

const char *maxpd_atom_get_string(t_atom *a)
{
#ifdef MAXMSP
    return atom_getsym(a)->s_name;
#else
    return (a)->a_w.w_symbol->s_name;
#endif
}

void maxpd_atom_set_string(t_atom *a, const char *string)
{
#ifdef MAXMSP
    if (strchr(string, ',')) {
        int count = 0, count2 = 0;
        while (1) {
            if (string[count] == 0)
                break;
            if (string[count] == ',')
                count2++;
            count++;
        }
        if (strlen(string) > char_buffer_len) {
            char_buffer_len = strlen(string) * 2;
            char_buffer = realloc(char_buffer, char_buffer_len);
        }
        count = count2 = 0;
        while (1) {
            if (string[count] == 0) {
                char_buffer[count2] = 0;
                break;
            }
            if (string[count] == ',') {
                char_buffer[count2++] = '\\';
                char_buffer[count2] = ',';
            }
            else {
                char_buffer[count2] = string[count];
            }
            count++;
            count2++;
        }
        atom_setsym(a, gensym((char *)char_buffer));
    }
    else
        atom_setsym(a, gensym((char *)string));
#else
    SETSYMBOL(a, gensym(string));
#endif
}

void maxpd_atom_set_int(t_atom *a, int i)
{
#ifdef MAXMSP
    atom_setlong(a, (long)i);
#else
    SETFLOAT(a, (double)i);
#endif
}

double maxpd_atom_get_float(t_atom *a)
{
    return (double)atom_getfloat(a);
}

void maxpd_atom_set_float(t_atom *a, float d)
{
#ifdef MAXMSP
    atom_setfloat(a, d);
#else
    SETFLOAT(a, d);
#endif
}
