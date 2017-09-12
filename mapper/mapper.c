//
// mapper.c
// a maxmsp and puredata external encapsulating the functionality of libmapper
// http://www.idmil.org/software/libmapper
// Joseph Malloch, IDMIL 2010
//
// This software was written in the Input Devices and Music Interaction
// Laboratory at McGill University in Montreal, and is copyright those
// found in the AUTHORS file.  It is licensed under the GNU Lesser Public
// General License version 2.1 or later.  Please see COPYING for details.
//

// *********************************************************
// -(Includes)----------------------------------------------

#ifdef MAXMSP
    #include "ext.h"            // standard Max include, always required
    #include "ext_obex.h"       // required for new style Max object
    #include "ext_dictionary.h"
    #include "jpatcher_api.h"
#else
    #include "m_pd.h"
    #define A_SYM A_SYMBOL
#endif
#include <mapper/mapper.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <lo/lo.h>
#ifndef WIN32
  #include <arpa/inet.h>
#endif

#include <unistd.h>

#define INTERVAL 1
#define MAX_LIST 256

#ifdef MAXMSP
#define POST(x, ...) { object_post((t_object *)x, __VA_ARGS__); }
#else
#define POST(x, ...) { post(__VA_ARGS__); }
#endif

// *********************************************************
// -(object struct)-----------------------------------------
typedef struct _mapper
{
    t_object ob;
#ifdef WIN32
#ifdef PD
    int pad; /* protect the object against observed writing beyond
                the bounds of t_object on Windows versions of PureData. */
#endif
#endif
    void *outlet1;
    void *outlet2;
    void *clock;          // pointer to clock object
    char *name;
    mapper_network network;
    mapper_database db;
    mapper_device device;
    mapper_timetag_t timetag;
    int updated;
    int ready;
    int learn_mode;
    t_atom buffer[MAX_LIST];
    char *definition;
#ifdef MAXMSP
    t_dictionary *d;
#endif
} t_mapper;

// *********************************************************
// -(function prototypes)-----------------------------------
static void *mapperobj_new(t_symbol *s, int argc, t_atom *argv);
static void mapperobj_free(t_mapper *x);

static void mapperobj_anything(t_mapper *x, t_symbol *s, int argc, t_atom *argv);

static void mapperobj_add_signal(t_mapper *x, t_symbol *s, int argc, t_atom *argv);
static void mapperobj_remove_signal(t_mapper *x, t_symbol *s, int argc, t_atom *argv);
static void mapperobj_clear_signals(t_mapper *x, t_symbol *s, int argc, t_atom *argv);

static void mapperobj_poll(t_mapper *x);

static void mapperobj_float_handler(mapper_signal sig, mapper_id instance,
                                    const void *value, int count,
                                    mapper_timetag_t *tt);
static void mapperobj_int_handler(mapper_signal sig, mapper_id instance,
                                  const void *value, int count,
                                  mapper_timetag_t *tt);
static void mapperobj_instance_event_handler(mapper_signal sig, mapper_id instance,
                                             mapper_instance_event event,
                                             mapper_timetag_t *tt);

static void mapperobj_print_properties(t_mapper *x);

static void mapperobj_learn(t_mapper *x, t_symbol *s, int argc, t_atom *argv);
static void mapperobj_set(t_mapper *x, t_symbol *s, int argc, t_atom *argv);

#ifdef MAXMSP
void mapperobj_assist(t_mapper *x, void *b, long m, long a, char *s);
static void mapperobj_register_signals(t_mapper *x);
static void mapperobj_read_definition(t_mapper *x);
#endif

static void maybe_start_queue(t_mapper *x);
static int maxpd_atom_strcmp(t_atom *a, const char *string);
static const char *maxpd_atom_get_string(t_atom *a);
static void maxpd_atom_set_string(t_atom *a, const char *string);
static void maxpd_atom_set_int(t_atom *a, int i);
static double maxpd_atom_get_float(t_atom *a);
static void maxpd_atom_set_float(t_atom *a, float d);

// *********************************************************
// -(global class pointer variable)-------------------------
static void *mapperobj_class;

// *********************************************************
// -(main)--------------------------------------------------
#ifdef MAXMSP
    int main(void)
    {
        t_class *c;
        c = class_new("mapper", (method)mapperobj_new, (method)mapperobj_free,
                      (long)sizeof(t_mapper), 0L, A_GIMME, 0);
        class_addmethod(c, (method)mapperobj_assist,         "assist",   A_CANT,     0);
        class_addmethod(c, (method)mapperobj_add_signal,     "add",      A_GIMME,    0);
        class_addmethod(c, (method)mapperobj_remove_signal,  "remove",   A_GIMME,    0);
        class_addmethod(c, (method)mapperobj_anything,       "anything", A_GIMME,    0);
        class_addmethod(c, (method)mapperobj_learn,          "learn",    A_GIMME,    0);
        class_addmethod(c, (method)mapperobj_set,            "set",      A_GIMME,    0);
        class_addmethod(c, (method)mapperobj_clear_signals,  "clear",    A_GIMME,    0);
        class_register(CLASS_BOX, c); /* CLASS_NOBOX */
        mapperobj_class = c;
        return 0;
    }
#else
    int mapper_setup(void)
    {
        t_class *c;
        c = class_new(gensym("mapper"), (t_newmethod)mapperobj_new, (t_method)mapperobj_free,
                      (long)sizeof(t_mapper), 0L, A_GIMME, 0);
        class_addmethod(c,   (t_method)mapperobj_add_signal,    gensym("add"),    A_GIMME, 0);
        class_addmethod(c,   (t_method)mapperobj_remove_signal, gensym("remove"), A_GIMME, 0);
        class_addanything(c, (t_method)mapperobj_anything);
        class_addmethod(c,   (t_method)mapperobj_learn,         gensym("learn"),  A_GIMME, 0);
        class_addmethod(c,   (t_method)mapperobj_set,           gensym("set"),    A_GIMME, 0);
        class_addmethod(c,   (t_method)mapperobj_clear_signals, gensym("clear"),  A_GIMME, 0);
        mapperobj_class = c;
        return 0;
    }
#endif

// *********************************************************
// -(new)---------------------------------------------------
static void *mapperobj_new(t_symbol *s, int argc, t_atom *argv)
{
    t_mapper *x = NULL;
    long i;
    int learn = 0;
    const char *alias = NULL;
    const char *iface = NULL;

#ifdef MAXMSP
    if ((x = object_alloc(mapperobj_class))) {
        x->outlet2 = listout((t_object *)x);
        x->outlet1 = listout((t_object *)x);
        x->name = 0;
#else
    if ((x = (t_mapper *) pd_new(mapperobj_class)) ) {
        x->outlet1 = outlet_new(&x->ob, gensym("list"));
        x->outlet2 = outlet_new(&x->ob, gensym("list"));
#endif

        for (i = 0; i < argc; i++) {
            if ((argv+i)->a_type == A_SYM) {
                if (maxpd_atom_strcmp(argv+i, "@alias") == 0) {
                    if ((argv+i+1)->a_type == A_SYM) {
                        alias = maxpd_atom_get_string(argv+i+1);
                        i++;
                    }
                }
#ifdef MAXMSP
                else if ((maxpd_atom_strcmp(argv+i, "@def") == 0) ||
                         (maxpd_atom_strcmp(argv+i, "@definition") == 0)) {
                    if ((argv+i+1)->a_type == A_SYM) {
                        x->definition = strdup(maxpd_atom_get_string(argv+i+1));
                        mapperobj_read_definition(x);
                        i++;
                    }
                }
#endif
                else if (maxpd_atom_strcmp(argv+i, "@learn") == 0) {
                    if ((argv+i+1)->a_type == A_FLOAT) {
                        learn = (maxpd_atom_get_float(argv+i+1) > 1) ? 0 : 1;
                        i++;
                    }
#ifdef MAXMSP
                    else if ((argv+i+1)->a_type == A_LONG) {
                        learn = (atom_getlong(argv+i+1) > 1) ? 0 : 1;
                        i++;
                    }
#endif
                }
                else if (maxpd_atom_strcmp(argv+i, "@interface") == 0) {
                    if ((argv+i+1)->a_type == A_SYM) {
                        iface = maxpd_atom_get_string(argv+i+1);
                        i++;
                    }
                }
            }
        }
        if (alias) {
            x->name = *alias == '/' ? strdup(alias+1) : strdup(alias);
        }
        else if (!x->name) {
#ifdef MAXMSP
            x->name = strdup("maxmsp");
#else
            x->name = strdup("puredata");
#endif
        }

        POST(x, "libmapper version %s – visit libmapper.org for more information.",
             mapper_version());

        x->network = mapper_network_new(iface, 0, 0);
        if (!x->network) {
            POST(x, "Error initializing libmapper network.");
            return 0;
        }

        x->device = mapper_device_new(x->name, 0, x->network);
        if (!x->device) {
            POST(x, "Error initializing libmapper device.");
            return 0;
        }
        x->db = mapper_device_database(x->device);
        POST(x, "Using network interface %s.",
             mapper_network_interface(x->network));

        // add other declared properties
        for (i = 0; i < argc; i++) {
            if (i > argc - 2) // need 2 arguments for key and value
                break;
            if ((maxpd_atom_strcmp(argv+i, "@alias") == 0) ||
                (maxpd_atom_strcmp(argv+i, "@def") == 0) ||
                (maxpd_atom_strcmp(argv+i, "@definition") == 0) ||
                (maxpd_atom_strcmp(argv+i, "@learn") == 0) ||
                (maxpd_atom_strcmp(argv+i, "@interface") == 0)){
                i++;
                continue;
            }
            else if (maxpd_atom_get_string(argv+i)[0] == '@') {
                switch ((argv+i+1)->a_type) {
                    case A_SYM: {
                        const char *value = maxpd_atom_get_string(argv+i+1);
                        mapper_device_set_property(x->device,
                                                   maxpd_atom_get_string(argv+i)+1,
                                                   1, 's', value, 1);
                        i++;
                        break;
                    }
                    case A_FLOAT:
                    {
                        float value = maxpd_atom_get_float(argv+i+1);
                        mapper_device_set_property(x->device,
                                                   maxpd_atom_get_string(argv+i)+1,
                                                   1, 'f', &value, 1);
                        i++;
                        break;
                    }
#ifdef MAXMSP
                    case A_LONG:
                    {
                        int value = atom_getlong(argv+i+1);
                        mapper_device_set_property(x->device,
                                                   maxpd_atom_get_string(argv+i)+1,
                                                   1, 'i', &value, 1);
                        i++;
                        break;
                    }
#endif
                    default:
                        break;
                }
            }
        }

        mapperobj_print_properties(x);
        x->ready = 0;
        x->updated = 0;
        x->learn_mode = learn;
#ifdef MAXMSP
        mapperobj_register_signals(x);
        // Create the timing clock
        x->clock = clock_new(x, (method)mapperobj_poll);
#else
        // Create the timing clock
        x->clock = clock_new(x, (t_method)mapperobj_poll);
#endif
        clock_delay(x->clock, INTERVAL);  // Set clock to go off after delay
    }
    return (x);
}

// *********************************************************
// -(free)--------------------------------------------------
static void mapperobj_free(t_mapper *x)
{
    clock_unset(x->clock);      // Remove clock routine from the scheduler
    clock_free(x->clock);       // Frees memeory used by clock

#ifdef MAXMSP
    object_free(x->d);          // Frees memory used by dictionary
#endif

    if (x->device) {
        mapper_device_free(x->device);
    }
    if (x->network) {
        mapper_network_free(x->network);
    }
    if (x->name) {
        free(x->name);
    }
}

// *********************************************************
// -(print properties)--------------------------------------
static void mapperobj_print_properties(t_mapper *x)
{
    if (x->ready) {
        //output name
        maxpd_atom_set_string(x->buffer, mapper_device_name(x->device));
        outlet_anything(x->outlet2, gensym("name"), 1, x->buffer);

        //output interface
        maxpd_atom_set_string(x->buffer, mapper_network_interface(x->network));
        outlet_anything(x->outlet2, gensym("interface"), 1, x->buffer);

        //output IP
        const struct in_addr *ip = mapper_network_ip4(x->network);
        maxpd_atom_set_string(x->buffer, inet_ntoa(*ip));
        outlet_anything(x->outlet2, gensym("IP"), 1, x->buffer);

        //output port
        maxpd_atom_set_int(x->buffer, mapper_device_port(x->device));
        outlet_anything(x->outlet2, gensym("port"), 1, x->buffer);

        //output ordinal
        maxpd_atom_set_int(x->buffer, mapper_device_ordinal(x->device));
        outlet_anything(x->outlet2, gensym("ordinal"), 1, x->buffer);

        //output numInputs
        maxpd_atom_set_int(x->buffer, mapper_device_num_signals(x->device,
                                                                MAPPER_DIR_INCOMING));
        outlet_anything(x->outlet2, gensym("numInputs"), 1, x->buffer);

        //output numOutputs
        maxpd_atom_set_int(x->buffer, mapper_device_num_signals(x->device,
                                                                MAPPER_DIR_OUTGOING));
        outlet_anything(x->outlet2, gensym("numOutputs"), 1, x->buffer);
    }
}

// *********************************************************
// -(inlet/outlet assist - maxmsp only)---------------------
#ifdef MAXMSP
void mapperobj_assist(t_mapper *x, void *b, long m, long a, char *s)
{
    if (m == ASSIST_INLET) { // inlet
        sprintf(s, "OSC input");
    }
    else {    // outlet
        if (a == 0) {
            sprintf(s, "Mapped OSC data");
        }
        else {
            sprintf(s, "Device information");
        }
    }
}
#endif // MAXMSP

// *********************************************************
// -(add signal)--------------------------------------------
static void mapperobj_add_signal(t_mapper *x, t_symbol *s,
                                 int argc, t_atom *argv)
{
    const char *sig_name = 0, *sig_units = 0;
    char sig_type = 0;
    int sig_length = 1, prop_int = 0;
    float prop_float;
    long i;
    mapper_signal sig = 0;
    mapper_direction dir;

    if (argc < 4) {
        POST(x, "Not enough arguments for 'add' message.");
        return;
    }

    if ((argv->a_type != A_SYM) || ((argv+1)->a_type != A_SYM))
        return;

    if (maxpd_atom_strcmp(argv, "input") == 0)
        dir = MAPPER_DIR_INCOMING;
    else if (maxpd_atom_strcmp(argv, "output") == 0)
        dir = MAPPER_DIR_OUTGOING;
    else
        return;

    // get signal name
    sig_name = maxpd_atom_get_string(argv+1);

    // get signal type, length, and units
    for (i = 2; i < argc; i++) {
        if (i > argc - 2) // need 2 arguments for key and value
            break;
        if ((argv+i)->a_type == A_SYM) {
            if (maxpd_atom_strcmp(argv+i, "@type") == 0) {
                if ((argv+i+1)->a_type == A_SYM) {
                    sig_type = maxpd_atom_get_string(argv+i+1)[0];
                    i++;
                }
            }
            else if (maxpd_atom_strcmp(argv+i, "@length") == 0) {
                if ((argv+i+1)->a_type == A_FLOAT) {
                    sig_length = (int)maxpd_atom_get_float(argv+i+1);
                    i++;
                }
#ifdef MAXMSP
                else if ((argv+i+1)->a_type == A_LONG) {
                    sig_length = atom_getlong(argv+i+1);
                    i++;
                }
#endif
            }
            else if(maxpd_atom_strcmp(argv+i, "@units") == 0) {
                if ((argv+i+1)->a_type == A_SYM) {
                    sig_units = maxpd_atom_get_string(argv+i+1);
                    i++;
                }
            }
        }
    }
    if (!sig_type) {
        POST(x, "Signal has no declared type!");
        return;
    }
    if (sig_length < 1) {
        POST(x, "Signals cannot have length < 1!");
        return;
    }

    sig = mapper_device_add_signal(x->device, dir, 1, sig_name, sig_length,
                                   sig_type, sig_units, 0, 0,
                                   sig_type == 'i' ? mapperobj_int_handler
                                   : mapperobj_float_handler, x);
    if (!sig) {
        POST(x, "Error adding signal!");
        return;
    }

    // add other declared properties
    for (i = 2; i < argc; i++) {
        if (i > argc - 2) // need 2 arguments for key and value
            break;
        if ((maxpd_atom_strcmp(argv+i, "@type") == 0) ||
            (maxpd_atom_strcmp(argv+i, "@length") == 0) ||
            (maxpd_atom_strcmp(argv+i, "@units") == 0)){
            i++;
            continue;
        }
        if (maxpd_atom_strcmp(argv+i, "@min") == 0) {
            if ((argv+i+1)->a_type == A_FLOAT) {
                prop_float = maxpd_atom_get_float(argv+i+1);
                prop_int = (int)prop_float;
                mapper_signal_set_minimum(sig, sig_type == 'i' ? (void *)&prop_int
                                          : (void *)&prop_float);
                i++;
            }
#ifdef MAXMSP
            else if ((argv + i + 1)->a_type == A_LONG) {
                prop_int = (int)atom_getlong(argv+i+1);
                prop_float = (float)prop_int;
                mapper_signal_set_minimum(sig, sig_type == 'i' ? (void *)&prop_int
                                          : (void *)&prop_float);
                i++;
            }
#endif
        }
        else if (maxpd_atom_strcmp(argv+i, "@max") == 0) {
            if ((argv+i+1)->a_type == A_FLOAT) {
                prop_float = maxpd_atom_get_float(argv+i+1);
                prop_int = (int)prop_float;
                mapper_signal_set_maximum(sig, sig_type == 'i' ? (void *)&prop_int
                                          : (void *)&prop_float);
                i++;
            }
#ifdef MAXMSP
            else if ((argv + i + 1)->a_type == A_LONG) {
                prop_int = (int)atom_getlong(argv+i+1);
                prop_float = (float)prop_int;
                mapper_signal_set_maximum(sig, sig_type == 'i' ? (void *)&prop_int
                                          : (void *)&prop_float);
                i++;
            }
#endif
        }
        else if (maxpd_atom_strcmp(argv+i, "@instances") == 0) {
            if ((argv+i+1)->a_type == A_FLOAT) {
                prop_int = (int)maxpd_atom_get_float(argv+i+1);
                i++;
            }
#ifdef MAXMSP
            else if ((argv+i+1)->a_type == A_LONG) {
                prop_int = atom_getlong(argv+i+1);
                i++;
            }
#endif
            if (prop_int > 1) {
                mapper_signal_reserve_instances(sig, prop_int - 1, 0, 0);
                mapper_signal_set_instance_event_callback(sig,
                    mapperobj_instance_event_handler, MAPPER_INSTANCE_ALL);
            }
        }
        else if (maxpd_atom_strcmp(argv+i, "@stealing") == 0) {
            if ((argv+i+1)->a_type == A_SYM) {
                if (maxpd_atom_strcmp(argv+i+1, "newest") == 0) {
                    mapper_signal_set_instance_stealing_mode(sig, MAPPER_STEAL_NEWEST);
                }
                if (maxpd_atom_strcmp(argv+i+1, "oldest") == 0) {
                    mapper_signal_set_instance_stealing_mode(sig, MAPPER_STEAL_OLDEST);
                }
                i++;
            }
        }
        else if (maxpd_atom_get_string(argv+i)[0] == '@') {
            switch ((argv+i+1)->a_type) {
                case A_SYM: {
                    const char *value = maxpd_atom_get_string(argv+i+1);
                    mapper_signal_set_property(sig, maxpd_atom_get_string(argv+i)+1,
                                               1, 's', value, 1);
                    i++;
                    break;
                }
                case A_FLOAT:
                {
                    float value = maxpd_atom_get_float(argv+i+1);
                    mapper_signal_set_property(sig, maxpd_atom_get_string(argv+i)+1,
                                               1, 'f', &value, 1);
                    i++;
                    break;
                }
#ifdef MAXMSP
                case A_LONG:
                {
                    int value = atom_getlong(argv+i+1);
                    mapper_signal_set_property(sig, maxpd_atom_get_string(argv+i)+1,
                                               1, 'i', &value, 1);
                    i++;
                    break;
                }
#endif
                default:
                    break;
            }
        }
    }

    // Update status outlet
    maxpd_atom_set_int(x->buffer, mapper_device_num_signals(x->device, dir));
    if (dir == MAPPER_DIR_OUTGOING)
        outlet_anything(x->outlet2, gensym("numOutputs"), 1, x->buffer);
    else
        outlet_anything(x->outlet2, gensym("numInputs"), 1, x->buffer);
}

// *********************************************************
// -(remove signal)-----------------------------------------
static void mapperobj_remove_signal(t_mapper *x, t_symbol *s,
                                    int argc, t_atom *argv)
{
    mapper_signal sig;
    char *sig_name = NULL, *direction = NULL;

    if (argc < 2) {
        return;
    }
    if (argv->a_type != A_SYM || (argv+1)->a_type != A_SYM) {
        POST(x, "Unable to parse remove message!");
        return;
    }
    direction = strdup(maxpd_atom_get_string(argv));
    sig_name = strdup(maxpd_atom_get_string(argv+1));

    if (strcmp(direction, "output") == 0) {
        if ((sig = mapper_device_signal_by_name(x->device, sig_name))) {
            mapper_device_remove_signal(x->device, sig);
            maxpd_atom_set_int(x->buffer, mapper_device_num_signals(x->device,
                                                                    MAPPER_DIR_OUTGOING));
            outlet_anything(x->outlet2, gensym("numOutputs"), 1, x->buffer);
        }
    }
    else if (strcmp(direction, "input") == 0) {
        if ((sig = mapper_device_signal_by_name(x->device, sig_name))) {
            mapper_device_remove_signal(x->device, sig);
            maxpd_atom_set_int(x->buffer, mapper_device_num_signals(x->device,
                                                                    MAPPER_DIR_INCOMING));
            outlet_anything(x->outlet2, gensym("numInputs"), 1, x->buffer);
        }
    }
}

// *********************************************************
// -(clear all signals)-------------------------------------
static void mapperobj_clear_signals(t_mapper *x, t_symbol *s,
                                    int argc, t_atom *argv)
{
    mapper_direction dir = 0;

    if (!argc)
        dir = MAPPER_DIR_ANY;
    else if (maxpd_atom_strcmp(argv, "inputs") == 0)
        dir |= MAPPER_DIR_INCOMING;
    else if (maxpd_atom_strcmp(argv, "outputs") == 0)
        dir |= MAPPER_DIR_OUTGOING;
    else
        return;

    mapper_signal *sigs;
    POST(x, "Clearing signals");
    sigs = mapper_device_signals(x->device, dir);
    while (sigs) {
        mapper_signal sig = *sigs;
        sigs = mapper_signal_query_next(sigs);
        mapper_device_remove_signal(x->device, sig);
    }

    if (dir & MAPPER_DIR_INCOMING) {
        maxpd_atom_set_int(x->buffer, mapper_device_num_signals(x->device,
                                                                MAPPER_DIR_INCOMING));
        outlet_anything(x->outlet2, gensym("numInputs"), 1, x->buffer);
    }
    if (dir & MAPPER_DIR_OUTGOING) {
        maxpd_atom_set_int(x->buffer, mapper_device_num_signals(x->device,
                                                                MAPPER_DIR_OUTGOING));
        outlet_anything(x->outlet2, gensym("numOutputs"), 1, x->buffer);
    }
}

// *********************************************************
// -(set signal value)--------------------------------------
static void mapperobj_set(t_mapper *x, t_symbol *s, int argc, t_atom *argv)
{
    /* This method sets the value of an input signal.
     * This allows storing of input signal state changes generated by
     * user actions rather than libmapper messaging. This state storage
     * is used by libmapper for (e.g.) retreiving information for training
     * implicit mapping algorithms. */

    int i;

    if (!argc)
        return;
    if (!x->ready)
        return;
    if (argv->a_type != A_SYM)
        return;

    // find matching input signal
    mapper_signal sig = mapper_device_signal_by_name(x->device,
                                                     maxpd_atom_get_string(argv));
    if (!sig) {
        POST(x, "Error setting value: signal named \"%s\" does not exist!",
             maxpd_atom_get_string(argv));
        return;
    }

    // check if input instance release
    if ((argc == 3) && ((argv+2)->a_type == A_SYM)) {
        if (strcmp(maxpd_atom_get_string(argv+2), "release") != 0)
            return;
        if ((argv+1)->a_type == A_FLOAT) {
            maybe_start_queue(x);
            mapper_signal_instance_release(sig, (int)atom_getfloat(argv+1),
                                           x->timetag);
        }
#ifdef MAXMSP
        else if ((argv+1)->a_type == A_LONG) {
            maybe_start_queue(x);
            mapper_signal_instance_release(sig, (int)atom_getlong(argv+1),
                                           x->timetag);
        }
#endif
        return;
    }

    int length = mapper_signal_length(sig);
    char type = mapper_signal_type(sig);

    // get signal properties
    if (length != argc - 1) {
        POST(x, "Error: vector length (%i) does not match signal definition (%i)!",
             argc - 1, length);
        return;
    }
    if (type == 'i') {
        int payload[length];
        for (i = 1; i < argc; i++) {
            if ((argv + i)->a_type == A_FLOAT)
                payload[i-1] = (int)atom_getfloat(argv + i);
#ifdef MAXMSP
            else if ((argv + i)->a_type == A_LONG)
                payload[i-1] = (int)atom_getlong(argv + i);
#endif
        }
        //update signal
        maybe_start_queue(x);
        mapper_signal_update(sig, payload, 1, x->timetag);
    }
    else if (type == 'f') {
        float payload[length];
        for (i = 1; i < argc; i++) {
            if ((argv + i)->a_type == A_FLOAT)
                payload[i-1] = atom_getfloat(argv + i);
#ifdef MAXMSP
            else if ((argv + i)->a_type == A_LONG)
                payload[i-1] = (float)atom_getlong(argv + i);
#endif
        }
        //update signal
        maybe_start_queue(x);
        mapper_signal_update(sig, payload, 1, x->timetag);
    }
    else {
        return;
    }
}

// *********************************************************
// -(anything)----------------------------------------------
static void mapperobj_anything(t_mapper *x, t_symbol *s, int argc, t_atom *argv)
{
    if (!x->ready)
        return;

    int i = 0, j = 0, id = -1;
    if (argc) {
        //find signal
        mapper_signal sig;
        if (!(sig = mapper_device_signal_by_name(x->device, s->s_name))) {
            if (x->learn_mode) {
                // register as new signal
                if (argv->a_type == A_FLOAT) {
                    sig = mapper_device_add_output_signal(x->device, s->s_name,
                                                          argc, 'f', 0, 0, 0);
                }
#ifdef MAXMSP
                else if (argv->a_type == A_LONG) {
                    sig = mapper_device_add_output_signal(x->device, s->s_name,
                                                          argc, 'i', 0, 0, 0);
                }
#endif
                else {
                    return;
                }
                //output updated numOutputs
                maxpd_atom_set_float(x->buffer,
                                     mapper_device_num_signals(x->device,
                                                               MAPPER_DIR_OUTGOING));
                outlet_anything(x->outlet2, gensym("numOutputs"), 1, x->buffer);
            }
            else {
                return;
            }
        }

        int length = mapper_signal_length(sig);
        char type = mapper_signal_type(sig);

        if (argc == length + 1) {
            // Special case: signal value may be preceded by instance number
            if ((argv)->a_type == A_FLOAT) {
                id = (int)maxpd_atom_get_float(argv);
                j = 1;
            }
#ifdef MAXMSP
            else if ((argv)->a_type == A_LONG) {
                id = (int)atom_getlong(argv);
                j = 1;
            }
#endif
            else {
                POST(x, "Instance ID is not int or float!");
                return;
            }
        }
        if (argc == 2 && (argv + 1)->a_type == A_SYM) {
            if ((argv)->a_type == A_FLOAT) {
                id = (int)atom_getfloat(argv);
            }
#ifdef MAXMSP
            else if ((argv)->a_type == A_LONG) {
                id = (int)atom_getlong(argv);
            }
#endif
            if (maxpd_atom_strcmp(argv+1, "release") == 0)
                mapper_signal_instance_release(sig, id, x->timetag);
        }
        else if (type == 'i') {
            int payload[length];
            for (i = 0; i < argc; i++) {
                if ((argv + i + j)->a_type == A_FLOAT)
                    payload[i] = (int)atom_getfloat(argv + i + j);
#ifdef MAXMSP
                else if ((argv + i + j)->a_type == A_LONG)
                    payload[i] = (int)atom_getlong(argv + i + j);
#endif
            }
            //update signal
            maybe_start_queue(x);
            if (id == -1) {
                mapper_signal_update(sig, payload, 1, x->timetag);
            }
            else {
                mapper_signal_instance_update(sig, id, payload, 1, x->timetag);
            }
        }
        else if (type == 'f') {
            float payload[length];
            for (i = 0; i < argc; i++) {
                if ((argv + i + j)->a_type == A_FLOAT)
                    payload[i] = atom_getfloat(argv + i + j);
#ifdef MAXMSP
                else if ((argv + i + j)->a_type == A_LONG)
                    payload[i] = (float)atom_getlong(argv + i + j);
#endif
            }
            //update signal
            maybe_start_queue(x);
            if (id == -1) {
                mapper_signal_update(sig, payload, 1, x->timetag);
            }
            else {
                mapper_signal_instance_update(sig, id, payload, 1, x->timetag);
            }
        }
        else {
            return;
        }
    }
}

// *********************************************************
// -(int handler)-------------------------------------------
static void mapperobj_int_handler(mapper_signal sig, mapper_id instance,
                                  const void *value, int count,
                                  mapper_timetag_t *tt)
{
    t_mapper *x = mapper_signal_user_data(sig);
    int poly = 0;
    if (mapper_signal_num_instances(sig) > 1) {
        maxpd_atom_set_int(x->buffer, instance);
        poly = 1;
    }
    if (value) {
        int i, length = mapper_signal_length(sig);
        int *v = (int*)value;

        if (length > (MAX_LIST-1)) {
            POST(x, "Maximum list length is %i!", MAX_LIST-1);
            length = MAX_LIST-1;
        }

        for (i = 0; i < length; i++)
            maxpd_atom_set_int(x->buffer + i + poly, v[i]);
        outlet_anything(x->outlet1, gensym((char *)mapper_signal_name(sig)),
                        length + poly, x->buffer);
    }
    else if (poly) {
        maxpd_atom_set_string(x->buffer+1, "release");
        maxpd_atom_set_string(x->buffer+2, "local");
        outlet_anything(x->outlet1, gensym((char *)mapper_signal_name(sig)),
                        3, x->buffer);
    }
}

// *********************************************************
// -(float handler)-----------------------------------------
static void mapperobj_float_handler(mapper_signal sig, mapper_id instance,
                                    const void *value, int count,
                                    mapper_timetag_t *time)
{
    t_mapper *x = mapper_signal_user_data(sig);
    int poly = 0;
    if (mapper_signal_num_instances(sig) > 1) {
        maxpd_atom_set_int(x->buffer, instance);
        poly = 1;
    }
    if (value) {
        int i, length = mapper_signal_length(sig);
        float *v = (float*)value;

        if (length > (MAX_LIST-1)) {
            POST(x, "Maximum list length is %i!", MAX_LIST-1);
            length = MAX_LIST-1;
        }

        for (i = 0; i < length; i++)
            maxpd_atom_set_float(x->buffer + i + poly, v[i]);
        outlet_anything(x->outlet1, gensym((char *)mapper_signal_name(sig)),
                        length + poly, x->buffer);
    }
}

// *********************************************************
// -(instance management handler)----------------------
static void mapperobj_instance_event_handler(mapper_signal sig,
                                             mapper_id instance,
                                             mapper_instance_event event,
                                             mapper_timetag_t *tt)
{
    int mode;
    t_mapper *x = mapper_signal_user_data(sig);
    maxpd_atom_set_int(x->buffer, instance);
    switch (event) {
        case MAPPER_UPSTREAM_RELEASE:
            maxpd_atom_set_string(x->buffer+1, "release");
            maxpd_atom_set_string(x->buffer+2, "upstream");
            outlet_anything(x->outlet1, gensym((char *)mapper_signal_name(sig)),
                            3, x->buffer);
            break;
        case MAPPER_DOWNSTREAM_RELEASE:
            maxpd_atom_set_string(x->buffer+1, "release");
            maxpd_atom_set_string(x->buffer+2, "downstream");
            outlet_anything(x->outlet1, gensym((char *)mapper_signal_name(sig)),
                            3, x->buffer);
            break;
        case MAPPER_INSTANCE_OVERFLOW:
            mode = mapper_signal_instance_stealing_mode(sig);
            switch (mode) {
                case MAPPER_STEAL_OLDEST:
                    instance = mapper_signal_oldest_active_instance(sig);
                    if (instance)
                        mapper_signal_instance_release(sig, instance, *tt);
                    break;
                case MAPPER_STEAL_NEWEST:
                    instance = mapper_signal_newest_active_instance(sig);
                    if (instance)
                        mapper_signal_instance_release(sig, instance, *tt);
                    break;
                case 0:
                    maxpd_atom_set_string(x->buffer+1, "overflow");
                    outlet_anything(x->outlet1,
                                    gensym((char *)mapper_signal_name(sig)),
                                    2, x->buffer);
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
    }
}

// *********************************************************
// -(read device definition - maxmsp only)------------------
#ifdef MAXMSP
static void mapperobj_read_definition (t_mapper *x)
{
    if (x->d) {
        object_free(x->d);
    }
    t_object *info;
    t_symbol *sym_device = gensym("device");
    t_symbol *sym_name = gensym("name");
    const char *my_name = 0;
    short path;
    unsigned int filetype = 'JSON', outtype;

    // TODO: add ".json" to end of string if missing (or pick new filetype!)

    if (locatefile_extended(x->definition, &path, &outtype, &filetype, 1) == 0) {
        POST(x, "Located file %s", x->definition);
        if (dictionary_read(x->definition, path, &(x->d)) == 0) {
            //check that first key is "device"
            if (dictionary_entryisdictionary(x->d, sym_device)) {
                //recover name from dictionary
                dictionary_getdictionary(x->d, sym_device, &info);
                dictionary_getstring((t_dictionary *)info, sym_name, &my_name);
                if (my_name) {
                    free(x->name);
                    x->name = *my_name == '/' ? strdup(my_name+1) : strdup(my_name);
                }
            }
        }
        else {
            POST(x, "Could not parse file %s", x->definition);
        }
    }
    else {
        POST(x, "Could not locate file %s", x->definition);
    }
}
#endif // MAXMSP

// *********************************************************
// -(register signals from dictionary - maxmsp only)--------
#ifdef MAXMSP
static void mapperobj_register_signals(t_mapper *x) {
    t_atom *signals;
    long num_signals, i;
    t_object *device, *inputs, *outputs, *temp;
    t_symbol *sym_device = gensym("device");
    t_symbol *sym_inputs = gensym("inputs");
    t_symbol *sym_outputs = gensym("outputs");
    t_symbol *sym_name = gensym("name");
    t_symbol *sym_type = gensym("type");
    t_symbol *sym_units = gensym("units");
    t_symbol *sym_minimum = gensym("minimum");
    t_symbol *sym_maximum = gensym("maximum");
    t_symbol *sym_length = gensym("length");

    const char *sig_name, *sig_units, *sig_type;
    char sig_type_char = 0;
    double sig_min_double, sig_max_double;
    float sig_min_float, sig_max_float;
    long sig_min_long, sig_max_long, sig_length;
    int sig_min_int, sig_max_int;

    mapper_signal temp_sig;
    short range_known[2];

    if (!x->d)
        return;

    // Get pointer to dictionary "device"
    if (dictionary_getdictionary(x->d, sym_device, &device) != MAX_ERR_NONE)
        return;

    // Get pointer to atom array "inputs"
    if (dictionary_getatomarray((t_dictionary *)device, sym_inputs,
                                &inputs) == MAX_ERR_NONE) {
        atomarray_getatoms((t_atomarray *)inputs, &num_signals, &signals);
        // iterate through array of atoms
        for (i=0; i<num_signals; i++) {
            // initialize variables
            range_known[0] = 0;
            range_known[1] = 0;

            // each atom object points to a dictionary, need to recover atoms by key
            temp = atom_getobj(&(signals[i]));
            if (dictionary_getstring((t_dictionary *)temp, sym_name,
                                     &sig_name) != MAX_ERR_NONE)
                continue;
            if (dictionary_getstring((t_dictionary *)temp, sym_type,
                                     &sig_type) != MAX_ERR_NONE)
                continue;
            if (dictionary_getlong((t_dictionary *)temp, sym_length,
                                   &sig_length) != MAX_ERR_NONE)
                sig_length = 1;
            if (dictionary_getstring((t_dictionary *)temp, sym_units,
                                     &sig_units) != MAX_ERR_NONE)
                sig_units = 0;

            if (dictionary_getfloat((t_dictionary *)temp, sym_minimum,
                                    &sig_min_double) == MAX_ERR_NONE) {
                sig_min_float = (float)sig_min_double;
                sig_min_int = (int)sig_min_double;
                range_known[0] = 1;
            }
            else if (dictionary_getlong((t_dictionary *)temp, sym_minimum,
                                        &sig_min_long) == MAX_ERR_NONE) {
                sig_min_float = (float)sig_min_long;
                sig_min_int = (int)sig_min_long;
                range_known[0] = 1;
            }
            if (dictionary_getfloat((t_dictionary *)temp, sym_maximum,
                                    &sig_max_double) == MAX_ERR_NONE) {
                sig_max_float = (float)sig_max_double;
                sig_max_int = (int)sig_max_double;
                range_known[1] = 1;
            }
            else if (dictionary_getlong((t_dictionary *)temp, sym_maximum,
                                        &sig_max_long) == MAX_ERR_NONE) {
                sig_max_float = (float)sig_max_long;
                sig_max_int = (int)sig_max_long;
                range_known[1] = 1;
            }
            if ((strcmp(sig_type, "int") == 0) || (strcmp(sig_type, "i") == 0))
                sig_type_char = 'i';
            else if ((strcmp(sig_type, "float") == 0) || (strcmp(sig_type, "f") == 0))
                sig_type_char = 'f';
            else {
                POST(x, "Skipping registration of signal %s (unknown type).",
                     sig_name);
                continue;
            }

            temp_sig = mapper_device_add_input_signal(x->device, sig_name,
                                                      (int)sig_length,
                                                      sig_type_char, sig_units,
                                                      0, 0,
                                                      sig_type_char == 'i'
                                                      ? mapperobj_int_handler
                                                      : mapperobj_float_handler, x);

            if (temp_sig) {
                if (range_known[0]) {
                    mapper_signal_set_minimum(temp_sig,
                                              sig_type_char == 'i'
                                              ? (void *)&sig_min_int
                                              : (void *)&sig_min_float);
                }
                if (range_known[1]) {
                    mapper_signal_set_maximum(temp_sig,
                                              sig_type_char == 'i'
                                              ? (void *)&sig_max_int
                                              : (void *)&sig_max_float);
                }
            }
        }
    }

    // Get pointer to atom array "outputs"
    if (dictionary_getatomarray((t_dictionary *)device, sym_outputs,
                                &outputs) == MAX_ERR_NONE) {
        atomarray_getatoms((t_atomarray *)outputs, &num_signals, &signals);
        // iterate through array of atoms
        for (i=0; i<num_signals; i++) {
            // initialize variables
            range_known[0] = 0;
            range_known[1] = 0;

            // each atom object points to a dictionary, need to recover atoms by key
            temp = atom_getobj(&(signals[i]));
            if (dictionary_getstring((t_dictionary *)temp, sym_name,
                                     &sig_name) != MAX_ERR_NONE)
                continue;
            if (dictionary_getstring((t_dictionary *)temp, sym_type,
                                     &sig_type) != MAX_ERR_NONE)
                continue;
            if (dictionary_getlong((t_dictionary *)temp, sym_length,
                                   &sig_length) != MAX_ERR_NONE)
                sig_length = 1;
            if (dictionary_getstring((t_dictionary *)temp, sym_units,
                                     &sig_units) != MAX_ERR_NONE)
                sig_units = 0;

            if (dictionary_getfloat((t_dictionary *)temp, sym_minimum,
                                    &sig_min_double) == MAX_ERR_NONE) {
                sig_min_float = (float)sig_min_double;
                sig_min_int = (int)sig_min_double;
                range_known[0] = 1;
            }
            else if (dictionary_getlong((t_dictionary *)temp, sym_minimum,
                                        &sig_min_long) == MAX_ERR_NONE) {
                sig_min_float = (float)sig_min_long;
                sig_min_int = (int)sig_min_long;
                range_known[0] = 1;
            }
            if (dictionary_getfloat((t_dictionary *)temp, sym_maximum,
                                    &sig_max_double) == MAX_ERR_NONE) {
                sig_max_float = (float)sig_max_double;
                sig_max_int = (int)sig_max_double;
                range_known[1] = 1;
            }
            else if (dictionary_getlong((t_dictionary *)temp, sym_maximum,
                                        &sig_max_long) == MAX_ERR_NONE) {
                sig_max_float = (float)sig_max_long;
                sig_max_int = (int)sig_max_long;
                range_known[1] = 1;
            }
            if ((strcmp(sig_type, "int") == 0) || (strcmp(sig_type, "i") == 0))
                sig_type_char = 'i';
            else if ((strcmp(sig_type, "float") == 0) || (strcmp(sig_type, "f") == 0))
                sig_type_char = 'f';
            else {
                POST("Skipping registration of signal %s (unknown type).",
                     sig_name);
                continue;
            }

            temp_sig = mapper_device_add_output_signal(x->device, sig_name,
                                                       (int)sig_length,
                                                       sig_type_char, sig_units,
                                                       0, 0);

            if (temp_sig) {
                if (range_known[0]) {
                    mapper_signal_set_minimum(temp_sig,
                                              sig_type_char == 'i'
                                              ? (void *)&sig_min_int
                                              : (void *)&sig_min_float);
                }
                if (range_known[1]) {
                    mapper_signal_set_maximum(temp_sig,
                                              sig_type_char == 'i'
                                              ? (void *)&sig_max_int
                                              : (void *)&sig_max_float);
                }
            }
        }
    }
}
#endif // MAXMSP

// *********************************************************
// -(poll libmapper)----------------------------------------
static void mapperobj_poll(t_mapper *x)
{
    int count = 10;
    while(count-- && mapper_device_poll(x->device, 0)) {};
    if (!x->ready) {
        if (mapper_device_ready(x->device)) {
            POST(x, "Joining mapping network as '%s'", mapper_device_name(x->device));
            x->ready = 1;
            mapperobj_print_properties(x);
        }
    }
    else if (x->updated) {
        mapper_device_send_queue(x->device, x->timetag);
        x->updated = 0;
    }
    clock_delay(x->clock, INTERVAL);  // Set clock to go off after delay
}

// *********************************************************
// -(toggle learning mode)----------------------------------
static void mapperobj_learn(t_mapper *x, t_symbol *s,
                            int argc, t_atom *argv)
{
    int mode = x->learn_mode;
    if (argc > 0) {
        if (argv->a_type == A_FLOAT) {
            mode = (int)atom_getfloat(argv);
        }
#ifdef MAXMSP
        else if (argv->a_type == A_LONG) {
            mode = (int)atom_getlong(argv);
        }
#endif
        if (mode != x->learn_mode) {
            x->learn_mode = mode;
            if (mode == 0) {
                POST(x, "Learning mode off.");
            }
            else {
                POST(x, "Learning mode on.");
            }
        }
    }
}

static void maybe_start_queue(t_mapper *x)
{
    if (!x->updated) {
        mapper_timetag_now(&x->timetag);
        mapper_device_start_queue(x->device, x->timetag);
        x->updated = 1;
    }
}


// *********************************************************
// some helper functions for abtracting differences
// between maxmsp and puredata

static int maxpd_atom_strcmp(t_atom *a, const char *string)
{
    if (a->a_type != A_SYM || !string)
        return 1;
#ifdef MAXMSP
    return strcmp(atom_getsym(a)->s_name, string);
#else
    return strcmp((a)->a_w.w_symbol->s_name, string);
#endif
}

static const char *maxpd_atom_get_string(t_atom *a)
{
#ifdef MAXMSP
    return atom_getsym(a)->s_name;
#else
    return (a)->a_w.w_symbol->s_name;
#endif
}

static void maxpd_atom_set_string(t_atom *a, const char *string)
{
#ifdef MAXMSP
    atom_setsym(a, gensym((char *)string));
#else
    SETSYMBOL(a, gensym(string));
#endif
}

static void maxpd_atom_set_int(t_atom *a, int i)
{
#ifdef MAXMSP
    atom_setlong(a, (long)i);
#else
    SETFLOAT(a, (double)i);
#endif
}

static double maxpd_atom_get_float(t_atom *a)
{
    return (double)atom_getfloat(a);
}

static void maxpd_atom_set_float(t_atom *a, float d)
{
#ifdef MAXMSP
    atom_setfloat(a, d);
#else
    SETFLOAT(a, d);
#endif
}
