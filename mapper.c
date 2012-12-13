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

#define INPUT 0
#define OUTPUT 1
#define METRONOME 2

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
    void *status_clock;
    char *name;
    mapper_admin admin;
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

static t_symbol *ps_list;
static t_symbol *ps_mute;

// *********************************************************
// -(function prototypes)-----------------------------------
static void *mapper_new(t_symbol *s, int argc, t_atom *argv);
static void mapper_free(t_mapper *x);
static void mapper_anything(t_mapper *x, t_symbol *s, int argc, t_atom *argv);
static void mapper_add(t_mapper *x, t_symbol *s,
                       int argc, t_atom *argv);
static void mapper_remove(t_mapper *x, t_symbol *s, int argc, t_atom *argv);
static void mapper_poll(t_mapper *x);
static void mapper_sync_status(t_mapper *x);
static void mapper_float_handler(mapper_signal sig, mapper_db_signal props,
                                 int instance_id, void *value, int count,
                                 mapper_timetag_t *tt);
static void mapper_int_handler(mapper_signal sig, mapper_db_signal props,
                               int instance_id, void *value, int count,
                               mapper_timetag_t *tt);
static void mapper_release_handler(mapper_signal sig, mapper_db_signal props,
                                   int instance_id, msig_instance_event_t event);
static void mapper_metro_handler(mapper_metronome m, unsigned int bar,
                                 unsigned int beat, void *user_data);
static void mapper_print_properties(t_mapper *x);
static void mapper_register_signals(t_mapper *x);
static void mapper_learn(t_mapper *x, t_symbol *s, int argc, t_atom *argv);
static void mapper_set(t_mapper *x, t_symbol *s, int argc, t_atom *argv);
static void mapper_read_definition(t_mapper *x);
#ifdef MAXMSP
    void mapper_assist(t_mapper *x, void *b, long m, long a, char *s);
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
static void *mapper_class;

// *********************************************************
// -(main)--------------------------------------------------
#ifdef MAXMSP
    int main(void)
    {
        t_class *c;
        c = class_new("mapper", (method)mapper_new, (method)mapper_free,
                      (long)sizeof(t_mapper), 0L, A_GIMME, 0);
        class_addmethod(c, (method)mapper_assist,   "assist",   A_CANT,     0);
        class_addmethod(c, (method)mapper_add,      "add",      A_GIMME,    0);
        class_addmethod(c, (method)mapper_remove,   "remove",   A_GIMME,    0);
        class_addmethod(c, (method)mapper_anything, "anything", A_GIMME,    0);
        class_addmethod(c, (method)mapper_learn,    "learn",    A_GIMME,    0);
        class_addmethod(c, (method)mapper_set,      "set",      A_GIMME,    0);
        class_register(CLASS_BOX, c); /* CLASS_NOBOX */
        mapper_class = c;
        ps_list = gensym("list");
        ps_mute = gensym("mute");
        return 0;
    }
#else
    int mapper_setup(void)
    {
        t_class *c;
        c = class_new(gensym("mapper"), (t_newmethod)mapper_new, (t_method)mapper_free,
                      (long)sizeof(t_mapper), 0L, A_GIMME, 0);
        class_addmethod(c,   (t_method)mapper_add,      gensym("add"),    A_GIMME, 0);
        class_addmethod(c,   (t_method)mapper_remove,   gensym("remove"), A_GIMME, 0);
        class_addanything(c, (t_method)mapper_anything);
        class_addmethod(c,   (t_method)mapper_learn,    gensym("learn"),  A_GIMME, 0);
        class_addmethod(c,   (t_method)mapper_set,      gensym("set"),    A_GIMME, 0);
        mapper_class = c;
        ps_list = gensym("list");
        ps_mute = gensym("mute");
        return 0;
    }
#endif

// *********************************************************
// -(new)---------------------------------------------------
static void *mapper_new(t_symbol *s, int argc, t_atom *argv)
{
    t_mapper *x = NULL;
    long i;
    int learn = 0;
    const char *alias = NULL;
    const char *iface = NULL;

#ifdef MAXMSP
    if ((x = object_alloc(mapper_class))) {
        x->outlet2 = listout((t_object *)x);
        x->outlet1 = listout((t_object *)x);
        x->name = strdup("maxmsp");
#else
    if ((x = (t_mapper *) pd_new(mapper_class)) ) {
        x->outlet1 = outlet_new(&x->ob, gensym("list"));
        x->outlet2 = outlet_new(&x->ob, gensym("list"));
        x->name = strdup("puredata");
#endif

        for (i = 0; i < argc; i++) {
            if ((argv+i)->a_type == A_SYM) {
                if (maxpd_atom_strcmp(argv+i, "@alias") == 0) {
                    if ((argv+i+1)->a_type == A_SYM) {
                        alias = maxpd_atom_get_string(argv+i+1);
                        i++;
                    }
                }
                else if ((maxpd_atom_strcmp(argv+i, "@def") == 0) ||
                         (maxpd_atom_strcmp(argv+i, "@definition") == 0)) {
                    if ((argv+i+1)->a_type == A_SYM) {
                        x->definition = strdup(maxpd_atom_get_string(argv+i+1));
                        mapper_read_definition(x);
                        i++;
                    }
                }
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
            free(x->name);
            x->name = *alias == '/' ? strdup(alias+1) : strdup(alias);
        }
        post("mapper: using name %s", x->name);

        if (iface)
            post("mapper: trying interface %s", iface);
        else
            post("mapper: using default interface.");

        x->admin = mapper_admin_new(iface, 0, 0);
        if (!x->admin) {
            post("Error initializing admin.");
            return 0;
        }
        x->device = mdev_new(x->name, 0, x->admin);
        if (!x->device) {
            post("Error initializing device.");
            return 0;
        }

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
                        mdev_set_property(x->device, maxpd_atom_get_string(argv+i)+1, 's', (lo_arg *)value);
                        i++;
                        break;
                    }
                    case A_FLOAT:
                    {
                        float value = maxpd_atom_get_float(argv+i+1);
                        mdev_set_property(x->device, maxpd_atom_get_string(argv+i)+1, 'f', (lo_arg *)&value);
                        i++;
                        break;
                    }
#ifdef MAXMSP
                    case A_LONG:
                    {
                        int value = atom_getlong(argv+i+1);
                        mdev_set_property(x->device, maxpd_atom_get_string(argv+i)+1, 'i', (lo_arg *)&value);
                        i++;
                        break;
                    }
#endif
                    default:
                        break;
                }
            }
        }

        mapper_print_properties(x);
        x->ready = 0;
        x->updated = 0;
        x->learn_mode = learn;
#ifdef MAXMSP
        mapper_register_signals(x);
        x->clock = clock_new(x, (method)mapper_poll);    // Create the timing clock
        x->status_clock = clock_new(x, (method)mapper_sync_status);
#else
        x->clock = clock_new(x, (t_method)mapper_poll);
        x->status_clock = clock_new(x, (t_method)mapper_sync_status);
#endif
        clock_delay(x->clock, INTERVAL);  // Set clock to go off after delay
        clock_delay(x->status_clock, 1000);
    }
    return (x);
}

// *********************************************************
// -(free)--------------------------------------------------
static void mapper_free(t_mapper *x)
{
    clock_unset(x->clock);    // Remove clock routine from the scheduler
    clock_free(x->clock);        // Frees memeory used by clock

    clock_unset(x->status_clock);
    clock_free(x->status_clock);

#ifdef MAXMSP
    object_free(x->d);          // Frees memory used by dictionary
#endif

    if (x->device) {
        mdev_free(x->device);
    }
    if (x->admin) {
        mapper_admin_free(x->admin);
    }
    if (x->name) {
        free(x->name);
    }
}

// *********************************************************
// -(print properties)--------------------------------------
static void mapper_print_properties(t_mapper *x)
{
    if (x->ready) {
        //output name
        maxpd_atom_set_string(x->buffer, mdev_name(x->device));
        outlet_anything(x->outlet2, gensym("name"), 1, x->buffer);

        //output interface
        maxpd_atom_set_string(x->buffer, mdev_interface(x->device));
        outlet_anything(x->outlet2, gensym("interface"), 1, x->buffer);

        //output IP
        const struct in_addr *ip = mdev_ip4(x->device);
        maxpd_atom_set_string(x->buffer, inet_ntoa(*ip));
        outlet_anything(x->outlet2, gensym("IP"), 1, x->buffer);

        //output port
        maxpd_atom_set_int(x->buffer, mdev_port(x->device));
        outlet_anything(x->outlet2, gensym("port"), 1, x->buffer);

        //output ordinal
        maxpd_atom_set_int(x->buffer, mdev_ordinal(x->device));
        outlet_anything(x->outlet2, gensym("ordinal"), 1, x->buffer);

        //output numInputs
        maxpd_atom_set_int(x->buffer, mdev_num_inputs(x->device));
        outlet_anything(x->outlet2, gensym("numInputs"), 1, x->buffer);

        //output numOutputs
        maxpd_atom_set_int(x->buffer, mdev_num_outputs(x->device));
        outlet_anything(x->outlet2, gensym("numOutputs"), 1, x->buffer);
    }
}

// *********************************************************
// -(inlet/outlet assist - maxmsp only)---------------------
#ifdef MAXMSP
void mapper_assist(t_mapper *x, void *b, long m, long a, char *s)
{
    if (m == ASSIST_INLET) { // inlet
        sprintf(s, "OSC input");
    }
    else {    // outlet
        if (a == 0) {
            sprintf(s, "Mapped OSC data");
        }
        else if (a == 1) {
            sprintf(s, "State queries");
        }
        else {
            sprintf(s, "Device information");
        }
    }
}
#endif

// *********************************************************
// -(add signal)--------------------------------------------
static void mapper_add(t_mapper *x, t_symbol *s, int argc, t_atom *argv)
{
    const char *name = 0, *sig_units = 0;
    char sig_type = 0;
    int object_type, sig_length = 1, prop_int = 0, count = 4;
    float prop_float;
    long i;
    mapper_signal msig = 0;
    mapper_timetag_t start = MAPPER_TIMETAG_NOW;
    double bpm = 120;

    if (argc < 2) {
        post("mapper: not enough arguments for 'add' message.");
        return;
    }

    if ((argv->a_type != A_SYM) || ((argv+1)->a_type != A_SYM))
        return;

    if (maxpd_atom_strcmp(argv, "input") == 0)
        object_type = INPUT;
    else if (maxpd_atom_strcmp(argv, "output") == 0)
        object_type = OUTPUT;
    else if (maxpd_atom_strcmp(argv, "metronome") == 0)
        object_type = METRONOME;
    else
        return;

    // get signal name
    name = maxpd_atom_get_string(argv+1);

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
    if (!sig_type && object_type != METRONOME) {
        post("mapper: signal has no declared type!");
        return;
    }
    if (sig_length < 1 && object_type != METRONOME) {
        post("mapper: signals cannot have length < 1!");
        return;
    }

    if (object_type == INPUT) {
        msig = mdev_add_input(x->device, name, sig_length,
                              sig_type, sig_units, 0, 0,
                              sig_type == 'i' ? mapper_int_handler : mapper_float_handler, x);
        if (!msig) {
            post("mapper: error creating input!");
            return;
        }
    }
    else if (object_type == OUTPUT) {
        msig = mdev_add_output(x->device, name, sig_length,
                               sig_type, sig_units, 0, 0);
        if (!msig) {
            post("mapper: error creating output!");
            return;
        }
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
                msig_set_minimum(msig, sig_type == 'i' ? (void *)&prop_int : (void *)&prop_float);
                i++;
            }
#ifdef MAXMSP
            else if ((argv + i + 1)->a_type == A_LONG) {
                prop_int = (int)atom_getlong(argv+i+1);
                prop_float = (float)prop_int;
                msig_set_minimum(msig, sig_type == 'i' ? (void *)&prop_int : (void *)&prop_float);
                i++;
            }
#endif
        }
        else if (maxpd_atom_strcmp(argv+i, "@max") == 0) {
            if ((argv+i+1)->a_type == A_FLOAT) {
                prop_float = maxpd_atom_get_float(argv+i+1);
                prop_int = (int)prop_float;
                msig_set_maximum(msig, sig_type == 'i' ? (void *)&prop_int : (void *)&prop_float);
                i++;
            }
#ifdef MAXMSP
            else if ((argv + i + 1)->a_type == A_LONG) {
                prop_int = (int)atom_getlong(argv+i+1);
                prop_float = (float)prop_int;
                msig_set_maximum(msig, sig_type == 'i' ? (void *)&prop_int : (void *)&prop_float);
                i++;
            }
#endif
        }
        else if (maxpd_atom_strcmp(argv+i, "@poly") == 0) {
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
            msig_reserve_instances(msig, prop_int - 1);
        }
        else if (maxpd_atom_strcmp(argv+i, "@allow_remote_release") == 0) {
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
            if (prop_int) {
                msig_set_instance_management_callback(msig, mapper_release_handler,
                                                      IN_REQUEST_RELEASE, x);
            }
        }
        else if (maxpd_atom_strcmp(argv+i, "@stealing") == 0) {
            if ((argv+i+1)->a_type == A_SYM) {
                if (maxpd_atom_strcmp(argv+i+1, "newest") == 0)
                    msig_set_instance_allocation_mode(msig, IN_STEAL_NEWEST);
                if (maxpd_atom_strcmp(argv+i+1, "oldest") == 0)
                    msig_set_instance_allocation_mode(msig, IN_STEAL_OLDEST);
                i++;
            }
        }
        else if (maxpd_atom_strcmp(argv+i, "@start") == 0) {
            if ((argv+i+1)->a_type == A_FLOAT) {
                mapper_timetag_set_from_float(&start, maxpd_atom_get_float(argv+i+1));
                i++;
            }
#ifdef MAXMSP
            else if ((argv+i+1)->a_type == A_LONG) {
                mapper_timetag_set_from_int(&start, atom_getlong(argv+i+1));
                i++;
            }
#endif
        }
        else if (maxpd_atom_strcmp(argv+i, "@bpm") == 0) {
            if ((argv+i+1)->a_type == A_FLOAT) {
                bpm = (double)maxpd_atom_get_float(argv+i+1);
                i++;
            }
#ifdef MAXMSP
            else if ((argv+i+1)->a_type == A_LONG) {
                bpm = (double)atom_getlong(argv+i+1);
                i++;
            }
#endif
        }
        else if (maxpd_atom_strcmp(argv+i, "@count") == 0) {
            if ((argv+i+1)->a_type == A_FLOAT) {
                count = (int)maxpd_atom_get_float(argv+i+1);
                i++;
            }
#ifdef MAXMSP
            else if ((argv+i+1)->a_type == A_LONG) {
                count = (int)atom_getlong(argv+i+1);
                i++;
            }
#endif
        }
        else if (maxpd_atom_get_string(argv+i)[0] == '@') {
            switch ((argv+i+1)->a_type) {
                case A_SYM: {
                    const char *value = maxpd_atom_get_string(argv+i+1);
                    msig_set_property(msig, maxpd_atom_get_string(argv+i)+1, 's', (lo_arg *)value);
                    i++;
                    break;
                }
                case A_FLOAT:
                {
                    float value = maxpd_atom_get_float(argv+i+1);
                    msig_set_property(msig, maxpd_atom_get_string(argv+i)+1, 'f', (lo_arg *)&value);
                    i++;
                    break;
                }
#ifdef MAXMSP
                case A_LONG:
                {
                    int value = atom_getlong(argv+i+1);
                    msig_set_property(msig, maxpd_atom_get_string(argv+i)+1, 'i', (lo_arg *)&value);
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
    if (object_type == INPUT) {
        //output numInputs
        maxpd_atom_set_int(x->buffer, mdev_num_inputs(x->device));
        outlet_anything(x->outlet2, gensym("numInputs"), 1, x->buffer);
    }
    else if (object_type == OUTPUT) {
        //output numOutputs
        maxpd_atom_set_int(x->buffer, mdev_num_outputs(x->device));
        outlet_anything(x->outlet2, gensym("numOutputs"), 1, x->buffer);
    }
    else if (object_type == METRONOME) {
        mdev_add_metronome(x->device, name, start, bpm,
                           count, mapper_metro_handler, x);
    }
}

// *********************************************************
// -(remove signal)-----------------------------------------
static void mapper_remove(t_mapper *x, t_symbol *s, int argc, t_atom *argv)
{
    mapper_signal msig;
    mapper_metronome m;
    const char *sig_name = NULL;

    if (argc < 2) {
        return;
    }
    if (argv->a_type != A_SYM || (argv+1)->a_type != A_SYM) {
        post("Unable to parse remove message!");
        return;
    }

    sig_name = maxpd_atom_get_string(argv+1);

    if (maxpd_atom_strcmp(argv, "output") == 0) {
        if ((msig=mdev_get_output_by_name(x->device, sig_name, 0))) {
            mdev_remove_output(x->device, msig);
            maxpd_atom_set_int(x->buffer, mdev_num_outputs(x->device));
            outlet_anything(x->outlet2, gensym("numOutputs"), 1, x->buffer);
        }
    }
    else if (maxpd_atom_strcmp(argv, "input") == 0) {
        if ((msig=mdev_get_input_by_name(x->device, sig_name, 0))) {
            mdev_remove_input(x->device, msig);
            maxpd_atom_set_int(x->buffer, mdev_num_inputs(x->device));
            outlet_anything(x->outlet2, gensym("numInputs"), 1, x->buffer);
        }
    }
    else if (maxpd_atom_strcmp(argv, "metronome") == 0) {
        if ((m=mdev_get_metronome_by_name(x->device, sig_name, 0)))
            mdev_remove_metronome(x->device, m);
    }
}

// *********************************************************
// -(set signal value)--------------------------------------
static void mapper_set(t_mapper *x, t_symbol *s, int argc, t_atom *argv)
{
    // This method sets the value of an input signal.
    // This allows storing of input signal state changes generated by user actions rather than
    // libmapper messaging. This state storage is used by libmapper for (e.g.) retreiving information
    // for training implicit mapping algorithms.

    int i;

    if (!argc)
        return;
    if (!x->ready)
        return;
    if (argv->a_type != A_SYM)
        return;

    // find matching input signal
    mapper_signal msig = mdev_get_input_by_name(x->device, maxpd_atom_get_string(argv), 0);
    if (!msig) {
        post("Error setting value: signal named \"%s\" does not exist!", maxpd_atom_get_string(argv));
        return;
    }

    // check if input instance release
    if ((argc == 3) && ((argv+2)->a_type == A_SYM)) {
        if (strcmp(maxpd_atom_get_string(argv+2), "mute") != 0)
            return;
        if ((argv+1)->a_type == A_FLOAT) {
            maybe_start_queue(x);
            msig_release_instance(msig, (int)atom_getfloat(argv+1), x->timetag);
        }
#ifdef MAXMSP
        else if ((argv+1)->a_type == A_LONG) {
            maybe_start_queue(x);
            msig_release_instance(msig, (int)atom_getlong(argv+1), x->timetag);
        }
#endif
        return;
    }

    // get signal properties
    mapper_db_signal props = msig_properties(msig);
    if (props->length != argc - 1) {
        post("Error: vector length (%i) does not match signal definition (%i)!", argc - 1, props->length);
        return;
    }
    if (props->type == 'i') {
        int payload[props->length];
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
        msig_update(msig, payload, 1, x->timetag);
    }
    else if (props->type == 'f') {
        float payload[props->length];
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
        msig_update(msig, payload, 1, x->timetag);
    }
    else {
        return;
    }
}

// *********************************************************
// -(anything)----------------------------------------------
static void mapper_anything(t_mapper *x, t_symbol *s, int argc, t_atom *argv)
{
    if (!x->ready)
        return;

    int i = 0, j = 0, id = -1;
    if (argc) {
        //find signal
        mapper_signal msig;
        if (!(msig=mdev_get_output_by_name(x->device, s->s_name, 0))) {
            if (x->learn_mode) {
                // register as new signal
                if (argv->a_type == A_FLOAT) {
                    msig = mdev_add_output(x->device, s->s_name, argc, 'f', 0, 0, 0);
                }
#ifdef MAXMSP
                else if (argv->a_type == A_LONG) {
                    msig = mdev_add_output(x->device, s->s_name, argc, 'i', 0, 0, 0);
                }
#endif
                else {
                    return;
                }
                //output updated numOutputs
#ifdef MAXMSP
                atom_setsym(x->buffer, gensym("numOutputs"));
                atom_setlong(x->buffer + 1, mdev_num_outputs(x->device));
                outlet_anything(x->outlet2, ps_list, 2, x->buffer);
#else
                SETFLOAT(x->buffer, mdev_num_outputs(x->device));
                outlet_anything(x->outlet2, gensym("numOutputs"), 1, x->buffer);
#endif
            }
            else {
                return;
            }
        }
        mapper_db_signal props = msig_properties(msig);

        if (argc == props->length + 1) {
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
                post("Instance ID is not int or float!");
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
            if (maxpd_atom_strcmp(argv+1, "mute") == 0)
                msig_release_instance(msig, id, x->timetag);
            else if (maxpd_atom_strcmp(argv+1, "new") == 0)
                msig_start_new_instance(msig, id);
        }
        else if (props->type == 'i') {
            int payload[props->length];
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
                msig_update(msig, payload, 1, x->timetag);
            }
            else {
                msig_update_instance(msig, id, payload, 1, x->timetag);
            }
        }
        else if (props->type == 'f') {
            float payload[props->length];
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
                msig_update(msig, payload, 1, x->timetag);
            }
            else {
                msig_update_instance(msig, id, payload, 1, x->timetag);
            }
        }
        else {
            return;
        }
    }
}

// *********************************************************
// -(int handler)-------------------------------------------
static void mapper_int_handler(mapper_signal msig, mapper_db_signal props,
                               int instance_id, void *value, int count,
                               mapper_timetag_t *tt)
{
    t_mapper *x = props->user_data;
    int poly = 0;
    if (props->num_instances > 1) {
        maxpd_atom_set_int(x->buffer, instance_id);
        poly = 1;
    }
    if (value) {
        int i, length = props->length;
        int *v = value;

        if (length > (MAX_LIST-1)) {
            post("Maximum list length is %i!", MAX_LIST-1);
            length = MAX_LIST-1;
        }

        for (i = 0; i < length; i++)
            maxpd_atom_set_int(x->buffer + i + poly, v[i]);
        outlet_anything(x->outlet1, gensym((char *)props->name),
                        length + poly, x->buffer);
    }
    else if (poly) {
        maxpd_atom_set_string(x->buffer + 1, "mute");
        outlet_anything(x->outlet1, gensym((char *)props->name),
                        2, x->buffer);
    }
}

// *********************************************************
// -(float handler)-----------------------------------------
static void mapper_float_handler(mapper_signal msig, mapper_db_signal props,
                                 int instance_id, void *value, int count,
                                 mapper_timetag_t *time)
{
    t_mapper *x = props->user_data;
    int poly = 0;
    if (props->num_instances > 1) {
        maxpd_atom_set_int(x->buffer, instance_id);
        poly = 1;
    }
    if (value) {
        int i, length = props->length;
        float *v = value;

        if (length > (MAX_LIST-1)) {
            post("Maximum list length is %i!", MAX_LIST-1);
            length = MAX_LIST-1;
        }

        for (i = 0; i < length; i++)
            maxpd_atom_set_float(x->buffer + i + poly, v[i]);
        outlet_anything(x->outlet1, gensym((char *)props->name),
                        length + poly, x->buffer);
    }
    else if (poly) {
        maxpd_atom_set_string(x->buffer + 1, "mute");
        outlet_anything(x->outlet1, gensym((char *)props->name),
                        2, x->buffer);
    }
}

// *********************************************************
// -(instance release request handler)----------------------
static void mapper_release_handler(mapper_signal sig, mapper_db_signal props,
                                   int instance_id, msig_instance_event_t event)
{
    t_mapper *x = props->user_data;
    maybe_start_queue(x);
    msig_release_instance(sig, instance_id, x->timetag);
}

// *********************************************************
// -(metronome handler)----------------------
static void mapper_metro_handler(mapper_metronome m, unsigned int bar,
                                 unsigned int beat, void *user_data)
{
    t_mapper *x = user_data;
    maxpd_atom_set_int(x->buffer, bar);
    maxpd_atom_set_int(x->buffer+1, beat);
    outlet_anything(x->outlet1, gensym(mapper_metronome_name(m)), 2, x->buffer);
}

// *********************************************************
// -(read device definition - maxmsp only)------------------
static void mapper_read_definition (t_mapper *x)
{
#ifdef MAXMSP
    if (x->d) {
        object_free(x->d);
    }
    t_object *info;
    t_symbol *sym_device = gensym("device");
    t_symbol *sym_name = gensym("name");
    const char *my_name = 0;
    short path;
    long filetype = 'JSON', outtype;

    // TODO: add ".json" to end of string if missing (or pick new filetype!)

    if (locatefile_extended(x->definition, &path, &outtype, &filetype, 1) == 0) {
        post("located file %s", x->definition);
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
            post("Could not parse file %s", x->definition);
        }
    }
    else {
        post("Could not locate file %s", x->definition);
    }
#endif
}

// *********************************************************
// -(register signals from dictionary - maxmsp only)--------
#ifdef MAXMSP
static void mapper_register_signals(t_mapper *x) {
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
    if (dictionary_getatomarray((t_dictionary *)device, sym_inputs, &inputs) == MAX_ERR_NONE) {
        atomarray_getatoms((t_atomarray *)inputs, &num_signals, &signals);
        // iterate through array of atoms
        for (i=0; i<num_signals; i++) {
            // initialize variables
            range_known[0] = 0;
            range_known[1] = 0;

            // each atom object points to a dictionary, need to recover atoms by key
            temp = atom_getobj(&(signals[i]));
            if (dictionary_getstring((t_dictionary *)temp, sym_name, &sig_name) != MAX_ERR_NONE)
                continue;
            if (dictionary_getstring((t_dictionary *)temp, sym_type, &sig_type) != MAX_ERR_NONE)
                continue;
            if (dictionary_getlong((t_dictionary *)temp, sym_length, &sig_length) != MAX_ERR_NONE)
                sig_length = 1;
            if (dictionary_getstring((t_dictionary *)temp, sym_units, &sig_units) != MAX_ERR_NONE)
                sig_units = 0;

            if (dictionary_getfloat((t_dictionary *)temp, sym_minimum, &sig_min_double) == MAX_ERR_NONE) {
                sig_min_float = (float)sig_min_double;
                sig_min_int = (int)sig_min_double;
                range_known[0] = 1;
            }
            else if (dictionary_getlong((t_dictionary *)temp, sym_minimum, &sig_min_long) == MAX_ERR_NONE) {
                sig_min_float = (float)sig_min_long;
                sig_min_int = (int)sig_min_long;
                range_known[0] = 1;
            }
            if (dictionary_getfloat((t_dictionary *)temp, sym_maximum, &sig_max_double) == MAX_ERR_NONE) {
                sig_max_float = (float)sig_max_double;
                sig_max_int = (int)sig_max_double;
                range_known[1] = 1;
            }
            else if (dictionary_getlong((t_dictionary *)temp, sym_maximum, &sig_max_long) == MAX_ERR_NONE) {
                sig_max_float = (float)sig_max_long;
                sig_max_int = (int)sig_max_long;
                range_known[1] = 1;
            }
            if ((strcmp(sig_type, "int") == 0) || (strcmp(sig_type, "i") == 0))
                sig_type_char = 'i';
            else if ((strcmp(sig_type, "float") == 0) || (strcmp(sig_type, "f") == 0))
                sig_type_char = 'f';
            else {
                post("Skipping registration of signal %s (unknown type).", sig_name);
                continue;
            }

            temp_sig = mdev_add_input(x->device, sig_name, (int)sig_length, sig_type_char, sig_units, 0, 0,
                                      sig_type_char == 'i' ? mapper_int_handler : mapper_float_handler, x);

            if (temp_sig) {
                if (range_known[0]) {
                    msig_set_minimum(temp_sig, sig_type_char == 'i' ? (void *)&sig_min_int : (void *)&sig_min_float);
                }
                if (range_known[1]) {
                    msig_set_maximum(temp_sig, sig_type_char == 'i' ? (void *)&sig_max_int : (void *)&sig_max_float);
                }
            }
        }
    }

    // Get pointer to atom array "outputs"
    if (dictionary_getatomarray((t_dictionary *)device, sym_outputs, &outputs) == MAX_ERR_NONE) {
        atomarray_getatoms((t_atomarray *)outputs, &num_signals, &signals);
        // iterate through array of atoms
        for (i=0; i<num_signals; i++) {
            // initialize variables
            range_known[0] = 0;
            range_known[1] = 0;

            // each atom object points to a dictionary, need to recover atoms by key
            temp = atom_getobj(&(signals[i]));
            if (dictionary_getstring((t_dictionary *)temp, sym_name, &sig_name) != MAX_ERR_NONE)
                continue;
            if (dictionary_getstring((t_dictionary *)temp, sym_type, &sig_type) != MAX_ERR_NONE)
                continue;
            if (dictionary_getlong((t_dictionary *)temp, sym_length, &sig_length) != MAX_ERR_NONE)
                sig_length = 1;
            if (dictionary_getstring((t_dictionary *)temp, sym_units, &sig_units) != MAX_ERR_NONE)
                sig_units = 0;

            if (dictionary_getfloat((t_dictionary *)temp, sym_minimum, &sig_min_double) == MAX_ERR_NONE) {
                sig_min_float = (float)sig_min_double;
                sig_min_int = (int)sig_min_double;
                range_known[0] = 1;
            }
            else if (dictionary_getlong((t_dictionary *)temp, sym_minimum, &sig_min_long) == MAX_ERR_NONE) {
                sig_min_float = (float)sig_min_long;
                sig_min_int = (int)sig_min_long;
                range_known[0] = 1;
            }
            if (dictionary_getfloat((t_dictionary *)temp, sym_maximum, &sig_max_double) == MAX_ERR_NONE) {
                sig_max_float = (float)sig_max_double;
                sig_max_int = (int)sig_max_double;
                range_known[1] = 1;
            }
            else if (dictionary_getlong((t_dictionary *)temp, sym_maximum, &sig_max_long) == MAX_ERR_NONE) {
                sig_max_float = (float)sig_max_long;
                sig_max_int = (int)sig_max_long;
                range_known[1] = 1;
            }
            if ((strcmp(sig_type, "int") == 0) || (strcmp(sig_type, "i") == 0))
                sig_type_char = 'i';
            else if ((strcmp(sig_type, "float") == 0) || (strcmp(sig_type, "f") == 0))
                sig_type_char = 'f';
            else {
                post("Skipping registration of signal %s (unknown type).", sig_name);
                continue;
            }

            temp_sig = mdev_add_output(x->device, sig_name, (int)sig_length, sig_type_char, sig_units, 0, 0);

            if (temp_sig) {
                if (range_known[0]) {
                    msig_set_minimum(temp_sig, sig_type_char == 'i' ? (void *)&sig_min_int : (void *)&sig_min_float);
                }
                if (range_known[1]) {
                    msig_set_maximum(temp_sig, sig_type_char == 'i' ? (void *)&sig_max_int : (void *)&sig_max_float);
                }
            }
        }
    }
}
#endif

// *********************************************************
// -(poll libmapper)----------------------------------------
static void mapper_poll(t_mapper *x)
{
    int count = 10;
    while(count-- && mdev_poll(x->device, 0)) {};
    if (!x->ready) {
        if (mdev_ready(x->device)) {
            //mapper_db_dump(db);
            x->ready = 1;
            mapper_print_properties(x);
        }
    }
    else if (x->updated) {
        mdev_send_queue(x->device, x->timetag);
        x->updated = 0;
    }
    clock_delay(x->clock, INTERVAL);  // Set clock to go off after delay
}

// *********************************************************
// -(poll libmapper)----------------------------------------
static void mapper_sync_status(t_mapper *x)
{
    maxpd_atom_set_float(x->buffer, (float)mdev_get_clock_offset(x->device));
    outlet_anything(x->outlet2, gensym("clock_offset"), 1, x->buffer);

    maxpd_atom_set_float(x->buffer, (float)mdev_get_sync_jitter(x->device));
    outlet_anything(x->outlet2, gensym("sync_jitter"), 1, x->buffer);

    clock_delay(x->status_clock, 1000);  // Set clock to go off after delay
}

// *********************************************************
// -(toggle learning mode)----------------------------------
static void mapper_learn(t_mapper *x, t_symbol *s, int argc, t_atom *argv)
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
            if (mode == 0)
                post("Learning mode off.");
            else
                post("Learning mode on.");
        }
    }
}

static void maybe_start_queue(t_mapper *x)
{
    if (!x->updated) {
        mdev_timetag_now(x->device, &x->timetag);
        mdev_start_queue(x->device, x->timetag);
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
