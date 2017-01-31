//
// mapin.c
// a maxmsp and puredata external encapsulating the functionality of a
// libmapper input signal, allowing name and metadata to be set
// http://www.libmapper.org
// Joseph Malloch, IDMIL 2013
//
// This software was written in the Input Devices and Music Interaction
// Laboratory at McGill University in Montreal, and is copyright those
// found in the AUTHORS file.  It is licensed under the GNU Lesser Public
// General License version 2.1 or later.  Please see COPYING for details.
//

// *********************************************************
// -(Includes)----------------------------------------------

#include "ext.h"            // standard Max include, always required
#include "ext_obex.h"       // required for new style Max object
#include "jpatcher_api.h"
#include <mapper/mapper.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#ifndef WIN32
  #include <arpa/inet.h>
#endif

#include <unistd.h>

#define MAX_LIST 256

// *********************************************************
// -(object struct)-----------------------------------------
typedef struct _mapin
{
    t_object            ob;
    t_symbol            *sig_name;
    long                sig_length;
    char                sig_type;
    mapper_device       dev_obj;
    mapper_signal       sig_ptr;
    mapper_timetag_t    *tt_ptr;
    long                is_instance;
    mapper_id           instance_id;
    void                *outlet;
    t_symbol            *myobjname;
    t_object            *patcher;
    t_hashtab           *ht;
    long                num_args;
    t_atom              *args;
    long                connect_state;
    int                 length;
    char                type;
} t_mapin;

// *********************************************************
// -(function prototypes)-----------------------------------
static void *mapin_new(t_symbol *s, int argc, t_atom *argv);
static void mapin_free(t_mapin *x);

static void add_to_hashtab(t_mapin *x, t_hashtab *ht);
static void remove_from_hashtab(t_mapin *x);
static t_max_err set_sig_ptr(t_mapin *x, t_object *attr, long argc, t_atom *argv);
static t_max_err set_dev_obj(t_mapin *x, t_object *attr, long argc, t_atom *argv);
static t_max_err set_tt_ptr(t_mapin *x, t_object *attr, long argc, t_atom *argv);

static void mapin_loadbang(t_mapin *x);
static void mapin_int(t_mapin *x, long i);
static void mapin_float(t_mapin *x, double f);
static void mapin_list(t_mapin *x, t_symbol *s, int argc, t_atom *argv);
static void mapin_release(t_mapin *x);

static int atom_strcmp(t_atom *a, const char *string);
static const char *atom_get_string(t_atom *a);
static void atom_set_string(t_atom *a, const char *string);
static int atom_coerce_int(t_atom *a);
static float atom_coerce_float(t_atom *a);

t_symbol *maybe_start_queue_sym;

// *********************************************************
// -(global class pointer variable)-------------------------
static void *mapin_class;

// *********************************************************
// -(main)--------------------------------------------------
int main(void)
{
    t_class *c;
    c = class_new("map.in", (method)mapin_new, (method)mapin_free,
                  (long)sizeof(t_mapin), 0L, A_GIMME, 0);

    class_addmethod(c, (method)mapin_loadbang, "loadbang", 0);
    class_addmethod(c, (method)mapin_int, "int", A_LONG, 0);
    class_addmethod(c, (method)mapin_float, "float", A_FLOAT, 0);
    class_addmethod(c, (method)mapin_list, "list", A_GIMME, 0);
    class_addmethod(c, (method)mapin_release, "release", 0);
    class_addmethod(c, (method)add_to_hashtab, "add_to_hashtab", A_CANT, 0);
    class_addmethod(c, (method)remove_from_hashtab, "remove_from_hashtab", A_CANT, 0);

    CLASS_ATTR_SYM(c, "sig_name", ATTR_GET_OPAQUE_USER | ATTR_SET_OPAQUE_USER, t_mapin, sig_name);
    CLASS_ATTR_LONG(c, "sig_length", ATTR_GET_OPAQUE_USER | ATTR_SET_OPAQUE_USER, t_mapin, sig_length);
    CLASS_ATTR_CHAR(c, "sig_type", ATTR_GET_OPAQUE_USER | ATTR_SET_OPAQUE_USER, t_mapin, sig_type);
    CLASS_ATTR_OBJ(c, "dev_obj", ATTR_GET_OPAQUE_USER | ATTR_SET_OPAQUE_USER, t_mapin, dev_obj);
    CLASS_ATTR_ACCESSORS(c, "dev_obj", 0, set_dev_obj);
    CLASS_ATTR_OBJ(c, "sig_ptr", ATTR_GET_OPAQUE_USER | ATTR_SET_OPAQUE_USER, t_mapin, sig_ptr);
    CLASS_ATTR_ACCESSORS(c, "sig_ptr", 0, set_sig_ptr);
    CLASS_ATTR_OBJ(c, "tt_ptr", ATTR_GET_OPAQUE_USER | ATTR_SET_OPAQUE_USER, t_mapin, tt_ptr);
    CLASS_ATTR_ACCESSORS(c, "tt_ptr", 0, set_tt_ptr);

    class_register(CLASS_BOX, c); /* CLASS_NOBOX */
    mapin_class = c;
    return 0;
}

static void mapin_usage()
{
    post("usage: [mapin <signal-name> <datatype> <opt: vectorlength>]");
}

// *********************************************************
// -(new)---------------------------------------------------
static void *mapin_new(t_symbol *s, int argc, t_atom *argv)
{
    t_mapin *x = NULL;

    long i = 0;

    if (argc < 2) {
        mapin_usage();
        return 0;
    }
    if ((argv)->a_type != A_SYM || (argv+1)->a_type != A_SYM) {
        mapin_usage();
        return 0;
    }

    if ((x = (t_mapin *)object_alloc(mapin_class))) {
        x->outlet = listout((t_object *)x);

        x->sig_name = gensym(atom_getsym(argv)->s_name);

        char *temp = atom_getsym(argv+1)->s_name;
        x->sig_type = temp[0];
        if (x->sig_type != 'i' && x->sig_type != 'f')
            return 0;

        x->sig_ptr = 0;
        x->length = 0;
        x->instance_id = 0;
        x->is_instance = 0;
        x->connect_state = 0;

        if (argc >= 3 && (argv+2)->a_type == A_LONG) {
            x->sig_length = atom_getlong(argv+2);
            if (x->sig_length > 100) {
                post("vector lengths > 100 not currently supported.");
                return 0;
            }
            i = 3;
        }
        else {
            x->sig_length = 1;
            i = 2;
        }

        // we need to cache any arguments to add later
        x->args = 0;
        x->num_args = argc - i;
        if (x->num_args) {
            long alloced = 0;
            char result = 0;
            atom_alloc_array(x->num_args, &alloced, &x->args, &result);
            if (!result || !alloced)
                return 0;
            sysmem_copyptr(argv+i, x->args, x->num_args * sizeof(t_atom));
        }

        // cache the registered name so we can remove self from hashtab later
        x = object_register(CLASS_BOX, x->myobjname = symbol_unique(), x);

        x->patcher = (t_object *)gensym("#P")->s_thing;
        mapin_loadbang(x);
    }
    maybe_start_queue_sym = gensym("maybe_start_queue");
    return (x);
}

// *********************************************************
// -(free)--------------------------------------------------
static void mapin_free(t_mapin *x)
{
    remove_from_hashtab(x);
    if (x->args)
        sysmem_freeptr(x->args);
}

void mapin_loadbang(t_mapin *x)
{
    t_hashtab *ht;

    if (!x->patcher)
    return;

    t_object *patcher = x->patcher;
    while (patcher) {
        object_obex_lookup(patcher, gensym("mapperhash"), (t_object **)&ht);
        if (ht) {
            add_to_hashtab(x, ht);
            break;
        }
        patcher = jpatcher_get_parentpatcher(patcher);
    }
}

void add_to_hashtab(t_mapin *x, t_hashtab *ht)
{
    if (x->connect_state) {
        // already registered
        return;
    }

    // store self in the hashtab. IMPORTANT: set the OBJ_FLAG_REF flag so the
    // hashtab knows not to free us when it is freed.
    hashtab_storeflags(ht, x->myobjname, (t_object *)x, OBJ_FLAG_REF);
    x->ht = ht;
    x->connect_state = 1;
}

void remove_from_hashtab(t_mapin *x)
{
    if (x->ht) {
        hashtab_chuckkey(x->ht, x->myobjname);
        x->ht = NULL;
    }
    x->dev_obj = 0;
    x->sig_ptr = 0;
    x->length = 0;
    x->connect_state = 0;
}

// *********************************************************
// -(parse props from object arguments)---------------------
void parse_extra_properties(t_mapin *x)
{
    int i;
    // add other declared properties
    for (i = 0; i < x->num_args; i++) {
        if (i > x->num_args - 2) // need 2 arguments for key and value
            break;
        else if ((x->args+i)->a_type != A_SYM)
            break;
        else if ((atom_strcmp(x->args+i, "@name") == 0) ||
            (atom_strcmp(x->args+i, "@type") == 0) ||
            (atom_strcmp(x->args+i, "@length") == 0)){
            i++;
            continue;
        }
        else if (atom_strcmp(x->args+i, "@instance") == 0) {
            if ((x->args+i+1)->a_type == A_SYM &&
                atom_strcmp(x->args+i+1, "polyindex") == 0) {
                /* Check if object is embedded in a poly~ object - if so,
                 * retrieve the index and use as instance id. */
                t_object *patcher = NULL;
                t_max_err err = object_obex_lookup(x, gensym("#P"), &patcher);
                if (err == MAX_ERR_NONE) {
                    t_object *assoc = NULL;
                    object_method(patcher, gensym("getassoc"), &assoc);
                    if (assoc) {
                        method m = zgetfn(assoc, gensym("getindex"));
                        if (m) {
                            x->instance_id = (long)(*m)(assoc, patcher);
                        }
                        else
                            continue;
                    }
                    else
                        continue;
                }
                else
                    continue;
            }
            else if ((x->args+i+1)->a_type == A_LONG) {
                x->instance_id = atom_getlong(x->args+i+1);
            }
            else {
                continue;
            }
            /* Remove the default signal instance (0) if it exists. Since the user
             * may have properly added an instance 0, we will check for user_data. */
            void *data = mapper_signal_instance_user_data(x->sig_ptr, 0);
            if (!data)
                mapper_signal_remove_instance(x->sig_ptr, 0);

            x->is_instance = 1;
            i++;
            mapper_signal_reserve_instances(x->sig_ptr, 1, &x->instance_id, (void **)&x);
        }
        else if (atom_strcmp(x->args+i, "@minimum") == 0 ||
                 atom_strcmp(x->args+i, "@min") == 0) {
            // check number of arguments
            int length = 1, scalar_min = 0;
            char type;
            while (length + i < x->num_args) {
                type = (x->args+i+length)->a_type;
                if (type == A_SYM && atom_get_string(x->args+i+length)[0] == '@') {
                    // reached next property name
                    break;
                }
                if (type != A_LONG && type != A_FLOAT) {
                    length = 0;
                    break;
                }
                length++;
            }
            length--;
            if (length != x->sig_length) {
                // we will allow using scalars to set min of entire vector
                if (length == 1)
                    scalar_min = 1;
                else {
                    post("'%s' property for signal %s requires %i arguments! (got %i)",
                         atom_get_string(x->args+i), x->sig_name->s_name,
                         x->sig_length, length);
                    continue;
                }
            }

            ++i;
            int j;
            switch (x->sig_type) {
                case 'i': {
                    int val[x->sig_length];
                    for (j = 0; j < length; j++, i++) {
                        val[j] = atom_coerce_int(x->args + i);
                    }
                    if (scalar_min) {
                        for (j = 1; j < x->sig_length; j++) {
                            val[j] = val[0];
                        }
                    }
                    mapper_signal_set_minimum(x->sig_ptr, val);
                    i--;
                    break;
                }
                case 'f': {
                    float val[x->sig_length];
                    for (j = 0; j < length; j++, i++) {
                        val[j] = atom_coerce_float(x->args + i);
                    }
                    if (scalar_min) {
                        for (j = 1; j < x->sig_length; j++) {
                            val[j] = val[0];
                        }
                    }
                    mapper_signal_set_minimum(x->sig_ptr, val);
                    i--;
                    break;
                }
                default:
                    break;
            }
        }
        else if (atom_strcmp(x->args+i, "@maximum") == 0 ||
                 atom_strcmp(x->args+i, "@max") == 0) {
            // check number of arguments
            int length = 1, scalar_max = 0;
            char type;
            while (length + i < x->num_args) {
                type = (x->args+i+length)->a_type;
                if (type == A_SYM && atom_get_string(x->args+i+length)[0] == '@') {
                    // reached next property name
                    break;
                }
                if (type != A_LONG && type != A_FLOAT) {
                    length = 0;
                    break;
                }
                length++;
            }
            length--;
            if (length != x->sig_length) {
                // we will allow using scalars to set max of entire vector
                if (length == 1)
                    scalar_max = 1;
                else {
                    post("'%s' property for signal %s requires %i arguments! (got %i)",
                         atom_get_string(x->args+i), x->sig_name->s_name,
                         x->sig_length, length);
                    continue;
                }
            }

            ++i;
            int j;
            switch (x->sig_type) {
                case 'i': {
                    int val[x->sig_length];
                    for (j = 0; j < length; j++, i++) {
                        val[j] = atom_coerce_int(x->args + i);
                    }
                    if (scalar_max) {
                        for (j = 1; j < x->sig_length; j++) {
                            val[j] = val[0];
                        }
                    }
                    mapper_signal_set_maximum(x->sig_ptr, val);
                    i--;
                    break;
                }
                case 'f': {
                    float val[x->sig_length];
                    for (j = 0; j < length; j++, i++) {
                        val[j] = atom_coerce_float(x->args + i);
                    }
                    if (scalar_max) {
                        for (j = 1; j < x->sig_length; j++) {
                            val[j] = val[0];
                        }
                    }
                    mapper_signal_set_maximum(x->sig_ptr, val);
                    i--;
                    break;
                }
                default:
                    break;
            }
        }
        else if (atom_get_string(x->args+i)[0] == '@') {
            // TODO: allow vector property values
            switch ((x->args+i+1)->a_type) {
                case A_SYM: {
                    const char *value = atom_get_string(x->args+i+1);
                    mapper_signal_set_property(x->sig_ptr,
                                               atom_get_string(x->args+i)+1,
                                               1, 's', value, 1);
                    i++;
                    break;
                }
                case A_FLOAT:
                {
                    float value = atom_getfloat(x->args+i+1);
                    mapper_signal_set_property(x->sig_ptr,
                                               atom_get_string(x->args+i)+1,
                                               1, 'f', &value, 1);
                    i++;
                    break;
                }
                case A_LONG:
                {
                    int value = atom_getlong(x->args+i+1);
                    mapper_signal_set_property(x->sig_ptr,
                                               atom_get_string(x->args+i)+1,
                                               1, 'i', &value, 1);
                    i++;
                    break;
                }
                default:
                    break;
            }
        }
    }
}

// *********************************************************
// -(set the device pointer)--------------------------------
t_max_err set_dev_obj(t_mapin *x, t_object *attr, long argc, t_atom *argv)
{
    x->dev_obj = (t_object *)argv->a_w.w_obj;
    return 0;
}

// *********************************************************
// -(set the signal pointer)--------------------------------
t_max_err set_sig_ptr(t_mapin *x, t_object *attr, long argc, t_atom *argv)
{
    x->sig_ptr = (mapper_signal)argv->a_w.w_obj;
    if (x->sig_ptr)
        parse_extra_properties(x);
    return 0;
}

// *********************************************************
// -(set the device pointer)--------------------------------
t_max_err set_tt_ptr(t_mapin *x, t_object *attr, long argc, t_atom *argv)
{
    x->tt_ptr = (mapper_timetag_t *)argv->a_w.w_obj;
    return 0;
}

static int check_ptrs(t_mapin *x)
{
    if (!x || !x->dev_obj || !x->sig_ptr) {
        return 1;
    }
    else if (!x->length) {
        x->length = mapper_signal_length(x->sig_ptr);
        x->type = mapper_signal_type(x->sig_ptr);
    }
    return 0;
}

// *********************************************************
// -(set int input)-----------------------------------------
static void mapin_int(t_mapin *x, long l)
{
    int i;
    float f;
    void *value = 0;

    if (check_ptrs(x))
        return;

    if (x->length != 1)
        return;
    if (x->type == 'i') {
        i = (int)l;
        value = &i;
    }
    else if (x->type == 'f') {
        f = (float)l;
        value = &f;
    }
    object_method(x->dev_obj, maybe_start_queue_sym);
    if (x->is_instance)
        mapper_signal_instance_update(x->sig_ptr, x->instance_id,
                                      value, 1, *x->tt_ptr);
    else
        mapper_signal_update(x->sig_ptr, value, 1, *x->tt_ptr);
}

// *********************************************************
// -(set float input)---------------------------------------
static void mapin_float(t_mapin *x, double d)
{
    int i;
    float f;
    void *value = 0;

    if (check_ptrs(x))
        return;

    if (x->length != 1)
        return;
    if (x->type == 'f') {
        f = (float)d;
        value = &f;
    }
    else if (x->type == 'i') {
        i = (int)d;
        value = &i;
    }
    object_method(x->dev_obj, maybe_start_queue_sym);
    if (x->is_instance)
        mapper_signal_instance_update(x->sig_ptr, x->instance_id,
                                      value, 1, *x->tt_ptr);
    else
        mapper_signal_update(x->sig_ptr, value, 1, *x->tt_ptr);
}

// *********************************************************
// -(set list input)----------------------------------------
static void mapin_list(t_mapin *x, t_symbol *s, int argc, t_atom *argv)
{
    int i = 0, count;
    void *value;

    if (check_ptrs(x) || !argc)
        return;

    if (argc < x->length || (argc % x->length) != 0) {
        object_post((t_object *)x, "Illegal list length (expected factor of %i)",
                    x->length);
        return;
    }
    count = argc / x->length;

    if (x->type == 'i') {
        int payload[argc];
        value = &payload;
        for (i = 0; i < argc; i++) {
            if ((argv+i)->a_type == A_FLOAT)
                payload[i] = (int)atom_getfloat(argv+i);
            else if ((argv+i)->a_type == A_LONG)
                payload[i] = (int)atom_getlong(argv+i);
            else {
                object_post((t_object *)x, "Illegal data type in list!");
                return;
            }
        }
        //update signal
        object_method(x->dev_obj, maybe_start_queue_sym);
        if (x->is_instance) {
            mapper_signal_instance_update(x->sig_ptr, x->instance_id,
                                          value, count, *x->tt_ptr);
        }
        else {
            mapper_signal_update(x->sig_ptr, value, count, *x->tt_ptr);
        }
    }
    else if (x->type == 'f') {
        float payload[argc];
        value = &payload;
        for (i = 0; i < argc; i++) {
            if ((argv+i)->a_type == A_FLOAT)
                payload[i] = atom_getfloat(argv+i);
            else if ((argv+i)->a_type == A_LONG)
                payload[i] = (float)atom_getlong(argv+i);
            else {
                object_post((t_object *)x, "Illegal data type in list!");
                return;
            }
        }
        //update signal
        object_method(x->dev_obj, maybe_start_queue_sym);
        if (x->is_instance) {
            mapper_signal_instance_update(x->sig_ptr, x->instance_id,
                                          value, count, *x->tt_ptr);
        }
        else {
            mapper_signal_update(x->sig_ptr, value, count, *x->tt_ptr);
        }
    }
}

// *********************************************************
// -(release instance)--------------------------------------
static void mapin_release(t_mapin *x)
{
    if (check_ptrs(x) || !x->is_instance)
        return;

    object_method(x->dev_obj, maybe_start_queue_sym);
    mapper_signal_instance_release(x->sig_ptr, x->instance_id, *x->tt_ptr);
}

// *********************************************************
// some helper functions

static int atom_strcmp(t_atom *a, const char *string)
{
    if (a->a_type != A_SYM || !string)
        return 1;
    return strcmp(atom_getsym(a)->s_name, string);
}

static const char *atom_get_string(t_atom *a)
{
    return atom_getsym(a)->s_name;
}

static void atom_set_string(t_atom *a, const char *string)
{
    atom_setsym(a, gensym((char *)string));
}

static int atom_coerce_int(t_atom *a)
{
    if (a->a_type == A_LONG)
    return (int)atom_getlong(a);
    else if (a->a_type == A_FLOAT)
    return (int)atom_getfloat(a);
    else
    return 0;
}

static float atom_coerce_float(t_atom *a)
{
    if (a->a_type == A_LONG)
    return (float)atom_getlong(a);
    else if (a->a_type == A_FLOAT)
    return atom_getfloat(a);
    else
    return 0.f;
}
