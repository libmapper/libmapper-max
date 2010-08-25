/**
	@file
	maxadmin - max object interface with libmapper
    http://www.idmil.org/software/mappingtools
	joseph malloch
*/

#include "ext.h"							// standard Max include, always required
#include "ext_obex.h"						// required for new style Max object
#include "src/operations.h"
#include "src/expression.h"
#include "src/mapper_internal.h"
#include "include/mapper/mapper.h"
#include <stdio.h>
#include <math.h>
#include "lo/lo.h"

#include <unistd.h>
#include <arpa/inet.h>

#define INTERVAL 10

////////////////////////// object struct
typedef struct _maxadmin 
{
	t_object ob;			// the object itself (must be first)
    void *m_outlet;
    void *m_outlet2;
    void *m_outlet3;
    void *m_clock;          // pointer to clock object
	char *basename;
    mapper_device device;
    mapper_signal sendsig;
    mapper_signal recvsig;
    int ready;
} t_maxadmin;

t_symbol *ps_list;

int port = 9000;

///////////////////////// function prototypes
//// standard set
void *maxadmin_new(t_symbol *s, long argc, t_atom *argv);
void maxadmin_free(t_maxadmin *x);
void maxadmin_assist(t_maxadmin *x, void *b, long m, long a, char *s);
void maxadmin_anything(t_maxadmin *x, t_symbol *msg, long argc, t_atom *argv);
void maxadmin_add_signal(t_maxadmin *x, t_symbol *msg, long argc, t_atom *argv);
void maxadmin_remove_signal(t_maxadmin *x, t_symbol *msg, long argc, t_atom *argv);
void poll(t_maxadmin *x);
void float_handler(mapper_signal msig, mapper_signal_value_t *v);
void int_handler(mapper_signal msig, mapper_signal_value_t *v);
void maxadmin_print_properties(t_maxadmin *x);

//////////////////////// global class pointer variable
void *maxadmin_class;


int main(void)
{	
	t_class *c;
	
	c = class_new("maxadmin", (method)maxadmin_new, (method)maxadmin_free, (long)sizeof(t_maxadmin), 
				  0L /* leave NULL!! */, A_GIMME, 0);
	
	/* you CAN'T call this from the patcher */
    class_addmethod(c, (method)maxadmin_assist,			"assist",   A_CANT,     0);
    class_addmethod(c, (method)maxadmin_add_signal,     "add",      A_GIMME,    0);
    class_addmethod(c, (method)maxadmin_remove_signal,  "remove",   A_GIMME,    0);
    class_addmethod(c, (method)maxadmin_anything,       "anything", A_GIMME,    0);
	
	class_register(CLASS_BOX, c); /* CLASS_NOBOX */
	maxadmin_class = c;
	
	ps_list = gensym("list");
	
	return 0;
}

void maxadmin_print_properties(t_maxadmin *x)
{
	if (x->device) {
		//get device properties
		//output them
		//outlet_list(x->m_outlet3, ps_list, 2, myList);
	}
}

void maxadmin_assist(t_maxadmin *x, void *b, long m, long a, char *s)
{
	if (m == ASSIST_INLET) { // inlet
		sprintf(s, "OSC input");
	} 
	else {	// outlet
        if (a == 0) {
            sprintf(s, "Mapped OSC data");
        }
        else if (a == 1) {
            sprintf(s, "Does nothing currently");
        }
        else {
            sprintf(s, "Device information");
        }
	}
}

void maxadmin_free(t_maxadmin *x)
{
    clock_unset(x->m_clock);	// Remove clock routine from the scheduler
	clock_free(x->m_clock);		// Frees memeory used by clock
    
    if (x->device) {
        if (x->device->routers) {
            post("Removing router.. ");
            //fflush(stdout);
            mdev_remove_router(x->device, x->device->routers);
            post("ok\n");
        }
        post("Freeing device.. ");
        //fflush(stdout);
        mdev_free(x->device);
        post("ok\n");
    }
}

void maxadmin_add_signal(t_maxadmin *x, t_symbol *s, long argc, t_atom *argv)
{
	t_atom myList[2];
	
	if (argc < 4)
		return;
	
	if (argv->a_type == A_SYM) {
		if (atom_getsym(argv)->s_name == "input") {
			//extract signal name
			if ((argv + 1)->a_type != A_SYM)
				return;
			//register all signals as floats for now
			x->recvsig = msig_float(1, atom_getsym(argv + 1)->s_name, 0, 0, 1, 0, float_handler, x);
			mdev_register_input(x->device, x->recvsig);
			
			//output numInputs
			atom_setsym(myList, gensym("numInputs"));
			atom_setlong(myList + 1, mdev_num_inputs(x->device));
			outlet_list(x->m_outlet, ps_list, 2, myList);
		} 
		else if (atom_getsym(argv)->s_name == "output") {
			//extract signal name
			if ((argv + 1)->a_type != A_SYM)
				return;
			//register all signals as floats for now
			x->sendsig = msig_float(1, atom_getsym(argv + 1)->s_name, 0, 0, 1, 0, 0, 0);
			mdev_register_output(x->device, x->sendsig);
			
			//output numOutputs
			atom_setsym(myList, gensym("numOutputs"));
			atom_setlong(myList + 1, mdev_num_outputs(x->device));
			outlet_list(x->m_outlet, ps_list, 2, myList);
		}
	}
}

void maxadmin_remove_signal(t_maxadmin *x, t_symbol *s, long argc, t_atom *argv)
{
	;
}

void maxadmin_anything(t_maxadmin *x, t_symbol *msg, long argc, t_atom *argv)
{
	//mdev_poll(x->device, 0);
	
	mapper_signal *msig;
	char *path;
	float payload;
	
	//get OSC path
	if (argv->a_type == A_SYM) {
		path = strdup(atom_getsym(argv)->s_name);
		
		//find signal
		if (mdev_find_output_by_name(x->device, path, msig) == -1)
			return;
		
		//get payload
		if (argv->a_type == A_FLOAT) {
			payload = atom_getfloat(argv);
		} else if (argv->a_type == A_LONG) {
			payload = (float) atom_getlong(argv);
		} else {
			return;
		}

	}
	
	//update signal
	msig_update_scalar(*msig, (mval) payload);
}

void int_handler(mapper_signal msig, mapper_signal_value_t *v)
{
    t_maxadmin *x = msig->user_data;
	char *path = strdup(msig->props.name);
	
    t_atom myList[2];
    atom_setsym(myList, gensym(path));
    atom_setlong(myList + 1, (*v).i32);
    outlet_list(x->m_outlet, ps_list, 2, myList);
}

void float_handler(mapper_signal msig, mapper_signal_value_t *v)
{
    t_maxadmin *x = msig->user_data;
	char *path = strdup(msig->props.name);
	
    t_atom myList[2];
    atom_setsym(myList, gensym(path));
    atom_setfloat(myList + 1, (*v).f);
    outlet_list(x->m_outlet, ps_list, 2, myList);
}

/*! Creation of a local sender. */
int setup_device(t_maxadmin *x)
{
    //use dummy name for now
	if (x->basename) {
		x->device = mdev_new(x->basename, port);
	}
	else {
		x->device = mdev_new("maxadmin", port);
	}

    if (!x->device)
        return 1;
    else
        maxadmin_print_properties(x);
    
    return 0;
}


void *maxadmin_new(t_symbol *s, long argc, t_atom *argv)
{
	t_maxadmin *x;
    long i;
    
    x = object_alloc(maxadmin_class);

    //intin(x,1);
    x->m_outlet = listout((t_object *)x);
    x->m_outlet2 = outlet_new((t_object *)x,0);
    x->m_outlet3 = outlet_new((t_object *)x,0);
        
    for (i = 0; i < argc; i++) {
        if ((argv + i)->a_type == A_SYM) {
			if(strcmp(atom_getsym(argv+i)->s_name, "@alias") == 0) {
				if ((argv + i + 1)->a_type == A_SYM) {
					x->basename = strdup(atom_getsym(argv+i+1)->s_name);
					//object_post((t_object *)x, "got alias: %s", atom_getsym(argv+i+1)->s_name);
					i++;
				}
			}
			else if(strcmp(atom_getsym(argv+i)->s_name, "@def") == 0) {
				if ((argv + i + 1)->a_type == A_SYM) {
					//x->definition = strdup(atom_getsym(argv+i+1)->s_name);
					object_post((t_object *)x, "got definition: %s", atom_getsym(argv+i+1)->s_name);
					i++;
				}
			}
        }
    }
    
    if (setup_device(x)) {
        post("Error initializing device.\n");
    }
    
    x->ready = 0;
    
    x->m_clock = clock_new(x, (method)poll);	// Create the timing clock
    
    clock_delay(x->m_clock, INTERVAL);  // Set clock to go off after delay
    
	return (x);
}

void poll(t_maxadmin *x)
{
	t_atom myList[2];
	char *message;
    
    mdev_poll(x->device, 0);
    
    if (!x->ready) {
        if (mdev_ready(x->device)) {
            mapper_db_dump();
			
			//output name
			message = strdup(mapper_admin_name(x->device->admin));
			atom_setsym(myList, gensym("name"));
			atom_setsym(myList + 1, gensym(message));
			outlet_list(x->m_outlet, ps_list, 2, myList);
			
			//output IP
			message = strdup(inet_ntoa(x->device->admin->interface_ip));
			atom_setsym(myList, gensym("IP"));
			atom_setsym(myList + 1, gensym(message));
			outlet_list(x->m_outlet, ps_list, 2, myList);
			
			//output port
			atom_setsym(myList, gensym("port"));
			atom_setlong(myList + 1, x->device->admin->port.value);
			outlet_list(x->m_outlet, ps_list, 2, myList);
			
			//output numInputs
			atom_setsym(myList, gensym("numInputs"));
			atom_setlong(myList + 1, mdev_num_inputs(x->device));
			outlet_list(x->m_outlet, ps_list, 2, myList);
			
			//output numOutputs
			atom_setsym(myList, gensym("numOutputs"));
			atom_setlong(myList + 1, mdev_num_outputs(x->device));
			outlet_list(x->m_outlet, ps_list, 2, myList);
			
			//output canAlias
			//atom_setsym(myList, gensym("canAlias"));
			//atom_setlong(myList + 1, x->device->can_alias);
			//outlet_list(x->m_outlet, ps_list, 2, myList);
			
            x->ready = 1;
        }
    }
	
	clock_delay(x->m_clock, INTERVAL);  // Set clock to go off after delay
}
