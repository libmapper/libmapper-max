//
// mapdevice.c
// a maxmsp and puredata external encapsulating the functionality of a
// libmapper "device", allowing name and metadata to be set
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
#include <lo/lo.h>
#ifndef WIN32
  #include <arpa/inet.h>
#endif

#include <unistd.h>

#define INTERVAL 1
#define MAX_LIST 256

// *********************************************************
// -(object struct)-----------------------------------------
typedef struct _mapdevice
{
    t_object            ob;
    void                *outlet;
    t_hashtab           *ht;
    void                *clock;
    char                *name;
    mapper_network      network;
    mapper_database     db;
    mapper_device       device;
    mapper_timetag_t    timetag;
    int                 updated;
    int                 ready;
    t_atom              buffer[MAX_LIST];
    t_object            *patcher;
} t_mapdevice;

typedef struct _map_ptrs
{
    t_mapdevice *home;
    int         num_objs;
    t_object    **objs;
} t_map_ptrs;

// *********************************************************
// -(function prototypes)-----------------------------------
static void *mapdevice_new(t_symbol *s, int argc, t_atom *argv);
static void mapdevice_free(t_mapdevice *x);

static void mapdevice_notify(t_mapdevice *x, t_symbol *s, t_symbol *msg,
                             void *sender, void *data);

static void mapdevice_detach_obj(t_hashtab_entry *e, void *arg);
static void mapdevice_detach(t_mapdevice *x);
static void mapdevice_attach_obj(t_hashtab_entry *e, void *arg);
static int mapdevice_attach(t_mapdevice *x);

static void mapdevice_add_signal(t_mapdevice *x, t_object *obj);
static void mapdevice_remove_signal(t_mapdevice *x, t_object *obj);

static void mapdevice_maybe_start_queue(t_mapdevice *x);

static void mapdevice_poll(t_mapdevice *x);

static void mapdevice_sig_handler(mapper_signal sig, mapper_id instance,
                                  const void *value, int count,
                                  mapper_timetag_t *tt);
static void mapdevice_instance_event_handler(mapper_signal sig,
                                             mapper_id instance,
                                             mapper_instance_event event,
                                             mapper_timetag_t *tt);

static void mapdevice_print_properties(t_mapdevice *x);

//static void maybe_start_queue(t_mapdevice *x);
static int atom_strcmp(t_atom *a, const char *string);
static const char *atom_get_string(t_atom *a);
static void atom_set_string(t_atom *a, const char *string);

// *********************************************************
// -(global class pointer variable)-------------------------
static void *mapdevice_class;

// *********************************************************
// -(main)--------------------------------------------------
int main(void)
{
    t_class *c;
    c = class_new("map.device", (method)mapdevice_new, (method)mapdevice_free,
                  (long)sizeof(t_mapdevice), 0L, A_GIMME, 0);

    class_addmethod(c, (method)mapdevice_notify, "notify", A_CANT, 0);
    class_addmethod(c, (method)mapdevice_maybe_start_queue, "maybe_start_queue", A_CANT, 0);

    class_register(CLASS_BOX, c); /* CLASS_NOBOX */
    mapdevice_class = c;
    return 0;
}

// *********************************************************
// -(new)---------------------------------------------------
static void *mapdevice_new(t_symbol *s, int argc, t_atom *argv)
{
    t_mapdevice *x = NULL;
    long i;
    const char *alias = NULL;
    const char *iface = NULL;

    if ((x = object_alloc(mapdevice_class))) {
        x->outlet = listout((t_object *)x);
        x->name = 0;

        if (argv->a_type == A_SYM && atom_get_string(argv)[0] != '@')
            alias = atom_get_string(argv);

        for (i = 0; i < argc-1; i++) {
            if ((argv+i)->a_type == A_SYM) {
                if (atom_strcmp(argv+i, "@alias") == 0) {
                    if ((argv+i+1)->a_type == A_SYM) {
                        alias = atom_get_string(argv+i+1);
                        i++;
                    }
                }
                else if (atom_strcmp(argv+i, "@interface") == 0) {
                    if ((argv+i+1)->a_type == A_SYM) {
                        iface = atom_get_string(argv+i+1);
                        i++;
                    }
                }
            }
        }
        if (alias) {
            x->name = *alias == '/' ? strdup(alias+1) : strdup(alias);
        }
        else {
            x->name = strdup("maxmsp");
        }

        x->network = mapper_network_new(iface, 0, 0);
        if (!x->network) {
            object_post((t_object *)x, "error initializing libmapper network.");
            return 0;
        }

        x->device = mapper_device_new(x->name, 0, x->network);
        if (!x->device) {
            object_post((t_object *)x, "error initializing libmapper device.");
            return 0;
        }
        x->db = mapper_device_database(x->device);

        if (mapdevice_attach(x)) {
            mapper_device_free(x->device);
            mapper_network_free(x->network);
            free(x->name);
            return 0;
        }

        object_post((t_object *)x, "Using libmapper version %s – visit libmapper.org for more information.",
                    mapper_version());
        object_post((t_object *)x, "Connecting to network interface %s...",
                    mapper_network_interface(x->network));

        // add other declared properties
        for (i = 0; i < argc; i++) {
            if (i > argc - 2) // need 2 arguments for key and value
                break;
            if ((atom_strcmp(argv+i, "@alias") == 0) ||
                (atom_strcmp(argv+i, "@interface") == 0)){
                i++;
                continue;
            }
            else if (atom_get_string(argv+i)[0] == '@') {
                // TODO: allow vector property values
                switch ((argv+i+1)->a_type) {
                    case A_SYM: {
                        const char *value = atom_get_string(argv+i+1);
                        mapper_device_set_property(x->device,
                                                   atom_get_string(argv+i)+1,
                                                   1, 's', value, 1);
                        i++;
                        break;
                    }
                    case A_FLOAT:
                    {
                        float value = atom_getfloat(argv+i+1);
                        mapper_device_set_property(x->device,
                                                   atom_get_string(argv+i)+1,
                                                   1, 'f', &value, 1);
                        i++;
                        break;
                    }
                    case A_LONG:
                    {
                        int value = atom_getlong(argv+i+1);
                        mapper_device_set_property(x->device,
                                                   atom_get_string(argv+i)+1,
                                                   1, 'i', &value, 1);
                        i++;
                        break;
                    }
                    default:
                        break;
                }
            }
        }

        mapdevice_print_properties(x);
        x->ready = 0;
        x->updated = 0;

        // Create the timing clock
        x->clock = clock_new(x, (method)mapdevice_poll);
        clock_delay(x->clock, INTERVAL);  // Set clock to go off after delay
    }
    return (x);
}

// *********************************************************
// -(free)--------------------------------------------------
static void mapdevice_free(t_mapdevice *x)
{
    mapdevice_detach(x);

    clock_unset(x->clock);      // Remove clock routine from the scheduler
    clock_free(x->clock);       // Frees memeory used by clock
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

void mapdevice_notify(t_mapdevice *x, t_symbol *s, t_symbol *msg, void *sender, void *data)
{
	if (msg == gensym("hashtab_entry_new")) { // something arrived in the hashtab
		t_symbol *key = (t_symbol *)data;
		t_object *obj = NULL;
		hashtab_lookup(sender, key, &obj);
        if (obj) {
            mapdevice_add_signal(x, obj);
            object_attach_byptr(x, obj); // attach to object
        }
	}
    else if (msg == gensym("hashtab_entry_free")) { // something left the hashtab
		t_symbol *key = (t_symbol *)data;
		t_object *obj = NULL;
		
		hashtab_lookup(sender, key, &obj);
		if (obj) {
            mapdevice_remove_signal(x, obj);
			object_detach_byptr(x, obj); // detach from it
        }
	}
}

void mapdevice_detach_obj(t_hashtab_entry *e, void *arg)
{
	t_mapdevice *x = (t_mapdevice *)arg;
	if (x) {
		// detach from the object, it's going away...
        atom_setobj(x->buffer, 0);
        object_attr_setvalueof(e->value, gensym("dev_obj"), 1, x->buffer);
        object_attr_setvalueof(e->value, gensym("sig_ptr"), 1, x->buffer);
		object_detach_byptr(x, e->value);
	}
}

void mapdevice_detach(t_mapdevice *x)
{
	if (x->ht) {
		hashtab_funall(x->ht, (method)mapdevice_detach_obj, x);
        hashtab_methodall(x->ht, gensym("remove_from_hashtab"));
		object_detach_byptr(x, x->ht); // detach from the hashtable
        hashtab_chuck(x->ht);
        object_obex_store(x->patcher, gensym("mapperhash"), NULL);
	}
}

void mapdevice_attach_obj(t_hashtab_entry *e, void *arg)
{
	t_mapdevice *x = (t_mapdevice *)arg;
	if (x) {
		// attach to the object to receive its notifications
		object_attach_byptr(x, e->value);
	}
}

long check_downstream(t_mapdevice *x, t_object *obj)
{
    t_symbol *cls = object_classname(obj);

    // if this is a device object, stop iterating
    if (cls == gensym("map.device"))
        return 1;
    else
        return 0;
}

long add_downstream(t_mapdevice *x, t_object *obj)
{
    t_symbol *cls = object_classname(obj);

    if (cls != gensym("map.in") && cls != gensym("map.out"))
        return 0;

    object_method(obj, gensym("add_to_hashtab"), x->ht);
    return 0;
}

int mapdevice_attach(t_mapdevice *x)
{
    t_object *patcher = NULL;
    t_hashtab *ht = 0;
    long result = 0;

	object_obex_lookup(x, gensym("#P"), &patcher); // get the object's patcher
	if (!patcher)
        return 1;

    x->patcher = patcher;

    // walk up the patcher hierarchy checking if there is an upstream mapdevice object
    while (patcher) {
        object_obex_lookup(patcher, gensym("mapperhash"), (t_object **)&ht);
        if (ht) {
            object_post((t_object *)x, "error: found mapdevice object in parent patcher!");
            return 1;
        }
        patcher = jpatcher_get_parentpatcher(patcher);
    }

    // walk down the patcher hierarchy checking if there is a downstream mapdevice object
    object_method(x->patcher, gensym("iterate"), check_downstream, (void *)x,
                  PI_DEEP, &result);
    if (result) {
        object_post((t_object *)x, "error: found mapdevice object in parent patcher!");
        return 1;
    }

    x->ht = hashtab_new(0);
    // objects stored in the obex will be freed when the obex's owner is freed
    // in this case, when the patcher object is freed. so we don't need to
    // manage the memory associated with the "mapperhash".
    object_obex_store(x->patcher, gensym("mapperhash"), (t_object *)x->ht);

    // attach to the hashtab, registering it if necessary
    // this way, we can receive notifications from the hashtab as things are added and removed
    object_attach_byptr_register(x, x->ht, CLASS_NOBOX);

    // add downstream map.in and map.out objects to hashtable
    object_method(x->patcher, gensym("iterate"), add_downstream, (void *)x,
                  PI_DEEP, &result);

    // call a method on every object in the hash table
    hashtab_funall(x->ht, (method)mapdevice_attach_obj, x);

    return 0;
}

static void mapdevice_add_signal(t_mapdevice *x, t_object *obj)
{
    mapper_signal sig = NULL;
    if (obj) {
        t_symbol *temp = object_attr_getsym(obj, gensym("sig_name"));
        const char *name = temp->s_name;
        char type = object_attr_getchar(obj, gensym("sig_type"));
        long length = object_attr_getlong(obj, gensym("sig_length"));
        mapper_direction dir = 0;

        if (object_classname(obj) == gensym("map.out"))
            dir = MAPPER_DIR_OUTGOING;
        else if (object_classname(obj) == gensym("map.in"))
            dir = MAPPER_DIR_INCOMING;
        else
            return;

        sig = mapper_device_signal_by_name(x->device, name);
        if (sig) {
            // another max object associated with this signal exists
            t_map_ptrs *ptrs = (t_map_ptrs *)mapper_signal_user_data(sig);
            ptrs->objs = realloc(ptrs->objs, (ptrs->num_objs+1) * sizeof(t_object *));
            ptrs->objs[ptrs->num_objs] = obj;
            ptrs->num_objs++;
        }
        else {
            t_map_ptrs *ptrs = (t_map_ptrs *)malloc(sizeof(struct _map_ptrs));
            ptrs->home = x;
            ptrs->objs = (t_object **)malloc(sizeof(t_object *));
            ptrs->num_objs = 1;
            ptrs->objs[0] = obj;
            sig = mapper_device_add_signal(x->device, dir, 1, name, length, type,
                                           0, 0, 0, mapdevice_sig_handler, ptrs);
            mapper_signal_set_instance_event_callback(sig,
                                                      mapdevice_instance_event_handler,
                                                      MAPPER_INSTANCE_ALL);
        }
        //output new numOutputs/numInputs
        atom_setlong(x->buffer, mapper_device_num_signals(x->device, dir));
        if (dir == MAPPER_DIR_OUTGOING)
            outlet_anything(x->outlet, gensym("numOutputs"), 1, x->buffer);
        else
            outlet_anything(x->outlet, gensym("numInputs"), 1, x->buffer);

        atom_setobj(x->buffer, (void *)x);
        object_attr_setvalueof(obj, gensym("dev_obj"), 1, x->buffer);
        atom_setobj(x->buffer, (void *)sig);
        object_attr_setvalueof(obj, gensym("sig_ptr"), 1, x->buffer);
        atom_setobj(x->buffer, (void *)&x->timetag);
        object_attr_setvalueof(obj, gensym("tt_ptr"), 1, x->buffer);
    }
}

static void mapdevice_remove_signal(t_mapdevice *x, t_object *obj)
{
    mapper_signal sig = 0;
    if (!obj)
        return;
    t_symbol *temp = object_attr_getsym(obj, gensym("sig_name"));
    const char *name = temp->s_name;

    sig = mapper_device_signal_by_name(x->device, name);

    if (sig) {
        t_map_ptrs *ptrs = (t_map_ptrs *)mapper_signal_user_data(sig);
        if (ptrs->num_objs == 1) {
            free(ptrs->objs);
            free(ptrs);
            mapper_device_remove_signal(x->device, sig);
        }
        else {
            // need to realloc obj ptr memory
            // find index of this obj in ptr array
            int i;
            for (i=0; i<ptrs->num_objs; i++) {
                if (ptrs->objs[i] == obj)
                    break;
            }
            if (i == ptrs->num_objs) {
                object_post((t_object *)x, "error: obj ptr not found in signal user_data!");
                return;
            }
            i++;
            for (; i<ptrs->num_objs; i++)
                ptrs->objs[i-1] = ptrs->objs[i];
            ptrs->objs = realloc(ptrs->objs, (ptrs->num_objs-1) * sizeof(t_object *));
            ptrs->num_objs--;
        }
    }
}

// *********************************************************
// -(print properties)--------------------------------------
static void mapdevice_print_properties(t_mapdevice *x)
{
    if (x->ready) {
        //output name
        atom_set_string(x->buffer, mapper_device_name(x->device));
        outlet_anything(x->outlet, gensym("name"), 1, x->buffer);

        //output interface
        atom_set_string(x->buffer, mapper_network_interface(x->network));
        outlet_anything(x->outlet, gensym("interface"), 1, x->buffer);

        //output IP
        const struct in_addr *ip = mapper_network_ip4(x->network);
        atom_set_string(x->buffer, inet_ntoa(*ip));
        outlet_anything(x->outlet, gensym("IP"), 1, x->buffer);

        //output port
        atom_setlong(x->buffer, mapper_device_port(x->device));
        outlet_anything(x->outlet, gensym("port"), 1, x->buffer);

        //output numInputs
        atom_setlong(x->buffer, mapper_device_num_signals(x->device,
                                                          MAPPER_DIR_INCOMING));
        outlet_anything(x->outlet, gensym("numInputs"), 1, x->buffer);

        //output numOutputs
        atom_setlong(x->buffer, mapper_device_num_signals(x->device,
                                                          MAPPER_DIR_OUTGOING));
        outlet_anything(x->outlet, gensym("numOutputs"), 1, x->buffer);
    }
}

static void outlet_data(void *outlet, char type, short length, t_atom *atoms)
{
    if (length > 1)
        outlet_list(outlet, NULL, length, atoms);
    else if (type == 'i')
        outlet_int(outlet, atom_getlong(atoms));
    else
        outlet_float(outlet, atom_getfloat(atoms));
}

// *********************************************************
// -(int handler)-------------------------------------------
static void mapdevice_sig_handler(mapper_signal sig, mapper_id instance,
                                  const void *value, int count,
                                  mapper_timetag_t *tt)
{
    t_map_ptrs *ptrs = mapper_signal_user_data(sig);
    t_mapdevice *x = ptrs->home;
    t_object *obj = NULL;
    int i, length = mapper_signal_length(sig);
    char type = mapper_signal_type(sig);

    if (mapper_signal_num_instances(sig) > 1) {
        obj = (t_object *)mapper_signal_instance_user_data(sig, instance);
    }

    if (value) {
        if (length > (MAX_LIST)) {
            object_post((t_object *)x, "Maximum list length is %i!", MAX_LIST);
            length = MAX_LIST;
        }

        if (type == 'i') {
            int *v = (int*)value;
            for (i = 0; i < length; i++)
                atom_setlong(x->buffer + i, v[i]);
        }
        else if (type == 'f') {
            float *v = (float*)value;
            for (i = 0; i < length; i++)
                atom_setfloat(x->buffer + i, v[i]);
        }

        if (obj) {
            outlet_data(obj->o_outlet, type, length, x->buffer);
        }
        else {
            for (i=0; i<ptrs->num_objs; i++)
                outlet_data(ptrs->objs[i]->o_outlet, type, length, x->buffer);
        }
    }
    else if (obj) {
        atom_set_string(x->buffer, "release");
        atom_set_string(x->buffer+1, "local");
        outlet_list(obj->o_outlet, NULL, 2, x->buffer);
    }
}

// *********************************************************
// -(instance management handler)----------------------
static void mapdevice_instance_event_handler(mapper_signal sig,
                                             mapper_id instance,
                                             mapper_instance_event event,
                                             mapper_timetag_t *tt)
{
    t_map_ptrs *ptrs = mapper_signal_user_data(sig);
    t_mapdevice *x = ptrs->home;

    t_object *obj = (t_object *)mapper_signal_instance_user_data(sig, instance);
    if (!obj)
        return;

    int i, mode;
    atom_setlong(x->buffer, instance);
    switch (event) {
        case MAPPER_UPSTREAM_RELEASE:
            atom_set_string(x->buffer, "release");
            atom_set_string(x->buffer+1, "upstream");
            outlet_list(obj->o_outlet, NULL, 2, x->buffer);
            break;
        case MAPPER_DOWNSTREAM_RELEASE:
            atom_set_string(x->buffer, "release");
            atom_set_string(x->buffer+1, "downstream");
            outlet_list(obj->o_outlet, NULL, 2, x->buffer);
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
                    atom_set_string(x->buffer+1, "overflow");
                    // send overflow message to all instances
                    for (i=0; i<ptrs->num_objs; i++)
                        outlet_list(ptrs->objs[i]->o_outlet, NULL, 2, x->buffer);
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
// -(start a new queue if necessary)------------------------
static void mapdevice_maybe_start_queue(t_mapdevice *x)
{
    if (!x->updated) {
        mapper_timetag_now(&x->timetag);
        mapper_device_start_queue(x->device, x->timetag);
        x->updated = 1;
    }
}

// *********************************************************
// -(poll libmapper)----------------------------------------
static void mapdevice_poll(t_mapdevice *x)
{
    int count = 10;
    while(count-- && mapper_device_poll(x->device, 0)) {};
    if (!x->ready) {
        if (mapper_device_ready(x->device)) {
            object_post((t_object *)x, "Joining mapping network as '%s'",
                        mapper_device_name(x->device));
            if (!mapper_device_num_signals(x->device, MAPPER_DIR_ANY))
                object_post((t_object *)x, "Waiting for inputs and outputs...");
            x->ready = 1;
            mapdevice_print_properties(x);
        }
    }
    else if (x->updated) {
        mapper_device_send_queue(x->device, x->timetag);
        x->updated = 0;
    }
    clock_delay(x->clock, INTERVAL);  // Set clock to go off after delay
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

