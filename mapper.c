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
#include <arpa/inet.h>

#include <unistd.h>

#define INTERVAL 1
#define MAX_LIST 256

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
    mapper_admin admin;
    mapper_device device;
    int ready;
    int learn_mode;
    t_atom buffer[MAX_LIST];
    char *definition;
#ifdef MAXMSP
    t_dictionary *d;
#endif
} t_mapper;

static t_symbol *ps_list;
static int port = 9000;

// *********************************************************
// -(function prototypes)-----------------------------------
static void *mapper_new(t_symbol *s, int argc, t_atom *argv);
static void mapper_free(t_mapper *x);
static void mapper_anything(t_mapper *x, t_symbol *s, int argc, t_atom *argv);
static void mapper_add_signal(t_mapper *x, t_symbol *s, int argc, t_atom *argv);
static void mapper_remove_signal(t_mapper *x, t_symbol *s, int argc, t_atom *argv);
static void mapper_poll(t_mapper *x);
static void mapper_float_handler(mapper_signal msig, mapper_db_signal props,
                                 mapper_timetag_t *time, void *value);
static void mapper_int_handler(mapper_signal msig, mapper_db_signal props,
                               mapper_timetag_t *time, void *value);
static void mapper_print_properties(t_mapper *x);
static void mapper_register_signals(t_mapper *x);
static void mapper_learn(t_mapper *x, t_symbol *s, int argc, t_atom *argv);
static void mapper_set(t_mapper *x, t_symbol *s, int argc, t_atom *argv);
static void mapper_read_definition(t_mapper *x);
#ifdef MAXMSP
    void mapper_assist(t_mapper *x, void *b, long m, long a, char *s);
#endif

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
        class_addmethod(c, (method)mapper_assist,         "assist",   A_CANT,     0);
        class_addmethod(c, (method)mapper_add_signal,     "add",      A_GIMME,    0);
        class_addmethod(c, (method)mapper_remove_signal,  "remove",   A_GIMME,    0);
        class_addmethod(c, (method)mapper_anything,       "anything", A_GIMME,    0);
        class_addmethod(c, (method)mapper_learn,          "learn",    A_GIMME,    0);
        class_addmethod(c, (method)mapper_set,            "set",      A_GIMME,    0);
        class_register(CLASS_BOX, c); /* CLASS_NOBOX */
        mapper_class = c;
        ps_list = gensym("list");
        return 0;
    }
#else
    int mapper_setup(void)
    {
        t_class *c;
        c = class_new(gensym("mapper"), (t_newmethod)mapper_new, (t_method)mapper_free, 
                      (long)sizeof(t_mapper), 0L, A_GIMME, 0);
        class_addmethod(c,   (t_method)mapper_add_signal,    gensym("add"),    A_GIMME, 0);
        class_addmethod(c,   (t_method)mapper_remove_signal, gensym("remove"), A_GIMME, 0);
        class_addanything(c, (t_method)mapper_anything);
        class_addmethod(c,   (t_method)mapper_learn,         gensym("learn"),  A_GIMME, 0);
        class_addmethod(c,   (t_method)mapper_set,           gensym("set"),    A_GIMME, 0);
        mapper_class = c;
        ps_list = gensym("list");
        return 0;
    }
#endif

// *********************************************************
// -(new)---------------------------------------------------
void *mapper_new(t_symbol *s, int argc, t_atom *argv)
{
    t_mapper *x = NULL;
    long i;
    int learn = 0;
    const char *alias = NULL;
    const char *iface = NULL;

#ifdef MAXMSP
    if (x = object_alloc(mapper_class)) {
        x->outlet2 = listout((t_object *)x);
        x->outlet1 = listout((t_object *)x);
        x->name = strdup("maxmsp");
#else
    if (x = (t_mapper *) pd_new(mapper_class) ) {
        x->outlet1 = outlet_new(&x->ob, gensym("list"));
        x->outlet2 = outlet_new(&x->ob, gensym("list"));
        x->name = strdup("puredata");
#endif

        for (i = 0; i < argc; i++) {
            if ((argv+i)->a_type == A_SYM) {
                if(strcmp(maxpd_atom_get_string(argv+i), "@alias") == 0) {
                    if ((argv+i+1)->a_type == A_SYM) {
                        alias = maxpd_atom_get_string(argv+i+1);
                        i++;
                    }
                }
                else if ((strcmp(maxpd_atom_get_string(argv+i), "@def") == 0) || 
                         (strcmp(maxpd_atom_get_string(argv+i), "@definition") == 0)) {
                    if ((argv+i+1)->a_type == A_SYM) {
                        x->definition = strdup(maxpd_atom_get_string(argv+i+1));
                        mapper_read_definition(x);
                        i++;
                    }
                }
                else if (strcmp(maxpd_atom_get_string(argv+i), "@learn") == 0) {
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
                else if (strcmp(maxpd_atom_get_string(argv+i), "@interface") == 0) {
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
        x->admin = mapper_admin_new(iface, 0, 0);
        if (!x->admin) {
            post("Error initializing admin.");
            return 0;
        }
        x->device = mdev_new(x->name, port, x->admin);
        if (!x->device) {
            post("Error initializing device.");
            return 0;
        }

        if (iface)
            post("mapper: trying interface %s", iface);
        else
            post("mapper: using default interface.");
        post("mapper: using name %s", x->name);
        mapper_print_properties(x);
        x->ready = 0;
        x->learn_mode = learn;
#ifdef MAXMSP
        mapper_register_signals(x);
        x->clock = clock_new(x, (method)mapper_poll);    // Create the timing clock
#else
        x->clock = clock_new(x, (t_method)mapper_poll);
#endif
        clock_delay(x->clock, INTERVAL);  // Set clock to go off after delay
    }
    return (x);
}

// *********************************************************
// -(free)--------------------------------------------------
void mapper_free(t_mapper *x)
{
    clock_unset(x->clock);    // Remove clock routine from the scheduler
    clock_free(x->clock);        // Frees memeory used by clock

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
void mapper_print_properties(t_mapper *x)
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
void mapper_add_signal(t_mapper *x, t_symbol *s, int argc, t_atom *argv)
{
    const char *sig_name = 0, *sig_units = 0;
    char sig_type = 0;
    int is_input, sig_min_int, sig_max_int, sig_length = 1;
    float sig_min_float, sig_max_float;
    long i;
    mapper_signal msig = 0;

    if (argc < 4) {
        post("mapper: not enough arguments for 'add' message.");
        return;
    }

    if ((argv->a_type != A_SYM) || ((argv+1)->a_type != A_SYM))
        return;

    if (strcmp(maxpd_atom_get_string(argv), "input") == 0)
        is_input = 1;
    else if (strcmp(maxpd_atom_get_string(argv), "output") == 0)
        is_input = 0;
    else
        return;

    // get signal name
    sig_name = maxpd_atom_get_string(argv+1);

    // get signal type, length, and units
    for (i = 2; i < argc; i++) {
        if (i > argc - 2) // need 2 arguments for key and value
            break;
        if ((argv+i)->a_type == A_SYM) {
            if (strcmp(maxpd_atom_get_string(argv+i), "@type") == 0) {
                if ((argv+i+1)->a_type == A_SYM) {
                    sig_type = maxpd_atom_get_string(argv+i+1)[0];
                    i++;
                }
            }
            else if (strcmp(maxpd_atom_get_string(argv+i), "@length") == 0) {
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
            else if(strcmp(maxpd_atom_get_string(argv+i), "@units") == 0) {
                if ((argv+i+1)->a_type == A_SYM) {
                    sig_units = maxpd_atom_get_string(argv+i+1);
                    i++;
                }
            }
        }
    }
    if (!sig_type) {
        post("mapper: signal has no declared type!");
        return;
    }
    if (sig_length < 1) {
        post("mapper: signals cannot have length < 1!");
        return;
    }

    if (is_input) {
        msig = mdev_add_input(x->device, sig_name, sig_length,
                              sig_type, sig_units, 0, 0,
                              sig_type == 'i' ? mapper_int_handler : mapper_float_handler, x);
        if (!msig) {
            post("mapper: error creating input!");
            return;
        }
    } 
    else {
        msig = mdev_add_output(x->device, sig_name, sig_length,
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
        if ((strcmp(maxpd_atom_get_string(argv+i), "@type") == 0) ||
            (strcmp(maxpd_atom_get_string(argv+i), "@length") == 0) ||
            (strcmp(maxpd_atom_get_string(argv+i), "@units") == 0)){
            i++;
            continue;
        }
        if (strcmp(maxpd_atom_get_string(argv+i), "@min") == 0) {
            if ((argv+i+1)->a_type == A_FLOAT) {
                sig_min_float = maxpd_atom_get_float(argv+i+1);
                sig_min_int = (int)sig_min_float;
                msig_set_minimum(msig, sig_type == 'i' ? (void *)&sig_min_int : (void *)&sig_min_float);
                i++;
            }
    #ifdef MAXMSP
            else if ((argv + i + 1)->a_type == A_LONG) {
                sig_min_int = (int)atom_getlong(argv+i+1);
                sig_min_float = (float)sig_min_int;
                msig_set_minimum(msig, sig_type == 'i' ? (void *)&sig_min_int : (void *)&sig_min_float);
                i++;
            }
    #endif
        }
        else if (strcmp(maxpd_atom_get_string(argv+i), "@max") == 0) {
            if ((argv+i+1)->a_type == A_FLOAT) {
                sig_max_float = maxpd_atom_get_float(argv+i+1);
                sig_max_int = (int)sig_max_float;
                msig_set_maximum(msig, sig_type == 'i' ? (void *)&sig_max_int : (void *)&sig_max_float);
                i++;
            }
    #ifdef MAXMSP
            else if ((argv + i + 1)->a_type == A_LONG) {
                sig_max_int = (int)atom_getlong(argv+i+1);
                sig_max_float = (float)sig_max_int;
                msig_set_maximum(msig, sig_type == 'i' ? (void *)&sig_max_int : (void *)&sig_max_float);
                i++;
            }
    #endif
        }
        else if (maxpd_atom_get_string(argv+i)[0] == '@') {
            lo_arg *value;
            switch ((argv+i+1)->a_type) {
                case A_SYM: {
                    value = (lo_arg *)(maxpd_atom_get_string(argv+i+1));
                    msig_set_property(msig, (maxpd_atom_get_string(argv+i)+1), LO_STRING, value);
                    i++;
                    break;
                }
                case A_FLOAT:
                    value->f = maxpd_atom_get_float(argv+i+1);
                    msig_set_property(msig, maxpd_atom_get_string(argv+i)+1, LO_FLOAT, value);
                    i++;
                    break;
#ifdef MAXMSP
                case A_LONG:
                    value->i32 = atom_getlong(argv+i+1);
                    msig_set_property(msig, maxpd_atom_get_string(argv+i)+1, LO_INT32, value);
                    i++;
                    break;
#endif
                default:
                    break;
            }
        }
    }    

    // Update status outlet
    if (is_input) {
        //output numInputs
        maxpd_atom_set_int(x->buffer, mdev_num_inputs(x->device));
        outlet_anything(x->outlet2, gensym("numInputs"), 1, x->buffer);
    } 
    else {
        //output numOutputs
        maxpd_atom_set_int(x->buffer, mdev_num_outputs(x->device));
        outlet_anything(x->outlet2, gensym("numOutputs"), 1, x->buffer);
    }
}

// *********************************************************
// -(remove signal)-----------------------------------------
void mapper_remove_signal(t_mapper *x, t_symbol *s, int argc, t_atom *argv)
{
    mapper_signal msig;
    char *sig_name = NULL, *direction = NULL;

    if (argc < 2) {
        return;
    }
    if (argv->a_type != A_SYM || (argv+1)->a_type != A_SYM) {
        post("Unable to parse remove message!");
        return;
    }
    direction = strdup(maxpd_atom_get_string(argv));
    sig_name = strdup(maxpd_atom_get_string(argv+1));

    if (strcmp(direction, "output") == 0) {
        if (msig=mdev_get_output_by_name(x->device, sig_name, 0)) {
            mdev_remove_output(x->device, msig);
            maxpd_atom_set_int(x->buffer, mdev_num_outputs(x->device));
            outlet_anything(x->outlet2, gensym("numOutputs"), 1, x->buffer);
        }
    }
    else if (strcmp(direction, "input") == 0) {
        if (msig=mdev_get_input_by_name(x->device, sig_name, 0)) {
            mdev_remove_input(x->device, msig);
            maxpd_atom_set_int(x->buffer, mdev_num_inputs(x->device));
            outlet_anything(x->outlet2, gensym("numInputs"), 1, x->buffer);
        }
    }
}
    
// *********************************************************
// -(set signal value)--------------------------------------
void mapper_set(t_mapper *x, t_symbol *s, int argc, t_atom *argv)
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
        msig_update(msig, payload);
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
        msig_update(msig, payload);
    }
    else {
        return;
    }
}

// *********************************************************
// -(anything)----------------------------------------------
void mapper_anything(t_mapper *x, t_symbol *s, int argc, t_atom *argv)
{
    int i;
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
        if (props->length != argc) {
            post("Error: vector length does not match signal definition!");
            return;
        }
        if (props->type == 'i') {
            int payload[props->length];
            for (i = 0; i < argc; i++) {
                if ((argv + i)->a_type == A_FLOAT)
                    payload[i] = (int)atom_getfloat(argv + i);
#ifdef MAXMSP
                else if ((argv + i)->a_type == A_LONG)
                    payload[i] = (int)atom_getlong(argv + i);
#endif
            }
            //update signal
            msig_update(msig, payload);
        }
        else if (props->type == 'f') {
            float payload[props->length];
            for (i = 0; i < argc; i++) {
                if ((argv + i)->a_type == A_FLOAT)
                    payload[i] = atom_getfloat(argv + i);
#ifdef MAXMSP
                else if ((argv + i)->a_type == A_LONG)
                    payload[i] = (float)atom_getlong(argv + i);
#endif
            }
            //update signal
            msig_update(msig, payload);
        }
        else {
            return;
        }
    }
}

// *********************************************************
// -(int handler)-------------------------------------------
void mapper_int_handler(mapper_signal msig, mapper_db_signal props, mapper_timetag_t *time, void *value)
{
    if (value) {
        t_mapper *x = props->user_data;
        int i, length = props->length;
        int *v = value;
        
        if (length > (MAX_LIST-1)) {
            post("Maximum list length is %i!", MAX_LIST-1);
            length = MAX_LIST-1;
        }

        for (i = 0; i < length; i++)
            maxpd_atom_set_int(x->buffer + i, v[i]);
        outlet_anything(x->outlet1, gensym((char *)props->name), length, x->buffer);
    }
}

// *********************************************************
// -(float handler)-----------------------------------------
void mapper_float_handler(mapper_signal msig, mapper_db_signal props, mapper_timetag_t *time, void *value)
{
    if (value) {
        t_mapper *x = props->user_data;
        int i, length = props->length;
        float *v = value;

        if (length > (MAX_LIST-1)) {
            post("Maximum list length is %i!", MAX_LIST-1);
            length = MAX_LIST-1;
        }

        for (i = 0; i < length; i++)
            maxpd_atom_set_float(x->buffer + i, v[i]);
        outlet_anything(x->outlet1, gensym((char *)props->name), length, x->buffer);
    }
}

// *********************************************************
// -(read device definition - maxmsp only)------------------
void mapper_read_definition (t_mapper *x)
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
void mapper_register_signals(t_mapper *x) {
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

    if (x->d) {
        // Get pointer to dictionary "device"
        if (dictionary_getdictionary(x->d, sym_device, &device) == MAX_ERR_NONE) {
            // Get pointer to atom array "inputs"
            if (dictionary_getatomarray((t_dictionary *)device, sym_inputs, &inputs) == MAX_ERR_NONE) {
                atomarray_getatoms((t_atomarray *)inputs, &num_signals, &signals);
                // iterate through array of atoms
                for (i=0; i<num_signals; i++) {
                    // initialize variables
                    if (sig_units) {
                        free(&sig_units);
                    }
                    if (sig_type) {
                        free(&sig_type);
                    }
                    sig_length = 1;
                    range_known[0] = 1;
                    range_known[1] = 1;

                    // each atom object points to a dictionary, need to recover atoms by key
                    temp = atom_getobj(&(signals[i]));
                    if (dictionary_getstring((t_dictionary *)temp, sym_name, &sig_name) == MAX_ERR_NONE) {
                        dictionary_getstring((t_dictionary *)temp, sym_units, &sig_units);
                        dictionary_getstring((t_dictionary *)temp, sym_type, &sig_type);
                        dictionary_getlong((t_dictionary *)temp, sym_length, &sig_length);
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
            }
            // Get pointer to atom array "outputs"
            if (dictionary_getatomarray((t_dictionary *)device, sym_outputs, &outputs) == MAX_ERR_NONE) {
                atomarray_getatoms((t_atomarray *)outputs, &num_signals, &signals);
                // iterate through array of atoms
                for (i=0; i<num_signals; i++) {
                    // initialize variables
                    if (sig_units) {
                        free(&sig_units);
                    }
                    if (sig_type) {
                        free(&sig_type);
                    }
                    sig_length = 1;
                    range_known[0] = 1;
                    range_known[1] = 1;

                    // each atom object points to a dictionary, need to recover atoms by key
                    temp = atom_getobj(&(signals[i]));
                    if (dictionary_getstring((t_dictionary *)temp, sym_name, &sig_name) == MAX_ERR_NONE) {
                        dictionary_getstring((t_dictionary *)temp, sym_units, &sig_units);
                        dictionary_getstring((t_dictionary *)temp, sym_type, &sig_type);
                        dictionary_getlong((t_dictionary *)temp, sym_length, &sig_length);
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
        }
    }
}
#endif

// *********************************************************
// -(poll libmapper)----------------------------------------
void mapper_poll(t_mapper *x)
{    
    mdev_poll(x->device, 0);
    if (!x->ready) {
        if (mdev_ready(x->device)) {
            //mapper_db_dump(db);
            x->ready = 1;
            mapper_print_properties(x);
        }
    }
    clock_delay(x->clock, INTERVAL);  // Set clock to go off after delay
}
    
// *********************************************************
// -(toggle learning mode)----------------------------------
void mapper_learn(t_mapper *x, t_symbol *s, int argc, t_atom *argv)
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