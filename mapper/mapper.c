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
#include <mpr/mpr.h>
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
    mpr_graph graph;
    mpr_dev device;
    mpr_time timetag;
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

static void mapperobj_sig_handler(mpr_sig sig, mpr_sig_evt evt, mpr_id inst,
                                  int len, mpr_type type, const void *val,
                                  mpr_time time);

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
             mpr_version());

        x->device = mpr_dev_new(x->name, 0);
        if (!x->device) {
            POST(x, "Error initializing libmapper device.");
            return 0;
        }
        x->graph = mpr_obj_graph(x->device);
        if (iface)
            mpr_graph_set_interface(x->graph, iface);
        POST(x, "Using network interface %s.", mpr_graph_interface(x->graph));

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
                        mpr_obj_set_prop(x->device, MPR_PROP_UNKNOWN,
                                         maxpd_atom_get_string(argv+i)+1, 1,
                                         MPR_STR, value, 1);
                        i++;
                        break;
                    }
                    case A_FLOAT:
                    {
                        float value = maxpd_atom_get_float(argv+i+1);
                        mpr_obj_set_prop(x->device, MPR_PROP_UNKNOWN,
                                         maxpd_atom_get_string(argv+i)+1, 1,
                                         MPR_FLT, &value, 1);
                        i++;
                        break;
                    }
#ifdef MAXMSP
                    case A_LONG:
                    {
                        int value = atom_getlong(argv+i+1);
                        mpr_obj_set_prop(x->device, MPR_PROP_UNKNOWN,
                                         maxpd_atom_get_string(argv+i)+1, 1,
                                         MPR_INT32, &value, 1);
                        i++;
                        break;
                    }
#endif
                    default:
                        break;
                }
            }
        }

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
        mpr_dev_free(x->device);
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
        maxpd_atom_set_string(x->buffer, mpr_obj_prop_as_str(x->device, MPR_PROP_NAME, NULL));
        outlet_anything(x->outlet2, gensym("name"), 1, x->buffer);

        //output interface
        maxpd_atom_set_string(x->buffer, mpr_graph_interface(x->graph));
        outlet_anything(x->outlet2, gensym("interface"), 1, x->buffer);

        //output IP
        maxpd_atom_set_string(x->buffer, mpr_graph_address(x->graph));
        outlet_anything(x->outlet2, gensym("IP"), 1, x->buffer);

        //output port
        maxpd_atom_set_int(x->buffer, mpr_obj_prop_as_i32(x->device, MPR_PROP_PORT, NULL));
        outlet_anything(x->outlet2, gensym("port"), 1, x->buffer);

        //output ordinal
        maxpd_atom_set_int(x->buffer, mpr_obj_prop_as_i32(x->device, MPR_PROP_ORDINAL, NULL));
        outlet_anything(x->outlet2, gensym("ordinal"), 1, x->buffer);

        //output numInputs
        maxpd_atom_set_int(x->buffer, mpr_list_count(mpr_dev_sigs(x->device, MPR_DIR_IN)));
        outlet_anything(x->outlet2, gensym("numInputs"), 1, x->buffer);

        //output numOutputs
        maxpd_atom_set_int(x->buffer, mpr_list_count(mpr_dev_sigs(x->device, MPR_DIR_OUT)));
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
    long i;
    mpr_sig sig = 0;
    mpr_dir dir;

    if (argc < 4) {
        POST(x, "Not enough arguments for 'add' message.");
        return;
    }

    if ((argv->a_type != A_SYM) || ((argv+1)->a_type != A_SYM))
        return;

    if (maxpd_atom_strcmp(argv, "input") == 0)
        dir = MPR_DIR_IN;
    else if (maxpd_atom_strcmp(argv, "output") == 0)
        dir = MPR_DIR_OUT;
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

    sig = mpr_sig_new(x->device, dir, sig_name, sig_length, sig_type, sig_units,
                      0, 0, 0, mapperobj_sig_handler, MPR_SIG_ALL);
    mpr_obj_set_prop(sig, MPR_PROP_DATA, NULL, 1, MPR_PTR, x, 0);
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
                float val = maxpd_atom_get_float(argv+i+1);
                mpr_obj_set_prop(sig, MPR_PROP_MIN, NULL, 1, MPR_FLT, &val, 1);
                i++;
            }
#ifdef MAXMSP
            else if ((argv + i + 1)->a_type == A_LONG) {
                int val = (int)atom_getlong(argv+i+1);
                mpr_obj_set_prop(sig, MPR_PROP_MIN, NULL, 1, MPR_INT32, &val, 1);
                i++;
            }
#endif
        }
        else if (maxpd_atom_strcmp(argv+i, "@max") == 0) {
            if ((argv+i+1)->a_type == A_FLOAT) {
                float val = maxpd_atom_get_float(argv+i+1);
                mpr_obj_set_prop(sig, MPR_PROP_MAX, NULL, 1, MPR_FLT, &val, 1);
                i++;
            }
#ifdef MAXMSP
            else if ((argv + i + 1)->a_type == A_LONG) {
                int val = (int)atom_getlong(argv+i+1);
                mpr_obj_set_prop(sig, MPR_PROP_MAX, NULL, 1, MPR_INT32, &val, 1);
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
                mpr_sig_reserve_inst(sig, prop_int - 1, 0, 0);
            }
        }
        else if (maxpd_atom_strcmp(argv+i, "@stealing") == 0) {
            if ((argv+i+1)->a_type == A_SYM) {
                int stl = MPR_STEAL_NONE;
                if (maxpd_atom_strcmp(argv+i+1, "newest") == 0) {
                    stl = MPR_STEAL_NEWEST;
                }
                if (maxpd_atom_strcmp(argv+i+1, "oldest") == 0) {
                    stl = MPR_STEAL_OLDEST;
                }
                mpr_obj_set_prop(sig, MPR_PROP_STEAL_MODE, NULL, 1, MPR_INT32, &stl, 1);
                i++;
            }
        }
        else if (maxpd_atom_get_string(argv+i)[0] == '@') {
            switch ((argv+i+1)->a_type) {
                case A_SYM: {
                    const char *value = maxpd_atom_get_string(argv+i+1);
                    mpr_obj_set_prop(sig, MPR_PROP_UNKNOWN,
                                     maxpd_atom_get_string(argv+i)+1, 1,
                                     MPR_STR, value, 1);
                    i++;
                    break;
                }
                case A_FLOAT:
                {
                    float value = maxpd_atom_get_float(argv+i+1);
                    mpr_obj_set_prop(sig, MPR_PROP_UNKNOWN,
                                     maxpd_atom_get_string(argv+i)+1, 1,
                                     MPR_FLT, &value, 1);
                    i++;
                    break;
                }
#ifdef MAXMSP
                case A_LONG:
                {
                    int value = atom_getlong(argv+i+1);
                    mpr_obj_set_prop(sig, MPR_PROP_UNKNOWN,
                                     maxpd_atom_get_string(argv+i)+1, 1,
                                     MPR_INT32, &value, 1);
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
    maxpd_atom_set_int(x->buffer, mpr_list_count(mpr_dev_sigs(x->device, dir)));
    if (dir == MPR_DIR_OUT)
        outlet_anything(x->outlet2, gensym("numOutputs"), 1, x->buffer);
    else
        outlet_anything(x->outlet2, gensym("numInputs"), 1, x->buffer);
}

// *********************************************************
// -(remove signal)-----------------------------------------
static void mapperobj_remove_signal(t_mapper *x, t_symbol *s,
                                    int argc, t_atom *argv)
{
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

    mpr_list sigs = mpr_dev_sigs(x->device, MPR_DIR_ANY);
    sigs = mpr_list_filter(sigs, MPR_PROP_NAME, NULL, 1, MPR_STR, sig_name, MPR_OP_EQ);
    if (sigs && *sigs)
        mpr_sig_free(*sigs);
    if (strcmp(direction, "output") == 0) {
        maxpd_atom_set_int(x->buffer, mpr_list_count(mpr_dev_sigs(x->device, MPR_DIR_OUT)));
        outlet_anything(x->outlet2, gensym("numOutputs"), 1, x->buffer);
    }
    else if (strcmp(direction, "input") == 0) {
        maxpd_atom_set_int(x->buffer, mpr_list_count(mpr_dev_sigs(x->device, MPR_DIR_IN)));
        outlet_anything(x->outlet2, gensym("numInputs"), 1, x->buffer);
    }
}

// *********************************************************
// -(clear all signals)-------------------------------------
static void mapperobj_clear_signals(t_mapper *x, t_symbol *s,
                                    int argc, t_atom *argv)
{
    mpr_dir dir = 0;

    if (!argc)
        dir = MPR_DIR_ANY;
    else if (maxpd_atom_strcmp(argv, "inputs") == 0)
        dir |= MPR_DIR_IN;
    else if (maxpd_atom_strcmp(argv, "outputs") == 0)
        dir |= MPR_DIR_OUT;
    else
        return;

    mpr_list sigs;
    POST(x, "Clearing signals");
    sigs = mpr_dev_sigs(x->device, dir);
    while (sigs) {
        mpr_sig sig = *sigs;
        sigs = mpr_list_next(sigs);
        mpr_sig_free(sig);
    }

    if (dir & MPR_DIR_IN) {
        maxpd_atom_set_int(x->buffer, mpr_list_count(mpr_dev_sigs(x->device, MPR_DIR_IN)));
        outlet_anything(x->outlet2, gensym("numInputs"), 1, x->buffer);
    }
    if (dir & MPR_DIR_OUT) {
        maxpd_atom_set_int(x->buffer, mpr_list_count(mpr_dev_sigs(x->device, MPR_DIR_OUT)));
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

    // forward to 'anything' handler
    mapperobj_anything(x, s, argc, argv);
}

// *********************************************************
// -(anything)----------------------------------------------
static void mapperobj_anything(t_mapper *x, t_symbol *s, int argc, t_atom *argv)
{
    if (!x->ready)
        return;

    int i = 0, j = 0, id, *id_ptr = NULL;
    if (!argc)
        return;

    //find signal
    mpr_sig sig = NULL;
    mpr_list sigs = mpr_dev_sigs(x->device, MPR_DIR_ANY);
    sigs = mpr_list_filter(sigs, MPR_PROP_NAME, NULL, 1, MPR_STR, s->s_name, MPR_OP_EQ);
    if (sigs && *sigs)
        sig = *sigs;

    if (!sig) {
        if (x->learn_mode) {
            // register as new signal
            if (argv->a_type == A_FLOAT) {
                sig = mpr_sig_new(x->device, MPR_DIR_OUT, s->s_name, argc,
                                  MPR_FLT, 0, 0, 0, 0, 0, 0);
            }
#ifdef MAXMSP
            else if (argv->a_type == A_LONG) {
                sig = mpr_sig_new(x->device, MPR_DIR_OUT, s->s_name, argc,
                                  MPR_INT32, 0, 0, 0, 0, 0, 0);
            }
#endif
            else {
                return;
            }
            //output updated numOutputs
            maxpd_atom_set_float(x->buffer, mpr_list_count(mpr_dev_sigs(x->device, MPR_DIR_OUT)));
            outlet_anything(x->outlet2, gensym("numOutputs"), 1, x->buffer);
        }
        else {
            return;
        }
    }

    int len = mpr_obj_prop_as_i32(sig, MPR_PROP_LEN, NULL);
    char type = mpr_obj_prop_as_i32(sig, MPR_PROP_TYPE, NULL);

    if (argc == len + 1) {
        // Special case: signal value may be preceded by instance number
        if ((argv)->a_type == A_FLOAT) {
            id = (int)maxpd_atom_get_float(argv);
            id_ptr = &id;
            j = 1;
        }
#ifdef MAXMSP
        else if ((argv)->a_type == A_LONG) {
            id = (int)atom_getlong(argv);
            id_ptr = &id;
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
            mpr_sig_release_inst(sig, id, x->timetag);
    }
    else if (type == 'i') {
        int payload[len];
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
        mpr_sig_set_value(sig, id_ptr, len, MPR_INT32, payload, x->timetag);
    }
    else if (type == 'f') {
        float payload[len];
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
        mpr_sig_set_value(sig, id_ptr, len, MPR_FLT, payload, x->timetag);
    }
    else {
        return;
    }
}

// *********************************************************
// -(sig handler)-------------------------------------------
static void mapperobj_sig_handler(mpr_sig sig, mpr_sig_evt evt, mpr_id inst,
                                  int len, mpr_type type, const void *val,
                                  mpr_time time)
{
    t_mapper *x = mpr_obj_prop_as_ptr(sig, MPR_PROP_DATA, NULL);
    t_symbol *name = gensym(mpr_obj_prop_as_str(sig, MPR_PROP_NAME, NULL));

    switch (evt) {
        case MPR_SIG_UPDATE: {
            int poly = 0;
            if (mpr_sig_num_inst(sig, MPR_STATUS_ALL) > 1) {
                maxpd_atom_set_int(x->buffer, inst);
                poly = 1;
            }
            if (val) {
                int i;
                int *v = (int*)val;

                if (len > (MAX_LIST-1)) {
                    POST(x, "Maximum list length is %i!", MAX_LIST-1);
                    len = MAX_LIST-1;
                }

                for (i = 0; i < len; i++)
                    maxpd_atom_set_int(x->buffer + i + poly, v[i]);
                outlet_anything(x->outlet1, name, len + poly, x->buffer);
            }
            else if (poly) {
                maxpd_atom_set_string(x->buffer+1, "release");
                maxpd_atom_set_string(x->buffer+2, "local");
                outlet_anything(x->outlet1, name, 3, x->buffer);
            }
            break;
        }
        case MPR_SIG_REL_UPSTRM:
            maxpd_atom_set_int(x->buffer, inst);
            maxpd_atom_set_string(x->buffer+1, "release");
            maxpd_atom_set_string(x->buffer+2, "upstream");
            outlet_anything(x->outlet1, name, 3, x->buffer);
            break;
        case MPR_SIG_REL_DNSTRM:
            maxpd_atom_set_int(x->buffer, inst);
            maxpd_atom_set_string(x->buffer+1, "release");
            maxpd_atom_set_string(x->buffer+2, "downstream");
            outlet_anything(x->outlet1, name, 3, x->buffer);
            break;
        case MPR_SIG_INST_OFLW: {
            maxpd_atom_set_int(x->buffer, inst);
            int mode = mpr_obj_prop_as_i32(sig, MPR_PROP_STEAL_MODE, NULL);
            switch (mode) {
                case MPR_STEAL_OLDEST:
                    inst = mpr_sig_oldest_inst_id(sig);
                    if (inst)
                        mpr_sig_release_inst(sig, inst, time);
                    break;
                case MPR_STEAL_NEWEST:
                    inst = mpr_sig_newest_inst_id(sig);
                    if (inst)
                        mpr_sig_release_inst(sig, inst, time);
                    break;
                case 0:
                    maxpd_atom_set_string(x->buffer+1, "overflow");
                    outlet_anything(x->outlet1, name, 2, x->buffer);
                    break;
                default:
                    break;
            }
            break;
        }
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

    const char *sig_name, *sig_units, *sig_type_str;
    mpr_type sig_type = 0;
    double val_d;
    long long val_l, sig_length;

    mpr_sig temp_sig;
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
                                     &sig_type_str) != MAX_ERR_NONE)
                continue;
            if (dictionary_getlong((t_dictionary *)temp, sym_length,
                                   &sig_length) != MAX_ERR_NONE)
                sig_length = 1;
            if (dictionary_getstring((t_dictionary *)temp, sym_units,
                                     &sig_units) != MAX_ERR_NONE)
                sig_units = 0;

            if ((strcmp(sig_type_str, "int") == 0) || (strcmp(sig_type_str, "i") == 0))
                sig_type = MPR_INT32;
            else if ((strcmp(sig_type_str, "float") == 0) || (strcmp(sig_type_str, "f") == 0))
                sig_type = MPR_FLT;
            else {
                POST(x, "Skipping registration of signal %s (unknown type).",
                     sig_name);
                continue;
            }

            temp_sig = mpr_sig_new(x->device, MPR_DIR_IN, sig_name,
                                   (int)sig_length, sig_type, sig_units,
                                   0, 0, 0, mapperobj_sig_handler, MPR_SIG_ALL);
            mpr_obj_set_prop(temp_sig, MPR_PROP_DATA, NULL, 1, MPR_PTR, x, 0);

            if (!temp_sig)
                continue;

            if (dictionary_getfloat((t_dictionary *)temp, sym_minimum,
                                    &val_d) == MAX_ERR_NONE) {
                mpr_obj_set_prop(temp_sig, MPR_PROP_MIN, NULL, 1, MPR_DBL,
                                 &val_d, 1);
            }
            else if (dictionary_getlong((t_dictionary *)temp, sym_minimum,
                                        &val_l) == MAX_ERR_NONE) {
                int val_i = (int)val_l;
                mpr_obj_set_prop(temp_sig, MPR_PROP_MIN, NULL, 1, MPR_INT32,
                                 &val_i, 1);
            }
            if (dictionary_getfloat((t_dictionary *)temp, sym_maximum,
                                    &val_d) == MAX_ERR_NONE) {
                mpr_obj_set_prop(temp_sig, MPR_PROP_MAX, NULL, 1, MPR_DBL,
                                 &val_d, 1);
            }
            else if (dictionary_getlong((t_dictionary *)temp, sym_maximum,
                                        &val_l) == MAX_ERR_NONE) {
                int val_i = (int)val_l;
                mpr_obj_set_prop(temp_sig, MPR_PROP_MAX, NULL, 1, MPR_INT32,
                                 &val_i, 1);
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
                                     &sig_type_str) != MAX_ERR_NONE)
                continue;
            if (dictionary_getlong((t_dictionary *)temp, sym_length,
                                   &sig_length) != MAX_ERR_NONE)
                sig_length = 1;
            if (dictionary_getstring((t_dictionary *)temp, sym_units,
                                     &sig_units) != MAX_ERR_NONE)
                sig_units = 0;

            if ((strcmp(sig_type_str, "int") == 0) || (strcmp(sig_type_str, "i") == 0))
                sig_type = MPR_INT32;
            else if ((strcmp(sig_type_str, "float") == 0) || (strcmp(sig_type_str, "f") == 0))
                sig_type = MPR_FLT;
            else {
                POST("Skipping registration of signal %s (unknown type).",
                     sig_name);
                continue;
            }

            temp_sig = mpr_sig_new(x->device, MPR_DIR_OUT, sig_name,
                                   (int)sig_length, sig_type, sig_units,
                                   0, 0, 0, 0, 0);

            if (!temp_sig)
                continue;

            if (dictionary_getfloat((t_dictionary *)temp, sym_minimum,
                                    &val_d) == MAX_ERR_NONE) {
                mpr_obj_set_prop(temp_sig, MPR_PROP_MIN, NULL, 1, MPR_DBL,
                                 &val_d, 1);
            }
            else if (dictionary_getlong((t_dictionary *)temp, sym_minimum,
                                        &val_l) == MAX_ERR_NONE) {
                int val_i = (int)val_l;
                mpr_obj_set_prop(temp_sig, MPR_PROP_MIN, NULL, 1, MPR_INT32,
                                 &val_i, 1);
            }
            if (dictionary_getfloat((t_dictionary *)temp, sym_maximum,
                                    &val_d) == MAX_ERR_NONE) {
                mpr_obj_set_prop(temp_sig, MPR_PROP_MAX, NULL, 1, MPR_DBL,
                                 &val_d, 1);
            }
            else if (dictionary_getlong((t_dictionary *)temp, sym_maximum,
                                        &val_l) == MAX_ERR_NONE) {
                int val_i = (int)val_l;
                mpr_obj_set_prop(temp_sig, MPR_PROP_MAX, NULL, 1, MPR_INT32,
                                 &val_i, 1);
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
    while(count-- && mpr_dev_poll(x->device, 0)) {};
    if (!x->ready) {
        if (mpr_dev_ready(x->device)) {
            POST(x, "Joining mapping network as '%s'",
                 mpr_obj_prop_as_str(x->device, MPR_PROP_NAME, NULL));
            x->ready = 1;
#ifdef MAXMSP
            defer_low((t_object *)x, (method)mapperobj_print_properties, NULL, 0, NULL);
#else
            mapperobj_print_properties(x);
#endif
        }
    }
    else if (x->updated) {
        mpr_dev_send_queue(x->device, x->timetag);
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
        mpr_time_now(&x->timetag);
        mpr_dev_start_queue(x->device, x->timetag);
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
