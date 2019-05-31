//
//  ndecoder~.c
//  pd-ne
//
//  Created by javiernonis on 5/27/19.
//  Copyright © 2019 jnonis. All rights reserved.
//

#include "m_pd.h"
#include "ne.h"

static t_class *ndecoder_tilde_class;

typedef struct _ndecoder_tilde {
    t_object x_obj;
    t_sample f_dummy;
    
    float attack;
    float release;
    float attenuverter;
    float offset;
    float scale;
    
    float out;
    
    t_outlet *x_out;
    t_outlet *x_gate;
} t_ndecoder_tilde;

/** Returns 1.f for positive numbers and -1.f for negative numbers (including positive/negative zero) */
float sgn(float x) {
    return copysignf(1.0f, x);
}

float crossfade(float a, float b, float frac) {
    return a + frac * (b - a);
}

float shapeDelta(float delta, float tau, float shape) {
    float lin = sgn(delta) * 10.0f / tau;
    if (shape < 0.0) {
        float log = sgn(delta) * 40.0f / tau / (fabsf(delta) + 1.0f);
        return crossfade(lin, log, -shape * 0.95f);
    }
    else {
        float exp = M_E * delta / tau;
        return crossfade(lin, exp, shape * 0.90f);
    }
}

float paramToMinTime(float timeInput) {
    timeInput = clamp(timeInput, 0.0f, 1.0f);
    int time = (int) (timeInput * 5);
    switch (time) {
        case 0: return 1e-3; break;
        case 1: return crossfade(1e-3, 1e-2, (timeInput * 5.0f) - 1.0f); break;
        case 2: return 1e-2; break;
        case 3: return crossfade(1e-2, 1e-1, (timeInput * 5.0f) - 3.0f); break;
        case 4: return 1e-1; break;
        case 5: return crossfade(1e-1, 1.0f, (timeInput * 5.0f) - 5.0f); break;
        default: return 1.0f; break;
    }
}

t_int *ndecoder_tilde_perform(t_int *w) {
    t_ndecoder_tilde *x = (t_ndecoder_tilde *) (w[1]);
    t_sample *input = (t_sample *) (w[2]);
    t_sample *output = (t_sample *) (w[3]);
    t_sample *outgate = (t_sample *) (w[4]);
    int n = (int) (w[5]);
    
    for (int i = 0; i < n; i++) {
        float in = input[i];
        float shape = 0.0f;
        float delta = in - x->out;
        
        // Integrator
        float minTimeRise = paramToMinTime(x->attack); //Effective range 1e-3 to 1
        float minTimeFall = paramToMinTime(x->release); //Effective range 1e-3 to 1
        
        int rising = 0;
        int falling = 0;
        
        if (delta > 0) {
            // Rise
            float riseCv = x->attack / 10.0f;
            riseCv = clamp(riseCv, 0.0f, 1.0f);
            float rise = minTimeRise * powf(2.0f, riseCv * 10.0f);
            x->out += shapeDelta(delta, rise, shape) / sys_getsr();
            rising = (in - x->out > 1e-15); //test values 1e-3,1e-15
        } else if (delta < 0) {
            // Fall
            float fallCv = x->release / 10.0f;
            fallCv = clamp(fallCv, 0.0f, 1.0f);
            float fall = minTimeFall * powf(2.0f, fallCv * 10.0f);
            x->out += shapeDelta(delta, fall, shape) / sys_getsr();
            falling = (in - x->out < -1e-3);
        }
        if (!rising && !falling) {
            x->out = in;
        }
        
        // Clamp at -15V/+15V for safety
        output[i] = clamp((x->out * x->attenuverter * x->scale) + x->offset, -1.0f, 1.0f);
        if (clamp((x->out * x->attenuverter * x->scale) + x->offset, -1.0f, 1.0f) >= 0.2f) {
            outgate[i] =  1.0f;
        } else {
            outgate[i] =  0.0f;
        }
    }
    
    return (w + 6);
}

void ndecoder_tilde_dsp(t_ndecoder_tilde *x, t_signal **sp) {
    dsp_add(ndecoder_tilde_perform,
            5,
            x,
            sp[0]->s_vec,
            sp[1]->s_vec,
            sp[2]->s_vec,
            sp[0]->s_n);
}

void ndecoder_tilde_free(t_ndecoder_tilde *x) {
    outlet_free(x->x_out);
    outlet_free(x->x_gate);
}

void *ndecoder_tilde_new(t_floatarg f) {
    t_ndecoder_tilde *x = (t_ndecoder_tilde *)pd_new(ndecoder_tilde_class);
    
    x->attack = 0.1f;
    x->release = 0.7f;
    x->attenuverter = 1.0f;
    x->offset = 0.0f;
    x->scale = 1.0f;
    
    x->out = 0.0f;
    
    x->x_out = outlet_new(&x->x_obj, &s_signal);
    x->x_gate = outlet_new(&x->x_obj, &s_signal);
    
    return (void *)x;
}

void ndecoder_tilde_attack(t_ndecoder_tilde *x, t_floatarg f) {
    x->attack = f;
}

void ndecoder_tilde_release(t_ndecoder_tilde *x, t_floatarg f) {
    x->release = f;
}

void ndecoder_tilde_attenuverter(t_ndecoder_tilde *x, t_floatarg f) {
    x->attenuverter = f;
}

void ndecoder_tilde_offset(t_ndecoder_tilde *x, t_floatarg f) {
    x->offset = f;
}

void ndecoder_tilde_scale(t_ndecoder_tilde *x, t_floatarg f) {
    x->scale = f;
}

void ndecoder_tilde_setup(void) {
    ndecoder_tilde_class = class_new(gensym("ndecoder~"),
                                 (t_newmethod)ndecoder_tilde_new,
                                 (t_method)ndecoder_tilde_free,
                                 sizeof(t_ndecoder_tilde),
                                 CLASS_DEFAULT,
                                 A_DEFFLOAT,
                                 0);
    class_addmethod(ndecoder_tilde_class,
                    (t_method)ndecoder_tilde_dsp,
                    gensym("dsp"),
                    A_NULL);
    CLASS_MAINSIGNALIN(ndecoder_tilde_class, t_ndecoder_tilde, f_dummy);
    
    class_addmethod(ndecoder_tilde_class,
                    (t_method)ndecoder_tilde_attack,
                    gensym("attack"),
                    A_DEFFLOAT,
                    A_NULL);
    class_addmethod(ndecoder_tilde_class,
                    (t_method)ndecoder_tilde_release,
                    gensym("release"),
                    A_DEFFLOAT,
                    A_NULL);
    class_addmethod(ndecoder_tilde_class,
                    (t_method)ndecoder_tilde_attenuverter,
                    gensym("attenuverter"),
                    A_DEFFLOAT,
                    A_NULL);
    class_addmethod(ndecoder_tilde_class,
                    (t_method)ndecoder_tilde_offset,
                    gensym("offset"),
                    A_DEFFLOAT,
                    A_NULL);
    class_addmethod(ndecoder_tilde_class,
                    (t_method)ndecoder_tilde_scale,
                    gensym("scale"),
                    A_DEFFLOAT,
                    A_NULL);
}
